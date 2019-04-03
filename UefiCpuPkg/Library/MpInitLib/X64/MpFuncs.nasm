;------------------------------------------------------------------------------ ;
; Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
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
    mov        ebp, eax                        ; Save BIST information

    mov        ax, cs
    mov        ds, ax
    mov        es, ax
    mov        ss, ax
    xor        ax, ax
    mov        fs, ax
    mov        gs, ax

    mov        si,  BufferStartLocation
    mov        ebx, [si]

    mov        si,  DataSegmentLocation
    mov        edx, [si]

    ;
    ; Get start address of 32-bit code in low memory (<1MB)
    ;
    mov        edi, ModeTransitionMemoryLocation

    mov        si, GdtrLocation
o32 lgdt       [cs:si]

    mov        si, IdtrLocation
o32 lidt       [cs:si]

    ;
    ; Switch to protected mode
    ;
    mov        eax, cr0                    ; Get control register 0
    or         eax, 000000003h             ; Set PE bit (bit #0) & MP
    mov        cr0, eax

    ; Switch to 32-bit code (>1MB)
o32 jmp far    [cs:di]

;
; Following code must be copied to memory with type of EfiBootServicesCode.
; This is required if NX is enabled for EfiBootServicesCode of memory.
;
BITS 32
Flat32Start:                                   ; protected mode entry point
    mov        ds, dx
    mov        es, dx
    mov        fs, dx
    mov        gs, dx
    mov        ss, dx

    ;
    ; Enable execute disable bit
    ;
    mov        esi, EnableExecuteDisableLocation
    cmp        byte [ebx + esi], 0
    jz         SkipEnableExecuteDisableBit

    mov        ecx, 0c0000080h             ; EFER MSR number
    rdmsr                                  ; Read EFER
    bts        eax, 11                     ; Enable Execute Disable Bit
    wrmsr                                  ; Write EFER

SkipEnableExecuteDisableBit:
    ;
    ; Enable PAE
    ;
    mov        eax, cr4
    bts        eax, 5
    mov        cr4, eax

    ;
    ; Load page table
    ;
    mov        esi, Cr3Location             ; Save CR3 in ecx
    mov        ecx, [ebx + esi]
    mov        cr3, ecx                    ; Load CR3

    ;
    ; Enable long mode
    ;
    mov        ecx, 0c0000080h             ; EFER MSR number
    rdmsr                                  ; Read EFER
    bts        eax, 8                      ; Set LME=1
    wrmsr                                  ; Write EFER

    ;
    ; Enable paging
    ;
    mov        eax, cr0                    ; Read CR0
    bts        eax, 31                     ; Set PG=1
    mov        cr0, eax                    ; Write CR0

    ;
    ; Far jump to 64-bit code
    ;
    mov        edi, ModeHighMemoryLocation
    add        edi, ebx
    jmp far    [edi]

BITS 64
LongModeStart:
    mov        esi, ebx
    lea        edi, [esi + InitFlagLocation]
    cmp        qword [edi], 1       ; ApInitConfig
    jnz        GetApicId

    ; Increment the number of APs executing here as early as possible
    ; This is decremented in C code when AP is finished executing
    mov        edi, esi
    add        edi, NumApsExecutingLocation
    lock inc   dword [edi]

    ; AP init
    mov        edi, esi
    add        edi, LockLocation
    mov        rax, NotVacantFlag

TestLock:
    xchg       qword [edi], rax
    cmp        rax, NotVacantFlag
    jz         TestLock

    lea        ecx, [esi + ApIndexLocation]
    inc        dword [ecx]
    mov        ebx, [ecx]

Releaselock:
    mov        rax, VacantFlag
    xchg       qword [edi], rax
    ; program stack
    mov        edi, esi
    add        edi, StackSizeLocation
    mov        eax, dword [edi]
    mov        ecx, ebx
    inc        ecx
    mul        ecx                               ; EAX = StackSize * (CpuNumber + 1)
    mov        edi, esi
    add        edi, StackStartAddressLocation
    add        rax, qword [edi]
    mov        rsp, rax
    jmp        CProcedureInvoke

GetApicId:
    mov        eax, 0
    cpuid
    cmp        eax, 0bh
    jb         NoX2Apic             ; CPUID level below CPUID_EXTENDED_TOPOLOGY

    mov        eax, 0bh
    xor        ecx, ecx
    cpuid
    test       ebx, 0ffffh
    jz         NoX2Apic             ; CPUID.0BH:EBX[15:0] is zero

    ; Processor is x2APIC capable; 32-bit x2APIC ID is already in EDX
    jmp        GetProcessorNumber

NoX2Apic:
    ; Processor is not x2APIC capable, so get 8-bit APIC ID
    mov        eax, 1
    cpuid
    shr        ebx, 24
    mov        edx, ebx

GetProcessorNumber:
    ;
    ; Get processor number for this AP
    ; Note that BSP may become an AP due to SwitchBsp()
    ;
    xor         ebx, ebx
    lea         eax, [esi + CpuInfoLocation]
    mov         edi, [eax]

GetNextProcNumber:
    cmp         dword [edi], edx                      ; APIC ID match?
    jz          ProgramStack
    add         edi, 20
    inc         ebx
    jmp         GetNextProcNumber

ProgramStack:
    mov         rsp, qword [edi + 12]

CProcedureInvoke:
    push       rbp               ; Push BIST data at top of AP stack
    xor        rbp, rbp          ; Clear ebp for call stack trace
    push       rbp
    mov        rbp, rsp

    mov        rax, qword [esi + InitializeFloatingPointUnitsAddress]
    sub        rsp, 20h
    call       rax               ; Call assembly function to initialize FPU per UEFI spec
    add        rsp, 20h

    mov        edx, ebx          ; edx is ApIndex
    mov        ecx, esi
    add        ecx, LockLocation ; rcx is address of exchange info data buffer

    mov        edi, esi
    add        edi, ApProcedureLocation
    mov        rax, qword [edi]

    sub        rsp, 20h
    call       rax               ; Invoke C function
    add        rsp, 20h
    jmp        $                 ; Should never reach here

RendezvousFunnelProcEnd:

;-------------------------------------------------------------------------------------
;  AsmRelocateApLoop (MwaitSupport, ApTargetCState, PmCodeSegment, TopOfApStack, CountTofinish);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmRelocateApLoop)
ASM_PFX(AsmRelocateApLoop):
AsmRelocateApLoopStart:
    cli                          ; Disable interrupt before switching to 32-bit mode
    mov        rax, [rsp + 40]   ; CountTofinish
    lock dec   dword [rax]       ; (*CountTofinish)--
    mov        rsp, r9
    push       rcx
    push       rdx

    lea        rsi, [PmEntry]    ; rsi <- The start address of transition code

    push       r8
    push       rsi
    DB         0x48
    retf
