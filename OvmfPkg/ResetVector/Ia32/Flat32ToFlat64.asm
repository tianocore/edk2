;------------------------------------------------------------------------------
; @file
; Transition from 32 bit flat protected mode into 64 bit flat protected mode
;
; Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
; Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

;
; Check if 5-level paging is supported.
; CPUID.(EAX=07H, ECX=0):ECX[bit 16] is a new feature flag
; that will enumerate basic support for 5-level paging
;
; Modified: EAX, ECX
;
; If 5-level paging is supported, EAX is 1
; If 5-level paging is *NOT* supported, EAX is 0
;
CheckLevel5PagingSupported:
    mov     eax, 07h
    xor     ecx, ecx
    cpuid
    xor     eax, eax
    test    ecx, 10000h
    jz      ExitCheckLevel5PagingSupported
    mov     eax, 1

ExitCheckLevel5PagingSupported:
    OneTimeCallRet  CheckLevel5PagingSupported

;
; Modified:  EAX, ECX, EDX
;
Transition32FlatTo64Flat:

    OneTimeCall SetCr3ForPageTables64

    ;
    ; Set Cr4
    ; But first should check if 5-level paging supported
    ;
    OneTimeCall CheckLevel5PagingSupported
    test    eax, eax
    jz      Level4Paging

    mov     ebx, cr4
    bts     ebx, 5                      ; enable PAE
    bts     ebx, 12                     ; enable LA57
    jmp     SetCr4

Level4Paging:
    mov     ebx, cr4
    bts     ebx, 5                      ; enable PAE


SetCr4:
    mov     cr4, ebx

    ;
    ; In TDX LME has already been set. So we're done and jump to enable
    ; paging directly if Tdx is enabled.
    ; EBX is cleared because in the later it will be used to check if
    ; the second step of the SEV-ES mitigation is to be performed.
    ;
    xor     ebx, ebx
    OneTimeCall IsTdxEnabled
    test    eax, eax
    jnz     EnablePaging

    mov     ecx, 0xc0000080
    rdmsr
    bts     eax, 8                      ; set LME
    wrmsr

    ;
    ; SEV-ES mitigation check support
    ;
    xor     ebx, ebx

    cmp     byte[SEV_ES_WORK_AREA], 0
    jz      EnablePaging

    ;
    ; SEV-ES is active, perform a quick sanity check against the reported
    ; encryption bit position. This is to help mitigate against attacks where
    ; the hypervisor reports an incorrect encryption bit position.
    ;
    ; This is the first step in a two step process. Before paging is enabled
    ; writes to memory are encrypted. Using the RDRAND instruction (available
    ; on all SEV capable processors), write 64-bits of random data to the
    ; SEV_ES_WORK_AREA and maintain the random data in registers (register
    ; state is protected under SEV-ES). This will be used in the second step.
    ;
RdRand1:
    rdrand  ecx
    jnc     RdRand1
    mov     dword[SEV_ES_WORK_AREA_RDRAND], ecx
RdRand2:
    rdrand  edx
    jnc     RdRand2
    mov     dword[SEV_ES_WORK_AREA_RDRAND + 4], edx

    ;
    ; Use EBX instead of the SEV_ES_WORK_AREA memory to determine whether to
    ; perform the second step.
    ;
    mov     ebx, 1

EnablePaging:
    mov     eax, cr0
    bts     eax, 31                     ; set PG
    mov     cr0, eax                    ; enable paging

    jmp     LINEAR_CODE64_SEL:ADDR_OF(jumpTo64BitAndLandHere)
BITS    64
jumpTo64BitAndLandHere:

    ;
    ; Check if the second step of the SEV-ES mitigation is to be performed.
    ;
    test    ebx, ebx
    jz      InsnCompare

    ;
    ; SEV-ES is active, perform the second step of the encryption bit postion
    ; mitigation check. The ECX and EDX register contain data from RDRAND that
    ; was stored to memory in encrypted form. If the encryption bit position is
    ; valid, the contents of ECX and EDX will match the memory location.
    ;
    cmp     dword[SEV_ES_WORK_AREA_RDRAND], ecx
    jne     SevEncBitHlt
    cmp     dword[SEV_ES_WORK_AREA_RDRAND + 4], edx
    jne     SevEncBitHlt

    ;
    ; If SEV or SEV-ES is active, perform a quick sanity check against
    ; the reported encryption bit position. This is to help mitigate
    ; against attacks where the hypervisor reports an incorrect encryption
    ; bit position. If SEV is not active, this check will always succeed.
    ;
    ; The cmp instruction compares the first four bytes of the cmp instruction
    ; itself (which will be read decrypted if SEV or SEV-ES is active and the
    ; encryption bit position is valid) against the immediate within the
    ; instruction (an instruction fetch is always decrypted correctly by
    ; hardware) based on RIP relative addressing.
    ;
InsnCompare:
    cmp     dword[rel InsnCompare], 0xFFF63D81
    je      GoodCompare

    ;
    ; The hypervisor provided an incorrect encryption bit position, do not
    ; proceed.
    ;
SevEncBitHlt:
    cli
    hlt
    jmp     SevEncBitHlt

GoodCompare:
    debugShowPostCode POSTCODE_64BIT_MODE

    OneTimeCallRet Transition32FlatTo64Flat

