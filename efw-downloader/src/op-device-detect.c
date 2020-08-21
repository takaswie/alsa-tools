// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "node-dispatcher.h"
#include "efw-commands.h"
#include "config-rom.h"

#include "heuristics.h"

static gboolean detect_by_heuristics_for_icelynx_and_fpga(EfwProto *proto)
{
    guint32 fpga[2] = {0};
    guint32 icelynx[2] = {0};
    GError *error = NULL;

    efw_flash_read(proto, OFFSET_FPGA + FPGA_REGION_SIZE - 8, fpga, G_N_ELEMENTS(fpga), &error);
    if (error != NULL)
        return FALSE;
    if (fpga[0] == 0xffffffff || fpga[1] == 0xffffffff)
        return FALSE;

    efw_flash_read(proto, OFFSET_ICELYNX + COMMON_REGION_SIZE - 8, icelynx, G_N_ELEMENTS(icelynx), &error);
    if (error != NULL)
        return FALSE;
    if (icelynx[0] == 0xffffffff || icelynx[1] == 0xffffffff)
        return FALSE;

    printf("Detect IceLynx and FPGA:\n");
    printf("  FPGA firmware:\n");
    printf("    offset:   0x00000000\n");
    printf("    version:  0x%08x\n", fpga[0]);
    printf("    crc32:    0x%08x\n", fpga[1]);
    printf("  IceLynx firmware:\n");
    printf("    offset:   0x00100000\n");
    printf("    version:  0x%08x\n", icelynx[0]);
    printf("    crc32:    0x%08x\n", icelynx[1]);

    return TRUE;
}

static gboolean detect_by_heuristics_for_icelynx_and_dsp(EfwProto *proto)
{
    guint32 bootstrap[2] = {0};
    guint32 dsp_a[2] = {0};
    guint32 dsp_b[2] = {0};
    guint32 icelynx[2] = {0};
    GError *error = NULL;

    efw_flash_read(proto, OFFSET_BOOTSTRAP + BOOTSTRAP_REGION_SIZE - 4, bootstrap, G_N_ELEMENTS(bootstrap), &error);
    if (error != NULL)
        return FALSE;
    if (bootstrap[0] == 0xffffffff || bootstrap[1] != 0xffffffff)
        return FALSE;

    efw_flash_read(proto, OFFSET_DSP_A + COMMON_REGION_SIZE - 8, dsp_a, G_N_ELEMENTS(dsp_a), &error);
    if (error != NULL)
        return FALSE;
    if (dsp_a[0] == 0xffffffff || dsp_a[1] == 0xffffffff)
        return FALSE;

    efw_flash_read(proto, OFFSET_ICELYNX + COMMON_REGION_SIZE - 8, icelynx, G_N_ELEMENTS(icelynx), &error);
    if (error != NULL)
        return FALSE;
    if (icelynx[0] == 0xffffffff || icelynx[1] == 0xffffffff)
        return FALSE;

    efw_flash_read(proto, OFFSET_DSP_B + COMMON_REGION_SIZE - 8, dsp_b, G_N_ELEMENTS(dsp_b), &error);
    if (error != NULL)
        return FALSE;
    if (dsp_b[0] == 0xffffffff || dsp_b[1] == 0xffffffff)
        return FALSE;

    printf("Detect IceLynx and DSP:\n");
    printf("  bootstrap:\n");
    printf("    offset:   0x00000000\n");
    printf("  DSP firmware A:\n");
    printf("    offset:   0x000c0000\n");
    printf("    version:  0x%08x\n", dsp_a[0]);
    printf("    crc32:    0x%08x\n", dsp_a[1]);
    printf("  IceLynx firmware:\n");
    printf("    offset:   0x00100000\n");
    printf("    version:  0x%08x\n", icelynx[0]);
    printf("    crc32:    0x%08x\n", icelynx[1]);
    printf("  DSP firmware B:\n");
    printf("    offset:   0x00140000\n");
    printf("    version:  0x%08x\n", dsp_b[0]);
    printf("    crc32:    0x%08x\n", dsp_b[1]);

    return TRUE;
}

static gboolean is_supported_hardware_type(struct hw_info *info)
{
    switch (info->type) {
    // VENDOR_LOUD:
    case MODEL_ONYX400F:
    case MODEL_ONYX1200F:
    // VENDOR_ECHO_AUDIO:
    case MODEL_AF2:
    case MODEL_AF4:
    case MODEL_AF8:
    case MODEL_AF8P:
    case MODEL_AF12:
    case MODEL_AF12HD:
    case MODEL_AF12_APPLE:
    case MODEL_FWHDMI:
    // VENDOR_GIBSON:
    case MODELRIP:
    case MODELAUDIOPUNK:
        return TRUE;
    default:
        return FALSE;
    }
}

void op_device_detect(int argc, char **argv, EfwProto *proto, GError **error)
{
    struct hw_info info = {0};
    gboolean has_fpga;

    efw_hw_info(proto, &info, error);
    if (*error != NULL)
        return;

    if (!is_supported_hardware_type(&info)) {
        fprintf(stderr, "The node is not for Fireworks device\n");
        g_set_error_literal(error, G_FILE_ERROR, g_file_error_from_errno(ENXIO),
                            "The node is not for Fireworks device");
        return;
    }

    efw_hw_info_has_fpga(&info, &has_fpga);

    if (has_fpga) {
        if (!detect_by_heuristics_for_icelynx_and_fpga(proto)) {
            fprintf(stderr, "Fail to detect for IceLynx Micro and FPGA\n");
            g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_NXIO,
                                "Fail to detect for IceLynx Micro and FPGA");
        }
    } else {
        if (!detect_by_heuristics_for_icelynx_and_dsp(proto)) {
            fprintf(stderr, "Fail to detect for IceLynx Micro and DSP\n");
            g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_NXIO,
                                "Fail to detect for IceLynx Micro and DSP");
        }
    }
}
