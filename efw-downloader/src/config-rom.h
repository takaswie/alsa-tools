// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#ifndef __CONFIG_ROM_H__
#define __CONFIG_ROM_H__

#include <glib.h>

#define VENDOR_LOUD         0x000ff2
#define   MODEL_ONYX400F    0x00400f
#define   MODEL_ONYX1200F   0x01200f
#define VENDOR_ECHO_AUDIO   0x001486
#define   MODEL_AF2         0x000af2
#define   MODEL_AF4         0x000af4
#define   MODEL_AF8         0x000af8
#define   MODEL_AF8P        0x000af9
#define   MODEL_AF12        0x00af12
#define   MODEL_AF12HD      0x0af12d
#define   MODEL_AF12_APPLE  0x0af12a
#define   MODEL_FWHDMI      0x00afd1
#define VENDOR_GIBSON       0x00075b
#define   MODELRIP          0x00afb2
#define   MODELAUDIOPUNK    0x00afb9

gboolean config_rom_detect_vendor_and_model(const guint8 *rom, guint32 *vendor_id, guint32 *model_id);

#endif
