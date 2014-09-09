/** @file
  Default exception handler

  Copyright (c) 2014, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD
  License which accompanies this distribution.  The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/ArmDisassemblerLib.h>

/**
  Place a disassembly of of **OpCodePtr into buffer, and update OpCodePtr to
  point to next instruction.

  @param  OpCodePtrPtr  Pointer to pointer of instruction to disassemble.
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
  )
{
  // Not yet supported for AArch64.
  // Put error in the buffer as we have no return code and the buffer may be
  // printed directly so needs a '\0'.
  AsciiSPrint (Buf, Size, "AArch64 not supported");
  return;
}
