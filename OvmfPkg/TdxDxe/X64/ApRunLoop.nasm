;------------------------------------------------------------------------------ ;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
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

SECTION .bss
global STACK_BASE
STACK_BASE:
 resb 1024
STACK_TOP:

SECTION .text

%define TDX_WORK_AREA_MAILBOX_GDTR   (FixedPcdGet32 (PcdOvmfWorkAreaBase) + 128)

%define PT_ADDR(Offset)                 (FixedPcdGet32 (PcdOvmfSecPageTablesBase) + (Offset))

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
; @param[in]  RBX:  Relocated mailbox address
; @param[in]  RBP:  vCpuId
;
; @return     None  This routine does not return
;
global ASM_PFX(AsmRelocateApMailBoxLoop)
ASM_PFX(AsmRelocateApMailBoxLoop):
AsmRelocateApMailBoxLoopStart:

    mov         rax, TDVMCALL
    mov         rcx, TDVMCALL_EXPOSE_REGS_MASK
    xor         r10, r10
    mov         r11, EXIT_REASON_CPUID
    mov         r12, 0xb
    tdcall
    test        r10, r10
    jnz         Panic
    mov         r8, r15
    mov         qword[rel mailbox_address], rbx

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
    cmp        dword [rbx + CommandOffset], MpProtectedModeWakeupCommandTest
    je         MailBoxTest
    cmp        dword [rbx + CommandOffset], MpProtectedModeWakeupCommandSleep
    je         MailBoxSleep
    ; Don't support this command, so ignore
    jmp        MailBoxLoop
MailBoxWakeUp:
    mov        rax, [rbx + WakeupVectorOffset]
    ; OS sends a wakeup command for a given APIC ID, firmware is supposed to reset
    ; the command field back to zero as acknowledgement.
    mov        qword [rbx + CommandOffset], 0
    jmp        rax
MailBoxTest:
    mov        qword [rbx + CommandOffset], 0
    jmp        MailBoxLoop
MailBoxSleep:
    jmp       $
Panic:
    ud2

AsmRelocateApResetVector:

.prepareStack:
    ; The stack can then be used to switch from long mode to compatibility mode
    mov rsp, STACK_TOP

.loadGDT:
    cli
    mov      rax, TDX_WORK_AREA_MAILBOX_GDTR
    lgdt     [rax]

.loadSwicthModeCode:
    mov     rcx, dword 0x10    ; load long mode selector
    shl     rcx, 32
    lea     rdx, [LongMode]    ; assume address < 4G
    or      rcx, rdx
    push    rcx 

    mov     rcx, dword 0x08     ; load compatible mode selector
    shl     rcx, 32
    lea     rdx, [Compatible]  ; assume address < 4G
    or      rcx, rdx
    push    rcx
    retf

BITS 32
Compatible:
    mov     eax, dword 0x18
;     ; reload DS/ES/SS to make sure they are correct referred to current GDT
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    ; reload the fs and gs
    mov     fs, ax
    mov     gs, ax

    ; Must clear the CR4.PCIDE before clearing paging
    mov     ecx, cr4
    btc     ecx, 17
    mov     cr4, ecx
    ;
    ; Disable paging
    ;
    mov     ecx, cr0
    btc     ecx, 31
    mov     cr0, ecx
    ;
RestoreCr0:
    ; Only enable  PE(bit 0), NE(bit 5), ET(bit 4) 0x31
    mov    eax, dword 0x31
    mov    cr0, eax


    ; Only Enable MCE(bit 6), VMXE(bit 13) 0x2040
    ; TDX enforeced the VMXE = 1 and mask it in VMM, so not set it.
RestoreCr4:
    mov     eax, 0x40
    mov     cr4, eax
SetCr3:
    ;
    ; Can use the boot page tables since it's reserved

    mov     eax, PT_ADDR (0)
    mov     cr3, eax

EnablePAE:
    mov     eax, cr4
    bts     eax, 5
    mov     cr4, eax

EnablePaging:
    mov     eax, cr0
    bts     eax, 31                     ; set PG
    mov     cr0, eax                    ; enable paging
    ; return to LongMode
    retf

BITS  64
LongMode:
    mov      rbx, qword[rel mailbox_address]
    jmp      AsmRelocateApMailBoxLoopStart
align 16
mailbox_address:
    dq 0
BITS 64
AsmRelocateApMailBoxLoopEnd:

;-------------------------------------------------------------------------------------
;  AsmGetRelocationMap (&RelocationMap);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmGetRelocationMap)
ASM_PFX(AsmGetRelocationMap):
    ; mov        byte[TDX_WORK_AREA_MB_PGTBL_READY], 0
    lea        rax, [AsmRelocateApMailBoxLoopStart]
    mov        qword [rcx], rax
    mov        qword [rcx +  8h], AsmRelocateApMailBoxLoopEnd - AsmRelocateApMailBoxLoopStart
    lea        rax, [AsmRelocateApResetVector]
    mov        qword [rcx + 10h], rax
    ret
