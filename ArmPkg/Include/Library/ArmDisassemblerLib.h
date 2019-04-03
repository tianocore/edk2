/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ARM_DISASSEBLER_LIB_H__
#define __ARM_DISASSEBLER_LIB_H__

/**
  Place a dissasembly of of **OpCodePtr into buffer, and update OpCodePtr to
  point to next instructin.

  We cheat and only decode instructions that access
  memory. If the instruction is not found we dump the instruction in hex.

  @param  OpCodePtrPtr  Pointer to pointer of ARM Thumb instruction to disassemble.
  @param  Thumb         TRUE for Thumb(2), FALSE for ARM instruction stream
  @param  Extended      TRUE dump hex for instruction too.
  @param  ItBlock       Size of IT Block
  @param  Buf           Buffer to sprintf disassembly into.
  @param  Size          Size of Buf in bytes.

**/
VOID
DisassembleInstruction (
  IN  UINT8     **OpCodePtr,
  IN  BOOLEAN   Thumb,
  IN  BOOLEAN   Extended,
  IN OUT UINT32 *ItBlock,
  OUT CHAR8     *Buf,
  OUT UINTN     Size
  );

#endif
