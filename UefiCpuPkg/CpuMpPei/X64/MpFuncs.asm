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
;   MpFuncs32.asm
;
; Abstract:
;
;   This is the assembly code for MP support
;
;-------------------------------------------------------------------------------

include  MpEqu.inc
.code


AsmInitializeGdt   PROC
    push       rbp
    mov        rbp, rsp

    lgdt       fword PTR [rcx]  ; update the GDTR

    sub        rsp, 0x10
    lea        rax, SetCodeSelectorFarJump
    mov        [rsp], rax
    mov        rdx, LONG_MODE_CS
    mov        [rsp + 4], dx    ; get new CS
    jmp        fword ptr [rsp]
SetCodeSelectorFarJump:
    add        rsp, 0x10

    mov        rax, LONG_MODE_DS          ; get new DS
    mov        ds, ax
    mov        es, ax
    mov        fs, ax
    mov        gs, ax
    mov        ss, ax

    pop        rbp
    ret
AsmInitializeGdt  ENDP

END
