// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include "efw-proto.h"
#include "efw-proto-sigs-marshal.h"

#include <sound/firewire.h>

#include <libhinawa/fw_req.h>

/**
 * SECTION:efw_proto
 * @Title: EfwProto
 * @Short_description: Transaction implementation for Fireworks protocol
 * @include: fw_fcp.h
 *
 * Fireworks board module from Echo Digital Audio corporation supports specific protocol based on
 * a pair of asynchronous transactions in IEEE 1394 bus for command and response. The #EfwProto class
 * is an implementation for the protocol. The Fireworks device transfers response against the
 * command to a certain address region on 1394 OHCI controller. The instance of #EfwProto reserves
 * the address region at call of #efw_proto_bind(), releases at call of #efw_proto_unbind().
 */
struct _EfwProtoPrivate {
    guint32 *buf;
    guint32 seqnum;
    HinawaFwNode *node;
    GMutex mutex;
};
G_DEFINE_TYPE_WITH_PRIVATE(EfwProto, efw_proto, HINAWA_TYPE_FW_RESP)

/**
 * efw_proto_error_quark:
 *
 * Return the GQuark for error domain of GError which has code in #HinawaSndEfwStatus.
 *
 * Returns: A #GQuark.
 */
G_DEFINE_QUARK(efw-proto-error-quark, efw_proto_error)

#define EFW_CMD_ADDR            0xecc000000000ull
#define EFW_RESP_ADDR           0xecc080000000ull
#define EFW_MAX_FRAME_SIZE      0x200u
#define MINIMUM_VERSION         1

enum efw_proto_sig_type {
    EFW_PROTO_SIG_TYPE_RESPONDED = 1,
    EFW_PROTO_SIG_COUNT,
};
static guint efw_proto_sigs[EFW_PROTO_SIG_COUNT] = { 0 };

static void proto_finalize(GObject *obj)
{
    EfwProto *self = EFW_PROTO(obj);
    EfwProtoPrivate *priv = efw_proto_get_instance_private(self);

    efw_proto_unbind(self);

    g_mutex_clear(&priv->mutex);

    G_OBJECT_CLASS(efw_proto_parent_class)->finalize(obj);
}

static HinawaFwRcode proto_handle_response(HinawaFwResp *resp, HinawaFwTcode tcode);

static void efw_proto_class_init(EfwProtoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = proto_finalize;

    HINAWA_FW_RESP_CLASS(klass)->requested = proto_handle_response;

    /**
     * EfwProto::responded:
     * @self: A #EfwProto.
     * @status: One of #HinawaSndEfwStatus.
     * @seqnum: The sequence number of response.
     * @category: The value of category field in the response.
     * @command: The value of command field in the response.
     * @frame: (array length=frame_size)(element-type guint32): The array with elements for
     *         quadlet data of response for Echo Fireworks protocol.
     * @frame_size: The number of elements of the array.
     *
     * When the unit transfers asynchronous packet as response for Fireworks protocol, and the
     * process successfully reads the content of response from ALSA Fireworks driver, the
     * #EfwProto::responded signal handler is called with parameters of the response.
     */
    efw_proto_sigs[EFW_PROTO_SIG_TYPE_RESPONDED] =
        g_signal_new("responded",
            G_OBJECT_CLASS_TYPE(klass),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(EfwProtoClass, responded),
            NULL, NULL,
            efw_proto_sigs_marshal_VOID__ENUM_UINT_UINT_UINT_POINTER_UINT,
            G_TYPE_NONE,
            6, HINAWA_TYPE_SND_EFW_STATUS, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_UINT);
}

static void efw_proto_init(EfwProto *self)
{
    EfwProtoPrivate *priv = efw_proto_get_instance_private(self);

    priv->seqnum = 0;
    g_mutex_init(&priv->mutex);
}

/**
 * efw_proto_new:
 *
 * Instantiate and return #EfwProto object.
 *
 * Returns: An instance of #EfwProto.
 */
