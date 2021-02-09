;------------------------------------------------------------------------------ ;
; Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
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

    lea        edi, [esi + MP_CPU_EXCHANGE_INFO_FIELD (SevEsIsEnabled)]
    cmp        byte [edi], 1        ; SevEsIsEnabled
    jne        CProcedureInvoke

    ;
    ; program GHCB
    ;   Each page after the GHCB is a per-CPU page, so the calculation programs
    ;   a GHCB to be every 8KB.
    ;
    mov        eax, SIZE_4KB
    shl        eax, 1                            ; EAX = SIZE_4K * 2
    mov        ecx, ebx
    mul        ecx                               ; EAX = SIZE_4K * 2 * CpuNumber
    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (GhcbBase)
    add        rax, qword [edi]
    mov        rdx, rax
    shr        rdx, 32
    mov        rcx, 0xc0010130
    wrmsr
    jmp        CProcedureInvoke

GetApicId:
    lea        edi, [esi + MP_CPU_EXCHANGE_INFO_FIELD (SevEsIsEnabled)]
    cmp        byte [edi], 1        ; SevEsIsEnabled
    jne        DoCpuid

    ;
    ; Since we don't have a stack yet, we can't take a #VC
    ; exception. Use the GHCB protocol to perform the CPUID
    ; calls.
    ;
    mov        rcx, 0xc0010130
    rdmsr
    shl        rdx, 32
    or         rax, rdx
    mov        rdi, rax             ; RDI now holds the original GHCB GPA

    mov        rdx, 0               ; CPUID function 0
    mov        rax, 0               ; RAX register requested
    or         rax, 4
    wrmsr
    rep vmmcall
    rdmsr
    cmp        edx, 0bh
    jb         NoX2ApicSevEs        ; CPUID level below CPUID_EXTENDED_TOPOLOGY

    mov        rdx, 0bh             ; CPUID function 0x0b
    mov        rax, 040000000h      ; RBX register requested
    or         rax, 4
    wrmsr
    rep vmmcall
    rdmsr
    test       edx, 0ffffh
    jz         NoX2ApicSevEs        ; CPUID.0BH:EBX[15:0] is zero

    mov        rdx, 0bh             ; CPUID function 0x0b
    mov        rax, 0c0000000h      ; RDX register requested
    or         rax, 4
    wrmsr
    rep vmmcall
    rdmsr

    ; Processor is x2APIC capable; 32-bit x2APIC ID is now in EDX
    jmp        RestoreGhcb

NoX2ApicSevEs:
    ; Processor is not x2APIC capable, so get 8-bit APIC ID
    mov        rdx, 1               ; CPUID function 1
    mov        rax, 040000000h      ; RBX register requested
    or         rax, 4
    wrmsr
    rep vmmcall
    rdmsr
    shr        edx, 24

RestoreGhcb:
    mov        rbx, rdx             ; Save x2APIC/APIC ID

    mov        rdx, rdi             ; RDI holds the saved GHCB GPA
    shr        rdx, 32
    mov        eax, edi
    wrmsr

    mov        rdx, rbx

    ; x2APIC ID or APIC ID is in EDX
    jmp        GetProcessorNumber

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
    push       rbp               ; Push BIST data at top of AP stack
    xor        rbp, rbp          ; Clear ebp for call stack trace
    push       rbp
    mov        rbp, rsp

    mov        rax, qword [esi + MP_CPU_EXCHANGE_INFO_FIELD (InitializeFloatingPointUnits)]
    sub        rsp, 20h
    call       rax               ; Call assembly function to initialize FPU per UEFI spec
    add        rsp, 20h

    mov        edx, ebx          ; edx is ApIndex
    mov        ecx, esi
    add        ecx, MP_CPU_EXCHANGE_INFO_OFFSET ; rcx is address of exchange info data buffer

    mov        edi, esi
    add        edi, MP_CPU_EXCHANGE_INFO_FIELD (CFunction)
    mov        rax, qword [edi]

    sub        rsp, 20h
    call       rax               ; Invoke C function
    add        rsp, 20h
    jmp        $                 ; Should never reach here

RendezvousFunnelProcEnd:

