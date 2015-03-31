;------------------------------------------------------------------------------
;
; Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;  SecEntry.asm
;
; Abstract:
;
;  This is the code that begins in protected mode.
;  It will transfer the control to pei core.
;
;------------------------------------------------------------------------------
#include <Base.h>

.686p
.xmm
.model small, c

EXTRN   SecStartup:NEAR

; Pcds
EXTRN   PcdGet32 (PcdPayloadFdMemBase):DWORD

    .code

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

_ModuleEntryPoint   PROC PUBLIC 
  ;
  ; Disable all the interrupts
  ;   
  cli
  ;
  ; Construct the temporary memory at 0x80000, length 0x10000
  ;
  mov  esp, (BASE_512KB + SIZE_64KB)
  
  ;
  ; Pass BFV into the PEI Core
  ;
  push    PcdGet32 (PcdPayloadFdMemBase)

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
  call SecStartup
_ModuleEntryPoint   ENDP

END
