/** @file

  Unified linker script for GCC based builds

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

SECTIONS {

  /*
   * The PE/COFF binary consists of DOS and PE/COFF headers, and a sequence of
   * section headers adding up to PECOFF_HEADER_SIZE bytes (which differs
   * between 32-bit and 64-bit builds). The actual start of the .text section
   * will be rounded up based on its actual alignment.
   */
  . = PECOFF_HEADER_SIZE;

  .text : ALIGN(CONSTANT(COMMONPAGESIZE)) {
    *(.text .text.* .stub .gnu.linkonce.t.*)
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.got .got.*)

    /*
     * The contents of AutoGen.c files are constant from the POV of the program,
     * but most of its contents end up in .data or .bss by default since few of
     * the variable definitions that get emitted are declared as CONST.
     */
    *:AutoGen.obj(.data .data.* .bss .bss.*)
  }

  /*
   * The alignment of the .data section should be less than or equal to the
   * alignment of the .text section. This ensures that the relative offset
   * between these sections is the same in the ELF and the PE/COFF versions of
   * this binary.
   */
  .data ALIGN(ALIGNOF(.text)) : ALIGN(CONSTANT(COMMONPAGESIZE)) {
    *(.data .data.* .gnu.linkonce.d.*)
    *(.bss .bss.*)
  }

  .eh_frame ALIGN(CONSTANT(COMMONPAGESIZE)) : {
    KEEP (*(.eh_frame))
  }

  .rela ALIGN(CONSTANT(COMMONPAGESIZE)) : {
    *(.rela .rela.*)
  }

  /DISCARD/ : {
    *(.note.GNU-stack)
    *(.gnu_debuglink)
    *(.interp)
    *(.dynsym)
    *(.dynstr)
    *(.dynamic)
    *(.hash)
    *(.comment)
    *(COMMON)
  }
}