EfwProto *efw_proto_new()
{
    return g_object_new(EFW_TYPE_PROTO, NULL);
}

/**
 * efw_proto_bind:
 * @self: A #EfwProto.
 * @node: A #HinawaFwNode.
 * @error: A #GError. The error can be generated with domain of #hinawa_fw_node_error_quark().
 *
 * Bind to Fireworks protocol for communication to the given node.
 */
void efw_proto_bind(EfwProto *self, HinawaFwNode *node, GError **error)
{
    EfwProtoPrivate *priv;

    g_return_if_fail(EFW_IS_PROTO(self));
    priv = efw_proto_get_instance_private(self);

    hinawa_fw_resp_reserve(HINAWA_FW_RESP(self), node, EFW_RESP_ADDR, EFW_MAX_FRAME_SIZE, error);
    if (*error != NULL)
        return;

    priv->buf = g_malloc0(EFW_MAX_FRAME_SIZE);
    priv->node = node;
}

/**
 * efw_proto_unbind:
 * @self: A #EfwProto.
 *
 * Unbind from Fireworks protocol.
 */
void efw_proto_unbind(EfwProto *self)
{
    EfwProtoPrivate *priv;

    g_return_if_fail(EFW_IS_PROTO(self));
    priv = efw_proto_get_instance_private(self);

    hinawa_fw_resp_release(HINAWA_FW_RESP(self));

    g_free(priv->buf);
    priv->buf = NULL;
    priv->node = NULL;
}

/**
 * efw_proto_command:
 * @self: A #EfwProto.
 * @category: One of category for the transaction.
 * @command: One of category for the transaction.
 * @args: (array length=arg_count)(nullable): An array with elements for quadlet data as arguments
 *        for command.
 * @arg_count: The number of quadlets in the args array.
 * @resp_seqnum: (out): The sequence number for response transaction.
 * @error: A #GError. Error can be generated with two domains of #hinawa_fw_req_error_quark() and
 *         #hinawa_fw_req_error_quark().
 *
 * Transfer asynchronous transaction for command frame of Fireworks protocol. When receiving
 * asynchronous transaction for response frame, #EfwProto::responded GObject signal is emitted.
 */
void efw_proto_command(EfwProto *self, guint category, guint command,
                       const guint32 *args, gsize arg_count, guint32 *resp_seqnum,
                       GError **error)
{
    EfwProtoPrivate *priv;
    HinawaFwReq *req;
    gsize length;
    struct snd_efw_transaction *frame;
    int i;

    g_return_if_fail(EFW_IS_PROTO(self));
    g_return_if_fail(sizeof(*args) * arg_count + sizeof(*frame) < EFW_MAX_FRAME_SIZE);
    g_return_if_fail(resp_seqnum != NULL);
    g_return_if_fail(error == NULL || *error == NULL);

    priv = efw_proto_get_instance_private(self);

    length = sizeof(*frame);
    if (args != NULL)
        length += sizeof(guint32) * arg_count;

    frame = g_malloc0(length);

    // Fill request frame for transaction.
    frame->length = GUINT32_TO_BE(length / sizeof(guint32));
    frame->version = GUINT32_TO_BE(MINIMUM_VERSION);
    frame->category = GUINT32_TO_BE(category);
    frame->command = GUINT32_TO_BE(command);
    if (args != NULL) {
        for (i = 0; i < arg_count; ++i)
            frame->params[i] = GUINT32_TO_BE(args[i]);
    }

    // Increment the sequence number for next transaction.
    g_mutex_lock(&priv->mutex);
    frame->seqnum = GUINT32_TO_BE(priv->seqnum);
    *resp_seqnum = priv->seqnum + 1;
    priv->seqnum += 2;
    if (priv->seqnum > SND_EFW_TRANSACTION_USER_SEQNUM_MAX)
        priv->seqnum = 0;
    g_mutex_unlock(&priv->mutex);

    // Send this request frame.
    req = hinawa_fw_req_new();

    hinawa_fw_req_transaction_sync(req, priv->node, HINAWA_FW_TCODE_WRITE_BLOCK_REQUEST,
                                   EFW_CMD_ADDR, length, (guint8 *const *)&frame, &length, 100,
                                   error);

    g_object_unref(req);
    g_free(frame);
}

