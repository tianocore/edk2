;------------------------------------------------------------------------------ ;
; Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
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
BITS 16
    mov        ebp, eax                        ; save BIST information

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

    mov        si, MP_CPU_EXCHANGE_INFO_FIELD (IdtrProfile)
o32 lidt       [cs:si]

    ;
    ; Switch to protected mode
    ;
    mov        eax, cr0                        ; Get control register 0
    or         eax, 000000003h                 ; Set PE bit (bit #0) & MP
    mov        cr0, eax

    ; Switch to 32-bit code in executable memory (>1MB)
o32 jmp far    [cs:di]

;
; Following code may be copied to memory with type of EfiBootServicesCode.
; This is required at DXE phase if NX is enabled for EfiBootServicesCode of
; memory.
;
BITS 32
Flat32Start:                                   ; protected mode entry point
    mov        ds, dx
    mov        es, dx
    mov        fs, dx
    mov        gs, dx
    mov        ss, dx

    mov        esi, ebx

    mov         edi, esi
    add         edi, MP_CPU_EXCHANGE_INFO_FIELD (EnableExecuteDisable)
    cmp         byte [edi], 0
    jz          SkipEnableExecuteDisable

    ;
    ; Enable IA32 PAE execute disable
    ;

    mov         ecx, 0xc0000080
    rdmsr
    bts         eax, 11
    wrmsr

    mov         edi, esi
    add         edi, MP_CPU_EXCHANGE_INFO_FIELD (Cr3)
    mov         eax, dword [edi]
    mov         cr3, eax

    mov         eax, cr4
    bts         eax, 5
    mov         cr4, eax

    mov         eax, cr0
    bts         eax, 31
    mov         cr0, eax

SkipEnableExecuteDisable:
    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (InitFlag)
    cmp        dword [edi], 1       ; 1 == ApInitConfig
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

    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (StackSize)
    mov        eax, [edi]
    mov        ecx, ebx
    inc        ecx
    mul        ecx                               ; EAX = StackSize * (CpuNumber + 1)
    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (StackStart)
    add        eax, [edi]
    mov        esp, eax
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
    lea         eax, [esi + MP_CPU_EXCHANGE_INFO_FIELD (CpuInfo)]
    mov         edi, [eax]

GetNextProcNumber:
    cmp         dword [edi + CPU_INFO_IN_HOB.InitialApicId], edx ; APIC ID match?
    jz          ProgramStack
    add         edi, CPU_INFO_IN_HOB_size
    inc         ebx
    jmp         GetNextProcNumber

ProgramStack:
    mov         esp, dword [edi + CPU_INFO_IN_HOB.ApTopOfStack]

CProcedureInvoke:
    ;
    ; Reserve 4 bytes for CpuMpData.
    ; When the AP wakes up again via INIT-SIPI-SIPI, push 0 will cause the existing CpuMpData to be overwritten with 0.
    ; CpuMpData is filled in via InitializeApData() during the first time INIT-SIPI-SIPI,
    ; while overwirrten may occurs when under ApInHltLoop but InitFlag is not set to ApInitConfig.
    ; Therefore reservation is implemented by sub esp instead of push 0.
    ;
    sub        esp, 4
    push       ebp               ; push BIST data at top of AP stack
    xor        ebp, ebp          ; clear ebp for call stack trace
    push       ebp
    mov        ebp, esp

    mov        eax, ASM_PFX(InitializeFloatingPointUnits)
    call       eax               ; Call assembly function to initialize FPU per UEFI spec

    push       ebx               ; Push ApIndex
    mov        eax, esi
    add        eax, MP_CPU_EXCHANGE_INFO_FIELD (CpuMpData)
    push       dword [eax]       ; push address of CpuMpData

    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (CFunction)
    mov        eax, [edi]

    call       eax               ; Invoke C function

    jmp        $                 ; Never reach here

;-------------------------------------------------------------------------------------
;SwitchToRealProc procedure follows.
;NOT USED IN 32 BIT MODE.
;-------------------------------------------------------------------------------------
SwitchToRealProcStart:
    jmp        $                 ; Never reach here
SwitchToRealProcEnd:

RendezvousFunnelProcEnd:

;-------------------------------------------------------------------------------------
;  AsmRelocateApLoopGeneric (MwaitSupport, ApTargetCState, PmCodeSegment, TopOfApStack, CountTofinish, Pm16CodeSegment, SevEsAPJumpTable, WakeupBuffer);
;
;  The last three parameters (Pm16CodeSegment, SevEsAPJumpTable and WakeupBuffer) are
;  specific to SEV-ES support and are not applicable on IA32.
;-------------------------------------------------------------------------------------
AsmRelocateApLoopGenericStart:
    mov        eax, cr0
    btr        eax, 31             ; Clear CR0.PG
    mov        cr0, eax            ; Disable paging since the page table might be unavailiable

    mov        eax, esp
    mov        esp, [eax + 12]     ; TopOfApStack
    push       dword [eax]         ; push return address for stack trace
    push       ebp
    mov        ebp, esp
    mov        ebx, [eax + 8]      ; ApTargetCState
    mov        ecx, [eax + 4]      ; MwaitSupport
    mov        eax, [eax + 16]     ; CountTofinish
    lock dec   dword [eax]         ; (*CountTofinish)--
    cmp        cl,  1              ; Check mwait-monitor support
    jnz        HltLoopGeneric
MwaitLoopGeneric:
    cli
    mov        eax, esp
    xor        ecx, ecx
    xor        edx, edx
    monitor
    mov        eax, ebx            ; Mwait Cx, Target C-State per eax[7:4]
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
    pushad
    mov        ebp,esp

    mov        ebx,  [ebp + 24h]
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.RendezvousFunnelAddress], RendezvousFunnelProcStart
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.ModeEntryOffset], Flat32Start - RendezvousFunnelProcStart
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.RendezvousFunnelSize], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.RelocateApLoopFuncAddressGeneric], AsmRelocateApLoopGenericStart
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.RelocateApLoopFuncSizeGeneric], AsmRelocateApLoopGenericEnd - AsmRelocateApLoopGenericStart
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.ModeTransitionOffset], Flat32Start - RendezvousFunnelProcStart
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.SwitchToRealNoNxOffset], SwitchToRealProcStart - Flat32Start
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.SwitchToRealPM16ModeOffset], 0
    mov        dword [ebx + MP_ASSEMBLY_ADDRESS_MAP.SwitchToRealPM16ModeSize], 0

    popad
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
    pushad
    mov        ebp,esp

    ; esi contains MyInfo pointer
    mov        esi, [ebp + 24h]

    ; edi contains OthersInfo pointer
    mov        edi, [ebp + 28h]

    ;Store EFLAGS to stack
    pushfd

    ; Store the its StackPointer
    mov        [esi + CPU_EXCHANGE_ROLE_INFO.StackPointer],esp

    ; update its switch state to STORED
    mov        byte [esi + CPU_EXCHANGE_ROLE_INFO.State], CPU_SWITCH_STATE_STORED

WaitForOtherStored:
    ; wait until the other CPU finish storing its state
    cmp        byte [edi + CPU_EXCHANGE_ROLE_INFO.State], CPU_SWITCH_STATE_STORED
    jz         OtherStored
    pause
    jmp        WaitForOtherStored

OtherStored:
    ; load its future StackPointer
    mov        esp, [edi + CPU_EXCHANGE_ROLE_INFO.StackPointer]

    ; update the other CPU's switch state to LOADED
    mov        byte [edi + CPU_EXCHANGE_ROLE_INFO.State], CPU_SWITCH_STATE_LOADED

WaitForOtherLoaded:
    ; wait until the other CPU finish loading new state,
    ; otherwise the data in stack may corrupt
    cmp        byte [esi + CPU_EXCHANGE_ROLE_INFO.State], CPU_SWITCH_STATE_LOADED
    jz         OtherLoaded
    pause
    jmp        WaitForOtherLoaded

OtherLoaded:
    ; since the other CPU already get the data it want, leave this procedure
    popfd

    popad
    ret
