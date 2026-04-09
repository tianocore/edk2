;; @file
;  This is the code that performs early platform initialization.
;  It consumes the reset vector, configures the stack.
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

;
; Define assembler characteristics
;

extern   ASM_PFX(TempRamInitApi)

SECTION .text

%macro RET_RSI  0

  movd    rsi, mm7                      ; restore RSI from MM7
  jmp     rsi

%endmacro

;
; Perform early platform initialization
;
global ASM_PFX(SecPlatformInit)
ASM_PFX(SecPlatformInit):

  RET_RSI

