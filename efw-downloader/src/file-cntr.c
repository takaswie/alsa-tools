// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <zlib.h>

#include "file-cntr.h"

#define MAGIC_BYTES             "1651 1 0 0 0\r\n"
#define PAYLOAD_OFFSET_QUADS    0x3f

static int parse_entry(FILE *handle, uint32_t *val)
{
    char buf[16];
    char *end;
    size_t len;
    int err = 0;

    if (fgets(buf, sizeof(buf), handle) != buf) {
        if (ferror(handle))
            err = -errno;
        else
            err = INT_MAX;  // Use the value for EOF.
        return err;
    }

    len = strlen(buf);
    if (buf[0] != '0' || buf[1] != 'x' || buf[len - 2] != '\r' || buf[len - 1] != '\n')
        return -EPROTO;
    buf[len - 2] = '\0';

    *val = strtoul(buf, &end, 16);
    if (*end != '\0')
        return -EPROTO;

    return err;
}

static int parse_header(FILE *handle, struct file_cntr_header *header)
{
    uint32_t *buf = (uint32_t *)header;
    int err = 0;
    int i;

    for (i = 0; i < 8; ++i) {
        uint32_t val;

        err = parse_entry(handle, &val);
        if (err != 0)
            break;

        buf[i] = val;
    }

    if (err == INT_MAX)
        err = -ENODATA;

    return err;
}

static int parse_payload(FILE *handle, unsigned int quads, uint32_t *blob)
{
    int err = 0;
    int i;

    for (i = 0; i < quads; ++i) {
        uint32_t val;

        err = parse_entry(handle, &val);
        if (err != 0)
            break;

        blob[i] = val;
    }

    if (i != quads || err == INT_MAX)
        err = -ENODATA;

    return err;
}

static int check_crc32(struct file_cntr *cntr)
{
    uint32_t blob_crc32;

    blob_crc32 = crc32(0ul, (const uint8_t *)cntr->payload.blob,
                       cntr->payload.count * sizeof(*cntr->payload.blob));
    if (blob_crc32 != cntr->header.blob_crc32)
        return -EINVAL;

    return 0;
}

static int check_checksum(struct file_cntr *cntr)
{
    uint32_t checksum = 0;
    int i, j;

    for (i = 0; i < cntr->payload.count; ++i) {
        for (j = 3; j >= 0; --j)
            checksum += (cntr->payload.blob[i] >> (j * 8)) & 0xff;
    }

    if (checksum != cntr->header.blob_checksum)
        return -EINVAL;

    return 0;
}

int file_cntr_parse(struct file_cntr *cntr, const char *filepath)
{
    FILE *handle;
    char buf[16];
    size_t till_data;
    int err = 0;

    handle = fopen(filepath, "r");
    if (handle == NULL)
        return -errno;

    // Check magic bytes.
    if (fgets(buf, sizeof(buf), handle) != buf) {
        if (ferror(handle))
            err = -errno;
        else
            err = -ENODATA;
        goto end;
    }

    if (memcmp(buf, MAGIC_BYTES, sizeof(MAGIC_BYTES))) {
        err = -EPROTO;
        goto end;
    }

    // Parse header.
    err = parse_header(handle, &cntr->header);
    if (err < 0)
        goto end;

    // Skip to area for data.
    till_data = (PAYLOAD_OFFSET_QUADS - 7) * 12;
    if (fseek(handle, till_data, SEEK_CUR) < 0) {
        err = -errno;
        goto end;
    }

    cntr->payload.blob = calloc(cntr->header.blob_quads, sizeof(*cntr->payload.blob));
    if (cntr->payload.blob == NULL) {
        err = -ENOMEM;
        goto end;
    }
    cntr->payload.count = cntr->header.blob_quads;

    err = parse_payload(handle, cntr->header.blob_quads, cntr->payload.blob);
    if (err < 0) {
        free(cntr->payload.blob);
        goto end;
    }

    err = check_crc32(cntr);
    if (err < 0) {
        free(cntr->payload.blob);
        goto end;
    }

    err = check_checksum(cntr);
    if (err < 0)
        free(cntr->payload.blob);
end:
    fclose(handle);

    return err;
}

void file_cntr_release(struct file_cntr *cntr)
{
    if (cntr->payload.blob != NULL)
        free(cntr->payload.blob);
    cntr->payload.blob = NULL;
}
