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
;   DisablePaging64.Asm
;
; Abstract:
;
;   AsmDisablePaging64 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

InternalX86DisablePaging64    PROC
    cli
    shl     rcx, 32
    lea     ecx, @F
    push    rcx
    mov     ebx, edx
    mov     esi, r8d
    mov     edi, r9d
    mov     eax, [rsp + 28h]
    retf
@@:
    mov     esp, eax                    ; set up new stack
    mov     rax, cr0
    btr     eax, 31
    mov     cr0, rax                    ; disable paging
    mov     ecx, 0c0000080h
    rdmsr
    and     ah, NOT 1                   ; clear LME
    wrmsr
    mov     rax, cr4
    and     al, NOT (1 SHL 5)           ; clear PAE
    mov     cr4, rax
    push    rdi
    push    rsi
    call    rbx
    jmp     $
InternalX86DisablePaging64    ENDP

    END
