// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "subcmds.h"

static void print_help()
{
    printf("Usage\n"
           "  efw-downloader SUBCOMMAND OPTIONS\n"
           "\n"
           "where:\n"
           "  SUBCOMMAND:\n"
           "    device:     operate for device for unit on IEEE 1394 bus\n"
           "    help:       print help\n"
           "  OPTIONS:      optional arguments dependent on the subcommand\n");
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
    size_t size;
        int (*op)(int argc, char **argv);
    } *entry, entries[] = {
        { "device", sizeof("device"), subcmd_device },
    };
    const char *subcmd;
    int i;

    if (argc < 2) {
        print_help();
        return EXIT_FAILURE;
    }
    subcmd = argv[1];

    for (i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
        entry = entries + i;
        if (strncmp(subcmd, entry->name, entry->size) == 0) {
            entry = &entries[i];
            break;
        }
    }
    if (i == sizeof(entries) / sizeof(entries[0])) {
        print_help();
        return EXIT_FAILURE;
    }

    return entry->op(argc, argv);
}
