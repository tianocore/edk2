;------------------------------------------------------------------------------ ;
; Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   MpFuncs.nasm
;
; Abstract:
;
;   This is the assembly code for Multi-processor S3 support
;
;-------------------------------------------------------------------------------

%define VacantFlag 0x0
%define NotVacantFlag 0xff

%define LockLocation RendezvousFunnelProcEnd - RendezvousFunnelProcStart
%define StackStartAddressLocation LockLocation + 0x8
%define StackSizeLocation LockLocation + 0x10
%define CProcedureLocation LockLocation + 0x18
%define GdtrLocation LockLocation + 0x20
%define IdtrLocation LockLocation + 0x2A
%define BufferStartLocation LockLocation + 0x34
%define Cr3OffsetLocation LockLocation + 0x38
%define InitializeFloatingPointUnitsAddress LockLocation + 0x3C

;-------------------------------------------------------------------------------------
;RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
;procedure serializes all the AP processors through an Init sequence. It must be
;noted that APs arrive here very raw...ie: real mode, no stack.
;ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;-------------------------------------------------------------------------------------
;RendezvousFunnelProc (&WakeUpBuffer,MemAddress);

;text      SEGMENT
DEFAULT REL
SECTION .text

BITS 16
global ASM_PFX(RendezvousFunnelProc)
ASM_PFX(RendezvousFunnelProc):
RendezvousFunnelProcStart:

; At this point CS = 0x(vv00) and ip= 0x0.

        mov        ax,  cs
        mov        ds,  ax
        mov        es,  ax
        mov        ss,  ax
        xor        ax,  ax
        mov        fs,  ax
        mov        gs,  ax

flat32Start:

        mov        si, BufferStartLocation
        mov        edx,dword [si]          ; EDX is keeping the start address of wakeup buffer

        mov        si, Cr3OffsetLocation
        mov        ecx,dword [si]          ; ECX is keeping the value of CR3

        mov        si, GdtrLocation
o32     lgdt       [cs:si]

        mov        si, IdtrLocation
o32     lidt       [cs:si]

        xor        ax,  ax
        mov        ds,  ax

        mov        eax, cr0                    ; Get control register 0
        or         eax, 0x000000001            ; Set PE bit (bit #0)
        mov        cr0, eax

FLAT32_JUMP:

a32     jmp   dword 0x20:0x0

BITS 32
PMODE_ENTRY:                         ; protected mode entry point

        mov        ax,  0x18
o16     mov        ds,  ax
o16     mov        es,  ax
o16     mov        fs,  ax
o16     mov        gs,  ax
o16     mov        ss,  ax                     ; Flat mode setup.

        mov        eax, cr4
        bts        eax, 5
        mov        cr4, eax

        mov        cr3, ecx

        mov        esi, edx                    ; Save wakeup buffer address

        mov        ecx, 0xc0000080             ; EFER MSR number.
        rdmsr                                  ; Read EFER.
        bts        eax, 8                      ; Set LME=1.
        wrmsr                                  ; Write EFER.

        mov        eax, cr0                    ; Read CR0.
        bts        eax, 31                     ; Set PG=1.
        mov        cr0, eax                    ; Write CR0.

LONG_JUMP:

a16     jmp   dword 0x38:0x0

BITS 64
LongModeStart:

        mov         ax,  0x30
o16     mov         ds,  ax
o16     mov         es,  ax
o16     mov         ss,  ax

        mov  edi, esi
        add  edi, LockLocation
        mov  al,  NotVacantFlag
TestLock:
        xchg byte [edi], al
        cmp  al, NotVacantFlag
        jz   TestLock

ProgramStack:

        mov  edi, esi
        add  edi, StackSizeLocation
        mov  rax, qword [edi]
        mov  edi, esi
        add  edi, StackStartAddressLocation
        add  rax, qword [edi]
        mov  rsp, rax
        mov  qword [edi], rax

Releaselock:

        mov  al,  VacantFlag
        mov  edi, esi
        add  edi, LockLocation
        xchg byte [edi], al

        ;
        ; Call assembly function to initialize FPU.
        ;
        mov         rax, qword [esi + InitializeFloatingPointUnitsAddress]
        sub         rsp, 0x20
        call        rax
        add         rsp, 0x20

        ;
        ; Call C Function
        ;
        mov         edi, esi
        add         edi, CProcedureLocation
        mov         rax, qword [edi]

        test        rax, rax
        jz          GoToSleep

        sub         rsp, 0x20
        call        rax
        add         rsp, 0x20

GoToSleep:
        cli
        hlt
        jmp         $-2

RendezvousFunnelProcEnd:

;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
; comments here for definition of address map
global ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):
        lea         rax, [RendezvousFunnelProcStart]
        mov         qword [rcx], rax
        mov         qword [rcx+0x8], PMODE_ENTRY - RendezvousFunnelProcStart
        mov         qword [rcx+0x10], FLAT32_JUMP - RendezvousFunnelProcStart
        mov         qword [rcx+0x18], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
        mov         qword [rcx+0x20], LongModeStart - RendezvousFunnelProcStart
        mov         qword [rcx+0x28], LONG_JUMP - RendezvousFunnelProcStart
        ret

