/** @file
  Implementation of SetJump() on IA-32.

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

VOID
EFIAPI
InternalAssertJumpBuffer (
  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
  );

_declspec (naked)
UINTN
EFIAPI
SetJump (
  OUT     BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
  )
{
  _asm {
    push    [esp + 4]
    call    InternalAssertJumpBuffer
    pop     ecx
    pop     ecx
    mov     edx, [esp]
    mov     [edx], ebx
    mov     [edx + 4], esi
    mov     [edx + 8], edi
    mov     [edx + 12], ebp
    mov     [edx + 16], esp
    mov     [edx + 20], ecx
    xor     eax, eax
    jmp     ecx
  }
}

