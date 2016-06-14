;------------------------------------------------------------------------------ ;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
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

global ASM_PFX(RendezvousFunnelProc)
ASM_PFX(RendezvousFunnelProc):
RendezvousFunnelProcStart:

; At this point CS = 0x(vv00) and ip= 0x0.

        db 0x8c,  0xc8                 ; mov        ax,  cs
        db 0x8e,  0xd8                 ; mov        ds,  ax
        db 0x8e,  0xc0                 ; mov        es,  ax
        db 0x8e,  0xd0                 ; mov        ss,  ax
        db 0x33,  0xc0                 ; xor        ax,  ax
        db 0x8e,  0xe0                 ; mov        fs,  ax
        db 0x8e,  0xe8                 ; mov        gs,  ax

flat32Start:

        db 0xBE
        dw BufferStart                ; mov        si, BufferStart
        db 0x66,  0x8B, 0x14             ; mov        edx,dword ptr [si]          ; EDX is keeping the start address of wakeup buffer

        db 0xBE
        dw GdtrProfile                ; mov        si, GdtrProfile
        db 0x66                        ; db         66h
        db 0x2E,  0xF, 0x1, 0x14        ; lgdt       fword ptr cs:[si]

        db 0xBE
        dw IdtrProfile                ; mov        si, IdtrProfile
        db 0x66                        ; db         66h
        db 0x2E,  0xF, 0x1, 0x1C        ; lidt       fword ptr cs:[si]

        db 0x33,  0xC0                 ; xor        ax,  ax
        db 0x8E,  0xD8                 ; mov        ds,  ax

        db 0xF,  0x20, 0xC0            ; mov        eax, cr0                    ; Get control register 0
        db 0x66,  0x83, 0xC8, 0x1       ; or         eax, 000000001h             ; Set PE bit (bit #0)
        db 0xF,  0x22, 0xC0            ; mov        cr0, eax

FLAT32_JUMP:

        db 0x66,  0x67, 0xEA            ; far jump
        dd 0x0                         ; 32-bit offset
        dw 0x20                        ; 16-bit selector

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

