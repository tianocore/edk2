;------------------------------------------------------------------------------
; @file
; Emits Page Tables for 1:1 mapping of the addresses 0 - 0x100000000 (4GB)
;
; Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

BITS    64

%define ALIGN_TOP_TO_4K_FOR_PAGING

%define PAGE_PRESENT            0x01
%define PAGE_READ_WRITE         0x02
%define PAGE_USER_SUPERVISOR    0x04
%define PAGE_WRITE_THROUGH      0x08
%define PAGE_CACHE_DISABLE     0x010
%define PAGE_ACCESSED          0x020
%define PAGE_DIRTY             0x040
%define PAGE_PAT               0x080
%define PAGE_GLOBAL           0x0100
%define PAGE_2M_MBO            0x080
%define PAGE_2M_PAT          0x01000

%define PAGE_2M_PDE_ATTR (PAGE_2M_MBO + \
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

    ;
    ; Top level Page Directory Pointers (1 * 512GB entry)
    ;
    DQ      PDP(0x1000)


    ;
    ; Next level Page Directory Pointers (4 * 1GB entries => 4GB)
    ;
    TIMES 0x1000-PGTBLS_OFFSET($) DB 0

    DQ      PDP(0x2000)
    DQ      PDP(0x3000)
    DQ      PDP(0x4000)
    DQ      PDP(0x5000)

    ;
    ; Page Table Entries (2048 * 2MB entries => 4GB)
    ;
    TIMES 0x2000-PGTBLS_OFFSET($) DB 0

%assign i 0
%rep    0x800
    DQ      PTE_2MB(i)
    %assign i i+1
%endrep

EndOfPageTables:
