;------------------------------------------------------------------------------
; @file
; Sets the CR3 register for 64-bit paging
;
; Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
; Copyright (c) 2017 - 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
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

%define PAGE_4K_PDE_ATTR (PAGE_ACCESSED + \
                          PAGE_DIRTY + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)

%define PAGE_2M_PDE_ATTR (PAGE_2M_MBO + \
                          PAGE_ACCESSED + \
                          PAGE_DIRTY + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)

%define PAGE_PDP_ATTR (PAGE_ACCESSED + \
                       PAGE_READ_WRITE + \
                       PAGE_PRESENT)

;
; Modified:  EAX, EBX, ECX, EDX
;
SetCr3ForPageTables64:

    OneTimeCall   CheckSevFeatures
    xor     edx, edx
    test    eax, eax
    jz      SevNotActive

    ; If SEV is enabled, C-bit is always above 31
    sub     eax, 32
    bts     edx, eax

SevNotActive:

    ;
    ; For OVMF, build some initial page tables at
    ; PcdOvmfSecPageTablesBase - (PcdOvmfSecPageTablesBase + 0x6000).
    ;
    ; This range should match with PcdOvmfSecPageTablesSize which is
    ; declared in the FDF files.
    ;
    ; At the end of PEI, the pages tables will be rebuilt into a
    ; more permanent location by DxeIpl.
    ;

    mov     ecx, 6 * 0x1000 / 4
    xor     eax, eax
clearPageTablesMemoryLoop:
    mov     dword[ecx * 4 + PT_ADDR (0) - 4], eax
    loop    clearPageTablesMemoryLoop

    ;
    ; Top level Page Directory Pointers (1 * 512GB entry)
    ;
    mov     dword[PT_ADDR (0)], PT_ADDR (0x1000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (4)], edx

    ;
    ; Next level Page Directory Pointers (4 * 1GB entries => 4GB)
    ;
    mov     dword[PT_ADDR (0x1000)], PT_ADDR (0x2000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x1004)], edx
    mov     dword[PT_ADDR (0x1008)], PT_ADDR (0x3000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x100C)], edx
    mov     dword[PT_ADDR (0x1010)], PT_ADDR (0x4000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x1014)], edx
    mov     dword[PT_ADDR (0x1018)], PT_ADDR (0x5000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x101C)], edx

    ;
    ; Page Table Entries (2048 * 2MB entries => 4GB)
    ;
    mov     ecx, 0x800
pageTableEntriesLoop:
    mov     eax, ecx
    dec     eax
    shl     eax, 21
    add     eax, PAGE_2M_PDE_ATTR
    mov     [ecx * 8 + PT_ADDR (0x2000 - 8)], eax
    mov     [(ecx * 8 + PT_ADDR (0x2000 - 8)) + 4], edx
    loop    pageTableEntriesLoop

    OneTimeCall   IsSevEsEnabled
    test    eax, eax
    jz      SetCr3

    ;
    ; The initial GHCB will live at GHCB_BASE and needs to be un-encrypted.
    ; This requires the 2MB page for this range be broken down into 512 4KB
    ; pages.  All will be marked encrypted, except for the GHCB.
    ;
    mov     ecx, (GHCB_BASE >> 21)
    mov     eax, GHCB_PT_ADDR + PAGE_PDP_ATTR
    mov     [ecx * 8 + PT_ADDR (0x2000)], eax

    ;
    ; Page Table Entries (512 * 4KB entries => 2MB)
    ;
    mov     ecx, 512
pageTableEntries4kLoop:
    mov     eax, ecx
    dec     eax
    shl     eax, 12
    add     eax, GHCB_BASE & 0xFFE0_0000
    add     eax, PAGE_4K_PDE_ATTR
    mov     [ecx * 8 + GHCB_PT_ADDR - 8], eax
    mov     [(ecx * 8 + GHCB_PT_ADDR - 8) + 4], edx
    loop    pageTableEntries4kLoop

    ;
    ; Clear the encryption bit from the GHCB entry
    ;
    mov     ecx, (GHCB_BASE & 0x1F_FFFF) >> 12
    mov     [ecx * 8 + GHCB_PT_ADDR + 4], strict dword 0

    mov     ecx, GHCB_SIZE / 4
    xor     eax, eax
clearGhcbMemoryLoop:
    mov     dword[ecx * 4 + GHCB_BASE - 4], eax
    loop    clearGhcbMemoryLoop

SetCr3:
    ;
    ; Set CR3 now that the paging structures are available
    ;
    mov     eax, PT_ADDR (0)
    mov     cr3, eax

    OneTimeCallRet SetCr3ForPageTables64
