;------------------------------------------------------------------------------ ;
; Copyright (c) 2015 - 2023, Intel Corporation. All rights reserved.<BR>
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

%macro  OneTimeCall 1
    jmp     %1
%1 %+ OneTimerCallReturn:
%endmacro

%macro  OneTimeCallRet 1
    jmp     %1 %+ OneTimerCallReturn
%endmacro

DEFAULT REL

SECTION .text

;-------------------------------------------------------------------------------------
;RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
;procedure serializes all the AP processors through an Init sequence. It must be
;noted that APs arrive here very raw...ie: real mode, no stack.
;ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;-------------------------------------------------------------------------------------
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

    mov        si,  MP_CPU_EXCHANGE_INFO_FIELD (BufferStart)
    mov        ebx, [si]

    mov        si,  MP_CPU_EXCHANGE_INFO_FIELD (DataSegment)
    mov        edx, [si]

    ;
    ; Get start address of 32-bit code in low memory (<1MB)
    ;
    mov        edi, MP_CPU_EXCHANGE_INFO_FIELD (ModeTransitionMemory)

    mov        si, MP_CPU_EXCHANGE_INFO_FIELD (GdtrProfile)
o32 lgdt       [cs:si]

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
    mov        esi, MP_CPU_EXCHANGE_INFO_FIELD (EnableExecuteDisable)
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

    mov        esi, MP_CPU_EXCHANGE_INFO_FIELD (Enable5LevelPaging)
    cmp        byte [ebx + esi], 0
    jz         SkipEnable5LevelPaging

    ;
    ; Enable 5 Level Paging
    ;
    bts        eax, 12                     ; Set LA57=1.

SkipEnable5LevelPaging:

    mov        cr4, eax

    ;
    ; Load page table
    ;
    mov        esi, MP_CPU_EXCHANGE_INFO_FIELD (Cr3)             ; Save CR3 in ecx
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
    mov        edi, MP_CPU_EXCHANGE_INFO_FIELD (ModeHighMemory)
    add        edi, ebx
    jmp far    [edi]

BITS 64

LongModeStart:
    mov        esi, ebx

    ; Set IDT table at the start of 64 bit code
    lea        edi, [esi + MP_CPU_EXCHANGE_INFO_FIELD (IdtrProfile)]
    lidt       [edi]

    lea        edi, [esi + MP_CPU_EXCHANGE_INFO_FIELD (InitFlag)]
    cmp        qword [edi], 1       ; ApInitConfig
    jnz        GetApicId

    ; Increment the number of APs executing here as early as possible
    ; This is decremented in C code when AP is finished executing
    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (NumApsExecuting)
    lock inc   dword [edi]

    ; AP init
    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (ApIndex)
    mov        ebx, 1
    lock xadd  dword [edi], ebx                 ; EBX = ApIndex++
    inc        ebx                              ; EBX is CpuNumber

    ; program stack
    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (StackSize)
    mov        eax, dword [edi]
    mov        ecx, ebx
    inc        ecx
    mul        ecx                               ; EAX = StackSize * (CpuNumber + 1)
    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (StackStart)
    add        rax, qword [edi]
    mov        rsp, rax

    ;
    ;  Setup the GHCB when AMD SEV-ES active.
    ;
    OneTimeCall SevEsSetupGhcb
    jmp        CProcedureInvoke

GetApicId:
    ;
    ; Use the GHCB protocol to get the ApicId when SEV-ES is active.
    ;
    OneTimeCall SevEsGetApicId

DoCpuid:
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
    lea         eax, [esi + MP_CPU_EXCHANGE_INFO_FIELD (CpuInfo)]
    mov         rdi, [eax]

GetNextProcNumber:
    cmp         dword [rdi + CPU_INFO_IN_HOB.InitialApicId], edx                      ; APIC ID match?
    jz          ProgramStack
    add         rdi, CPU_INFO_IN_HOB_size
    inc         ebx
    jmp         GetNextProcNumber

ProgramStack:
    mov         rsp, qword [rdi + CPU_INFO_IN_HOB.ApTopOfStack]

CProcedureInvoke:
    ;
    ; Reserve 8 bytes for CpuMpData.
    ; When the AP wakes up again via INIT-SIPI-SIPI, push 0 will cause the existing CpuMpData to be overwritten with 0.
    ; CpuMpData is filled in via InitializeApData() during the first time INIT-SIPI-SIPI,
    ; while overwirrten may occurs when under ApInHltLoop but InitFlag is not set to ApInitConfig.
    ; Therefore reservation is implemented by sub rsp instead of push 0.
    ;
    sub        rsp, 8
    push       rbp               ; Push BIST data at top of AP stack
    xor        rbp, rbp          ; Clear ebp for call stack trace
    push       rbp
    mov        rbp, rsp

    push       qword 0          ; Push 8 bytes for alignment
    mov        rax, qword [esi + MP_CPU_EXCHANGE_INFO_FIELD (InitializeFloatingPointUnits)]
    sub        rsp, 20h
    call       rax               ; Call assembly function to initialize FPU per UEFI spec
    add        rsp, 20h

    mov        edx, ebx          ; edx is ApIndex
    mov        rcx, qword [esi + MP_CPU_EXCHANGE_INFO_FIELD (CpuMpData)]

    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (CFunction)
    mov        rax, qword [edi]

    sub        rsp, 20h
    call       rax               ; Invoke C function
    add        rsp, 20h
    jmp        $                 ; Should never reach here

