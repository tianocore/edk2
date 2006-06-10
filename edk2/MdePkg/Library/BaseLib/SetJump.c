/** @file
  Internal ASSERT () functions for SetJump.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  SetJumpLongJump.c

**/

VOID
EFIAPI
InternalAssertJumpBuffer (
  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
  )
{
  ASSERT (JumpBuffer != NULL);

#ifdef MDE_CPU_IPF
  ASSERT (((UINTN)JumpBuffer & 0xf) == 0);
#endif
}
