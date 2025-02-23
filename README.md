[![Build](https://github.com/OlekMazur/dsmtk/actions/workflows/makefile.yml/badge.svg)](https://github.com/OlekMazur/dsmtk/actions/workflows/makefile.yml)

Microcontroller Tool Kit for DS89C4X0
=====================================

Copyright © 2005 Aleksander Mazur

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the [GNU General Public License]
along with this program.  If not, see <https://www.gnu.org/licenses/>.

Description
-----------

This program is PC-side software for programming Dallas
Semiconductor's Ultra-High-Speed Flash Microcontrollers
DS89C420/430/440/450. It is basically a terminal program, giving
access to boot-rom functions in programming mode. It also provides
functions for loading/verifying flash memory, encryption vector and
external memory, as well as grabbing output to a file.
See application notes for information on how to build an interface
board for In-System Programming.
This program is portable, open-source supplement to software available
at Maxim's website, which runs only on Microsoft Windows. It is
written in C and depends on GNU libc.
Before using this program, read at least section 15 ("Program
Loading") of "DS89C420 Ultra-High-Speed Flash Microcontroller User's
Guide".

dsload is a simple shell script that takes one argument - a name of
Intel hex file to be loaded into flash memory. When run, first it
clears flash by issuing 'K' command, and then loads new content from
given file.

Issue "dsmtk --help" to get list of available command-line options.

Examples
--------

Commands accepted by high-speed microcontroller are descripted in the
document mentioned above. You can list additional (built-in) functions
of dsmtk issuing /help command.

Example 1. Loading hex file into flash (program) memory

```dsload file.hex```

or (in dsmtk shell):

```
K
/load file.hex
```

All the following examples assume that dsmtk is running and connection
is estabilished.

Example 2. Display values of registers

```R```

output example:

```
R
LB:00 OCR:FF ACON:1F CKCON:01 P0:00 P1:FF P2:FF P3:FF FCNTL:B1
```

Example 3. Dump program memory from 0 to 40 (hex)

```D 0 40```

output example:

```
D 0 40

:200000000200FAFFFFFFFFFFFFFFFF0200F9FFFFFFFFFF020027FFFFFFFFFFFFFFFFFFFFD7
:20002000FFFFFF02002632C2A7B280D281C0E0C0F0E8C0E0EAC0E0C0D078A07400F67A008D
:01004000C2FD
:00000001FF
```

Example 4. Dump external (MOVX) memory from 0 to 40, grabbing output
to 'dump.hex' (input mixed with output, input is indented by 2 spaces)

```
  /grab dump.hex
Grabbing output to dump.hex
  DX 0 40
DX 0 40

:20000000000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1FF0
:20002000202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3FD0
:01004000407F
:00000001FF

>
  /grab
Wrote grab file
```

[GNU General Public License]: COPYING.md
