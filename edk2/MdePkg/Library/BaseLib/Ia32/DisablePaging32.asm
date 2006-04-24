;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   DisablePaging32.Asm
;
; Abstract:
;
;   AsmDisablePaging32 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .386
    .model  flat,C
    .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86DisablePaging32 (
;   IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
;   IN      VOID                      *Context1,    OPTIONAL
;   IN      VOID                      *Context2,    OPTIONAL
;   IN      VOID                      *NewStack
;   );
;------------------------------------------------------------------------------
InternalX86DisablePaging32    PROC
    mov     ebx, [esp + 4]
    mov     ecx, [esp + 8]
    mov     edx, [esp + 12]
    pushfd
    pop     edi
    cli
    mov     eax, cr0
    btr     eax, 31
    mov     esp, [esp + 16]
    mov     cr0, eax
    push    edi
    popfd
    push    edx
    push    ecx
    call    ebx
    jmp     $
InternalX86DisablePaging32    ENDP

    END
