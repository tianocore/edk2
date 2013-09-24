;------------------------------------------------------------------------------
; @file
; Sets the CR3 register for 64-bit paging
;
; Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

BITS    32

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


;
; Modified:  EAX, ECX
;
SetCr3ForPageTables64:

    ;
    ; For OVMF, build some initial page tables at 0x80000. This is just
    ; after the early stack/temp RAM.
    ;
    ; At the end of PEI, the pages tables will be rebuilt into a
    ; more permanent location by DxeIpl.
    ;

    mov     ecx, 6 * 0x1000 / 4
    xor     eax, eax
clearPageTablesMemoryLoop:
    mov     dword[ecx * 4 + 0x80000 - 4], eax
    loop    clearPageTablesMemoryLoop

    ;
    ; Top level Page Directory Pointers (1 * 512GB entry)
    ;
    mov     dword[0x80000], 0x81000 + PAGE_PDP_ATTR

    ;
    ; Next level Page Directory Pointers (4 * 1GB entries => 4GB)
    ;
    mov     dword[0x81000], 0x82000 + PAGE_PDP_ATTR
    mov     dword[0x81008], 0x83000 + PAGE_PDP_ATTR
    mov     dword[0x81010], 0x84000 + PAGE_PDP_ATTR
    mov     dword[0x81018], 0x85000 + PAGE_PDP_ATTR

    ;
    ; Page Table Entries (2048 * 2MB entries => 4GB)
    ;
    mov     ecx, 0x800
pageTableEntriesLoop:
    mov     eax, ecx
    dec     eax
    shl     eax, 21
    add     eax, PAGE_2M_PDE_ATTR
    mov     [ecx * 8 + 0x82000 - 8], eax
    loop    pageTableEntriesLoop

    ;
    ; Set CR3 now that the paging structures are available
    ;
    mov     eax, 0x80000
    mov     cr3, eax

    OneTimeCallRet SetCr3ForPageTables64
