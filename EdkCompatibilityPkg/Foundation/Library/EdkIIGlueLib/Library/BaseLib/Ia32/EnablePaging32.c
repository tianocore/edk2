/**
  AsmEnablePaging32 function

  Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BaseLibInternals.h"

__declspec (naked)
VOID
EFIAPI
InternalX86EnablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,    OPTIONAL
  IN      VOID                      *Context2,    OPTIONAL
  IN      VOID                      *NewStack
  )
{
  _asm {
    push    ebp
    mov     ebp, esp
    mov     ebx, EntryPoint
    mov     ecx, Context1
    mov     edx, Context2
    pushfd
    pop     edi
    cli
    mov     eax, cr0
    bts     eax, 31
    mov     esp, NewStack
    mov     cr0, eax
    push    edi
    popfd
    push    edx
    push    ecx
    call    ebx
    jmp     $
  }
}

