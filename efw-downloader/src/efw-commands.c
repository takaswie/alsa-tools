// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "efw-commands.h"

// Categories in Echo Audio Fireworks protocol.
#define CATEGORY_HW             0
#define CATEGORY_FLASH          1

// Commands in hardware category.
#define HW_CMD_INFO             0

// Commands in flash category.
#define FLASH_CMD_ERASE         0
#define FLASH_CMD_READ          1
#define FLASH_CMD_WRITE         2
#define FLASH_CMD_STATE         3
#define FLASH_CMD_SESSION_BASE  4
#define FLASH_CMD_LOCK          5

#define EFW_FLASH_FRAME_MAX_QUADS   64

// Between 0x00000000 - 0x00010000.
#define EFW_FLASH_BLOCK_SIZE_LOW    0x00002000
// Between 0x00010000 - 0x00200000.
#define EFW_FLASH_BLOCK_SIZE_HIGH   0x00010000

#define TIMEOUT 200

#define CAP_HAS_DSP     0x00000010
#define CAP_HAS_FPGA    0x00000020

void efw_hw_info(EfwProto *proto, struct hw_info *info, GError **error)
{
    gsize param_count = sizeof(*info) / sizeof(guint32);
    efw_proto_transaction(proto, CATEGORY_HW, HW_CMD_INFO, NULL, 0,
                          (guint32 *const *)&info, &param_count, TIMEOUT, error);
}

void efw_hw_info_has_fpga(struct hw_info *info, gboolean *has_fpga)
{
    *has_fpga = !!(info->flags & CAP_HAS_FPGA);
}

int efw_flash_get_block_size(size_t offset, size_t *block_size)
{
    if (offset < 0x00010000)
        *block_size = EFW_FLASH_BLOCK_SIZE_LOW;
    else if (offset < 0x00200000)
        *block_size = EFW_FLASH_BLOCK_SIZE_HIGH;
    else
        return -ENXIO;

    return 0;
}

void efw_flash_erase(EfwProto *proto, size_t offset, GError **error)
{
    size_t block_size;
    guint32 args[1] = {0};
    guint32 *params = (guint32 [1]){0};
    gsize param_count = 1;
    int err;

    err = efw_flash_get_block_size(offset, &block_size);
    if (err < 0) {
        g_set_error(error, EFW_PROTO_ERROR, -err,
                    "%s %d: %s", __FILE__, __LINE__, strerror(-err));
        return;
    }

    if (offset % block_size > 0) {
        g_set_error(error, EFW_PROTO_ERROR, EINVAL,
                    "%s %d: %s", __FILE__, __LINE__, strerror(EINVAL));
        return;
    }

    args[0] = offset;
    efw_proto_transaction(proto, CATEGORY_FLASH, FLASH_CMD_ERASE, args, G_N_ELEMENTS(args),
                          (guint32 *const *)&params, &param_count, TIMEOUT, error);
}

void efw_flash_erase_and_wait(EfwProto *proto, size_t offset, GError **error)
{
    efw_flash_erase(proto, offset, error);
    if (*error != NULL)
        return;

    while (TRUE) {
        gboolean state;
        struct timespec req = {
            .tv_sec = 0,
            .tv_nsec = 500000000,
        };

        efw_flash_state(proto, &state, error);
        if (state == TRUE)
            break;

        if (*error != NULL)
            g_clear_error(error);

        clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
    }
}

void efw_flash_read(EfwProto *proto, size_t offset, guint32 *buf, size_t quads, GError **error)
{
    guint32 args[2] = {0};
    guint32 *params = (guint32 [2 + EFW_FLASH_FRAME_MAX_QUADS]){0};
    gsize param_count = 2 + EFW_FLASH_FRAME_MAX_QUADS;

    if (quads > EFW_FLASH_FRAME_MAX_QUADS) {
        g_set_error(error, EFW_PROTO_ERROR, EINVAL,
                    "%s %d: %s", __FILE__, __LINE__, strerror(EINVAL));
        return;
    }

    args[0] = offset;
    args[1] = quads;

    efw_proto_transaction(proto, CATEGORY_FLASH, FLASH_CMD_READ, args, G_N_ELEMENTS(args),
                          (guint32 *const *)&params, &param_count, TIMEOUT, error);
    if (*error != NULL)
        return;

    if (params[0] != offset || params[1] != quads) {
        g_set_error(error, EFW_PROTO_ERROR, EIO,
                    "%s %d: %s", __FILE__, __LINE__, strerror(EIO));
        return;
    }

    memcpy(buf, (const void *)&params[2], quads * sizeof(*buf));
}

