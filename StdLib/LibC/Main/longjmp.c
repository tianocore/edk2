/** @file
  The longjmp function.
  The C standard requires that longjmp be a function and not a macro.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Library/BaseLib.h>
#include  <setjmp.h>

void longjmp(jmp_buf env, int val)
{
  LongJump(env, (UINTN)((val == 0) ? 1 : val));
}
