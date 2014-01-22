;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
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


.const
;
; Float control word initial value: 
; all exceptions masked, double-extended-precision, round-to-nearest
;
mFpuControlWord       DW      037Fh
;
; Multimedia-extensions control word:
; all exceptions masked, round-to-nearest, flush to zero for masked underflow
;
mMmxControlWord       DD      01F80h 

.code


;
; Initializes floating point units for requirement of UEFI specification.
;
; This function initializes floating-point control word to 0x027F (all exceptions
; masked,double-precision, round-to-nearest) and multimedia-extensions control word
; (if supported) to 0x1F80 (all exceptions masked, round-to-nearest, flush to zero
; for masked underflow).
;
InitializeFloatingPointUnits PROC PUBLIC

    ;
    ; Initialize floating point units
    ;
    ; The following opcodes stand for instruction 'finit' 
    ; to be supported by some 64-bit assemblers
    ;
    DB      9Bh, 0DBh, 0E3h
    fldcw   mFpuControlWord
    
    ;
    ; Set OSFXSR bit 9 in CR4
    ;
    mov     rax, cr4
    or      rax, BIT9
    mov     cr4, rax

    ldmxcsr mMmxControlWord
    
    ret
InitializeFloatingPointUnits ENDP

END
