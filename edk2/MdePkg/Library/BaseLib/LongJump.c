/** @file
  Long Jump functions.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  LongJump.c

**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "BaseLibInternals.h"

/**
  Restores the CPU context that was saved with SetJump().

  Restores the CPU context from the buffer specified by JumpBuffer.
  This function never returns to the caller.
  Instead is resumes execution based on the state of JumpBuffer.

  If JumpBuffer is NULL, then ASSERT().
  For IPF CPUs, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().
  If Value is 0, then ASSERT().

  @param  JumpBuffer    A pointer to CPU context buffer.
  @param  Value         The value to return when the SetJump() context is restored.

**/
VOID
EFIAPI
LongJump (
  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer,
  IN      UINTN                     Value
  )
{
  InternalAssertJumpBuffer (JumpBuffer);
  ASSERT (Value != 0);

  InternalLongJump (JumpBuffer, Value);
}
