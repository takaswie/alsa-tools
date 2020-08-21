// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#ifndef __FILE_CNTR__
#define __FILE_CNTR__

#include <stdint.h>

enum blob_type {
    BLOB_TYPE_DSP = 0,
    BLOB_TYPE_ICELYNX = 1,
    BLOB_TYPE_DATA = 2,
    BLOB_TYPE_FPGA = 3,
};

struct file_cntr_header {
    enum blob_type type;
    uint32_t offset_addr;
    uint32_t blob_quads;
    uint32_t blob_crc32;
    uint32_t blob_checksum;
    uint32_t version;
    uint32_t crc_in_region_end;
    uint32_t cntr_quads;
};

struct file_cntr_payload {
    uint32_t *blob;
    size_t count;
};

struct file_cntr {
    struct file_cntr_header header;
    struct file_cntr_payload payload;
};

int file_cntr_parse(struct file_cntr *cntr, const char *filepath);
void file_cntr_release(struct file_cntr *cntr);

#endif
