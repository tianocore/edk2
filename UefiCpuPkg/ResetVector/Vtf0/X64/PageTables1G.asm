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

;
; Page table non-leaf entry
;
%define PAGE_NLE(address) (ADDR_OF(address) + \
                    PAGE_NLE_ATTR)

%define PAGE_PDPTE_1GB(x) ((x << 30) + PAGE_BLE_ATTR)

ALIGN 16

Pml4:
    ;
    ; PML4 (1 * 512GB entry)
    ;
    DQ      PAGE_NLE(Pdp)
    TIMES   0x1000 - ($ - Pml4) DB 0

Pdp:
    ;
    ; Page-directory pointer table (512 * 1GB entries => 512GB)
    ;
%assign i 0
%rep      512
    DQ    PAGE_PDPTE_1GB(i)
    %assign i i+1
%endrep

EndOfPageTables:
