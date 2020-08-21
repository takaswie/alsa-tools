// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#ifndef __HEURISTICS_H__
#define __HEURISTICS_H__

// For the combination of IceLnyx Micro and FPGA.
#define OFFSET_FPGA             0x00000000
#define OFFSET_ICELYNX          0x00100000

// For the combination of IceLynx Micro and DSP.
#define OFFSET_BOOTSTRAP        0x00000000
#define OFFSET_DSP_A            0x000c0000
// OFFSET_ICELYNX is the same as the above.
#define OFFSET_DSP_B            0x00140000

#define BOOTSTRAP_REGION_SIZE   0x00000800
#define COMMON_REGION_SIZE      0x00040000
#define FPGA_REGION_SIZE        0x00060000

#endif
