;------------------------------------------------------------------------------
; Return the current FPU rounding mode.
;
; MASM implementation of the flt_rounds function from NetBSD.
;
; Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

    .code

;_map  BYTE  1     ; round to nearest
;      BYTE  3     ; round to negative infinity
;      BYTE  2     ; round to positive infinity
;      BYTE  0     ; round to zero

;------------------------------------------------------------------------------
; int
; EFIAPI
; fpu_rmode( void );
;
; VC++ always creates space for 4 parameters on the stack, whether they are
; used or not.  We use one for temporary storage since the only variant of
; fnstcw saves to memory, NOT a register.
;------------------------------------------------------------------------------
internal_FPU_rmode    PROC
    fnstcw    [rsp + 8]       ; save 16-bit FPU Control Word
    mov       eax, [rsp + 8]  ; get the saved FPU Control Word
    shr       eax, 10
    and       rax, 3          ; index is only the LSB two bits in RAX
    ret                       ; Return rounding mode in RAX
internal_FPU_rmode    ENDP

    END
