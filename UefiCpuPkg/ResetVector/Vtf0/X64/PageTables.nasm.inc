;------------------------------------------------------------------------------
; @file
; Emits Page Tables for 1:1 mapping.
; If using 1G page table, map addresses 0 - (PAGE_TABLE_SIZE_X_512GB * 512GB)
; (default 512GB, 0 - 0x8000000000), else, map addresses 0 - 0x100000000 (4GB).
;
; Copyright (c) 2021 - 2023, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
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
%define PAGE_PDE_2MB(x) ((x << 21) + PAGE_BLE_ATTR)

ALIGN 16

%ifdef PAGE_TABLE_1G

;
; PAGE_TABLE_SIZE_X_512GB is the number of 512GB regions to map. Each region
; needs one PDPT (512 PDPTEs * 1GB = 512GB) referenced by one PML4E, so the
; value is limited by the 512 entries of the PML4 (512 * 512GB = 256TB).
; Default to one region (512GB) if it is not specified.
;
%ifndef PAGE_TABLE_SIZE_X_512GB
  %define PAGE_TABLE_SIZE_X_512GB 1
%endif

%if (PAGE_TABLE_SIZE_X_512GB < 1) || (PAGE_TABLE_SIZE_X_512GB > 512)
  %error "PAGE_TABLE_SIZE_X_512GB must be in the range 1 to 512"
%endif

;
; Generate Pdp0..PdpN. PdpK maps 512GB with 1GB pages.
;
%assign PdpIndex 0

%rep PAGE_TABLE_SIZE_X_512GB

Pdp%+PdpIndex:
    %assign i (PdpIndex * 512)

    %rep 512
        DQ      PAGE_PDPTE_1GB(i)
        %assign i i+1
    %endrep

    %assign PdpIndex PdpIndex + 1

%endrep

%else

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

Pdp:
    ;
    ; Page-directory pointer table (4 * 1GB entries => 4GB)
    ;
    DQ      PAGE_NLE(Pd)
    DQ      PAGE_NLE(Pd + 0x1000)
    DQ      PAGE_NLE(Pd + 0x2000)
    DQ      PAGE_NLE(Pd + 0x3000)
    TIMES   0x1000 - ($ - Pdp) DB 0

%endif

Pml4:
    ;
    ; PML4
    ;
%ifdef PAGE_TABLE_1G

    ;
    ; One PML4 entry points to each PDP.
    ;
    %assign i 0

    %rep PAGE_TABLE_SIZE_X_512GB
        DQ      PAGE_NLE(Pdp%+i)
        %assign i i+1
    %endrep

%else

    ;
    ; 4GB configuration.
    ;
    DQ      PAGE_NLE(Pdp)

%endif

    TIMES   0x1000 - ($ - Pml4) DB 0

%ifdef USE_5_LEVEL_PAGE_TABLE
Pml5:
    ;
    ; Pml5 table (only first entry is present, pointing to Pml4)
    ;
    DQ      PAGE_NLE(Pml4)
    TIMES   0x1000 - ($ - Pml5) DB 0
%endif

EndOfPageTables:
