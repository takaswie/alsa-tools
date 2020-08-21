// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "efw-commands.h"

// Categories in Echo Audio Fireworks protocol.
#define CATEGORY_HW             0

// Commands in hardware category.
#define HW_CMD_INFO             0

#define TIMEOUT 200

#define CAP_HAS_DSP     0x00000010
#define CAP_HAS_FPGA    0x00000020

void efw_hw_info(EfwProto *proto, struct hw_info *info, GError **error)
{
    gsize param_count = sizeof(*info) / sizeof(guint32);
    efw_proto_transaction(proto, CATEGORY_HW, HW_CMD_INFO, NULL, 0,
                          (guint32 *const *)&info, &param_count, TIMEOUT, error);
}

void efw_hw_info_has_fpga(struct hw_info *info, gboolean *has_fpga)
{
    *has_fpga = !!(info->flags & CAP_HAS_FPGA);
}
