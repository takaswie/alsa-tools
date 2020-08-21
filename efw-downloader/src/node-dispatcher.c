// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#include "node-dispatcher.h"
#include <time.h>

struct thread_arg {
    GMainLoop *loop;
    GCond cond;
    GMutex mutex;
};

static gpointer run_node_dispatcher(gpointer data)
{
    struct thread_arg *args = (struct thread_arg *)data;

    g_mutex_lock(&args->mutex);
    g_cond_signal(&args->cond);
    g_mutex_unlock(&args->mutex);

    g_main_loop_run(args->loop);

    return NULL;
}

void node_dispatcher_start(struct node_dispatcher *dispatcher, HinawaFwNode *node, GError **error)
{
    struct thread_arg args;
    GSource *src;

    dispatcher->ctx = g_main_context_new();

    hinawa_fw_node_create_source(node, &src, error);
    if (*error != NULL)
        return;

    g_source_attach(src, dispatcher->ctx);
    g_source_unref(src);

    dispatcher->loop = g_main_loop_new(dispatcher->ctx, FALSE);

    args.loop = dispatcher->loop;
    g_cond_init(&args.cond);
    g_mutex_init(&args.mutex);

    dispatcher->th = g_thread_try_new("node-dispatcher", run_node_dispatcher, &args, error);
    if (*error != NULL) {
        g_main_loop_quit(dispatcher->loop);
        g_main_loop_unref(dispatcher->loop);
        dispatcher->loop = NULL;

        g_main_context_unref(dispatcher->ctx);
        dispatcher->ctx = NULL;

        goto end;
    }

    g_mutex_lock(&args.mutex);
    while (!g_main_loop_is_running(dispatcher->loop))
        g_cond_wait(&args.cond, &args.mutex);
    g_mutex_unlock(&args.mutex);
end:
    g_cond_clear(&args.cond);
    g_mutex_clear(&args.mutex);
}

void node_dispatcher_stop(struct node_dispatcher *dispatcher)
{
    if (dispatcher->loop != NULL)
        g_main_loop_quit(dispatcher->loop);

    if (dispatcher->th != NULL) {
        g_thread_join(dispatcher->th);
        g_thread_unref(dispatcher->th);
        dispatcher->th = NULL;
    }

    if (dispatcher->loop != NULL) {
        g_main_loop_unref(dispatcher->loop);
        dispatcher->loop = NULL;
    }

    if (dispatcher->ctx != NULL) {
        g_main_context_unref(dispatcher->ctx);
        dispatcher->ctx = NULL;
    }
}
