// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include "config-rom.h"

gboolean config_rom_detect_vendor_and_model(const guint8 *rom, guint32 *vendor_id, guint32 *model_id)
{
    gboolean detected = FALSE;

    if (rom[24] != 0x03)
        return FALSE;

    *vendor_id = (rom[25] << 16) | (rom[26] << 8) | rom[27];

    if (rom[32] != 0x17)
        return FALSE;

    *model_id = (rom[33] << 16) | (rom[34] << 8) | rom[35];

    switch (*vendor_id) {
    case VENDOR_LOUD:
        switch (*model_id) {
        case MODEL_ONYX400F:
        case MODEL_ONYX1200F:
            detected = TRUE;
            break;
        default:
            break;
        }
        break;
    case VENDOR_ECHO_AUDIO:
        switch (*model_id) {
        case MODEL_AF2:
        case MODEL_AF4:
        case MODEL_AF8:
        case MODEL_AF8P:
        case MODEL_AF12:
        case MODEL_AF12HD:
        case MODEL_AF12_APPLE:
        case MODEL_FWHDMI:
            detected = TRUE;
            break;
        default:
            break;
        }
        break;
    case VENDOR_GIBSON:
        switch (*model_id) {
        case MODELRIP:
        case MODELAUDIOPUNK:
            detected = TRUE;
            break;
        default:
            break;
        }
    default:
        break;
    }

    return detected;
}
