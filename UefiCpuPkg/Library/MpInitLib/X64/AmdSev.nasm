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

SevEsGetApicIdExit:
    OneTimeCallRet    SevEsGetApicId
