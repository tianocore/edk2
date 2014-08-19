//------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
// Copyright (c) 2011 - 2012, ARM Ltd. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

#include <Library/PcdLib.h>

/*

This is the stack constructed by the exception handler (low address to high address)
                # R0 - IFAR is EFI_SYSTEM_CONTEXT for ARM
  Reg   Offset
  ===   ======
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
  CPSR  0x5c

 */

  EXPORT  DebugAgentVectorTable
  IMPORT  DefaultExceptionHandler

  PRESERVE8
  AREA  DebugAgentException, CODE, READONLY, CODEALIGN, ALIGN=5

//
// This code gets copied to the ARM vector table
// ExceptionHandlersStart - ExceptionHandlersEnd gets copied
//
DebugAgentVectorTable FUNCTION
  b   ResetEntry
  b   UndefinedInstructionEntry
  b   SoftwareInterruptEntry
  b   PrefetchAbortEntry
  b   DataAbortEntry
  b   ReservedExceptionEntry
  b   IrqEntry
  b   FiqEntry
  ENDFUNC

ResetEntry
  srsfd     #0x13!                    ; Store return state on SVC stack
                                      ; We are already in SVC mode
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#0                     ; ExceptionType
  ldr       R1,CommonExceptionEntry
  bx        R1

UndefinedInstructionEntry
  sub       LR, LR, #4                ; Only -2 for Thumb, adjust in CommonExceptionEntry
  srsfd     #0x13!                    ; Store return state on SVC stack
  cps       #0x13                     ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#1                     ; ExceptionType
  ldr       R1,CommonExceptionEntry;
  bx        R1

SoftwareInterruptEntry
  sub       LR, LR, #4                ; Only -2 for Thumb, adjust in CommonExceptionEntry
  srsfd     #0x13!                    ; Store return state on SVC stack
                                      ; We are already in SVC mode
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#2                     ; ExceptionType
  ldr       R1,CommonExceptionEntry
  bx        R1

PrefetchAbortEntry
  sub       LR,LR,#4
  srsfd     #0x13!                    ; Store return state on SVC stack
  cps       #0x13                     ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#3                     ; ExceptionType
  ldr       R1,CommonExceptionEntry
  bx        R1

DataAbortEntry
  sub       LR,LR,#8
  srsfd     #0x13!                    ; Store return state on SVC stack
  cps       #0x13                     ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#4                     ; ExceptionType
  ldr       R1,CommonExceptionEntry
  bx        R1

ReservedExceptionEntry
  srsfd     #0x13!                    ; Store return state on SVC stack
  cps       #0x13                     ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#5                     ; ExceptionType
  ldr       R1,CommonExceptionEntry
  bx        R1

IrqEntry
  sub       LR,LR,#4
  srsfd     #0x13!                    ; Store return state on SVC stack
  cps       #0x13                     ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state

  mov       R0,#6                     ; ExceptionType
  ldr       R1,CommonExceptionEntry
  bx        R1

FiqEntry
  sub       LR,LR,#4
  srsfd     #0x13!                    ; Store return state on SVC stack
  cps       #0x13                     ; Switch to SVC for common stack
  stmfd     SP!,{LR}                  ; Store the link register for the current mode
  sub       SP,SP,#0x20               ; Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              ; Store the register state
                                      ; Since we have already switch to SVC R8_fiq - R12_fiq
                                      ; never get used or saved
  mov       R0,#7                     ; ExceptionType
  ldr       R1,CommonExceptionEntry
  bx        R1

//
// This gets patched by the C code that patches in the vector table
//
CommonExceptionEntry
  dcd       AsmCommonExceptionEntry

ExceptionHandlersEnd

//
// This code runs from CpuDxe driver loaded address. It is patched into
// CommonExceptionEntry.
//
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

  add       R2, SP, #0x38           ; Make R2 point to EFI_SYSTEM_CONTEXT_ARM.LR
  and       R3, R1, #0x1f           ; Check CPSR to see if User or System Mode
  cmp       R3, #0x1f               ; if ((CPSR == 0x10) || (CPSR == 0x1df))
  cmpne     R3, #0x10               ;
  stmeqed   R2, {lr}^               ;   save unbanked lr
                                    ; else
  stmneed   R2, {lr}                ;   save SVC lr


  ldr       R5, [SP, #0x58]         ; PC is the LR pushed by srsfd
                                    ; Check to see if we have to adjust for Thumb entry
  sub       r4, r0, #1              ; if (ExceptionType == 1 || ExceptionType ==2)) {
  cmp       r4, #1                  ;   // UND & SVC have differnt LR adjust for Thumb
  bhi       NoAdjustNeeded

  tst       r1, #0x20               ;   if ((CPSR & T)) == T) {  // Thumb Mode on entry
  addne     R5, R5, #2              ;     PC += 2;
  str       R5,[SP,#0x58]           ; Update LR value pused by srsfd

NoAdjustNeeded

  str       R5, [SP, #0x3c]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.PC

  sub       R1, SP, #0x60           ; We pused 0x60 bytes on the stack
  str       R1, [SP, #0x34]         ; Store it in EFI_SYSTEM_CONTEXT_ARM.SP

                                    ; R0 is ExceptionType
  mov       R1,SP                   ; R1 is SystemContext

#if (FixedPcdGet32(PcdVFPEnabled))
  vpush    {d0-d15}                  ; save vstm registers in case they are used in optimizations
#endif

/*
VOID
EFIAPI
DefaultExceptionHandler (
  IN     EFI_EXCEPTION_TYPE           ExceptionType,   R0
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext    R1
  )

*/
  blx       DefaultExceptionHandler ; Call exception handler

#if (FixedPcdGet32(PcdVFPEnabled))
  vpop      {d0-d15}
#endif

  ldr       R1, [SP, #0x4c]         ; Restore EFI_SYSTEM_CONTEXT_ARM.IFSR
  mcr       p15, 0, R1, c5, c0, 1   ; Write IFSR

  ldr       R1, [SP, #0x44]         ; sRestore EFI_SYSTEM_CONTEXT_ARM.DFSR
  mcr       p15, 0, R1, c5, c0, 0   ; Write DFSR

  ldr       R1,[SP,#0x3c]           ; EFI_SYSTEM_CONTEXT_ARM.PC
  str       R1,[SP,#0x58]           ; Store it back to srsfd stack slot so it can be restored

  ldr       R1,[SP,#0x40]           ; EFI_SYSTEM_CONTEXT_ARM.CPSR
  str       R1,[SP,#0x5c]           ; Store it back to srsfd stack slot so it can be restored

  add       R3, SP, #0x54           ; Make R3 point to SVC LR saved on entry
  add       R2, SP, #0x38           ; Make R2 point to EFI_SYSTEM_CONTEXT_ARM.LR
  and       R1, R1, #0x1f           ; Check to see if User or System Mode
  cmp       R1, #0x1f               ; if ((CPSR == 0x10) || (CPSR == 0x1f))
  cmpne     R1, #0x10               ;
  ldmeqed   R2, {lr}^               ;   restore unbanked lr
                                    ; else
  ldmneed   R3, {lr}                ;   restore SVC lr, via ldmfd SP!, {LR}

  ldmfd     SP!,{R0-R12}            ; Restore general purpose registers
                                    ; Exception handler can not change SP

  add       SP,SP,#0x20             ; Clear out the remaining stack space
  ldmfd     SP!,{LR}                ; restore the link register for this context
  rfefd     SP!                     ; return from exception via srsfd stack slot

  END
