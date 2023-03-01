;------------------------------------------------------------------------------ ;
; Copyright (c) 2021, AMD Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   AmdSev.nasm
;
; Abstract:
;
;   This provides helper used by the MpFunc.nasm. If AMD SEV-ES is active
;   then helpers perform the additional setups (such as GHCB).
;
;-------------------------------------------------------------------------------

%define SIZE_4KB    0x1000

RegisterGhcbGpa:
    ;
    ; Register GHCB GPA when SEV-SNP is enabled
    ;
    lea        edi, [esi + MP_CPU_EXCHANGE_INFO_FIELD (SevSnpIsEnabled)]
    cmp        byte [edi], 1        ; SevSnpIsEnabled
    jne        RegisterGhcbGpaDone

    ; Save the rdi and rsi to used for later comparison
    push       rdi
    push       rsi
    mov        edi, eax
    mov        esi, edx
    or         eax, 18              ; Ghcb registration request
    wrmsr
    rep vmmcall
    rdmsr
    mov        r12, rax
    and        r12, 0fffh
    cmp        r12, 19              ; Ghcb registration response
    jne        GhcbGpaRegisterFailure

    ; Verify that GPA is not changed
    and        eax, 0fffff000h
    cmp        edi, eax
    jne        GhcbGpaRegisterFailure
    cmp        esi, edx
    jne        GhcbGpaRegisterFailure
    pop        rsi
    pop        rdi
    jmp        RegisterGhcbGpaDone

    ;
    ; Request the guest termination
    ;
GhcbGpaRegisterFailure:
    xor        edx, edx
    mov        eax, 256             ; GHCB terminate
    wrmsr
    rep vmmcall

    ; We should not return from the above terminate request, but if we do
    ; then enter into the hlt loop.
DoHltLoop:
    cli
    hlt
    jmp        DoHltLoop

RegisterGhcbGpaDone:
    OneTimeCallRet    RegisterGhcbGpa

;
; The function checks whether SEV-ES is enabled, if enabled
; then setup the GHCB page.
;
SevEsSetupGhcb:
    lea        edi, [esi + MP_CPU_EXCHANGE_INFO_FIELD (SevEsIsEnabled)]
    cmp        byte [edi], 1        ; SevEsIsEnabled
    jne        SevEsSetupGhcbExit

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

    OneTimeCall RegisterGhcbGpa

    wrmsr

SevEsSetupGhcbExit:
    OneTimeCallRet    SevEsSetupGhcb

;
; The function checks whether SEV-ES is enabled, if enabled, use
; the GHCB
;
SevEsGetApicId:
    lea        edi, [esi + MP_CPU_EXCHANGE_INFO_FIELD (SevEsIsEnabled)]
    cmp        byte [edi], 1        ; SevEsIsEnabled
    jne        SevEsGetApicIdExit

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

    ;
    ; For SEV-SNP, the recommended handling for getting the x2APIC ID
    ; would be to use the SNP CPUID table to fetch CPUID.00H:EAX and
    ; CPUID:0BH:EBX[15:0] instead of the GHCB MSR protocol vmgexits
    ; below.
    ;
    ; To avoid the unecessary ugliness to accomplish that here, the BSP
    ; has performed these checks in advance (where #VC handler handles
    ; the CPUID table lookups automatically) and cached them in a flag
    ; so those checks can be skipped here.
    ;
    mov        eax, [esi + MP_CPU_EXCHANGE_INFO_FIELD (SevSnpIsEnabled)]
    cmp        al, 1
    jne        CheckExtTopoAvail

    ;
    ; Even with SEV-SNP, the actual x2APIC ID in CPUID.0BH:EDX
    ; fetched from the hypervisor the same way SEV-ES does it.
    ;
    mov        eax, [esi + MP_CPU_EXCHANGE_INFO_FIELD (ExtTopoAvail)]
    cmp        al, 1
    je         GetApicIdSevEs
    ; The 8-bit APIC ID fallback is also the same as with SEV-ES
    jmp        NoX2ApicSevEs

CheckExtTopoAvail:
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

GetApicIdSevEs:
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

SevEsGetApicIdExit:
    OneTimeCallRet    SevEsGetApicId


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
    retfq

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
;  AsmRelocateApLoopAmdSev (MwaitSupport, ApTargetCState, PmCodeSegment, TopOfApStack, CountTofinish, Pm16CodeSegment, SevEsAPJumpTable, WakeupBuffer);
;-------------------------------------------------------------------------------------

AsmRelocateApLoopAmdSevStart:
BITS 64
    cmp        qword [rsp + 56], 0  ; SevEsAPJumpTable
    je         NoSevEsAmdSev

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

NoSevEsAmdSev:
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

    lea        rax, [PmEntryAmdSev]    ; rax <- The start address of transition code

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
PmEntryAmdSev:
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

MwaitCheckAmdSev:
    cmp        cl, 1              ; Check mwait-monitor support
    jnz        HltLoopAmdSev
    mov        ebx, edx           ; Save C-State to ebx
MwaitLoopAmdSev:
    cli
    mov        eax, esp           ; Set Monitor Address
    xor        ecx, ecx           ; ecx = 0
    xor        edx, edx           ; edx = 0
    monitor
    mov        eax, ebx           ; Mwait Cx, Target C-State per eax[7:4]
    shl        eax, 4
    mwait
    jmp        MwaitLoopAmdSev

HltLoopAmdSev:
    pop        edx                ; PM16CodeSegment
    add        esp, 4
    pop        ebx                ; WakeupBuffer
    add        esp, 4
    pop        eax                ; SevEsAPJumpTable
    add        esp, 4
    cmp        eax, 0             ; Check for SEV-ES
    je         DoHltAmdSev

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

DoHltAmdSev:
    cli
    hlt
    jmp        DoHltAmdSev

BITS 64
AsmRelocateApLoopAmdSevEnd:
