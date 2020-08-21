// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include "efw-proto.h"
#include "efw-proto-sigs-marshal.h"

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
};
G_DEFINE_TYPE_WITH_PRIVATE(EfwProto, efw_proto, HINAWA_TYPE_FW_RESP)

#define EFW_RESP_ADDR           0xecc080000000ull
#define EFW_MAX_FRAME_SIZE      0x200u

enum efw_proto_sig_type {
    EFW_PROTO_SIG_TYPE_RESPONDED = 1,
    EFW_PROTO_SIG_COUNT,
};
static guint efw_proto_sigs[EFW_PROTO_SIG_COUNT] = { 0 };

static void proto_finalize(GObject *obj)
{
    EfwProto *self = EFW_PROTO(obj);

    efw_proto_unbind(self);

    G_OBJECT_CLASS(efw_proto_parent_class)->finalize(obj);
}

static void efw_proto_class_init(EfwProtoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = proto_finalize;

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
    return;
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
}
