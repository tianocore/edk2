#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

.text
.align 3

GCC_ASM_EXPORT(ExceptionHandlersStart)
GCC_ASM_EXPORT(ExceptionHandlersEnd)
GCC_ASM_EXPORT(CommonExceptionEntry)
GCC_ASM_EXPORT(AsmCommonExceptionEntry)
GCC_ASM_EXPORT(CommonCExceptionHandler)

ASM_PFX(ExceptionHandlersStart):

ASM_PFX(Reset):
  b ASM_PFX(ResetEntry)

ASM_PFX(UndefinedInstruction):
  b ASM_PFX(UndefinedInstructionEntry)

ASM_PFX(SoftwareInterrupt):
  b ASM_PFX(SoftwareInterruptEntry)

ASM_PFX(PrefetchAbort):
  b ASM_PFX(PrefetchAbortEntry)

ASM_PFX(DataAbort):
  b ASM_PFX(DataAbortEntry)

ASM_PFX(ReservedException):
  b ASM_PFX(ReservedExceptionEntry)

ASM_PFX(Irq):
  b ASM_PFX(IrqEntry)

ASM_PFX(Fiq):
  b ASM_PFX(FiqEntry)

ASM_PFX(ResetEntry):
  srsdb     #0x13!                    @ Store return state on SVC stack
  stmfd     SP!,{LR}                  @ Store the link register for the current mode
  sub       SP,SP,#0x20               @ Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              @ Store the register state
  
  mov       R0,#0
  ldr       R1,ASM_PFX(CommonExceptionEntry)
  bx        R1

ASM_PFX(UndefinedInstructionEntry):
  srsdb     #0x13!                    @ Store return state on SVC stack
  cps       #0x13                     @ Switch to SVC for common stack
  stmfd     SP!,{LR}                  @ Store the link register for the current mode
  sub       SP,SP,#0x20               @ Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              @ Store the register state

  mov       r0,#1
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(SoftwareInterruptEntry):
  srsdb     #0x13!                    @ Store return state on SVC stack
  stmfd     SP!,{LR}                  @ Store the link register for the current mode
  sub       SP,SP,#0x20               @ Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              @ Store the register state

  mov       r0,#2
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(PrefetchAbortEntry):
  sub       LR,LR,#4
  srsdb     #0x13!                    @ Store return state on SVC stack
  cps       #0x13                     @ Switch to SVC for common stack
  stmfd     SP!,{LR}                  @ Store the link register for the current mode
  sub       SP,SP,#0x20               @ Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              @ Store the register state

  mov       r0,#3
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(DataAbortEntry):
  sub       LR,LR,#8
  srsdb     #0x13!                    @ Store return state on SVC stack
  cps       #0x13                     @ Switch to SVC for common stack
  stmfd     SP!,{LR}                  @ Store the link register for the current mode
  sub       SP,SP,#0x20               @ Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              @ Store the register state

  mov       r0,#4
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(ReservedExceptionEntry):
  srsdb     #0x13!                    @ Store return state on SVC stack
  cps       #0x13                     @ Switch to SVC for common stack
  stmfd     SP!,{LR}                  @ Store the link register for the current mode
  sub       SP,SP,#0x20               @ Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              @ Store the register state

  mov       r0,#5
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(IrqEntry):
  sub       LR,LR,#4
  srsdb     #0x13!                    @ Store return state on SVC stack
  cps       #0x13                     @ Switch to SVC for common stack
  stmfd     SP!,{LR}                  @ Store the link register for the current mode
  sub       SP,SP,#0x20               @ Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              @ Store the register state

  mov       r0,#6
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(FiqEntry):
  sub       LR,LR,#4
  srsdb     #0x13!                    @ Store return state on SVC stack
  cps       #0x13                     @ Switch to SVC for common stack
  stmfd     SP!,{LR}                  @ Store the link register for the current mode
  sub       SP,SP,#0x20               @ Save space for SP, LR, PC, IFAR - CPSR
  stmfd     SP!,{R0-R12}              @ Store the register state

  mov       r0,#7
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(CommonExceptionEntry):
  .byte       0x12
  .byte       0x34
  .byte       0x56
  .byte       0x78

ASM_PFX(ExceptionHandlersEnd):

ASM_PFX(AsmCommonExceptionEntry):
  mrc       p15, 0, R1, c6, c0, 2   @ Read IFAR
  str       R1, [SP, #0x50]         @ Store it in EFI_SYSTEM_CONTEXT_ARM.IFAR 
  
  mrc       p15, 0, R1, c5, c0, 1   @ Read IFSR
  str       R1, [SP, #0x4c]         @ Store it in EFI_SYSTEM_CONTEXT_ARM.IFSR
  
  mrc       p15, 0, R1, c6, c0, 0   @ Read DFAR
  str       R1, [SP, #0x48]         @ Store it in EFI_SYSTEM_CONTEXT_ARM.DFAR
  
  mrc       p15, 0, R1, c5, c0, 0   @ Read DFSR
  str       R1, [SP, #0x44]         @ Store it in EFI_SYSTEM_CONTEXT_ARM.DFSR
  
  ldr       R1, [SP, #0x5c]         @ srsdb saved pre-exception CPSR on the stack 
  str       R1, [SP, #0x40]         @ Store it in EFI_SYSTEM_CONTEXT_ARM.CPSR
  and       r1, r1, #0x1f           @ Check to see if User or System Mode
  cmp       r1, #0x1f
  cmpne     r1, #0x10
  add       R2, SP, #0x38           @ Store it in EFI_SYSTEM_CONTEXT_ARM.LR
  ldmneed   r2, {lr}^               @ User or System mode, use unbanked register
  ldmneed   r2, {lr}                @ All other modes used banked register

  ldr       R1, [SP, #0x58]         @ PC is the LR pushed by srsdb 
  str       R1, [SP, #0x3c]         @ Store it in EFI_SYSTEM_CONTEXT_ARM.PC
  
  sub       R1, SP, #0x60           @ We pused 0x60 bytes on the stack 
  str       R1, [SP, #0x34]         @ Store it in EFI_SYSTEM_CONTEXT_ARM.SP
  
                                              @ R0 is exception type 
  mov       R1,SP                             @ Prepare System Context pointer as an argument for the exception handler
  blx       ASM_PFX(CommonCExceptionHandler)  @ Call exception handler
  
  ldr       R2,[SP,#0x40]           @ EFI_SYSTEM_CONTEXT_ARM.CPSR
  str       R2,[SP,#0x5c]           @ Store it back to srsdb stack slot so it can be restored 

  ldr       R2,[SP,#0x3c]           @ EFI_SYSTEM_CONTEXT_ARM.PC
  str       R2,[SP,#0x58]           @ Store it back to srsdb stack slot so it can be restored 

  ldmfd     SP!,{R0-R12}            @ Restore general purpose registers
                                    @ Exception handler can not change SP or LR as we would blow chunks
                                    
  add       SP,SP,#0x20             @ Clear out the remaining stack space
  ldmfd     SP!,{LR}                @ restore the link register for this context
  rfefd     SP!                     @ return from exception via srsdb stack slot
