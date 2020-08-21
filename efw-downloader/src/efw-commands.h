// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#ifndef __EFW_CMDS_H__
#define __EFW_CMDS_H__

#include "efw-proto.h"

struct hw_info {
    guint32 flags;
    guint32 guid_hi;
    guint32 guid_lo;
    guint32 type;
    guint32 version;
    guint32 vendor_name[8];
    guint32 model_name[8];
    guint32 supported_clocks;
    guint32 amdtp_rx_pcm_channels;
    guint32 amdtp_tx_pcm_channels;
    guint32 phys_out;
    guint32 phys_in;
    guint32 phys_out_grp_count;
    guint32 phys_out_grp[4];
    guint32 phys_in_grp_count;
    guint32 phys_in_grp[4];
    guint32 midi_out_ports;
    guint32 midi_in_ports;
    guint32 max_sample_rate;
    guint32 min_sample_rate;
    guint32 dsp_version;
    guint32 arm_version;
    guint32 mixer_playback_channels;
    guint32 mixer_capture_channels;
    guint32 fpga_version;
    guint32 amdtp_rx_pcm_channels_2x;
    guint32 amdtp_tx_pcm_channels_2x;
    guint32 amdtp_rx_pcm_channels_4x;
    guint32 amdtp_tx_pcm_channels_4x;
    guint32 reserved[16];
};

void efw_hw_info(EfwProto *proto, struct hw_info *info, GError **error);

void efw_hw_info_has_fpga(struct hw_info *info, gboolean *has_fpga);

void efw_flash_erase(EfwProto *proto, size_t offset, GError **error);
void efw_flash_read(EfwProto *proto, size_t offset, guint32 *buf, size_t quads, GError **error);
void efw_flash_write(EfwProto *proto, size_t offset, guint32 *buf, size_t quads, GError **error);
void efw_flash_state(EfwProto *proto, gboolean *state, GError **error);
void efw_flash_get_session_base(EfwProto *proto, size_t *offset, GError **error);
void efw_flash_lock(EfwProto *proto, gboolean locked, GError **error);

int efw_flash_get_block_size(size_t offset, size_t *block_size);
void efw_flash_erase_and_wait(EfwProto *proto, size_t offset, GError **error);
void efw_flash_recursive_read(EfwProto *proto, size_t offset, guint32 *buf, size_t quads, GError **error);
void efw_flash_recursive_write(EfwProto *proto, size_t offset, guint32 *buf, size_t quads, GError **error);

#endif
