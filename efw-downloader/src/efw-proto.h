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

struct _EfwProto {
    HinawaFwResp parent_instance;
};

struct _EfwProtoClass {
    HinawaFwRespClass parent_class;
};

EfwProto *efw_proto_new();

void efw_proto_bind(EfwProto *self, HinawaFwNode *node, GError **error);
void efw_proto_unbind(EfwProto *self);

G_END_DECLS

#endif
