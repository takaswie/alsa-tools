// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include "efw-proto.h"

/**
 * SECTION:efw_proto
 * @Title: EfwProto
 * @Short_description: Transaction implementation for Fireworks protocol
 * @include: fw_fcp.h
 *
 * Fireworks board module from Echo Digital Audio corporation supports specific protocol based on
 * a pair of asynchronous transactions in IEEE 1394 bus. The EfwProto class is an implementation
 * for the protocol.
 */
G_DEFINE_TYPE(EfwProto, efw_proto, HINAWA_TYPE_FW_RESP)

static void efw_proto_class_init(EfwProtoClass *klass)
{
    return;
}

static void efw_proto_init(EfwProto *self)
{
    return;
}
