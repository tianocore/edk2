;------------------------------------------------------------------------------
;*
;* Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
;* SPDX-License-Identifier: BSD-2-Clause-Patent
;*
;*
;------------------------------------------------------------------------------

DEFAULT REL
SECTION .text

%define TDVMCALL_EXPOSE_REGS_MASK       0xffec
%define TDVMCALL                        0x0
%define TDVMCALL_MAPGPA                 0x10001
%define TDVMCALL_STATUS_RETRY           0x1

%macro tdcall 0
    db 0x66,0x0f,0x01,0xcc
%endmacro

%macro tdcall_push_regs 0
    push rbp
    mov  rbp, rsp
    push r15
    push r14
    push r13
    push r12
    push rbx
    push rsi
    push rdi
%endmacro

%macro tdcall_pop_regs 0
    pop rdi
    pop rsi
    pop rbx
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
%endmacro

%macro tdcall_regs_preamble 2
    mov rax, %1

    xor rcx, rcx
    mov ecx, %2

    ; R10 = 0 (standard TDVMCALL)

    xor r10d, r10d

    ; Zero out unused (for standard TDVMCALL) registers to avoid leaking
    ; secrets to the VMM.

    xor ebx, ebx
    xor esi, esi
    xor edi, edi

    xor edx, edx
    xor ebp, ebp
    xor r8d, r8d
    xor r9d, r9d
%endmacro

%macro tdcall_regs_postamble 0
    xor ebx, ebx
    xor esi, esi
    xor edi, edi

    xor ecx, ecx
    xor edx, edx
    xor r8d,  r8d
    xor r9d,  r9d
    xor r10d, r10d
    xor r11d, r11d
%endmacro

;------------------------------------------------------------------------------
; 0   => RAX = TDCALL leaf
; M   => RCX = TDVMCALL register behavior
; 1   => R10 = standard vs. vendor
; 0xa => R11 = TDVMCALL function / MapGPA
; RCX => R12 = p1
; RDX => R13 = p2

;  UINT64
;  EFIAPI
;  TdVmCallMapGPA (
;    UINT64  Address,  // Rcx
;    UINT64  Length,   // Rdx
;    UINT64  *Results  // r8
;    )
global ASM_PFX(TdVmCallMapGPA)
ASM_PFX(TdVmCallMapGPA):
       tdcall_push_regs

       mov r11, TDVMCALL_MAPGPA
       mov r12, rcx
       mov r13, rdx

       push r8

       tdcall_regs_preamble TDVMCALL, TDVMCALL_EXPOSE_REGS_MASK

       tdcall

       ; ignore return dataif TDCALL reports failure.
       test rax, rax
       jnz .no_return_data

       ; Propagate TDVMCALL success/failure to return value.
       mov rax, r10

       ; Retrieve the Val pointer.
       pop r8
       test r8, r8
       jz .no_return_data

       ; On Retry, propagate TDVMCALL output value to output param
       cmp  rax, TDVMCALL_STATUS_RETRY
       jnz .no_return_data
       mov [r8], r11
.no_return_data:
       tdcall_regs_postamble

       tdcall_pop_regs

       ret