BITS 32
PmEntry:
    mov        eax, cr0
    btr        eax, 31           ; Clear CR0.PG
    mov        cr0, eax          ; Disable paging and caches

    mov        ebx, edx          ; Save EntryPoint to rbx, for rdmsr will overwrite rdx
    mov        ecx, 0xc0000080
    rdmsr
    and        ah, ~ 1           ; Clear LME
    wrmsr
    mov        eax, cr4
    and        al, ~ (1 << 5)    ; Clear PAE
    mov        cr4, eax

    pop        edx
    add        esp, 4
    pop        ecx,
    add        esp, 4
    cmp        cl, 1              ; Check mwait-monitor support
    jnz        HltLoop
    mov        ebx, edx           ; Save C-State to ebx
MwaitLoop:
    cli
    mov        eax, esp           ; Set Monitor Address
    xor        ecx, ecx           ; ecx = 0
    xor        edx, edx           ; edx = 0
    monitor
    mov        eax, ebx           ; Mwait Cx, Target C-State per eax[7:4]
    shl        eax, 4
    mwait
    jmp        MwaitLoop
HltLoop:
    cli
    hlt
    jmp        HltLoop
BITS 64
AsmRelocateApLoopEnd:

;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):
    lea        rax, [ASM_PFX(RendezvousFunnelProc)]
    mov        qword [rcx], rax
    mov        qword [rcx +  8h], LongModeStart - RendezvousFunnelProcStart
    mov        qword [rcx + 10h], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
    lea        rax, [ASM_PFX(AsmRelocateApLoop)]
    mov        qword [rcx + 18h], rax
    mov        qword [rcx + 20h], AsmRelocateApLoopEnd - AsmRelocateApLoopStart
    mov        qword [rcx + 28h], Flat32Start - RendezvousFunnelProcStart
    ret

;-------------------------------------------------------------------------------------
;AsmExchangeRole procedure follows. This procedure executed by current BSP, that is
;about to become an AP. It switches its stack with the current AP.
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
