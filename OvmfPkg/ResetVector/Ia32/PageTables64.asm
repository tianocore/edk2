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

%define TDX_BSP         1
%define TDX_AP          2

;
; Check if 5-level paging is supported.
; CPUID.(EAX=07H, ECX=0):ECX[bit 16] is a new feature flag
; that will enumerate basic support for 5-level paging
;
; Modified: EAX, ECX
;
; If 5-level paging is supported, EAX is 1
; If 5-level paging is *NOT* supported, EAX is 0
;
IsLevel5PagingSupported:
    mov     eax, 07h
    xor     ecx, ecx
    cpuid
    xor     eax, eax
    test    ecx, 10000h
    jz      ExitIsLevel5PagingSupported
    mov     eax, 1

ExitIsLevel5PagingSupported:
    OneTimeCallRet  IsLevel5PagingSupported

;
; Modified:  EAX, EBX, ECX, EDX
;
SetCr3ForPageTables64:
    ; Check the TDX features.
    ; If it is TDX APs, then jump to SetCr3 directly.
    ; In TD guest the initialization is done by BSP, including building
    ; the page tables. APs will spin on until byte[TDX_WORK_AREA_PGTBL_READY]
    ; is set.
    OneTimeCall   CheckTdxFeaturesBeforeBuildPagetables
    cmp       eax, TDX_BSP
    je        ClearOvmfPageTables
    cmp       eax, TDX_AP
    je        SetCr3

    ; Check whether the SEV is active and populate the SevEsWorkArea
    OneTimeCall   CheckSevFeatures

    ; If SEV is enabled, the C-bit position is always above 31.
    ; The mask will be saved in the EDX and applied during the
    ; the page table build below.
    OneTimeCall   GetSevCBitMaskAbove31

ClearOvmfPageTables:
    ;
    ; For OVMF, build some initial page tables at
    ; PcdOvmfSecPageTablesBase - (PcdOvmfSecPageTablesBase + 0x7000).
    ;
    ; This range should match with PcdOvmfSecPageTablesSize which is
    ; declared in the FDF files.
    ;
    ; At the end of PEI, the pages tables will be rebuilt into a
    ; more permanent location by DxeIpl.
    ;

    mov     ecx, 7 * 0x1000 / 4
    xor     eax, eax
clearPageTablesMemoryLoop:
    mov     dword[ecx * 4 + PT_ADDR (0) - 4], eax
    loop    clearPageTablesMemoryLoop

    ;
    ; Top level Page Directory Pointers (1 * 256TB entry)
    ;
    mov     dword[PT_ADDR (0)], PT_ADDR(0x1000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (4)], edx

    ;
    ; Top level Page Directory Pointers (1 * 512GB entry)
    ;
    mov     dword[PT_ADDR (0x1000)], PT_ADDR (0x2000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x1004)], edx

    ;
    ; Next level Page Directory Pointers (4 * 1GB entries => 4GB)
    ;
    mov     dword[PT_ADDR (0x2000)], PT_ADDR (0x3000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x2004)], edx
    mov     dword[PT_ADDR (0x2008)], PT_ADDR (0x4000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x200C)], edx
    mov     dword[PT_ADDR (0x2010)], PT_ADDR (0x5000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x2014)], edx
    mov     dword[PT_ADDR (0x2018)], PT_ADDR (0x6000) + PAGE_PDP_ATTR
    mov     dword[PT_ADDR (0x201C)], edx

    ;
    ; Page Table Entries (2048 * 2MB entries => 4GB)
    ;
    mov     ecx, 0x800
pageTableEntriesLoop:
    mov     eax, ecx
    dec     eax
    shl     eax, 21
    add     eax, PAGE_2M_PDE_ATTR
    mov     [ecx * 8 + PT_ADDR (0x3000 - 8)], eax
    mov     [(ecx * 8 + PT_ADDR (0x3000 - 8)) + 4], edx
    loop    pageTableEntriesLoop

    ; Clear the C-bit from the GHCB page if the SEV-ES is enabled.
    OneTimeCall   SevClearPageEncMaskForGhcbPage

    ; Set byte[TDX_WORK_AREA_PGTBL_READY] if TDX is enabled.
    OneTimeCall   TdxPostBuildPageTables

SetCr3:
    ;
    ; Set CR3 now that the paging structures are available
    ; But we should determine 4-level or 5-leve paging
    ;
    OneTimeCall IsLevel5PagingSupported
    test    eax, eax
    jnz      Level5Paging

    mov     eax, PT_ADDR (0x1000)
    jmp     SetCr3Value

Level5Paging:
    mov     eax, PT_ADDR (0)

SetCr3Value:
    mov     cr3, eax

    OneTimeCallRet SetCr3ForPageTables64
