;------------------------------------------------------------------------------
; @file
; Emits Page Tables for 1:1 mapping of the addresses 0 - 0x8000000000 (512GB)
;
; Copyright (c) 2021 - 2023, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
; Linear-Address Translation to a 1-GByte Page
;
;------------------------------------------------------------------------------

BITS    64

%define ALIGN_TOP_TO_4K_FOR_PAGING

;
; Page table non-leaf entry attribute
;
%define PAGE_NLE_ATTR (PAGE_ACCESSED + \
                        PAGE_READ_WRITE + \
                        PAGE_PRESENT)

;
; Page table big leaf entry attribute:
; PDPTE 1GB entry or PDE 2MB entry
;
%define PAGE_BLE_ATTR (PAGE_ACCESSED + \
                        PAGE_READ_WRITE + \
                        PAGE_DIRTY + \
                        PAGE_PRESENT + \
                        PAGE_SIZE)

%define PGTBLS_OFFSET(x) ((x) - TopLevelPageDirectory)
%define PGTBLS_ADDR(x) (ADDR_OF(TopLevelPageDirectory) + (x))

;
; Page table non-leaf entry
;
%define PAGE_NLE(offset) (ADDR_OF(TopLevelPageDirectory) + (offset) + \
                    PAGE_NLE_ATTR)

%define PAGE_PDPTE_1GB(x) ((x << 30) + PAGE_BLE_ATTR)

ALIGN 16

TopLevelPageDirectory:

    ;
    ; Top level Page Directory Pointers (1 * 512GB entry)
    ;
    DQ      PAGE_NLE(0x1000)

    TIMES 0x1000-PGTBLS_OFFSET($) DB 0
    ;
    ; Next level Page Directory Pointers (512 * 1GB entries => 512GB)
    ;
%assign i 0
%rep      512
    DQ    PAGE_PDPTE_1GB(i)
    %assign i i+1
%endrep
    TIMES 0x2000-PGTBLS_OFFSET($) DB 0

EndOfPageTables:
