;; @file
;  SEC CAR function
;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
