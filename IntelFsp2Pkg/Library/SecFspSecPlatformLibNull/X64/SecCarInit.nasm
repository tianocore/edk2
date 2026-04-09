;; @file
;  SEC CAR function
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

;
; Define assembler characteristics
;

%macro RET_RSI 0

  movd    rsi, mm7                      ; move ReturnAddress from MM7 to RSI
  jmp     rsi

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

  xor    rax, rax

SecCarInitExit:

  RET_RSI

