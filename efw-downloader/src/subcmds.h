// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Takashi Sakamoto
#ifndef __SUBCMDS_H__
#define __SUBCMDS_H__

#include "efw-proto.h"
#include "file-cntr.h"

int subcmd_device(int argc, char **argv);
int subcmd_file(int argc, char **argv);

void op_device_read(int argc, char **argv, EfwProto *proto, GError **error);
void op_device_detect(int argc, char **argv, EfwProto *proto, GError **error);
void op_device_update(int argc, char **argv, EfwProto *proto, GError **error);

int op_file_parse(int argc, char **argv, struct file_cntr *cntr);

#endif
