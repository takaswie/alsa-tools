// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "file-cntr.h"

static void print_help()
{
    printf("Usage\n"
           "  efw-downloader file PATH parse [--help | -h]\n"
           "\n"
           "where:\n"
           "  --help, -h: print this help message\n");
}

static const char *get_blob_type_name(enum blob_type type)
{
    const char *name;

    switch (type) {
    case BLOB_TYPE_DSP:
        name = "DSP";
        break;
    case BLOB_TYPE_ICELYNX:
        name = "IceLynx";
        break;
    case BLOB_TYPE_DATA:
        name = "data";
        break;
    case BLOB_TYPE_FPGA:
        name = "FPGA";
        break;
    default:
        name = "invalid";
        break;
    }

    return name;
}

static void file_cntr_dump_header(const struct file_cntr *cntr)
{
    printf("Container header:\n");
    printf("  type:               %d (%s)\n", cntr->header.type, get_blob_type_name(cntr->header.type));
    printf("  offset_addr:        0x%08x\n", cntr->header.offset_addr);
    printf("  blob_quads:         %u\n", cntr->header.blob_quads);
    printf("  blob_crc32:         0x%08x\n", cntr->header.blob_crc32);
    printf("  blob_checksum:      0x%08x\n", cntr->header.blob_checksum);
    printf("  version:            0x%08x\n", cntr->header.version);
    printf("  crc_in_region_end:  %d\n", cntr->header.crc_in_region_end);
    printf("  total_quads:        %d\n", cntr->header.cntr_quads);
}

static void file_cntr_dump_payload(const struct file_cntr *cntr)
{
    int i;

    printf("Container payload:\n");
    for (i = 0; i < cntr->payload.count; ++i)
        printf("  %08x: %08x\n", cntr->header.offset_addr + i * 4, cntr->payload.blob[i]);
}

static int parse_args(int argc, char **argv, bool *help)
{
    int i;

    if (argc < 4)
        return -EINVAL;
    assert(!strncmp(argv[3], "parse", sizeof("parse")));

    *help = false;
    for (i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--help", sizeof("--help")) == 0 ||
            strncmp(argv[i], "-h", sizeof("-h")) == 0) {
            *help = true;
            break;
        }
    }

    return 0;
}

int op_file_parse(int argc, char **argv, struct file_cntr *cntr)
{
    bool help = false;
    int err;

    err = parse_args(argc, argv, &help);
    if (err < 0)
        return err;

    if (help) {
        print_help();
        return 0;
    }

    file_cntr_dump_header(cntr);
    file_cntr_dump_payload(cntr);

    return 0;
}
