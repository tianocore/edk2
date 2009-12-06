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

        EXPORT	ExceptionHandlersStart
        EXPORT	ExceptionHandlersEnd
        EXPORT	CommonExceptionEntry
        EXPORT	AsmCommonExceptionEntry
        IMPORT	gExceptionHandlers

        AREA	DxeExceptionHandlers, CODE, READONLY

ExceptionHandlersStart

Reset
        B               ResetEntry

UndefinedInstruction
        B               UndefinedInstructionEntry

SoftwareInterrupt
        B               SoftwareInterruptEntry

PrefetchAbort
        B               PrefetchAbortEntry

DataAbort
        B               DataAbortEntry

ReservedException
        B               ReservedExceptionEntry

Irq
        B               IrqEntry

Fiq
        B               FiqEntry

ResetEntry
        STMFD           SP!,{R0-R1}
        MOV             R0,#0
        LDR             R1,CommonExceptionEntry
        BX              R1

UndefinedInstructionEntry
        STMFD           SP!,{R0-R1}
        MOV             R0,#1
        LDR             R1,CommonExceptionEntry
        BX              R1

SoftwareInterruptEntry
        STMFD           SP!,{R0-R1}
        MOV             R0,#2
        LDR             R1,CommonExceptionEntry
        BX              R1

PrefetchAbortEntry
        STMFD           SP!,{R0-R1}
        MOV             R0,#3
        SUB             LR,LR,#4
        LDR             R1,CommonExceptionEntry
        BX              R1

DataAbortEntry
        STMFD           SP!,{R0-R1}
        MOV             R0,#4
        SUB             LR,LR,#8
        LDR             R1,CommonExceptionEntry
        BX              R1

ReservedExceptionEntry
        STMFD           SP!,{R0-R1}
        MOV             R0,#5
        LDR             R1,CommonExceptionEntry
        BX              R1

IrqEntry
        STMFD           SP!,{R0-R1}
        MOV             R0,#6
        SUB             LR,LR,#4
        LDR             R1,CommonExceptionEntry
        BX              R1

FiqEntry
        STMFD           SP!,{R0-R1}
        MOV             R0,#7
        SUB             LR,LR,#4
        LDR             R1,CommonExceptionEntry
        BX              R1

CommonExceptionEntry
        DCD             0x12345678

ExceptionHandlersEnd

AsmCommonExceptionEntry
        MRC             p15, 0, r1, c6, c0, 2   ; Read IFAR
        STMFD           SP!,{R1}                ; Store the IFAR
        
        MRC             p15, 0, r1, c5, c0, 1   ; Read IFSR
        STMFD           SP!,{R1}                ; Store the IFSR
        
        MRC             p15, 0, r1, c6, c0, 0   ; Read DFAR
        STMFD           SP!,{R1}                ; Store the DFAR
        
        MRC             p15, 0, r1, c5, c0, 0   ; Read DFSR
        STMFD           SP!,{R1}                ; Store the DFSR
        
        MRS             R1,SPSR                 ; Read SPSR (which is the pre-exception CPSR)
        STMFD           SP!,{R1}                ; Store the SPSR
        
        STMFD           SP!,{LR}                ; Store the link register (which is the pre-exception PC)
        STMFD           SP,{SP,LR}^             ; Store user/system mode stack pointer and link register
        NOP                                     ; Required by ARM architecture
        SUB             SP,SP,#0x08             ; Adjust stack pointer
        STMFD           SP!,{R2-R12}            ; Store general purpose registers
        
        LDR             R3,[SP,#0x40]           ; Read saved R1 from the stack (it was saved by the exception entry routine)
        LDR             R2,[SP,#0x3C]           ; Read saved R0 from the stack (it was saved by the exception entry routine)
        STMFD           SP!,{R2-R3}	            ; Store general purpose registers R0 and R1
        
        MOV             R1,SP                   ; Prepare System Context pointer as an argument for the exception handler
        
        LDR             R2,=gExceptionHandlers  ; Load exception handler table
        LDR             R3,[R2,R0,LSL #2]       ; Index to find the handler for this exception
        
        BLX             R3                      ; Call exception handler
        
        LDR             R2,[SP,#0x40]           ; Load CPSR from context, in case it has changed
        MSR             SPSR_cxsf,R2            ; Store it back to the SPSR to be restored when exiting this handler

        LDMFD           SP!,{R0-R12}            ; Restore general purpose registers
        LDM             SP,{SP,LR}^             ; Restore user/system mode stack pointer and link register
        NOP                                     ; Required by ARM architecture
        ADD             SP,SP,#0x08             ; Adjust stack pointer
        LDMFD           SP!,{LR}                ; Restore the link register (which is the pre-exception PC)
        ADD             SP,SP,#0x1C             ; Clear out the remaining stack space
        MOVS            PC,LR                   ; Return from exception
        
        END


