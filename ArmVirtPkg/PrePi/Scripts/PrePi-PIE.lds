/** @file

  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
