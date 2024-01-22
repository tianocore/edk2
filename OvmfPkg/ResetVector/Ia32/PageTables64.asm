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

; common for all levels
%define PAGE_PRESENT            0x01
%define PAGE_READ_WRITE         0x02
%define PAGE_USER_SUPERVISOR    0x04
%define PAGE_WRITE_THROUGH      0x08
%define PAGE_CACHE_DISABLE     0x010
%define PAGE_ACCESSED          0x020
%define PAGE_DIRTY             0x040
%define PAGE_GLOBAL           0x0100

; page table entries (level 1)
%define PAGE_PTE_PAT           0x080

; page directory entries (level 2+)
%define PAGE_PDE_LARGEPAGE     0x080
%define PAGE_PDE_PAT         0x01000

%define PAGE_4K_PDE_ATTR (PAGE_ACCESSED + \
                          PAGE_DIRTY + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)

%define PAGE_PDE_LARGEPAGE_ATTR (PAGE_PDE_LARGEPAGE + \
                                 PAGE_ACCESSED + \
                                 PAGE_DIRTY + \
                                 PAGE_READ_WRITE + \
                                 PAGE_PRESENT)

%define PAGE_PDE_DIRECTORY_ATTR (PAGE_ACCESSED + \
                                 PAGE_READ_WRITE + \
                                 PAGE_PRESENT)

%define NOT_TDX         0
%define TDX_BSP         1
%define TDX_AP          2
%define TDX_AP_LA57     3

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
    cmp       eax, NOT_TDX
    je        CheckSev
    cmp       eax, TDX_BSP
    je        ClearOvmfPageTables
%if PG_5_LEVEL
    cmp       eax, TDX_AP
    je        SetCr3
    ; TDX_AP_LA57 -> set cr4.la57
    mov       eax, cr4
    bts       eax, 12
    mov       cr4, eax
%endif
    jmp       SetCr3

CheckSev:
    ; Check whether the SEV is active and populate the SevEsWorkArea
    OneTimeCall   CheckSevFeatures

    ; If SEV is enabled, the C-bit position is always above 31.
    ; The mask will be saved in the EDX and applied during the
    ; the page table build below.
    OneTimeCall   GetSevCBitMaskAbove31

ClearOvmfPageTables:
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

%if PG_5_LEVEL

    ; save GetSevCBitMaskAbove31 result (cpuid changes edx)
    mov     edi, edx

    ; check for cpuid leaf 0x07
    mov     eax, 0x00
    cpuid
    cmp     eax, 0x07
    jb      Paging4Lvl

    ; check for la57 (aka 5-level paging)
    mov     eax, 0x07
    mov     ecx, 0x00
    cpuid
    bt      ecx, 16
    jnc     Paging4Lvl

    ; check for cpuid leaf 0x80000001
    mov     eax, 0x80000000
    cpuid
    cmp     eax, 0x80000001
    jb      Paging4Lvl

    ; check for 1g pages
    mov     eax, 0x80000001
    cpuid
    bt      edx, 26
    jnc     Paging4Lvl

    ;
    ; Use 5-level paging with gigabyte pages.
    ;
    ; We have 6 pages available for the early page tables,
    ; we use four of them:
    ;    PT_ADDR(0)      - level 5 directory
    ;    PT_ADDR(0x1000) - level 4 directory
    ;    PT_ADDR(0x2000) - level 2 directory (0 -> 1GB)
    ;    PT_ADDR(0x3000) - level 3 directory
    ;
    ; The level 2 directory for the first gigabyte has the same
    ; physical address in both 4-level and 5-level paging mode,
    ; SevClearPageEncMaskForGhcbPage depends on this.
    ;
    ; The 1 GB -> 4 GB range is mapped using 1G pages in the
    ; level 3 directory.
    ;
    debugShowPostCode 0x51      ; 5-level paging

    ; restore GetSevCBitMaskAbove31 result
    mov     edx, edi

    ; level 5
    mov     dword[PT_ADDR (0)], PT_ADDR (0x1000) + PAGE_PDE_DIRECTORY_ATTR
    mov     dword[PT_ADDR (4)], edx

    ; level 4
    mov     dword[PT_ADDR (0x1000)], PT_ADDR (0x3000) + PAGE_PDE_DIRECTORY_ATTR
    mov     dword[PT_ADDR (0x1004)], edx

    ; level 3 (1x -> level 2, 3x 1GB)
    mov     dword[PT_ADDR (0x3000)], PT_ADDR (0x2000) + PAGE_PDE_DIRECTORY_ATTR
    mov     dword[PT_ADDR (0x3004)], edx
    mov     dword[PT_ADDR (0x3008)], (1 << 30) + PAGE_PDE_LARGEPAGE_ATTR
    mov     dword[PT_ADDR (0x300c)], edx
    mov     dword[PT_ADDR (0x3010)], (2 << 30) + PAGE_PDE_LARGEPAGE_ATTR
    mov     dword[PT_ADDR (0x3014)], edx
    mov     dword[PT_ADDR (0x3018)], (3 << 30) + PAGE_PDE_LARGEPAGE_ATTR
    mov     dword[PT_ADDR (0x301c)], edx

    ;
    ; level 2 (512 * 2MB entries => 1GB)
    ;
    mov     ecx, 0x200
