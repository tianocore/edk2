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

extern ASM_PFX(InitializeFloatingPointUnits)

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
        dw BufferStartLocation        ; mov        si, BufferStartLocation
        db 0x66,  0x8B, 0x14             ; mov        edx,dword ptr [si]          ; EDX is keeping the start address of wakeup buffer

        db 0xBE
        dw Cr3OffsetLocation          ; mov        si, Cr3Location
        db 0x66,  0x8B, 0xC             ; mov        ecx,dword ptr [si]          ; ECX is keeping the value of CR3

        db 0xBE
        dw GdtrLocation               ; mov        si, GdtrProfile
        db 0x66                        ; db         66h
        db 0x2E,  0xF, 0x1, 0x14        ; lgdt       fword ptr cs:[si]

        db 0xBE
        dw IdtrLocation               ; mov        si, IdtrProfile
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

        db 0x66,  0xB8, 0x18,  0x0      ; mov        ax,  18h
        db 0x66,  0x8E,  0xD8           ; mov        ds,  ax
        db 0x66,  0x8E,  0xC0           ; mov        es,  ax
        db 0x66,  0x8E,  0xE0           ; mov        fs,  ax
        db 0x66,  0x8E,  0xE8           ; mov        gs,  ax
        db 0x66,  0x8E,  0xD0           ; mov        ss,  ax                     ; Flat mode setup.

        db 0xF,  0x20,  0xE0           ; mov        eax, cr4
        db 0xF,  0xBA, 0xE8, 0x5      ; bts        eax, 5
        db 0xF,  0x22,  0xE0           ; mov        cr4, eax

        db 0xF,  0x22,  0xD9           ; mov        cr3, ecx

        db 0x8B,  0xF2                 ; mov        esi, edx                    ; Save wakeup buffer address

        db 0xB9
        dd 0xC0000080                 ; mov        ecx, 0c0000080h             ; EFER MSR number.
        db 0xF,  0x32                  ; rdmsr                                  ; Read EFER.
        db 0xF,  0xBA, 0xE8, 0x8      ; bts        eax, 8                      ; Set LME=1.
        db 0xF,  0x30                  ; wrmsr                                  ; Write EFER.

        db 0xF,  0x20,  0xC0           ; mov        eax, cr0                    ; Read CR0.
        db 0xF,  0xBA, 0xE8, 0x1F      ; bts        eax, 31                     ; Set PG=1.
        db 0xF,  0x22,  0xC0           ; mov        cr0, eax                    ; Write CR0.

LONG_JUMP:

        db 0x67,  0xEA                 ; far jump
        dd 0x0                         ; 32-bit offset
        dw 0x38                        ; 16-bit selector

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
        mov         rax, ASM_PFX(InitializeFloatingPointUnits)
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
        mov         rax, RendezvousFunnelProcStart
        mov         qword [rcx], rax
        mov         qword [rcx+0x8], PMODE_ENTRY - RendezvousFunnelProcStart
        mov         qword [rcx+0x10], FLAT32_JUMP - RendezvousFunnelProcStart
        mov         qword [rcx+0x18], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
        mov         qword [rcx+0x20], LongModeStart - RendezvousFunnelProcStart
        mov         qword [rcx+0x28], LONG_JUMP - RendezvousFunnelProcStart
        ret

