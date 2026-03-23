;------------------------------------------------------------------------------
; @file
; Sets the CR3 register for 64-bit paging
;
; Copyright (c) 2008 - 2023, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

;
; Modified:  EAX
;
SetCr3ForPageTables64:

    ;
    ; These pages are built into the ROM image in X64/PageTables.asm
    ;
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

    OneTimeCallRet SetCr3ForPageTables64

