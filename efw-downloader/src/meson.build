# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Takashi Sakamoto

# Depends on glib-2.0 and gobject-2.0.
gobject = dependency('gobject-2.0',
  version: '>=2.34.0',
)

# Depends on libhinawa v2.1.0 or later.
hinawa = dependency('hinawa',
  version: '>=2.1',
)

zlib = dependency('zlib')

sources = [
  'main.c',
  'efw-proto.c',
  'config-rom.c',
  'node-dispatcher.c',
  'efw-commands.c',
  'file-cntr.c',
  'subcmd-device.c',
  'op-device-read.c',
  'subcmd-file.c',
  'op-file-parse.c',
]

headers = [
  'efw-proto.h',
  'config-rom.h',
  'node-dispatcher.h',
  'efw-commands.h',
  'file-cntr.h',
  'subcmds.h',
]

gnome = import('gnome')
marshallers = gnome.genmarshal('efw-proto-sigs-marshal',
  prefix: 'efw_proto_sigs_marshal',
  sources: 'efw-proto-sigs-marshal.list',
  stdinc: true,
)
sources += marshallers

executable('efw-downloader',
  sources: sources + headers,
  dependencies: [
    gobject,
    hinawa,
    zlib,
  ],
  install: true,
)
