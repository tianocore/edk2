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

global ASM_PFX(AsmCliHltLoop)
ASM_PFX(AsmCliHltLoop):
    cli
    hlt
    jmp $-2

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

;-------------------------------------------------------------------------------------
;AsmExchangeRole procedure follows. This procedure executed by current BSP, that is
;about to become an AP. It switches it'stack with the current AP.
;AsmExchangeRole (IN   CPU_EXCHANGE_INFO    *MyInfo, IN   CPU_EXCHANGE_INFO    *OthersInfo);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmExchangeRole)
ASM_PFX(AsmExchangeRole):
    ; DO NOT call other functions in this function, since 2 CPU may use 1 stack
    ; at the same time. If 1 CPU try to call a function, stack will be corrupted.

    push       rax
    push       rbx
    push       rcx
    push       rdx
    push       rsi
    push       rdi
    push       rbp
    push       r8
    push       r9
    push       r10
    push       r11
    push       r12
    push       r13
    push       r14
    push       r15

    mov        rax, cr0
    push       rax

    mov        rax, cr4
    push       rax

    ; rsi contains MyInfo pointer
    mov        rsi, rcx

    ; rdi contains OthersInfo pointer
    mov        rdi, rdx

    ;Store EFLAGS, GDTR and IDTR regiter to stack
    pushfq
    sgdt       [rsi + 16]
    sidt       [rsi + 26]

    ; Store the its StackPointer
    mov        [rsi + 8], rsp

    ; update its switch state to STORED
    mov        byte [rsi], CPU_SWITCH_STATE_STORED

WaitForOtherStored:
    ; wait until the other CPU finish storing its state
    cmp        byte [rdi], CPU_SWITCH_STATE_STORED
    jz         OtherStored
    pause
    jmp        WaitForOtherStored

OtherStored:
    ; Since another CPU already stored its state, load them
    ; load GDTR value
    lgdt       [rdi + 16]

    ; load IDTR value
    lidt       [rdi + 26]

    ; load its future StackPointer
    mov        rsp, [rdi + 8]

    ; update the other CPU's switch state to LOADED
    mov        byte [rdi], CPU_SWITCH_STATE_LOADED

WaitForOtherLoaded:
    ; wait until the other CPU finish loading new state,
    ; otherwise the data in stack may corrupt
    cmp        byte [rsi], CPU_SWITCH_STATE_LOADED
    jz         OtherLoaded
    pause
    jmp        WaitForOtherLoaded

OtherLoaded:
    ; since the other CPU already get the data it want, leave this procedure
    popfq

    pop        rax
    mov        cr4, rax

    pop        rax
    mov        cr0, rax

    pop        r15
    pop        r14
    pop        r13
    pop        r12
    pop        r11
    pop        r10
    pop        r9
    pop        r8
    pop        rbp
    pop        rdi
    pop        rsi
    pop        rdx
    pop        rcx
    pop        rbx
    pop        rax

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