;-------------------------------------------------------------------------------------
;SwitchToRealProc procedure follows.
;ALSO THIS PROCEDURE IS EXECUTED BY APs TRANSITIONING TO 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;  SwitchToRealProc (UINTN BufferStart, UINT16 Code16, UINT16 Code32, UINTN StackStart)
;  rcx - Buffer Start
;  rdx - Code16 Selector Offset
;  r8  - Code32 Selector Offset
;  r9  - Stack Start
;-------------------------------------------------------------------------------------
global ASM_PFX(SwitchToRealProc)
ASM_PFX(SwitchToRealProc):
SwitchToRealProcStart:
BITS 64
    cli

    ;
    ; Get RDX reset value before changing stacks since the
    ; new stack won't be able to accomodate a #VC exception.
    ;
    push       rax
    push       rbx
    push       rcx
    push       rdx

    mov        rax, 1
    cpuid
    mov        rsi, rax                    ; Save off the reset value for RDX

    pop        rdx
    pop        rcx
    pop        rbx
    pop        rax

    ;
    ; Establish stack below 1MB
    ;
    mov        rsp, r9

    ;
    ; Push ultimate Reset Vector onto the stack
    ;
    mov        rax, rcx
    shr        rax, 4
    push       word 0x0002                 ; RFLAGS
    push       ax                          ; CS
    push       word 0x0000                 ; RIP
    push       word 0x0000                 ; For alignment, will be discarded

    ;
    ; Get address of "16-bit operand size" label
    ;
    lea        rbx, [PM16Mode]

    ;
    ; Push addresses used to change to compatibility mode
    ;
    lea        rax, [CompatMode]
    push       r8
    push       rax

    ;
    ; Clear R8 - R15, for reset, before going into 32-bit mode
    ;
    xor        r8, r8
    xor        r9, r9
    xor        r10, r10
    xor        r11, r11
    xor        r12, r12
    xor        r13, r13
    xor        r14, r14
    xor        r15, r15

    ;
    ; Far return into 32-bit mode
    ;
o64 retf

BITS 32
CompatMode:
    ;
    ; Set up stack to prepare for exiting protected mode
    ;
    push       edx                         ; Code16 CS
    push       ebx                         ; PM16Mode label address

    ;
    ; Disable paging
    ;
    mov        eax, cr0                    ; Read CR0
    btr        eax, 31                     ; Set PG=0
    mov        cr0, eax                    ; Write CR0

    ;
    ; Disable long mode
    ;
    mov        ecx, 0c0000080h             ; EFER MSR number
    rdmsr                                  ; Read EFER
    btr        eax, 8                      ; Set LME=0
    wrmsr                                  ; Write EFER

    ;
    ; Disable PAE
    ;
    mov        eax, cr4                    ; Read CR4
    btr        eax, 5                      ; Set PAE=0
    mov        cr4, eax                    ; Write CR4

    mov        edx, esi                    ; Restore RDX reset value

    ;
    ; Switch to 16-bit operand size
    ;
    retf

BITS 16
    ;
    ; At entry to this label
    ;   - RDX will have its reset value
    ;   - On the top of the stack
    ;     - Alignment data (two bytes) to be discarded
    ;     - IP for Real Mode (two bytes)
    ;     - CS for Real Mode (two bytes)
    ;
    ; This label is also used with AsmRelocateApLoop. During MP finalization,
    ; the code from PM16Mode to SwitchToRealProcEnd is copied to the start of
    ; the WakeupBuffer, allowing a parked AP to be booted by an OS.
    ;
PM16Mode:
    mov        eax, cr0                    ; Read CR0
    btr        eax, 0                      ; Set PE=0
    mov        cr0, eax                    ; Write CR0

    pop        ax                          ; Discard alignment data

    ;
    ; Clear registers (except RDX and RSP) before going into 16-bit mode
    ;
    xor        eax, eax
    xor        ebx, ebx
    xor        ecx, ecx
    xor        esi, esi
    xor        edi, edi
    xor        ebp, ebp

    iret

SwitchToRealProcEnd:

;-------------------------------------------------------------------------------------
;  AsmRelocateApLoop (MwaitSupport, ApTargetCState, PmCodeSegment, TopOfApStack, CountTofinish, Pm16CodeSegment, SevEsAPJumpTable, WakeupBuffer);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmRelocateApLoop)
ASM_PFX(AsmRelocateApLoop):
AsmRelocateApLoopStart:
BITS 64
    cmp        qword [rsp + 56], 0  ; SevEsAPJumpTable
    je         NoSevEs

    ;
    ; Perform some SEV-ES related setup before leaving 64-bit mode
    ;
    push       rcx
    push       rdx

    ;
    ; Get the RDX reset value using CPUID
    ;
    mov        rax, 1
    cpuid
    mov        rsi, rax          ; Save off the reset value for RDX

    ;
    ; Prepare the GHCB for the AP_HLT_LOOP VMGEXIT call
    ;   - Must be done while in 64-bit long mode so that writes to
    ;     the GHCB memory will be unencrypted.
    ;   - No NAE events can be generated once this is set otherwise
    ;     the AP_RESET_HOLD SW_EXITCODE will be overwritten.
    ;
    mov        rcx, 0xc0010130
    rdmsr                        ; Retrieve current GHCB address
    shl        rdx, 32
    or         rdx, rax

    mov        rdi, rdx
    xor        rax, rax
    mov        rcx, 0x800
    shr        rcx, 3
    rep stosq                    ; Clear the GHCB

    mov        rax, 0x80000004   ; VMGEXIT AP_RESET_HOLD
    mov        [rdx + 0x390], rax
    mov        rax, 114          ; Set SwExitCode valid bit
    bts        [rdx + 0x3f0], rax
    inc        rax               ; Set SwExitInfo1 valid bit
    bts        [rdx + 0x3f0], rax
    inc        rax               ; Set SwExitInfo2 valid bit
    bts        [rdx + 0x3f0], rax

    pop        rdx
    pop        rcx

