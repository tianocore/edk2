;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2008, Intel Corporation
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

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86DisablePaging64 (
;   IN      UINT16                    Cs,
;   IN      UINT32                    EntryPoint,
;   IN      UINT32                    Context1,  OPTIONAL
;   IN      UINT32                    Context2,  OPTIONAL
;   IN      UINT32                    NewStack
;   );
;------------------------------------------------------------------------------
InternalX86DisablePaging64    PROC
    cli
    lea     r10, @F
    mov     esi, r8d
    mov     edi, r9d
    mov     eax, [rsp + 28h]            ; eax <- New Stack
    push    rcx                         ; push Cs to stack
    push    r10
    DB      48h                         ; prefix to composite "retq" with next "retf"
    retf                                ; Use far return to load CS register from stack
@@:
    mov     esp, eax                    ; set up new stack
    mov     rax, cr0
    btr     eax, 31
    mov     cr0, rax                    ; disable paging

    mov     rbx, rdx                    ; save EntryPoint to rbx, for rdmsr will overwrite rdx
    mov     ecx, 0c0000080h
    rdmsr
    and     ah, NOT 1                   ; clear LME
    wrmsr
    mov     rax, cr4
    and     al, NOT (1 SHL 5)           ; clear PAE
    mov     cr4, rax
    push    rdi                         ; push Context2
    push    rsi                         ; push Context1
    call    rbx                         ; transfer control to EntryPoint
    hlt                                 ; no one should get here
InternalX86DisablePaging64    ENDP

    END
