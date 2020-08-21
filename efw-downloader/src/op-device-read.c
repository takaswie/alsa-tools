// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "efw-commands.h"

static void print_help()
{
    printf("Usage\n"
           "  efw-downloader device CDEV read OFFSET LENGTH [OPTIONS]\n"
           "\n"
           "where:\n"
           "  CDEV:   The firewire character device corresponding to the node for proto\n"
           "  OFFSET: The hexadecimal offset address in on-board flash memory\n"
           "  LENGTH: The hexadecimal number to read. The value is finally aligned to quadlet.\n"
           "  OPTIONS:\n"
           "    --help, -h: Print this help message and exit.\n"
           "    --debug:    Output debug message to stderr\n");
}

static int parse_args(int argc, char **argv, size_t *offset, size_t *quads, gboolean *help)
{
    unsigned long val;
    char *end;
    int i;

    if (argc < 4)
        return -EINVAL;
    assert(strncmp(argv[3], "read", sizeof("read")) == 0);

    if (argc < 5)
        return -EINVAL;
    val = strtol(argv[4], &end, 16);
    if (*end != '\0') {
        printf("Invalid argument for offset address.\n");
        return -EINVAL;
    }
    *offset = (size_t)val;

    if (argc < 6)
        return -EINVAL;
    val = strtol(argv[5], &end, 16);
    if (*end != '\0') {
        printf("Invalid argument for quadlet count.\n");
        return -EINVAL;
    }
    *quads = (size_t)(val + 3) / 4;

    *help = FALSE;
    for (i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--help", sizeof("--help")) == 0 ||
            strncmp(argv[i], "-h", sizeof("-h")) == 0) {
            *help = TRUE;
        }
    }

    return 0;
}

void op_device_read(int argc, char **argv, EfwProto *proto, GError **error)
{
    size_t offset;
    size_t quads;
    guint32 *buf;
    gboolean help;
    int err;
    int i;

    err = parse_args(argc, argv, &offset, &quads, &help);
    if (err < 0) {
        print_help();
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Invalid arguments");
        return;
    }

    if (help) {
        print_help();
        return;
    }

    buf = g_try_malloc0_n(quads, sizeof(*buf));
    if (buf == NULL) {
        fprintf(stderr, "Memory allocation fails.\n");
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_NOSPC, "Memory allocation error");
        return;
    }

    efw_flash_recursive_read(proto, offset, buf, quads, error);
    if (*error != NULL) {
        fprintf(stderr,
                "Fail to read contents of flash memory: %s %d %s\n",
                g_quark_to_string((*error)->domain), (*error)->code, (*error)->message);
        goto end;
    }

    for (i = 0; i < quads; ++i)
        printf("  %08lx: %08x\n", offset + 4 * i, buf[i]);
end:
    g_free(buf);
}
