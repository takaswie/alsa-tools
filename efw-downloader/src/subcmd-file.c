// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "subcmds.h"

static void print_help()
{
    printf("Usage\n"
           "  efw-downloader file FILEPATH OPERATION ARGUMENTS\n"
           "\n"
           "where:\n"
           "  FILEPATH: The path to file.\n"
           "  OPERATION:\n"
           "    help:   print this help message\n"
           "  ARGUMENTS:\n"
           "    depending on the OPERATION\n");
}

static int parse_args(int argc, char **argv, const char **filepath, const char **op_name)
{
    if (argc < 2)
        return -EINVAL;
    assert(strncmp(argv[1], "file", sizeof("file")) == 0);

    if (argc < 3)
        return -EINVAL;
    *filepath = argv[2];

    if (argc < 4)
        return -EINVAL;
    *op_name = argv[3];

    return 0;
}

int subcmd_file(int argc, char **argv)
{
    struct {
        const char *name;
        size_t size;
        int (*op)(int argc, char **argv, struct file_cntr *cntr);
    } *entry, entries[] = {
    };
    const char *op_name;
    const char *filepath;
    struct file_cntr cntr = {0};
    int err;
    int i;

    err = parse_args(argc, argv, &filepath, &op_name);
    if (err < 0) {
        print_help();
        return EXIT_FAILURE;
    }

    for (i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
        entry = entries + i;
        if (strncmp(op_name, entry->name, entry->size) == 0)
            break;
    }
    if (i == sizeof(entries) / sizeof(entries[0])) {
        print_help();
        return EXIT_FAILURE;
    }

    err = file_cntr_parse(&cntr, filepath);
    if (err < 0) {
        printf("Fail to parse: %s\n", strerror(-err));
        return EXIT_FAILURE;
    }

    err = entry->op(argc, argv, &cntr);

    file_cntr_release(&cntr);

    if (err < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
