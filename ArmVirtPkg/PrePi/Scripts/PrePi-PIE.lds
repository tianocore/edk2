/** @file

  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

SECTIONS
{
  PROVIDE(__reloc_base = .);

  . = PECOFF_HEADER_SIZE;
  .text : ALIGN(CONSTANT(COMMONPAGESIZE)) {
    *(.text .text*)
    *(.got .got*)
    *(.rodata .rodata*)
    *(.data .data*)
    *(.bss .bss*)

    . = ALIGN(0x20);
    PROVIDE(__reloc_start = .);
    *(.rel .rel.*)
    *(.rela .rela.*)
    PROVIDE(__reloc_end = .);
  }

  .note (INFO) : { *(.note.gnu.build-id) }

  /DISCARD/ : {
    *(.note.GNU-stack)
    *(.gnu.hash)
    *(.gnu_debuglink)
    *(.interp)
    *(.dynamic)
    *(.dynsym)
    *(.dynstr)
    *(.hash)
    *(.comment)
  }
}
