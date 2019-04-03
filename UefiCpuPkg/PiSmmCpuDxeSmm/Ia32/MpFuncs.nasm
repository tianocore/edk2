;------------------------------------------------------------------------------ ;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
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

SECTION .text

extern ASM_PFX(InitializeFloatingPointUnits)

%define VacantFlag 0x0
%define NotVacantFlag 0xff

%define LockLocation RendezvousFunnelProcEnd - RendezvousFunnelProcStart
%define StackStart LockLocation + 0x4
%define StackSize LockLocation + 0x8
%define RendezvousProc LockLocation + 0xC
%define GdtrProfile LockLocation + 0x10
%define IdtrProfile LockLocation + 0x16
%define BufferStart LockLocation + 0x1C

;-------------------------------------------------------------------------------------
;RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
;procedure serializes all the AP processors through an Init sequence. It must be
;noted that APs arrive here very raw...ie: real mode, no stack.
;ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;-------------------------------------------------------------------------------------
;RendezvousFunnelProc (&WakeUpBuffer,MemAddress);

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

        mov        si, BufferStart
        mov        edx,dword [si]          ; EDX is keeping the start address of wakeup buffer

        mov        si, GdtrProfile
o32     lgdt       [cs:si]

        mov        si, IdtrProfile
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

        mov         ax,  0x8
o16     mov         ds,  ax
o16     mov         es,  ax
o16     mov         fs,  ax
o16     mov         gs,  ax
o16     mov         ss,  ax           ; Flat mode setup.

        mov         esi, edx

        mov         edi, esi
        add         edi, LockLocation
        mov         al,  NotVacantFlag
TestLock:
        xchg        byte [edi], al
        cmp         al, NotVacantFlag
        jz          TestLock

ProgramStack:

        mov         edi, esi
        add         edi, StackSize
        mov         eax, dword [edi]
        mov         edi, esi
        add         edi, StackStart
        add         eax, dword [edi]
        mov         esp, eax
        mov         dword [edi], eax

Releaselock:

        mov         al,  VacantFlag
        mov         edi, esi
        add         edi, LockLocation
        xchg        byte [edi], al

        ;
        ; Call assembly function to initialize FPU.
        ;
        mov         ebx, ASM_PFX(InitializeFloatingPointUnits)
        call        ebx
        ;
        ; Call C Function
        ;
        mov         edi, esi
        add         edi, RendezvousProc
        mov         eax, dword [edi]

        test        eax, eax
        jz          GoToSleep
        call        eax                           ; Call C function

GoToSleep:
        cli
        hlt
        jmp         $-2

RendezvousFunnelProcEnd:
;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):

        pushad
        mov         ebp,esp

        mov         ebx, dword [ebp+0x24]
        mov         dword [ebx], RendezvousFunnelProcStart
        mov         dword [ebx+0x4], PMODE_ENTRY - RendezvousFunnelProcStart
        mov         dword [ebx+0x8], FLAT32_JUMP - RendezvousFunnelProcStart
        mov         dword [ebx+0xc], RendezvousFunnelProcEnd - RendezvousFunnelProcStart

        popad
        ret

