==============
efw-downloader
==============

2020/08/21
Takashi Sakamoto

Introduction
============

This tool is designed to operate on-board flash memory for devices with Fireworks board module. The
goal is to download firmware for the module.

At present, the tool can perform read operation from flash memory, and parse operation for firmware
file, therefore it's unlikely to make your device bricked.

Target devices
==============

* LOUD Audio, LLC (Mackie)

    * Onyx 400F
    * Onyx 1200F

* Echo Digital Audio corporation

    * Audiofire 12 (with DSP)
    * Audiofire 12 (with FPGA)
    * Audiofire 8 (with DSP)
    * Audiofire 8 (with FPGA)
    * Audiofire 2
    * Audiofire 4
    * Audiofire Pre8

* Gibson guitar corporation

    * Robot Interface Pack (RIP)

Background
==========

Echo Digital Audio corporation shipped Fireworks board module. This module is designed for audio and
music units on IEEE 1394 bus with rich features for direct monitoring, on-board routing, and so on.

The module consists of two combinations of two ICs:

* A combination:

    * Texus Instruments TSB43Cx43A (IceLynx Micro)
    * Texus Instruments TMS320C6713B (DSP)

* Another combination:

    * Texus Instruments TSB43Cx43A (IceLynx Micro)
    * Xilinx Spartan XC35250E (FPGA)

The module has on-board flash memory to store firmware for the above ICs and allows software to
read, erase, and write to the memory by Fireworks protocol.

For your information, Echo Digital Audio corporation have US patent. The patent describes that two
processors are used for data processing in IEEE 1394 bus, and for sample processing from/to DAC/ADC.
These two processors independently run via memory bank. I guess that IceLynx Micro is used for the
former role, and DSP/FPGA is used for the latter role.

Echo Digital Audio corporation provided a series of firmware blob to their customers and bundled the
firmware to driver package for Windows and macOS. The firmware blob has specific structure designed
by Echo Digital Audio corporation.

Prerequities
============

Dependencies for build and runtime
-------------------------------------

* libglib-2.0 and libgobject v2.34 or later. (https://gitlab.gnome.org/GNOME/glib)
* libhinawa v2.1 or later (https://github.com/alsa-project/libhinawa)
* zlib (https://zlib.net/)

For build
---------

* Meson Build system (https://mesonbuild.com/) is used.

For runtime
-----------

* ALSA fireworks driver uses the same address space on 1394 OHCI controller for Fireworks protocol,
  therefore it's better to unload the driver in advance of using the tool.
* FFADO applications should be stopped to avoid misfortune.

Build and Install
=================

::

    $ meson (--prefix=xxx, -Dman=false). build
    $ cd build
    $ ninja
    $ meson install

* The ``man`` meson option is to install online manual for the tool. Default is ``true``.

Instruction
===========

The tool consists of two sub commands; ``device`` and ``file``. The former is to operate on-board
flash memory, and the latter is to handle file of firmware blob.
::

    efw-downloader SUBCOMMAND OPTIONS ...

    SUBCOMMAND = device | file

    OPTIONS = ( depends on subcommand )

The ``device`` sub command consists of several operations. At present, ``read`` operation is just
supported but the other operations are planned to add for future release.
::

    efw-downloader device PATH OPERATION ARGUMENTS

    PATH   The path to special file for firewire character device corresponding to node of Fireworks board module.

    OPERATION
           One of read , and help operations.

    ARGUMENTS for read operation

           efw-downloader device PATH read OFFSET SIZE [ --debug | --help | -h ]

           The offset argument is the hexadecimal number of offset on flash memory.

           The size argument is the hexadecimal number of size to read, aligned to quadlet automatically.

           The optional --debug argument is to enable debug output for Fireworks protocol.

           The optional --help and -h arguments are for help message.

    ARGUMENTS for help operation

           efw-downloader device PATH help

           The help operation have no arguments.

The ``file`` sub command consists of several operations as well. At present, ``parse`` operation is
just supported but the other operations are planned to add for future release.
::

    efw-downloader file FILEPATH OPERATION ARGUMENTS

    FILEPATH
           The path to file of firmware blob included in driver package shipped by vendors.

    OPERATION
           One of parse , and help operations.

    ARGUMENTS for parse operation

           efw-downloader file FILEPATH parse [ --help | -h ]

           The --help and -h argument is for help message.

    ARGUMENTS for help operation

           efw-downloder file FILEPATH help

           The help operation have no arguments.
