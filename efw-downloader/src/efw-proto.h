// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#ifndef __EFW_PROTO_H__
#define __EFW_PROTO_H__

#include <glib.h>
#include <glib-object.h>

#include <libhinawa/fw_resp.h>
#include <libhinawa/hinawa_enum_types.h>

G_BEGIN_DECLS

#define EFW_TYPE_PROTO      (efw_proto_get_type())

#define EFW_PROTO(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), EFW_TYPE_PROTO, EfwProto))
#define EFW_IS_PROTO(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), EFW_TYPE_PROTO))

#define EFW_PROTO_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), EFW_TYPE_PROTO, EfwProtoClass))
#define EFW_IS_PROTO_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), EFW_TYPE_PROTO))
#define EFW_PROTO_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), EFW_TYPE_PROTO, EfwProtoClass))

typedef struct _EfwProto            EfwProto;
typedef struct _EfwProtoClass       EfwProtoClass;
typedef struct _EfwProtoPrivate     EfwProtoPrivate;

struct _EfwProto {
    HinawaFwResp parent_instance;

    EfwProtoPrivate *priv;
};

struct _EfwProtoClass {
    HinawaFwRespClass parent_class;

    /**
     * EfwProtoClass::responded:
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
    void (*responded)(EfwProto *self, HinawaSndEfwStatus status, guint seqnum,
                      guint category, guint command, const guint32 *frame, guint frame_size);
};

EfwProto *efw_proto_new();

void efw_proto_bind(EfwProto *self, HinawaFwNode *node, GError **error);
void efw_proto_unbind(EfwProto *self);

void efw_proto_command(EfwProto *self, guint category, guint command,
                       const guint32 *args, gsize arg_count, guint32 *resp_seqnum,
                       GError **exception);

G_END_DECLS

#endif
