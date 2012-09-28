//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
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
  stmfd     SP!,{R0-R1}
  mov       R0,#0
  ldr       R1,CommonExceptionEntry
  bx        R1

UndefinedInstructionEntry
  stmfd     SP!,{R0-R1}
  mov       R0,#1
  ldr       R1,CommonExceptionEntry
  bx        R1

SoftwareInterruptEntry
  stmfd     SP!,{R0-R1}
  mov       R0,#2
  ldr       R1,CommonExceptionEntry
  bx        R1

PrefetchAbortEntry
  stmfd     SP!,{R0-R1}
  mov       R0,#3
  SUB       LR,LR,#4
  ldr       R1,CommonExceptionEntry
  bx        R1

DataAbortEntry
  stmfd     SP!,{R0-R1}
  mov       R0,#4
  SUB       LR,LR,#8
  ldr       R1,CommonExceptionEntry
  bx        R1

ReservedExceptionEntry
  stmfd     SP!,{R0-R1}
  mov       R0,#5
  ldr       R1,CommonExceptionEntry
  bx        R1

IrqEntry
  stmfd     SP!,{R0-R1}
  mov       R0,#6
  SUB       LR,LR,#4
  ldr       R1,CommonExceptionEntry
  bx        R1

FiqEntry
  stmfd     SP!,{R0-R1}
  mov       R0,#7
  SUB       LR,LR,#4
  ldr       R1,CommonExceptionEntry
  bx        R1

CommonExceptionEntry
  dcd       0x12345678

ExceptionHandlersEnd

AsmCommonExceptionEntry
  mrc       p15, 0, r1, c6, c0, 2   ; Read IFAR
  stmfd     SP!,{R1}                ; Store the IFAR
  
  mrc       p15, 0, r1, c5, c0, 1   ; Read IFSR
  stmfd     SP!,{R1}                ; Store the IFSR
  
  mrc       p15, 0, r1, c6, c0, 0   ; Read DFAR
  stmfd     SP!,{R1}                ; Store the DFAR
  
  mrc       p15, 0, r1, c5, c0, 0   ; Read DFSR
  stmfd     SP!,{R1}                ; Store the DFSR
  
  mrs       R1,SPSR                 ; Read SPSR (which is the pre-exception CPSR)
  stmfd     SP!,{R1}                ; Store the SPSR
  
  stmfd     SP!,{LR}                ; Store the link register (which is the pre-exception PC)
  stmfd     SP,{SP,LR}^             ; Store user/system mode stack pointer and link register
  nop                               ; Required by ARM architecture
  SUB       SP,SP,#0x08             ; Adjust stack pointer
  stmfd     SP!,{R2-R12}            ; Store general purpose registers
  
  ldr       R3,[SP,#0x50]           ; Read saved R1 from the stack (it was saved by the exception entry routine)
  ldr       R2,[SP,#0x4C]           ; Read saved R0 from the stack (it was saved by the exception entry routine)
  stmfd     SP!,{R2-R3}             ; Store general purpose registers R0 and R1
  
  mov       R1,SP                   ; Prepare System Context pointer as an argument for the exception handler
  
  sub       SP,SP,#4                ; Adjust SP to preserve 8-byte alignment
  blx       CommonCExceptionHandler ; Call exception handler
  add       SP,SP,#4                ; Adjust SP back to where we were
  
  ldr       R2,[SP,#0x40]           ; Load CPSR from context, in case it has changed
  MSR       SPSR_cxsf,R2            ; Store it back to the SPSR to be restored when exiting this handler

  ldmfd     SP!,{R0-R12}            ; Restore general purpose registers
  ldm       SP,{SP,LR}^             ; Restore user/system mode stack pointer and link register
  nop                               ; Required by ARM architecture
  add       SP,SP,#0x08             ; Adjust stack pointer
  ldmfd     SP!,{LR}                ; Restore the link register (which is the pre-exception PC)
  add       SP,SP,#0x1C             ; Clear out the remaining stack space
  movs      PC,LR                   ; Return from exception
  
  END


