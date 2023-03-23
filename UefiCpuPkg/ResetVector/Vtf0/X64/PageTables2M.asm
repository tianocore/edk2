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

%define PAGE_2M_PDE_ATTR (PAGE_SIZE + \
                          PAGE_ACCESSED + \
                          PAGE_DIRTY + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)

%define PAGE_PDP_ATTR (PAGE_ACCESSED + \
                       PAGE_READ_WRITE + \
                       PAGE_PRESENT)

%define PGTBLS_OFFSET(x) ((x) - TopLevelPageDirectory)
%define PGTBLS_ADDR(x) (ADDR_OF(TopLevelPageDirectory) + (x))

%define PDP(offset) (ADDR_OF(TopLevelPageDirectory) + (offset) + \
                     PAGE_PDP_ATTR)
%define PTE_2MB(x) ((x << 21) + PAGE_2M_PDE_ATTR)

TopLevelPageDirectory:

%assign i 0
%rep    0x800
    DQ      PTE_2MB(i)
    %assign i i+1
%endrep

    ;
    ; Page-directory pointer table Pointers (4 * 1GB entries => 4GB)
    ;
    DQ      PDP(0)
    DQ      PDP(0x1000)
    DQ      PDP(0x2000)
    DQ      PDP(0x3000)
    TIMES   0x4000-PGTBLS_OFFSET($) DB 0

    ;
    ; PML4 table Pointers (1 * 512GB entry)
    ;
    DQ      PDP(0x4000)

    TIMES   0x5000-PGTBLS_OFFSET($) DB 0
