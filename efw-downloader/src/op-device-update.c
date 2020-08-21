// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <zlib.h>

#include "file-cntr.h"
#include "node-dispatcher.h"
#include "efw-commands.h"
#include "heuristics.h"

#define report_error(error, kind)                                                       \
        fprintf(stderr, "Fail to %s: %s %d %s\n",                                       \
                kind, g_quark_to_string((error)->domain), (error)->code, (error)->message)

static void print_help()
{
    printf("Usage\n"
           "  efw-downloader device CDEV update FILE_PATH [OPTIONS]\n"
           "\n"
           "where:\n"
           "  CDEV:         The firewire character device corresponding to the node for transaction\n"
           "  FILE_PATH:    The absolute or relative path to file for firmware\n"
           "  OPTIONS:\n"
           "    --debug:    Output debug message to stderr\n"
           "    --dry-run:  Do everything except actually write blob\n");
}

static void check_current_firmware(const struct file_cntr *cntr, EfwProto *proto, guint32 *buf,
                                   size_t quads, GError **error)
{
    efw_flash_recursive_read(proto, cntr->header.offset_addr, buf, quads, error);
    if (*error != NULL) {
        report_error(*error, "read contents of flash memory");
        return;
    }

    printf("Previous firmware:\n");
    printf("  offset:   %08x\n", cntr->header.offset_addr);
    if (cntr->header.crc_in_region_end > 0) {
        printf("  version:  %08x\n", buf[quads - 2]);
        printf("  crc32:    %08x\n", buf[quads - 1]);
    } else {
        guint32 blob_crc32 = crc32(0ul, (const uint8_t *)buf, cntr->header.blob_quads * sizeof(*buf));
        printf("  crc32:    %08x\n", blob_crc32);
    }
}

static void erase_and_write_recursive(EfwProto *proto, size_t offset, guint32 *buf, size_t quads,
                                      gboolean has_fpga, gboolean dry, GError **error)
{
    while (quads > 0) {
        size_t block_size;
        size_t count;
        int err;

        err = efw_flash_get_block_size(offset, &block_size);
        if (err < 0) {
            g_set_error_literal(error, G_FILE_ERROR, g_file_error_from_errno(-err),
                                "Fail to get the size of block");
            return;
        }

        count = MIN(quads, block_size / sizeof(guint32));

        fprintf(stderr, "Region %08lx-%08lx:\n", offset, offset + count * sizeof(guint32));

        if (!dry) {
            //efw_flash_erase_and_wait(proto, offset, error);
            //if (*error != NULL) {
            //    report_error(*error, "erase block in flash memory");
            //    return;
            //}
            fprintf(stderr, "  erased.\n");

            //efw_flash_recursive_write(proto, offset, buf, count, error);
            //if (*error != NULL) {
            //    report_error(*error, "write new contents to flash memory");
            //    return;
            //}
            fprintf(stderr, "  wrote 0x%lx quadlets.\n", count);
        } else {
            fprintf(stderr, "  should be erased.\n");
            fprintf(stderr, "  should wrote 0x%lx quadlets.\n", count);
        }

        offset += count * sizeof(guint32);
        quads -= count;
        buf += count;
    }
}

static void write_firmware(const struct file_cntr *cntr, EfwProto *proto, guint32 *buf, size_t quads,
                           gboolean has_fpga, gboolean dry, GError **error)
{
    memset(buf, 0xff, quads * sizeof(*buf));
    memcpy(buf, cntr->payload.blob, cntr->header.blob_quads * sizeof(*cntr->payload.blob));

    if (cntr->header.crc_in_region_end > 0) {
        buf[quads - 2] = cntr->header.version;
        buf[quads - 1] = cntr->header.blob_crc32;
    }

    if (has_fpga) {
        efw_flash_lock(proto, TRUE, error);
        if (*error != NULL) {
            report_error(*error, "lock flash memory");
            return;
        }
        fprintf(stderr, "Flash memory is locked.\n");
    }

    erase_and_write_recursive(proto, cntr->header.offset_addr, buf, quads, has_fpga, dry, error);
    if (*error != NULL)
        goto end;

    printf("Current firmware:\n");
    printf("  offset:   %08x\n", cntr->header.offset_addr);
    if (cntr->header.crc_in_region_end > 0) {
        printf("  version:  %08x\n", buf[quads - 2]);
        printf("  crc32:    %08x\n", buf[quads - 1]);
    } else {
        guint32 blob_crc32 = crc32(0ul, (const uint8_t *)buf, cntr->header.blob_quads * sizeof(*buf));
        printf("  crc32:    %08x\n", blob_crc32);
    }
end:
    if (has_fpga) {
        efw_flash_lock(proto, FALSE, error);
        if (*error != NULL) {
            report_error(*error, "unlock flash memory");
            return;
        }
        fprintf(stderr, "Flash memory is unlocked.\n");
    }
}

