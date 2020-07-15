;------------------------------------------------------------------------------
;
; Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Entry point for the coreboot UEFI payload.
;
;------------------------------------------------------------------------------

SECTION .text

; C Functions
extern  ASM_PFX(SecStartup)

; Pcds
extern  ASM_PFX(PcdGet32 (PcdPayloadFdMemBase))
extern  ASM_PFX(PcdGet32 (PcdPayloadStackTop))

;
; SecCore Entry Point
;
; Processor is in flat protected mode
;
; @param[in]  EAX   Initial value of the EAX register (BIST: Built-in Self Test)
; @param[in]  DI    'BP': boot-strap processor, or 'AP': application processor
; @param[in]  EBP   Pointer to the start of the Boot Firmware Volume
;
; @return     None  This routine does not return
;
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  ;
  ; Disable all the interrupts
  ;
  cli

  ;
  ; Save the Payload HOB base address before switching the stack
  ;
  mov     eax, [esp + 4]

  ;
  ; Construct the temporary memory at 0x80000, length 0x10000
  ;
  mov     esp, DWORD [ASM_PFX(PcdGet32 (PcdPayloadStackTop))]

  ;
  ; Push the Payload HOB base address onto new stack
  ;
  push    eax

  ;
  ; Pass BFV into the PEI Core
  ;
  push    DWORD [ASM_PFX(PcdGet32 (PcdPayloadFdMemBase))]

  ;
  ; Pass stack base into the PEI Core
  ;
  push    BASE_512KB

  ;
  ; Pass stack size into the PEI Core
  ;
  push    SIZE_64KB

  ;
  ; Pass Control into the PEI Core
  ;
  call    ASM_PFX(SecStartup)

  ;
  ; Should never return
  ;
  jmp     $

