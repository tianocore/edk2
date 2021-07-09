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
; Modified:  EAX, ECX, EDX
;
Transition32FlatTo64Flat:

    OneTimeCall SetCr3ForPageTables64

    cmp     dword[TDX_WORK_AREA], 0x47584454 ; 'TDXG'
    jz      TdxTransition32FlatTo64Flat

    mov     eax, cr4
    bts     eax, 5                      ; enable PAE
    mov     cr4, eax

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

    jmp     _jumpTo64Bit

;
; Tdx Transition from 32Flat to 64Flat
;
TdxTransition32FlatTo64Flat:

    mov     eax, cr4
    bts     eax, 5                      ; enable PAE

    ;
    ; byte[TDX_WORK_AREA_PAGELEVEL5] holds the indicator whether 52bit is supported.
    ; if it is the case, need to set LA57 and use 5-level paging
    ;
    cmp     byte[TDX_WORK_AREA_PAGELEVEL5], 0
    jz      .set_cr4
    bts     eax, 12
.set_cr4:
    mov     cr4, eax
    mov     ebx, cr3

    ;
    ; if la57 is not set, we are ok
    ; if using 5-level paging, adjust top-level page directory
    ;
    bt      eax, 12
    jnc     .set_cr3
    mov     ebx, TDX_PT_ADDR (0)
.set_cr3:
    mov     cr3, ebx

    mov     eax, cr0
    bts     eax, 31                     ; set PG
    mov     cr0, eax                    ; enable paging

_jumpTo64Bit:
    jmp     LINEAR_CODE64_SEL:ADDR_OF(jumpTo64BitAndLandHere)

BITS    64
jumpTo64BitAndLandHere:

    ;
    ; For Td guest we are done and jump to the end
    ;
    mov     eax, TDX_WORK_AREA
    cmp     dword [eax], 0x47584454 ; 'TDXG'
    jz      GoodCompare

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

