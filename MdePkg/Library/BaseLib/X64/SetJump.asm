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
;   SetJump.Asm
;
; Abstract:
;
;   Implementation of SetJump() on x64.
;
;------------------------------------------------------------------------------

    .code

SetJump     PROC
    pop     rdx
    mov     [rcx], rbx
    mov     [rcx + 8], rsp
    mov     [rcx + 10h], rbp
    mov     [rcx + 18h], rdi
    mov     [rcx + 20h], rsi
    mov     [rcx + 28h], r12
    mov     [rcx + 30h], r13
    mov     [rcx + 38h], r14
    mov     [rcx + 40h], r15
    mov     [rcx + 48h], rdx
    xor     rax, rax
    jmp     rdx
SetJump     ENDP

    END
