/** @file
  Implementation of SetJump() on IA-32.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "BaseLibInternals.h"

/**
  Worker function that checks ASSERT condition for JumpBuffer

  Checks ASSERT condition for JumpBuffer.

  If JumpBuffer is NULL, then ASSERT().
  For IPF CPUs, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().

  @param  JumpBuffer    A pointer to CPU context buffer.

**/
VOID
EFIAPI
InternalAssertJumpBuffer (
  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
  );

/**
  Saves the current CPU context that can be restored with a call to LongJump()
  and returns 0.

  Saves the current CPU context in the buffer specified by JumpBuffer and
  returns 0. The initial call to SetJump() must always return 0. Subsequent
  calls to LongJump() cause a non-zero value to be returned by SetJump().

  If JumpBuffer is NULL, then ASSERT().
  For IPF CPUs, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().

  @param  JumpBuffer  A pointer to CPU context buffer.

  @retval 0 Indicates a return from SetJump().

**/
_declspec (naked)
RETURNS_TWICE
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

    xor     eax, eax
    mov     [edx + 24], eax        ; save 0 to SSP

    mov     eax, [PcdGet32 (PcdControlFlowEnforcementPropertyMask)]
    test    eax, eax
    jz      CetDone
    _emit      0x0F
    _emit      0x20
    _emit      0xE0                ; mov     eax, cr4
    bt      eax, 23                ; check if CET is enabled
    jnc     CetDone

    mov     eax, 1
    _emit      0xF3
    _emit      0x0F
    _emit      0xAE
    _emit      0xE8                ; INCSSP EAX to read original SSP
    _emit      0xF3
    _emit      0x0F
    _emit      0x1E
    _emit      0xC8                ; READSSP EAX
    mov     [edx + 0x24], eax      ; save SSP

CetDone:

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

