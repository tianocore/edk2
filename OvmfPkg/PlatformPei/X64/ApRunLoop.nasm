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

%macro  tdcall  0
  db  0x66, 0x0f, 0x01, 0xcc
%endmacro

;
; Relocated Ap Mailbox loop
;
; @param[in]  RBX:  Relocated mailbox address
; @param[in]  RBP:  vCpuId
;
; @return     None  This routine does not return
;
global ASM_PFX(AsmRelocateApMailBoxLoop)
ASM_PFX(AsmRelocateApMailBoxLoop):
AsmRelocateApMailBoxLoopStart:

    ;
    ; TdCall[TDINFO] to get the vCpuId
    ;
    ;mov     rax, 1
    ;tdcall
    ;
    ; R8  [31:0]  NUM_VCPUS
    ;     [63:32] MAX_VCPUS
    ; R9  [31:0]  VCPU_INDEX
    ;

    mov       r8, rbp
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