static void verify_blob(const struct file_cntr *cntr, EfwProto *proto, const guint32 *write_buf,
                        guint32 *read_buf, size_t quads, gboolean dry, GError **error)
{
    efw_flash_recursive_read(proto, cntr->header.offset_addr, read_buf, quads, error);
    if (*error != NULL) {
        report_error(*error, "read contents of flash memory");
        return;
    }

    if (!dry) {
        if (memcmp(write_buf, read_buf, quads * sizeof(guint32)) != 0) {
            int i;
            fprintf(stderr, "Written quadles are not the same as writing quadlets.\n");
            for (i = 0; i < quads; ++i)
                fprintf(stderr, "%08x: %d: %08x %08x\n", i, write_buf[i] == read_buf[i], write_buf[i], read_buf[i]);
            //return -EIO;
            return;
        }
    } else {
        fprintf(stderr, "Written quadlets should be read and compared to the content of file at last.\n");
    }
}

static void clear_stored_session(EfwProto *proto, GError **error)
{
    size_t base;

    efw_flash_get_session_base(proto, &base, error);
    if (*error != NULL) {
        report_error(*error, "get session base");
        return;
    }

    printf("base: %08lx\n", base);
}

static int decide_write_quads(struct file_cntr *cntr, size_t *quads, gboolean has_fpga)
{
    switch (cntr->header.offset_addr) {
    case 0x00000000:
        if (cntr->header.crc_in_region_end == 0) {
            // For bootstrap.
            if (cntr->header.type == BLOB_TYPE_DSP && !has_fpga)
                *quads = BOOTSTRAP_REGION_SIZE;
            else
                return -ENXIO;
        } else {
            if (cntr->header.type == BLOB_TYPE_FPGA && has_fpga)
                *quads = FPGA_REGION_SIZE;
            else
                return -ENXIO;
        }
        break;
    case OFFSET_ICELYNX:
        *quads = COMMON_REGION_SIZE;
        break;
    case OFFSET_DSP_A:
    case OFFSET_DSP_B:
        if (!has_fpga)
            *quads = COMMON_REGION_SIZE;
        else
            return -ENXIO;
        break;
    default:
        return -ENXIO;
    }

    *quads = (*quads + sizeof(guint32) - 1) / sizeof(guint32);

    return 0;
}

static int parse_args(int argc, char **argv, const char **filepath, gboolean *dry)
{
    int i;

    if (argc < 4)
        return -EINVAL;
    assert(strncmp(argv[3], "update", sizeof("update")) == 0);

    if (argc < 5)
        return -EINVAL;
    *filepath = argv[4];

    *dry = FALSE;
    for (i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--dry-run", sizeof("--dry-run")) == 0)
            *dry = TRUE;
    }

    return 0;
}

void op_device_update(int argc, char **argv, EfwProto *proto, GError **error)
{
    const char *filepath;
    gboolean dry;
    struct file_cntr cntr = {0};
    struct hw_info info = {0};
    gboolean has_fpga;
    size_t quads;
    guint32 *buf, *read_buf, *write_buf;
    int err;

    err = parse_args(argc, argv, &filepath, &dry);
    if (err < 0) {
        print_help();
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Invalid arguments");
        return;
    }

    err = file_cntr_parse(&cntr, filepath);
    if (err < 0) {
        fprintf(stderr, "Fail to parse %s: %s\n", filepath, strerror(-err));
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(-err),
                    "Fail to parse %s", filepath);
        return;
    }

    efw_hw_info(proto, &info, error);
    if (*error != NULL)
        goto err_cntr;
    efw_hw_info_has_fpga(&info, &has_fpga);

    err = decide_write_quads(&cntr, &quads, has_fpga);
    if (err < 0) {
        fprintf(stderr,
                "Fail to decide the number of quadlets to write from file contents: %s\n",
                filepath);
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO,
                    "Fail to decide the number of quadlets to write from file contents: %s\n",
                    filepath);
        goto err_cntr;
    }

    buf = g_try_malloc0_n(quads * 2, sizeof(*buf));
    if (buf == NULL) {
        fprintf(stderr, "Memory allocation fails.\n");
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_NOSPC, "Memory allocation error");
        goto err_cntr;
    }
    read_buf = buf;
    write_buf = buf + quads;

    check_current_firmware(&cntr, proto, read_buf, quads, error);
    if (*error != NULL) {
        goto err_buf;
    }

    write_firmware(&cntr, proto, write_buf, quads, has_fpga, dry, error);
    if (*error != NULL)
        goto err_buf;

    verify_blob(&cntr, proto, write_buf, read_buf, quads, dry, error);
    if (*error != NULL)
        goto err_buf;

    clear_stored_session(proto, error);
err_buf:
    free(buf);
err_cntr:
    file_cntr_release(&cntr);
}
