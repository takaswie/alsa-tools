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
