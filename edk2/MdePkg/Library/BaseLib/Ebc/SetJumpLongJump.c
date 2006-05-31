/** @file
  Switch Stack functions. 

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  SetJumpLongJump.c

**/



UINTN
EFIAPI
SetJump (
  IN BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
  )
{
  ASSERT (JumpBuffer != NULL);
  ASSERT (FALSE);
  return 0;
}

VOID
EFIAPI
LongJump (
  IN BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer,
  IN      UINTN                Value
  )
{
  ASSERT (JumpBuffer != NULL);
  ASSERT (Value != 0);
  ASSERT (FALSE);
}
