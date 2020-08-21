// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "efw-proto.h"

static int print_help()
{
    printf("Usage\n"
           "  efw-downloader device CDEV OPERATION ARGUMENTS\n"
           "\n"
           "where:\n"
           "  CDEV:   The firewire character device corresponding to the node for transaction\n"
           "  OPERATION:\n"
           "    help:   print this help message\n"
           "  ARGUMENTS:\n"
           "    depending on OPERATION\n"
           "\n");
    return EXIT_FAILURE;
}

static int parse_args(int argc, char **argv, const char **path, const char **op_name)
{
    if (argc < 2)
        return -EINVAL;
    assert(strncmp(argv[1], "device", sizeof("device")) == 0);

    if (argc < 3)
        return -EINVAL;
    *path = argv[2];

    if (argc < 4)
        return -EINVAL;
    *op_name = argv[3];

    return 0;
}

int subcmd_device(int argc, char **argv)
{
    struct {
        const char *name;
        size_t size;
        void (*op)(int argc, char **argv, EfwProto *proto, GError **error);
    } *entry, entries[] = {
    };
    GError *error = NULL;
    const char *path;
    const char *op_name;
    EfwProto *proto;
    int err;
    int i;

    err = parse_args(argc, argv, &path, &op_name);
    if (err < 0)
        return print_help(0, NULL, NULL, NULL);

    for (i = 0; i < G_N_ELEMENTS(entries); ++i) {
        entry = entries + i;
        if (strncmp(op_name, entry->name, entry->size) == 0)
            break;
    }
    if (i == G_N_ELEMENTS(entries))
        return print_help();

    entry->op(argc, argv, proto, &error);

    if (error != NULL) {
        g_clear_error(&error);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
