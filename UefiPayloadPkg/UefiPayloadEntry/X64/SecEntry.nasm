;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2020, Intel Corporation. All rights reserved.<BR>
;*   SPDX-License-Identifier: BSD-2-Clause-Patent

;------------------------------------------------------------------------------

#include <Base.h>

DEFAULT REL
SECTION .text

extern ASM_PFX(PayloadEntry)
extern  ASM_PFX(PcdGet32 (PcdPayloadStackTop))

;
; SecCore Entry Point
;
; Processor is in flat protected mode

global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

  ;
  ; Disable all the interrupts
  ;
  cli


  mov   rsp, FixedPcdGet32 (PcdPayloadStackTop)

  ;
  ; Push the bootloader parameter address onto new stack
  ;
  push  rcx
  mov   rax, 0
  push  rax ; shadow space
  push  rax
  push  rax
  push  rax

  ;
  ; Call into C code
  ;
  call  ASM_PFX(PayloadEntry)
  jmp   $