NoSevEs:
    cli                          ; Disable interrupt before switching to 32-bit mode
    mov        rax, [rsp + 40]   ; CountTofinish
    lock dec   dword [rax]       ; (*CountTofinish)--

    mov        r10, [rsp + 48]   ; Pm16CodeSegment
    mov        rax, [rsp + 56]   ; SevEsAPJumpTable
    mov        rbx, [rsp + 64]   ; WakeupBuffer
    mov        rsp, r9           ; TopOfApStack

    push       rax               ; Save SevEsAPJumpTable
    push       rbx               ; Save WakeupBuffer
    push       r10               ; Save Pm16CodeSegment
    push       rcx               ; Save MwaitSupport
    push       rdx               ; Save ApTargetCState

    lea        rax, [PmEntry]    ; rax <- The start address of transition code

    push       r8
    push       rax

    ;
    ; Clear R8 - R15, for reset, before going into 32-bit mode
    ;
    xor        r8, r8
    xor        r9, r9
    xor        r10, r10
    xor        r11, r11
    xor        r12, r12
    xor        r13, r13
    xor        r14, r14
    xor        r15, r15

    ;
    ; Far return into 32-bit mode
    ;
o64 retf

BITS 32
PmEntry:
    mov        eax, cr0
    btr        eax, 31           ; Clear CR0.PG
    mov        cr0, eax          ; Disable paging and caches

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

MwaitCheck:
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
    pop        edx                ; PM16CodeSegment
    add        esp, 4
    pop        ebx                ; WakeupBuffer
    add        esp, 4
    pop        eax                ; SevEsAPJumpTable
    add        esp, 4
    cmp        eax, 0             ; Check for SEV-ES
    je         DoHlt

    cli
    ;
    ; SEV-ES is enabled, use VMGEXIT (GHCB information already
    ; set by caller)
    ;
BITS 64
    rep        vmmcall
BITS 32

    ;
    ; Back from VMGEXIT AP_HLT_LOOP
    ;   Push the FLAGS/CS/IP values to use
    ;
    push       word 0x0002        ; EFLAGS
    xor        ecx, ecx
    mov        cx, [eax + 2]      ; CS
    push       cx
    mov        cx, [eax]          ; IP
    push       cx
    push       word 0x0000        ; For alignment, will be discarded

    push       edx
    push       ebx

    mov        edx, esi           ; Restore RDX reset value

    retf

DoHlt:
    cli
    hlt
    jmp        DoHlt

BITS 64
AsmRelocateApLoopEnd:

;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):
    lea        rax, [ASM_PFX(RendezvousFunnelProc)]
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RendezvousFunnelAddress], rax
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.ModeEntryOffset], LongModeStart - RendezvousFunnelProcStart
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RendezvousFunnelSize], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
    lea        rax, [ASM_PFX(AsmRelocateApLoop)]
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RelocateApLoopFuncAddress], rax
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.RelocateApLoopFuncSize], AsmRelocateApLoopEnd - AsmRelocateApLoopStart
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.ModeTransitionOffset], Flat32Start - RendezvousFunnelProcStart
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.SwitchToRealSize], SwitchToRealProcEnd - SwitchToRealProcStart
    mov        qword [rcx + MP_ASSEMBLY_ADDRESS_MAP.SwitchToRealOffset], SwitchToRealProcStart - RendezvousFunnelProcStart
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
    sgdt       [rsi + CPU_EXCHANGE_ROLE_INFO.Gdtr]
    sidt       [rsi + CPU_EXCHANGE_ROLE_INFO.Idtr]

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
    ; Since another CPU already stored its state, load them
    ; load GDTR value
    lgdt       [rdi + CPU_EXCHANGE_ROLE_INFO.Gdtr]

    ; load IDTR value
    lidt       [rdi + CPU_EXCHANGE_ROLE_INFO.Idtr]

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
