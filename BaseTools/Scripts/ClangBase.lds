/** @file

  Unified linker script for CLANG based builds

  Copyright (c) 2010 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

SECTIONS {

  /*
   * The PE/COFF binary consists of DOS and PE/COFF headers, and a sequence of
   * section headers adding up to PECOFF_HEADER_SIZE bytes (which differs
   * between 32-bit and 64-bit builds). The actual start of the .text section
   * will be rounded up based on its actual alignment.
   */
  . = PECOFF_HEADER_SIZE;

  .text : ALIGN(CONSTANT(MAXPAGESIZE)) {
    *(.text .text.* .stub .gnu.linkonce.t.*)
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.got .got.*)

    /*
     * The contents of AutoGen.c files are mostly constant from the POV of the
     * program, but most of it ends up in .data or .bss by default since few of
     * the variable definitions that get emitted are declared as CONST.
     * Unfortunately, we cannot pull it into the .text section entirely, since
     * patchable PCDs are also emitted here, but we can at least move all of the
     * emitted GUIDs here.
     */
    *:AutoGen.obj(.data.g*Guid)
  }

  /*
   * The alignment of the .data section should be less than or equal to the
   * alignment of the .text section. This ensures that the relative offset
   * between these sections is the same in the ELF and the PE/COFF versions of
   * this binary.
   */
  .data ALIGN(ALIGNOF(.text)) : ALIGN(CONSTANT(MAXPAGESIZE)) {
    *(.data .data.* .gnu.linkonce.d.*)
    *(.bss .bss.*)
  }

  .eh_frame ALIGN(CONSTANT(MAXPAGESIZE)) : {
    KEEP (*(.eh_frame))
  }

  .rela (INFO) : {
    *(.rela .rela.*)
  }

  .hii : ALIGN(CONSTANT(MAXPAGESIZE)) {
    KEEP (*(.hii))
  }

  .got : {
    *(.got)
  }
  ASSERT(SIZEOF(.got) == 0, "Unexpected GOT entries detected!")

  .got.plt (INFO) : {
    *(.got.plt)
  }
  ASSERT(SIZEOF(.got.plt) == 0 || SIZEOF(.got.plt) == 0xc || SIZEOF(.got.plt) == 0x18, "Unexpected GOT/PLT entries detected!")

  /*
   * Retain the GNU build id but in a non-allocatable section so GenFw
   * does not copy it into the PE/COFF image.
   */
  .build-id (INFO) : { *(.note.gnu.build-id) }

  /DISCARD/ : {
    *(.note.GNU-stack)
    *(.gnu_debuglink)
    *(.interp)
    *(.dynsym)
    *(.dynstr)
    *(.hash .gnu.hash)
    *(.comment)
    *(COMMON)
  }
}
