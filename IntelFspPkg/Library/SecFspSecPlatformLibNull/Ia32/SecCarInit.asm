;; @file
;  SEC CAR function
;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

;
; Define assembler characteristics
;
.586p
.xmm
.model flat, c

RET_ESI  MACRO

  movd    esi, mm7                      ; move ReturnAddress from MM7 to ESI
  jmp     esi

ENDM

.code 

;-----------------------------------------------------------------------------
;
;  Section:     SecCarInit
;
;  Description: This function initializes the Cache for Data, Stack, and Code
;
;-----------------------------------------------------------------------------
SecCarInit    PROC    NEAR    PUBLIC

  ;
  ; Set up CAR
  ;

  xor    eax, eax

SecCarInitExit:

  RET_ESI

SecCarInit    ENDP

END
