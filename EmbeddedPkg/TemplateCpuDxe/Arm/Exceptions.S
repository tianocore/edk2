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
.align 2

.globl _ExceptionHandlersStart
.globl _ExceptionHandlersEnd
.globl _CommonExceptionEntry
.globl _AsmCommonExceptionEntry
.globl _gExceptionHandlers

_ExceptionHandlersStart:

_Reset:
        b               _ResetEntry

_UndefinedInstruction:
        b               _UndefinedInstructionEntry

_SoftwareInterrupt:
        b               _SoftwareInterruptEntry

_PrefetchAbort:
        b               _PrefetchAbortEntry

_DataAbort:
        b               _DataAbortEntry

_ReservedException:
        b               _ReservedExceptionEntry

_Irq:
        b               _IrqEntry

_Fiq:
        b               _FiqEntry

_ResetEntry:
        stmfd           sp!,{r0-r1}
        mov             r0,#0
        ldr             r1,_CommonExceptionEntry
        bx              r1

_UndefinedInstructionEntry:
        stmfd           sp!,{r0-r1}
        mov             r0,#1
        ldr             r1,_CommonExceptionEntry
        bx              r1

_SoftwareInterruptEntry:
        stmfd           sp!,{r0-r1}
        mov             r0,#2
        ldr             r1,_CommonExceptionEntry
        bx              r1

_PrefetchAbortEntry:
        stmfd           sp!,{r0-r1}
        mov             r0,#3
        sub             lr,lr,#4
        ldr             r1,_CommonExceptionEntry
        bx              r1

_DataAbortEntry:
        stmfd           sp!,{r0-r1}
        mov             r0,#4
        sub             lr,lr,#8
        ldr             r1,_CommonExceptionEntry
        bx              r1

_ReservedExceptionEntry:
        stmfd           sp!,{r0-r1}
        mov             r0,#5
        ldr             r1,_CommonExceptionEntry
        bx              r1

_IrqEntry:
        stmfd           sp!,{r0-r1}
        mov             r0,#6
        sub             lr,lr,#4
        ldr             r1,_CommonExceptionEntry
        bx              r1

_FiqEntry:
        stmfd           sp!,{r0-r1}
        mov             r0,#7
        sub             lr,lr,#4
        ldr             r1,_CommonExceptionEntry
        bx              r1

_CommonExceptionEntry:
        .byte             0x12
        .byte             0x34
        .byte             0x56
        .byte             0x78

_ExceptionHandlersEnd:

LIndirectgExceptionHandlers:
        .long             _gExceptionHandlers

_AsmCommonExceptionEntry:
        mrc             p15, 0, r1, c6, c0, 2   @ Read IFAR
        stmfd           sp!,{r1}                @ Store the IFAR
        
        mrc             p15, 0, r1, c5, c0, 1   @ Read IFSR
        stmfd           sp!,{r1}                @ Store the IFSR
        
        mrc             p15, 0, r1, c6, c0, 0   @ Read DFAR
        stmfd           sp!,{r1}                @ Store the DFAR
        
        mrc             p15, 0, r1, c5, c0, 0   @ Read DFSR
        stmfd           sp!,{r1}                @ Store the DFSR
        
        mrs             r1,spsr                 @ Read SPSR (which is the pre-exception CPSR)
        stmfd           sp!,{r1}                @ Store the SPSR
        
        stmfd           sp!,{lr}                @ Store the link register (which is the pre-exception PC)
        stmfd           sp,{sp,lr}^             @ Store user/system mode stack pointer and link register
        nop                                     @ Required by ARM architecture
        sub             sp,sp,#0x08             @ Adjust stack pointer
        stmfd           sp!,{r2-r12}            @ Store general purpose registers
        
        ldr             r3,[sp,#0x40]           @ Read saved R1 from the stack (it was saved by the exception entry routine)
        ldr             r2,[sp,#0x3C]           @ Read saved R0 from the stack (it was saved by the exception entry routine)
        stmfd           sp!,{r2-r3}	            @ Store general purpose registers R0 and R1
        
        mov             r1,sp                   @ Prepare System Context pointer as an argument for the exception handler
        
        ldr             r2,LIndirectgExceptionHandlers  @ Offset to 32-bit address of exception handler
        ldr             r2,[r2]                         @ Load exception handler table
        ldr             r3,[r2,r0,lsl #2]               @ Index to find the handler for this exception
        
//        blx             r3                      @ Call exception handler
        bx             r3                      @ Call exception handler
        
        ldr             r2,[sp,#0x40]           @ Load CPSR from context, in case it has changed
        msr             SPSR_cxsf,r2            @ Store it back to the SPSR to be restored when exiting this handler

        ldmfd           sp!,{r0-r12}            @ Restore general purpose registers
        ldmia           sp,{sp,lr}^             @ Restore user/system mode stack pointer and link register
        nop                                     @ Required by ARM architecture
        add             sp,sp,#0x08             @ Adjust stack pointer
        ldmfd           sp!,{lr}                @ Restore the link register (which is the pre-exception PC)
        add             sp,sp,#0x1C             @ Clear out the remaining stack space
        movs            pc,lr                   @ Return from exception
        
