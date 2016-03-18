;------------------------------------------------------------------------------
; Return the current FPU rounding mode.
;
; MASM implementation of the flt_rounds function by:
;   J.T. Conklin, Apr 4, 1995
;   Public domain.
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
; NetBSD: flt_rounds.S,v 1.6 1999/08/23 08:45:09 kleink Exp
;------------------------------------------------------------------------------

    .686
    .model  flat,C
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
;------------------------------------------------------------------------------

internal_FPU_rmode    PROC
    sub     esp, 4          ; Create a local variable for fnstcw
    fnstcw  [esp]
    mov     eax, [esp]
    shr     eax, 10
    and     eax, 3
    add     esp, 4          ; Delete the local variable
    ret
internal_FPU_rmode    ENDP

    END