pageTableEntriesLoopLa57:
    mov     eax, ecx
    dec     eax
    shl     eax, 21
    add     eax, PAGE_PDE_LARGEPAGE_ATTR
    mov     [ecx * 8 + PT_ADDR (0x2000 - 8)], eax
    mov     [(ecx * 8 + PT_ADDR (0x2000 - 8)) + 4], edx
    loop    pageTableEntriesLoopLa57

    ; set la57 bit in cr4
    mov     eax, cr4
    bts     eax, 12
    mov     cr4, eax

    ; done
    jmp     PageTablesReadyLa57

Paging4Lvl:
    debugShowPostCode 0x41      ; 4-level paging

    ; restore GetSevCBitMaskAbove31 result
    mov     edx, edi

%endif ; PG_5_LEVEL

    ;
    ; Top level Page Directory Pointers (1 * 512GB entry)
    ;
    mov     dword[PT_ADDR (0)], PT_ADDR (0x1000) + PAGE_PDE_DIRECTORY_ATTR
    mov     dword[PT_ADDR (4)], edx

    ;
    ; Next level Page Directory Pointers (4 * 1GB entries => 4GB)
    ;
    mov     dword[PT_ADDR (0x1000)], PT_ADDR (0x2000) + PAGE_PDE_DIRECTORY_ATTR
    mov     dword[PT_ADDR (0x1004)], edx
    mov     dword[PT_ADDR (0x1008)], PT_ADDR (0x3000) + PAGE_PDE_DIRECTORY_ATTR
    mov     dword[PT_ADDR (0x100C)], edx
    mov     dword[PT_ADDR (0x1010)], PT_ADDR (0x4000) + PAGE_PDE_DIRECTORY_ATTR
    mov     dword[PT_ADDR (0x1014)], edx
    mov     dword[PT_ADDR (0x1018)], PT_ADDR (0x5000) + PAGE_PDE_DIRECTORY_ATTR
    mov     dword[PT_ADDR (0x101C)], edx

    ;
    ; Page Table Entries (2048 * 2MB entries => 4GB)
    ;
    mov     ecx, 0x800
pageTableEntriesLoop:
    mov     eax, ecx
    dec     eax
    shl     eax, 21
    add     eax, PAGE_PDE_LARGEPAGE_ATTR
    mov     [ecx * 8 + PT_ADDR (0x2000 - 8)], eax
    mov     [(ecx * 8 + PT_ADDR (0x2000 - 8)) + 4], edx
    loop    pageTableEntriesLoop

%if PG_5_LEVEL

PageTablesReadyLa57:
    ; TDX will do some PostBuildPages task, such as setting
    ; byte[TDX_WORK_AREA_PGTBL_READY].
    OneTimeCall   TdxPostBuildPageTablesLa57
    jmp SevPostBuildPageTables

%endif

PageTablesReady:
    ; TDX will do some PostBuildPages task, such as setting
    ; byte[TDX_WORK_AREA_PGTBL_READY].
    OneTimeCall   TdxPostBuildPageTables

SevPostBuildPageTables:
    ; Clear the C-bit from the GHCB page if the SEV-ES is enabled.
    OneTimeCall   SevClearPageEncMaskForGhcbPage

SetCr3:
    ;
    ; Set CR3 now that the paging structures are available
    ;
    mov     eax, PT_ADDR (0)
    mov     cr3, eax

    OneTimeCallRet SetCr3ForPageTables64
