;; @file
;  SEC CAR function
;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

;
; Define assembler characteristics
;

%macro RET_ESI 0

  movd    esi, mm7                      ; move ReturnAddress from MM7 to ESI
  jmp     esi

%endmacro

SECTION .text

;-----------------------------------------------------------------------------
;
;  Section:     SecCarInit
;
;  Description: This function initializes the Cache for Data, Stack, and Code
;
;-----------------------------------------------------------------------------
global ASM_PFX(SecCarInit)
ASM_PFX(SecCarInit):

  ;
  ; Set up CAR
  ;

  xor    eax, eax

SecCarInitExit:

  RET_ESI

