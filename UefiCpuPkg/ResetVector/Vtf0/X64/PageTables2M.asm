;------------------------------------------------------------------------------
; @file
; Emits Page Tables for 1:1 mapping of the addresses 0 - 0x100000000 (4GB)
;
; Copyright (c) 2008 - 2023, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    64

%define ALIGN_TOP_TO_4K_FOR_PAGING

;
; Page table big leaf entry attribute:
; PDPTE 1GB entry or PDE 2MB entry
;
%define PAGE_BLE_ATTR   (PAGE_SIZE + \
                          PAGE_ACCESSED + \
                          PAGE_DIRTY + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)

;
; Page table non-leaf entry attribute
;
%define PAGE_NLE_ATTR (PAGE_ACCESSED + \
                        PAGE_READ_WRITE + \
                        PAGE_PRESENT)

%define PAGE_NLE(address) (ADDR_OF(address) + \
                    PAGE_NLE_ATTR)
%define PAGE_PDE_2MB(x) ((x << 21) + PAGE_BLE_ATTR)

Pml4:
    ;
    ; PML4 (1 * 512GB entry)
    ;
    DQ      PAGE_NLE(Pdp)
    TIMES   0x1000 - ($ - Pml4) DB 0

Pdp:
    ;
    ; Page-directory pointer table (4 * 1GB entries => 4GB)
    ;
    DQ      PAGE_NLE(Pd)
    DQ      PAGE_NLE(Pd + 0x1000)
    DQ      PAGE_NLE(Pd + 0x2000)
    DQ      PAGE_NLE(Pd + 0x3000)
    TIMES   0x1000 - ($ - Pdp) DB 0

Pd:
    ;
    ; Page-Directory (2048 * 2MB entries => 4GB)
    ; Four pages below, each is pointed by one entry in Pdp.
    ;
%assign i 0
%rep    0x800
    DQ      PAGE_PDE_2MB(i)
    %assign i i+1
%endrep

EndOfPageTables:
