#/** @file
#
#  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/

SECTIONS
{
  .text 0x0 : {
    PROVIDE(__reloc_base = .);

    *(.text .text*)
    *(.got .got*)
    *(.rodata .rodata*)
    *(.data .data*)

    . = ALIGN(0x20);
    PROVIDE(__reloc_start = .);
    *(.rela .rela*)
    PROVIDE(__reloc_end = .);
  }
  .bss ALIGN(0x20) : { *(.bss .bss*) }

  /DISCARD/ : {
    *(.note.GNU-stack)
    *(.gnu_debuglink)
    *(.interp)
    *(.dynamic)
    *(.dynsym)
    *(.dynstr)
    *(.hash)
    *(.comment)
  }
}
