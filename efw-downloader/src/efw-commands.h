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

#endif