;
; Required for the AMD SEV helper functions
;
%include "AmdSev.nasm"

RendezvousFunnelProcEnd:

;-------------------------------------------------------------------------------------
;  AsmRelocateApLoop (MwaitSupport, ApTargetCState, TopOfApStack, CountTofinish, Cr3);
;  This function is called during the finalizaiton of Mp initialization before booting
;  to OS, and aim to put Aps either in Mwait or HLT.
;-------------------------------------------------------------------------------------
; +----------------+
; | Cr3            |  rsp+40
; +----------------+
; | CountTofinish  |  r9
; +----------------+
; | TopOfApStack   |  r8
; +----------------+
; | ApTargetCState |  rdx
; +----------------+
; | MwaitSupport   |  rcx
; +----------------+
; | the return     |
; +----------------+ low address

AsmRelocateApLoopGenericStart:
    mov        rax, r9           ; CountTofinish
    lock dec   dword [rax]       ; (*CountTofinish)--

    mov        rax, [rsp + 40]    ; Cr3
    ; Do not push on old stack, since old stack is not mapped
    ; in the page table pointed by cr3
    mov        cr3, rax
    mov        rsp, r8            ; TopOfApStack

MwaitCheckGeneric:
    cmp        cl, 1              ; Check mwait-monitor support
    jnz        HltLoopGeneric
    mov        rbx, rdx           ; Save C-State to ebx

MwaitLoopGeneric:
    cli
    mov        rax, rsp           ; Set Monitor Address
    sub        eax, 8             ; To ensure the monitor address is in the page table
    xor        ecx, ecx           ; ecx = 0
    xor        edx, edx           ; edx = 0
    monitor
    mov        rax, rbx           ; Mwait Cx, Target C-State per eax[7:4]
    shl        eax, 4
    mwait
    jmp        MwaitLoopGeneric

HltLoopGeneric:
    cli
    hlt
    jmp        HltLoopGeneric

AsmRelocateApLoopGenericEnd:

;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):
    lea        rax, [RendezvousFunnelProcStart]
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RendezvousFunnelAddress], rax
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.ModeEntryOffset], LongModeStart - RendezvousFunnelProcStart
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RendezvousFunnelSize], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
lea        rax, [AsmRelocateApLoopGenericStart]
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RelocateApLoopFuncAddressGeneric], rax
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RelocateApLoopFuncSizeGeneric], AsmRelocateApLoopGenericEnd - AsmRelocateApLoopGenericStart
    lea        rax, [AsmRelocateApLoopAmdSevStart]
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RelocateApLoopFuncAddressAmdSev], rax
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RelocateApLoopFuncSizeAmdSev], AsmRelocateApLoopAmdSevEnd - AsmRelocateApLoopAmdSevStart
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.ModeTransitionOffset], Flat32Start - RendezvousFunnelProcStart
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.SwitchToRealNoNxOffset], SwitchToRealProcStart - Flat32Start
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.SwitchToRealPM16ModeOffset], PM16Mode - RendezvousFunnelProcStart
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.SwitchToRealPM16ModeSize], SwitchToRealProcEnd - PM16Mode
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

    ; rsi contains MyInfo pointer
    mov        rsi, rcx

    ; rdi contains OthersInfo pointer
    mov        rdi, rdx

    pushfq

    ; Store the its StackPointer
    mov        [rsi + CPU_EXCHANGE_ROLE_INFO.StackPointer], rsp

    ; update its switch state to STORED
    mov        byte [rsi + CPU_EXCHANGE_ROLE_INFO.State], CPU_SWITCH_STATE_STORED

WaitForOtherStored:
    ; wait until the other CPU finish storing its state
    cmp        byte [rdi + CPU_EXCHANGE_ROLE_INFO.State], CPU_SWITCH_STATE_STORED
    jz         OtherStored
    pause
    jmp        WaitForOtherStored

OtherStored:

    ; load its future StackPointer
    mov        rsp, [rdi + CPU_EXCHANGE_ROLE_INFO.StackPointer]

    ; update the other CPU's switch state to LOADED
    mov        byte [rdi + CPU_EXCHANGE_ROLE_INFO.State], CPU_SWITCH_STATE_LOADED

WaitForOtherLoaded:
    ; wait until the other CPU finish loading new state,
    ; otherwise the data in stack may corrupt
    cmp        byte [rsi + CPU_EXCHANGE_ROLE_INFO.State], CPU_SWITCH_STATE_LOADED
    jz         OtherLoaded
    pause
    jmp        WaitForOtherLoaded

OtherLoaded:
    ; since the other CPU already get the data it want, leave this procedure
    popfq

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