struct waiter {
    guint32 seqnum;

    guint32 category;
    guint32 command;
    HinawaSndEfwStatus status;
    guint32 *params;
    gsize param_count;

    GCond cond;
    GMutex mutex;
};

static void handle_responded_signal(EfwProto *self, HinawaSndEfwStatus status, guint32 seqnum,
                    guint category, guint command,
                    const guint32 *params, guint32 param_count, gpointer user_data)
{
    struct waiter *w = (struct waiter *)user_data;

    if (seqnum == w->seqnum) {
        g_mutex_lock(&w->mutex);

        if (category != w->category || command != w->command)
            status = HINAWA_SND_EFW_STATUS_BAD;
        w->status = status;

        if (param_count > 0 && param_count <= w->param_count)
            memcpy(w->params, params, param_count * sizeof(*params));
        w->param_count = param_count;

        g_cond_signal(&w->cond);

        g_mutex_unlock(&w->mutex);
    }
}

static const char *const err_msgs[] = {
    [HINAWA_SND_EFW_STATUS_OK]              = "The transaction finishes successfully",
    [HINAWA_SND_EFW_STATUS_BAD]             = "The request or response includes invalid header",
    [HINAWA_SND_EFW_STATUS_BAD_COMMAND]     = "The request includes invalid category or command",
    [HINAWA_SND_EFW_STATUS_COMM_ERR]        = "The transaction fails due to communication error",
    [HINAWA_SND_EFW_STATUS_BAD_QUAD_COUNT]  = "The number of quadlets in transaction is invalid",
    [HINAWA_SND_EFW_STATUS_UNSUPPORTED]     = "The request is not supported",
    [HINAWA_SND_EFW_STATUS_TIMEOUT]         = "The transaction is canceled due to response timeout",
    [HINAWA_SND_EFW_STATUS_DSP_TIMEOUT]     = "The operation for DSP did not finish within timeout",
    [HINAWA_SND_EFW_STATUS_BAD_RATE]        = "The request includes invalid value for sampling frequency",
    [HINAWA_SND_EFW_STATUS_BAD_CLOCK]       = "The request includes invalid value for source of clock",
    [HINAWA_SND_EFW_STATUS_BAD_CHANNEL]     = "The request includes invalid value for the number of channel",
    [HINAWA_SND_EFW_STATUS_BAD_PAN]         = "The request includes invalid value for panning",
    [HINAWA_SND_EFW_STATUS_FLASH_BUSY]      = "The on-board flash is busy and not operable",
    [HINAWA_SND_EFW_STATUS_BAD_MIRROR]      = "The request includes invalid value for mirroring channel",
    [HINAWA_SND_EFW_STATUS_BAD_LED]         = "The request includes invalid value for LED",
    [HINAWA_SND_EFW_STATUS_BAD_PARAMETER]   = "The request includes invalid value of parameter",
    [HINAWA_SND_EFW_STATUS_LARGE_RESP]      = "The size of response is larger than expected",
};

#define generate_error(error, code) \
    g_set_error_literal(error, EFW_PROTO_ERROR, code, err_msgs[code])

/**
 * efw_proto_transaction:
 * @self: A #EfwProto.
 * @category: One of category for the transaction.
 * @command: One of category for the transaction.
 * @args: (array length=arg_count)(nullable): An array with elements for quadlet data as arguments
 *        for command.
 * @arg_count: The number of quadlets in the args array.
 * @params: (array length=param_count)(inout)(nullable): An array with elements for quadlet data to
 *          save parameters in response frame.
 * @param_count: The number of quadlets in the params array.
 * @timeout_ms: The timeout to wait for response of the transaction since request is initiated, in
 *              milliseconds.
 * @error: A #GError. Error can be generated with two domains of #hinawa_fw_node_error_quark(),
 *         #hinawa_fw_req_error_quark(), and #efw_proto_error_quark().
 *
 * Transfer asynchronous transaction for command frame of Fireworks protocol. When receiving
 * asynchronous transaction for response frame, #EfwProto::responded GObject signal is emitted.
 */
