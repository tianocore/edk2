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
;   LongJump.Asm
;
; Abstract:
;
;   Implementation of _LongJump() on x64.
;
;------------------------------------------------------------------------------

    .code

_LongJump    PROC
    mov     rbx, [rcx]
    mov     rsp, [rcx + 8]
    mov     rbp, [rcx + 10h]
    mov     rdi, [rcx + 18h]
    mov     rsi, [rcx + 20h]
    mov     r12, [rcx + 28h]
    mov     r13, [rcx + 30h]
    mov     r14, [rcx + 38h]
    mov     r15, [rcx + 40h]
    mov     rax, rdx
    jmp     qword ptr [rcx + 48h]
_LongJump    ENDP

    END
