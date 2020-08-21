// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#ifndef __NODE_DISPATCHER_H__
#define __NODE_DISPATCHER_H__

#include <glib.h>
#include <glib-object.h>

#include <libhinawa/fw_node.h>
#include <libhinawa/fw_resp.h>

struct node_dispatcher {
    GMainContext *ctx;
    GMainLoop *loop;
    GThread *th;
};

void node_dispatcher_start(struct node_dispatcher *dispatcher, HinawaFwNode *node, GError **error);
void node_dispatcher_stop(struct node_dispatcher *dispatcher);

#endif
