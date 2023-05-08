;------------------------------------------------------------------------------
; @file
; Transition from 32 bit flat protected mode into 64 bit flat protected mode
;
; Copyright (c) 2008 - 2023, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

;
; Modified:  EAX
;
Transition32FlatTo64Flat:

%ifdef USE_5_LEVEL_PAGE_TABLE
    mov     eax, 0
    cpuid
    cmp     eax, 07h                    ; check if basic CPUID leaf contains leaf 07
    jb      NotSupport5LevelPaging      ; 5level paging not support, downgrade to 4level paging
    mov     eax, 07h                    ; check cpuid leaf 7, subleaf 0
    mov     ecx, 0
    cpuid
    bt      ecx, 16                     ; [Bits 16] Supports 5-level paging if 1.
    jnc     NotSupport5LevelPaging      ; 5level paging not support, downgrade to 4level paging
    mov     eax, ADDR_OF(Pml5)
    mov     cr3, eax
    mov     eax, cr4
    bts     eax, 12                     ; Set LA57=1.
    mov     cr4, eax
    jmp     SetCr3Done
NotSupport5LevelPaging:
%endif

    mov     eax, ADDR_OF(Pml4)
    mov     cr3, eax
SetCr3Done:

    mov     eax, cr4
    bts     eax, 5                      ; enable PAE
    mov     cr4, eax

    mov     ecx, 0xc0000080
    rdmsr
    bts     eax, 8                      ; set LME
    wrmsr

    mov     eax, cr0
    bts     eax, 31                     ; set PG
    mov     cr0, eax                    ; enable paging

    jmp     LINEAR_CODE64_SEL:ADDR_OF(jumpTo64BitAndLandHere)
BITS    64
jumpTo64BitAndLandHere:

    debugShowPostCode POSTCODE_64BIT_MODE

    OneTimeCallRet Transition32FlatTo64Flat

