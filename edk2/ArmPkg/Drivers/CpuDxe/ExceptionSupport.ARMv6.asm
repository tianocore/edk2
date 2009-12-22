//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008-2009 Apple Inc. All rights reserved.
//
// All rights reserved. This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------



/*

This is the stack constructed by the exception handler

  R0    0x00    # stmfd     SP!,{R0-R12}
  R1    0x04
  R2    0x08
  R3    0x0c
  R4    0x10
  R5    0x14
  R6    0x18
  R7    0x1c
  R8    0x20
  R9    0x24
  R10   0x28
  R11   0x2c
  R12   0x30
  SP    0x34    # reserved via adding 0x20 (32) to the SP
  LR    0x38
  PC    0x3c
  CPSR  0x40
  DFSR  0x44
  DFAR  0x48
  IFSR  0x4c
  IFAR  0x50
  
  LR    0x54    # SVC Link register (we need to restore it)
  
  LR    0x58    # pushed by srsfd    
  CPSR  0x5c    # pushed by srsfd

 */
 
 
  EXPORT  ExceptionHandlersStart
  EXPORT  ExceptionHandlersEnd
  EXPORT  CommonExceptionEntry
  EXPORT  AsmCommonExceptionEntry
  IMPORT  CommonCExceptionHandler

  PRESERVE8
  AREA  DxeExceptionHandlers, CODE, READONLY
  
ExceptionHandlersStart

Reset
  b   ResetEntry

UndefinedInstruction
  b   UndefinedInstructionEntry

SoftwareInterrupt
  b   SoftwareInterruptEntry

PrefetchAbort
  b   PrefetchAbortEntry

DataAbort
  b   DataAbortEntry

ReservedException
  b   ReservedExceptionEntry

Irq
  b   IrqEntry

Fiq
  b   FiqEntry

ResetEntry
  srsfd     #0x13!                    ; Store return state on SVC stack
  cpsid     if,#0x13                  ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state
  
  mov       R0,#0
  ldr       R1,CommonExceptionEntry
  bx        R1

UndefinedInstructionEntry
  srsfd     #0x13!                    ; Store return state on SVC stack
  cpsid     i,#0x13                   ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#1
  ldr       R1,CommonExceptionEntry
  bx        R1

SoftwareInterruptEntry
  srsfd     #0x13!                    ; Store return state on SVC stack
  cpsid     i,#0x13                   ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#2
  ldr       R1,CommonExceptionEntry
  bx        R1

PrefetchAbortEntry
  sub       LR,LR,#4
  srsfd     #0x13!                    ; Store return state on SVC stack
  cpsid     i,#0x13                   ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#3
  ldr       R1,CommonExceptionEntry
  bx        R1

DataAbortEntry
  sub       LR,LR,#8
  srsfd     #0x13!                    ; Store return state on SVC stack
  cpsid     i,#0x13                   ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#4
  ldr       R1,CommonExceptionEntry
  bx        R1

ReservedExceptionEntry
  srsfd     #0x13!                    ; Store return state on SVC stack
  cpsid     if,#0x13                  ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#5
  ldr       R1,CommonExceptionEntry
  bx        R1

IrqEntry
  sub       LR,LR,#4
  srsfd     #0x13!                    ; Store return state on SVC stack
  cpsid     i,#0x13                   ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#6
  ldr       R1,CommonExceptionEntry
  bx        R1

FiqEntry
  sub       LR,LR,#4
  srsfd     #0x13!                    ; Store return state on SVC stack
  cpsid     if,#0x13                  ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#7
  ldr       R1,CommonExceptionEntry
  bx        R1

CommonExceptionEntry
  dcd       0x12345678

ExceptionHandlersEnd

AsmCommonExceptionEntry
  mrc       p15, 0, R1, c6, c0, 2   ; Read IFAR
  str       R1, [SP, #0x50]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.IFAR 
  
  mrc       p15, 0, R1, c5, c0, 1   ; Read IFSR
  str       R1, [SP, #0x4c]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.IFSR
  
  mrc       p15, 0, R1, c6, c0, 0   ; Read DFAR
  str       R1, [SP, #0x48]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.DFAR
  
  mrc       p15, 0, R1, c5, c0, 0   ; Read DFSR
  str       R1, [SP, #0x44]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.DFSR
  
  ldr       R1, [SP, #0x5c]         ; srsfd saved pre-exception CPSR on the stack 
  str       R1, [SP, #0x40]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.CPSR

  ldr       R1, [SP, #0x58]         ; PC is the LR pushed by srsfd 
  str       R1, [SP, #0x3c]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.PC
  str       R1, [SP, #0x38]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.LR
  
  sub       R1, SP, #0x60           ; We pused 0x60 bytes on the stack 
  str       R1, [SP, #0x34]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.SP
  
                                    ; R0 is exception type 
  mov       R1,SP                   ; Prepare System Context pointer as an argument for the exception handler
  blx       CommonCExceptionHandler ; Call exception handler
  
  ldr       R2,[SP,#0x40]           ; EFI_SYSTEM_CONTEXT_ARM.CPSR
  str       R2,[SP,#0x5c]           ; Store it back to srsfd stack slot so it can be restored 

  ldr       R2,[SP,#0x3c]           ; EFI_SYSTEM_CONTEXT_ARM.PC
  str       R2,[SP,#0x58]           ; Store it back to srsfd stack slot so it can be restored 

  ldmfd     SP!,{R0-R12}            ; Restore general purpose registers
                                    ; Exception handler can not change SP or LR as we would blow chunks
                                    
  add       SP,SP,#0x20             ; Clear out the remaining stack space
  ldmfd     SP!,{LR}                ; restore the link register for this context
  rfefd     SP!                     ; return from exception via srsfd stack slot
  
  END