void efw_flash_recursive_read(EfwProto *proto, size_t offset, guint32 *buf, size_t quads, GError **error)
{
    while (quads > 0) {
        size_t count = MIN(quads, EFW_FLASH_FRAME_MAX_QUADS);

        efw_flash_read(proto, offset, buf, count, error);
        if (*error != NULL)
            return;

        offset += count * 4;
        quads -= count;
        buf += count;
    }
}

void efw_flash_write(EfwProto *proto, size_t offset, guint32 *buf, size_t quads, GError **error)
{
    guint32 args[2 + EFW_FLASH_FRAME_MAX_QUADS] = {0};
    guint32 *params = (guint32 [2]){0};
    gsize param_count = 2;

    if (quads > EFW_FLASH_FRAME_MAX_QUADS) {
        g_set_error(error, EFW_PROTO_ERROR, EINVAL,
                    "%s %d: %s", __FILE__, __LINE__, strerror(EINVAL));
        return;
    }

    args[0] = offset;
    args[1] = quads;
    memcpy((void *)&args[2], buf, quads * sizeof(*buf));

    efw_proto_transaction(proto, CATEGORY_FLASH, FLASH_CMD_WRITE, args, G_N_ELEMENTS(args),
                          (guint32 *const *)&params, &param_count, TIMEOUT, error);
}

void efw_flash_recursive_write(EfwProto *proto, size_t offset, guint32 *buf, size_t quads, GError **error)
{
    while (quads > 0) {
        size_t count = MIN(quads, EFW_FLASH_FRAME_MAX_QUADS);
        gboolean state;

        efw_flash_write(proto, offset, buf, count, error);
        if (*error != NULL)
            return;

        offset += count * 4;
        quads -= count;
        buf += count;

        while (TRUE) {
            struct timespec req = {
                .tv_sec = 0,
                .tv_nsec = 500000000,
            };

            efw_flash_state(proto, &state, error);
            if (state == TRUE)
                break;

            if (*error != NULL)
                g_clear_error(error);

            clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
        }
    }
}

void efw_flash_state(EfwProto *proto, gboolean *state, GError **error)
{
    gsize param_count = 0;
    efw_proto_transaction(proto, CATEGORY_FLASH, FLASH_CMD_STATE, NULL, 0, NULL, &param_count,
                          TIMEOUT, error);
    if (*error == NULL) {
        *state = TRUE;
    } else if (g_error_matches(*error, EFW_PROTO_ERROR, HINAWA_SND_EFW_STATUS_FLASH_BUSY)) {
        *state = FALSE;
        g_clear_error(error);
    }
}

void efw_flash_get_session_base(EfwProto *proto, size_t *offset, GError **error)
{
    guint32 *params = (guint32 [1]){0};
    gsize param_count = 1;

    efw_proto_transaction(proto, CATEGORY_FLASH, FLASH_CMD_SESSION_BASE, NULL, 0,
                          (guint32 *const *)&params, &param_count, TIMEOUT, error);
    if (*error != NULL)
        return;

    *offset = params[0];
}

// MEMO: Lock operation is additional to the combination of IceLynx Micro and FPGA.
void efw_flash_lock(EfwProto *proto, gboolean locked, GError **error)
{
    guint32 args[1] = {0};
    guint32 *params = (guint32 [1]){0};
    gsize param_count = 1;

    args[0] = locked;

    efw_proto_transaction(proto, CATEGORY_FLASH, FLASH_CMD_LOCK, args, G_N_ELEMENTS(args),
                          (guint32 *const *)&params, &param_count, TIMEOUT, error);
}