void efw_proto_transaction(EfwProto *self, guint category, guint command,
                           const guint32 *args, gsize arg_count,
                           guint32 *const *params, gsize *param_count,
                           guint timeout_ms, GError **error)
{
    gulong handler_id;
    struct waiter w;
    guint64 expiration;

    g_return_if_fail(EFW_IS_PROTO(self));
    g_return_if_fail(param_count != NULL);
    g_return_if_fail(error == NULL || *error == NULL);

    // This predicates against suprious wakeup.
    w.status = 0xffffffff;
    w.category = category;
    w.command = command;
    if (*param_count > 0)
        w.params = *params;
    else
        w.params = NULL;
    w.param_count = *param_count;
    g_cond_init(&w.cond);
    g_mutex_init(&w.mutex);

    handler_id = g_signal_connect(self, "responded", (GCallback)handle_responded_signal, &w);

    // Timeout is set in advance as a parameter of this object.
    expiration = g_get_monotonic_time() + timeout_ms * G_TIME_SPAN_MILLISECOND;

    efw_proto_command(self, category, command, args, arg_count, &w.seqnum, error);
    if (*error != NULL) {
        g_signal_handler_disconnect(self, handler_id);
        goto end;
    }

    g_mutex_lock(&w.mutex);
    while (w.status == 0xffffffff) {
        // Wait for a response with timeout, waken by the response handler.
        if (!g_cond_wait_until(&w.cond, &w.mutex, expiration))
            break;
    }
    g_signal_handler_disconnect(self, handler_id);
    g_mutex_unlock(&w.mutex);

    if (w.status == 0xffffffff)
        generate_error(error, HINAWA_SND_EFW_STATUS_TIMEOUT);
    else if (w.status != HINAWA_SND_EFW_STATUS_OK)
        generate_error(error, w.status);
    else if (w.param_count > *param_count)
        generate_error(error, HINAWA_SND_EFW_STATUS_LARGE_RESP);
    else
        *param_count = w.param_count;
end:
    g_cond_clear(&w.cond);
    g_mutex_clear(&w.mutex);
}


static HinawaFwRcode proto_handle_response(HinawaFwResp *resp, HinawaFwTcode tcode)
{
    EfwProto *self = EFW_PROTO(resp);
    EfwProtoPrivate *priv = efw_proto_get_instance_private(self);
    const guint8 *req_frame = NULL;
    gsize length = 0;
    const struct snd_efw_transaction *frame;
    guint status;
    guint seqnum;
    guint category;
    guint command;
    guint param_count;
    int i;

    hinawa_fw_resp_get_req_frame(resp, &req_frame, &length);
    if (length < sizeof(*frame))
        return HINAWA_FW_RCODE_DATA_ERROR;
    frame = (const struct snd_efw_transaction *)req_frame;

    status = GUINT32_FROM_BE(frame->status);
    if (status > HINAWA_SND_EFW_STATUS_BAD_PARAMETER)
        status = HINAWA_SND_EFW_STATUS_BAD;

    seqnum = GUINT32_FROM_BE(frame->seqnum);
    category = GUINT32_FROM_BE(frame->category);
    command = GUINT32_FROM_BE(frame->command);
    param_count = GUINT32_FROM_BE(frame->length) - sizeof(*frame) / sizeof(guint32);

    for (i = 0; i < param_count; ++i)
        priv->buf[i] = GUINT32_FROM_BE(frame->params[i]);

    g_signal_emit(self, efw_proto_sigs[EFW_PROTO_SIG_TYPE_RESPONDED], 0,
                  status, seqnum, category, command, priv->buf, param_count);

    return HINAWA_FW_RCODE_COMPLETE;
}
