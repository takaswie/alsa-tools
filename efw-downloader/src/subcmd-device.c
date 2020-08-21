// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "efw-proto.h"
#include "config-rom.h"
#include "node-dispatcher.h"

#define report_error(error, msg)                                                    \
        fprintf(stderr, "Fail to %s: %s %d %s\n",                                   \
                msg, g_quark_to_string(error->domain), error->code, error->message)

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

static void handle_responded_signal(EfwProto *proto, HinawaSndEfwStatus status, guint seqnum,
                      guint category, guint command, const guint32 *frame, guint frame_size)
{
    gchar *label = g_enum_to_string(HINAWA_TYPE_SND_EFW_STATUS, status);
    int i;

    printf("responded:\n");
    printf("  status: %s\n", label);
    printf("  seqnum: %d\n", seqnum);
    printf("  category; %d\n", category);
    printf("  command: %d\n", command);

    if (frame_size > 0) {
        printf("  frame:\n");
        for (i = 0; i < frame_size; ++i)
            printf("  [%3x]: %08x\n", i, frame[i]);
    }

    g_free(label);
}

static int parse_args(int argc, char **argv, const char **path, const char **op_name, gboolean *debug)
{
    int i;

    if (argc < 2)
        return -EINVAL;
    assert(strncmp(argv[1], "device", sizeof("device")) == 0);

    if (argc < 3)
        return -EINVAL;
    *path = argv[2];

    if (argc < 4)
        return -EINVAL;
    *op_name = argv[3];

    *debug = FALSE;
    for (i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--debug", sizeof(--debug)) == 0 ||
            strncmp(argv[i], "-d", sizeof(--debug)) == 0) {
            *debug = TRUE;
            break;
        }
    }

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
    gboolean debug;
    const char *path;
    const char *op_name;
    HinawaFwNode *node;
    const guint8 *rom;
    gsize length;
    guint32 vendor_id, model_id;
    EfwProto *proto;
    struct node_dispatcher dispatcher = {0};
    int err;
    int i;

    err = parse_args(argc, argv, &path, &op_name, &debug);
    if (err < 0)
        return print_help(0, NULL, NULL, NULL);

    for (i = 0; i < G_N_ELEMENTS(entries); ++i) {
        entry = entries + i;
        if (strncmp(op_name, entry->name, entry->size) == 0)
            break;
    }
    if (i == G_N_ELEMENTS(entries))
        return print_help();

    node = hinawa_fw_node_new();
    hinawa_fw_node_open(node, path, &error);
    if (error != NULL) {
        if (g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            fprintf(stderr, "File not found: %s\n", path);
        else if (g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_ACCES))
            fprintf(stderr, "Permission denied: %s\n", path);
        else
            report_error(error, "open the node");
        goto err;
    }

    hinawa_fw_node_get_config_rom(node, &rom, &length, &error);
    if (error != NULL) {
        report_error(error, "get config rom");
        goto err_node;
    }

    if (!config_rom_detect_vendor_and_model(rom, &vendor_id, &model_id)) {
        fprintf(stderr, "The node is not for Fireworks device: %s\n", path);
        g_set_error_literal(&error, G_FILE_ERROR, g_file_error_from_errno(ENXIO),
                            "The node is not for Fireworks device");
        goto err_node;
    }

    proto = efw_proto_new();
    if (debug)
        g_signal_connect(proto, "responded", (GCallback)handle_responded_signal, NULL);

    efw_proto_bind(proto, node, &error);
    if (error != NULL) {
        if (g_error_matches(error, HINAWA_FW_NODE_ERROR, HINAWA_FW_NODE_ERROR_FAILED)) {
            if (strstr(error->message, "16") != NULL) {
                fprintf(stderr, "The range of address on 1394 OHCI controller already used by "
                        "ALSA fireworks driver.\n");
            } else {
                report_error(error, "bind protocol");
            }
        } else {
            report_error(error, "bind protocol");
        }
        goto err_node;
    }

    node_dispatcher_start(&dispatcher, node, &error);
    if (error != NULL) {
        report_error(error, "begin dispatcher");
        goto err_proto;
    }

    entry->op(argc, argv, proto, &error);

    node_dispatcher_stop(&dispatcher);
err_proto:
    efw_proto_unbind(proto);
    g_object_unref(proto);
err_node:
    g_object_unref(node);
err:
    if (error != NULL) {
        g_clear_error(&error);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
