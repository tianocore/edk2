;------------------------------------------------------------------------------ ;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   MpFuncs.nasm
;
; Abstract:
;
;   This is the assembly code for MP support
;
;-------------------------------------------------------------------------------

%include "MpEqu.inc"
extern ASM_PFX(InitializeFloatingPointUnits)

DEFAULT REL

SECTION .text

;-------------------------------------------------------------------------------------
;RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
;procedure serializes all the AP processors through an Init sequence. It must be
;noted that APs arrive here very raw...ie: real mode, no stack.
;ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;-------------------------------------------------------------------------------------
global ASM_PFX(RendezvousFunnelProc)
ASM_PFX(RendezvousFunnelProc):
RendezvousFunnelProcStart:
; At this point CS = 0x(vv00) and ip= 0x0.
; Save BIST information to ebp firstly
BITS 16

    mov        eax, 1234h
    mov        ebp, eax                        ; save BIST information

    mov        ax, cs
    mov        ds, ax
    mov        es, ax
    mov        ss, ax
    xor        ax, ax
    mov        fs, ax
    mov        gs, ax

    mov        si,  BufferStartLocation
    mov        ebx, [si]

    mov        di,  PmodeOffsetLocation
    mov        eax, [di]
    mov        di,  ax
    sub        di,  06h
    add        eax, ebx
    mov        [di],eax

    mov        di, LmodeOffsetLocation
    mov        eax, [di]
    mov        di,  ax
    sub        di,  06h
    add        eax, ebx
    mov        [di],eax


    mov        si, Cr3Location
    mov        ecx,[si]                        ; ECX is keeping the value of CR3

    mov        si, GdtrLocation
o32 lgdt       [cs:si]

    mov        si, IdtrLocation
o32 lidt       [cs:si]


    xor        ax,  ax
    mov        ds,  ax

    mov        eax, cr0                        ;Get control register 0
    or         eax, 000000003h                 ;Set PE bit (bit #0) & MP
    mov        cr0, eax

    jmp        PROTECT_MODE_CS:strict dword 0  ; far jump to protected mode
BITS 32
Flat32Start:                                   ; protected mode entry point
    mov        ax, PROTECT_MODE_DS
    mov        ds, ax
    mov        es, ax
    mov        fs, ax
    mov        gs, ax
    mov        ss, ax

    mov        eax, cr4
    bts        eax, 5
    mov        cr4, eax

    mov        cr3, ecx


    mov        ecx, 0c0000080h             ; EFER MSR number.
    rdmsr                                  ; Read EFER.
    bts        eax, 8                      ; Set LME=1.
    wrmsr                                  ; Write EFER.

    mov        eax, cr0                    ; Read CR0.
    bts        eax, 31                     ; Set PG=1.
    mov        cr0, eax                    ; Write CR0.

    jmp        LONG_MODE_CS:strict dword 0 ; far jump to long mode
BITS 64
LongModeStart:
    mov        ax,  LONG_MODE_DS
    mov        ds,  ax
    mov        es,  ax
    mov        ss,  ax

    mov        esi, ebx
    mov        edi, esi
    add        edi, LockLocation
    mov        rax, NotVacantFlag

TestLock:
    xchg       qword [edi], rax
    cmp        rax, NotVacantFlag
    jz         TestLock

    mov        edi, esi
    add        edi, NumApsExecutingLoction
    inc        dword [edi]
    mov        ebx, [edi]

ProgramStack:
    mov        edi, esi
    add        edi, StackSizeLocation
    mov        rax, qword [edi]
    mov        edi, esi
    add        edi, StackStartAddressLocation
    add        rax, qword [edi]
    mov        rsp, rax
    mov        qword [edi], rax

Releaselock:
    mov        rax, VacantFlag
    mov        edi, esi
    add        edi, LockLocation
    xchg       qword [edi], rax

CProcedureInvoke:
    push       rbp               ; push BIST data at top of AP stack
    xor        rbp, rbp          ; clear ebp for call stack trace
    push       rbp
    mov        rbp, rsp

    mov        rax, ASM_PFX(InitializeFloatingPointUnits)
    sub        rsp, 20h
    call       rax               ; Call assembly function to initialize FPU per UEFI spec
    add        rsp, 20h

    mov        edx, ebx          ; edx is NumApsExecuting
    mov        ecx, esi
    add        ecx, LockLocation ; rcx is address of exchange info data buffer

    mov        edi, esi
    add        edi, ApProcedureLocation
    mov        rax, qword [edi]

    sub        rsp, 20h
    call       rax               ; invoke C function
    add        rsp, 20h

RendezvousFunnelProcEnd:

;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):
    mov        rax, ASM_PFX(RendezvousFunnelProc)
    mov        qword [rcx], rax
    mov        qword [rcx +  8h], Flat32Start - RendezvousFunnelProcStart
    mov        qword [rcx + 10h], LongModeStart - RendezvousFunnelProcStart
    mov        qword [rcx + 18h], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
    ret

global ASM_PFX(AsmInitializeGdt)
ASM_PFX(AsmInitializeGdt):
    push       rbp
    mov        rbp, rsp

    lgdt       [rcx]  ; update the GDTR

    sub        rsp, 0x10
    mov        rax, ASM_PFX(SetCodeSelectorFarJump)
    mov        [rsp], rax
    mov        rdx, LONG_MODE_CS
    mov        [rsp + 4], dx    ; get new CS
    jmp        far dword [rsp]  ; far jump with new CS
ASM_PFX(SetCodeSelectorFarJump):
    add        rsp, 0x10

    mov        rax, LONG_MODE_DS          ; get new DS
    mov        ds, ax
    mov        es, ax
    mov        fs, ax
    mov        gs, ax
    mov        ss, ax

    pop        rbp

  ret
