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

sources = [
  'main.c',
  'efw-proto.c',
]

headers = [
  'efw-proto.h',
]

executable('efw-downloader',
  sources: sources + headers,
  dependencies: [
    gobject,
    hinawa,
  ],
  install: true,
)
