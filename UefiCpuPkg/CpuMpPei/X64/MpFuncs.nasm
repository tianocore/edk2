;------------------------------------------------------------------------------ ;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   MpFuncs.nasm
;
; Abstract:
;
;   This is the assembly code for MP support
;
;-------------------------------------------------------------------------------

%include "MpEqu.inc"
DEFAULT REL
SECTION .text

global ASM_PFX(AsmInitializeGdt)
ASM_PFX(AsmInitializeGdt):
    push       rbp
    mov        rbp, rsp

    lgdt       [rcx]  ; update the GDTR

    sub        rsp, 0x10
    mov        rax, ASM_PFX(SetCodeSelectorFarJump)
    mov        [rsp], rax
    mov        rdx, LONG_MODE_CS
    mov        [rsp + 4], dx    ; get new CS
    jmp        far dword [rsp]  ; far jump with new CS
ASM_PFX(SetCodeSelectorFarJump):
    add        rsp, 0x10

    mov        rax, LONG_MODE_DS          ; get new DS
    mov        ds, ax
    mov        es, ax
    mov        fs, ax
    mov        gs, ax
    mov        ss, ax

    pop        rbp

  ret
