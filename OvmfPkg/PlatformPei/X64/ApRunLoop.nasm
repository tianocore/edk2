;------------------------------------------------------------------------------ ;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   ApRunLoop.nasm
;
; Abstract:
;
;   This is the assembly code for run loop for APs in the guest TD
;
;-------------------------------------------------------------------------------

%include "TdxCommondefs.inc"

DEFAULT REL

SECTION .text

BITS 64

%define TDVMCALL_EXPOSE_REGS_MASK       0xffec
%define TDVMCALL                        0x0
%define EXIT_REASON_CPUID               0xa

%macro  tdcall  0
  db  0x66, 0x0f, 0x01, 0xcc
%endmacro

;
; Relocated Ap Mailbox loop
;
; @return     None  This routine does not return
;
global ASM_PFX(AsmRelocateApMailBoxLoop)
ASM_PFX(AsmRelocateApMailBoxLoop):
AsmRelocateApMailBoxLoopStart:

    ;
    ; AP doesn't know its ApicId. So TdVmcall[CPUID(0xb)] is invoked to get
    ; the X2ApicId from host Vmm.
    ; After the call, the X2ApicId is stored in R8.
    ;
    mov        rax, TDVMCALL
    mov        rcx, TDVMCALL_EXPOSE_REGS_MASK
    mov        r11, EXIT_REASON_CPUID
    mov        r12, 0xb
    tdcall
    test       rax, rax
    jnz        Panic
    mov        r8, r15

MailBoxLoop:
    ; Spin until command set
    cmp        dword [rbx + CommandOffset], MpProtectedModeWakeupCommandNoop
    je         MailBoxLoop
    ; Determine if this is a broadcast or directly for my apic-id, if not, ignore
    cmp        dword [rbx + ApicidOffset], MailboxApicidBroadcast
    je         MailBoxProcessCommand
    cmp        dword [rbx + ApicidOffset], r8d
    jne        MailBoxLoop
MailBoxProcessCommand:
    cmp        dword [rbx + CommandOffset], MpProtectedModeWakeupCommandWakeup
    je         MailBoxWakeUp
    cmp        dword [rbx + CommandOffset], MpProtectedModeWakeupCommandSleep
    je         MailBoxSleep
    ; Don't support this command, so ignore
    jmp        MailBoxLoop
MailBoxWakeUp:
    mov       rax, [rbx + WakeupVectorOffset]
    jmp       rax
MailBoxSleep:
    jmp       $
Panic:
    ud2
BITS 64
AsmRelocateApMailBoxLoopEnd:

;-------------------------------------------------------------------------------------
;  AsmGetRelocationMap (&RelocationMap);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmGetRelocationMap)
ASM_PFX(AsmGetRelocationMap):
    lea        rax, [ASM_PFX(AsmRelocateApMailBoxLoopStart)]
    mov        qword [rcx], rax
    mov        qword [rcx +  8h], AsmRelocateApMailBoxLoopEnd - AsmRelocateApMailBoxLoopStart
    ret
