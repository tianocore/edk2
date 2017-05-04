;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;*
;*
;------------------------------------------------------------------------------

    SECTION .rodata
;
; Float control word initial value:
; all exceptions masked, double-extended-precision, round-to-nearest
;
mFpuControlWord: DW 0x37F
;
; Multimedia-extensions control word:
; all exceptions masked, round-to-nearest, flush to zero for masked underflow
;
mMmxControlWord: DD 0x1F80

DEFAULT REL
SECTION .text

;
; Initializes floating point units for requirement of UEFI specification.
;
; This function initializes floating-point control word to 0x027F (all exceptions
; masked,double-precision, round-to-nearest) and multimedia-extensions control word
; (if supported) to 0x1F80 (all exceptions masked, round-to-nearest, flush to zero
; for masked underflow).
;
global ASM_PFX(InitializeFloatingPointUnits)
ASM_PFX(InitializeFloatingPointUnits):

    ;
    ; Initialize floating point units
    ;
    finit
    fldcw   [mFpuControlWord]

    ;
    ; Set OSFXSR bit 9 in CR4
    ;
    mov     rax, cr4
    or      rax, BIT9
    mov     cr4, rax

    ldmxcsr [mMmxControlWord]

    ret

