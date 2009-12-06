#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008-2009 Apple Inc. All rights reserved.
#
# All rights reserved. This program and the accompanying materials
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

.globl ASM_PFX(ExceptionHandlersStart)
.globl ASM_PFX(ExceptionHandlersEnd)
.globl ASM_PFX(CommonExceptionEntry)
.globl ASM_PFX(AsmCommonExceptionEntry)
.globl ASM_PFX(CommonCExceptionHandler)

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
  stmfd     sp!,{r0-r1}
  mov       r0,#0
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(UndefinedInstructionEntry):
  stmfd     sp!,{r0-r1}
  mov       r0,#1
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(SoftwareInterruptEntry):
  stmfd     sp!,{r0-r1}
  mov       r0,#2
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(PrefetchAbortEntry):
  stmfd     sp!,{r0-r1}
  mov       r0,#3
  sub       lr,lr,#4
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(DataAbortEntry):
  stmfd     sp!,{r0-r1}
  mov       r0,#4
  sub       lr,lr,#8
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(ReservedExceptionEntry):
  stmfd     sp!,{r0-r1}
  mov       r0,#5
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(IrqEntry):
  stmfd     sp!,{r0-r1}
  mov       r0,#6
  sub       lr,lr,#4
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(FiqEntry):
  stmfd     sp!,{r0-r1}
  mov       r0,#7
  sub       lr,lr,#4
  ldr       r1,ASM_PFX(CommonExceptionEntry)
  bx        r1

ASM_PFX(CommonExceptionEntry):
  .byte       0x12
  .byte       0x34
  .byte       0x56
  .byte       0x78

ASM_PFX(ExceptionHandlersEnd):

ASM_PFX(AsmCommonExceptionEntry):
  mrc       p15, 0, r1, c6, c0, 2   @ Read IFAR
  stmfd     sp!,{r1}                @ Store the IFAR
  
  mrc       p15, 0, r1, c5, c0, 1   @ Read IFSR
  stmfd     sp!,{r1}                @ Store the IFSR
  
  mrc       p15, 0, r1, c6, c0, 0   @ Read DFAR
  stmfd     sp!,{r1}                @ Store the DFAR
  
  mrc       p15, 0, r1, c5, c0, 0   @ Read DFSR
  stmfd     sp!,{r1}                @ Store the DFSR
  
  mrs       r1,spsr                 @ Read SPSR (which is the pre-exception CPSR)
  stmfd     sp!,{r1}                @ Store the SPSR
  
  stmfd     sp!,{lr}                @ Store the link register (which is the pre-exception PC)
  stmfd     sp,{sp,lr}^             @ Store user/system mode stack pointer and link register
  nop                               @ Required by ARM architecture
  sub       sp,sp,#0x08             @ Adjust stack pointer
  stmfd     sp!,{r2-r12}            @ Store general purpose registers
  
  ldr       r3,[sp,#0x50]           @ Read saved R1 from the stack (it was saved by the exception entry routine)
  ldr       r2,[sp,#0x4C]           @ Read saved R0 from the stack (it was saved by the exception entry routine)
  stmfd     sp!,{r2-r3}	            @ Store general purpose registers R0 and R1
  
  mov       r1,sp                   @ Prepare System Context pointer as an argument for the exception handler
  
  sub       sp,sp,#4                @ Adjust SP to preserve 8-byte alignment
  bl        ASM_PFX(CommonCExceptionHandler) @ Call exception handler
  add       sp,sp,#4                @ Adjust SP back to where we were
  
  ldr       r2,[sp,#0x40]           @ Load CPSR from context, in case it has changed
  msr       SPSR_cxsf,r2            @ Store it back to the SPSR to be restored when exiting this handler

  ldmfd     sp!,{r0-r12}            @ Restore general purpose registers
  ldmia     sp,{sp,lr}^             @ Restore user/system mode stack pointer and link register
  nop                               @ Required by ARM architecture
  add       sp,sp,#0x08             @ Adjust stack pointer
  ldmfd     sp!,{lr}                @ Restore the link register (which is the pre-exception PC)
  add       sp,sp,#0x1C             @ Clear out the remaining stack space
  movs      pc,lr                   @ Return from exception
  
