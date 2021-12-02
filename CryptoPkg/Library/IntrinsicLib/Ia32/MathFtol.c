/** @file
  64-bit Math Worker Function.
  The 32-bit versions of C compiler generate calls to library routines
  to handle 64-bit math. These functions use non-standard calling conventions.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
 * Floating point to integer conversion.
 */
__declspec(naked) void
_ftol2 (
  void
  )
{
  _asm {
    fistp qword ptr [esp-8]
    mov   edx, [esp-4]
    mov   eax, [esp-8]
    ret
  }
}
