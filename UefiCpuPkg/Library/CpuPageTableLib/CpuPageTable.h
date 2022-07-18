/** @file
  Internal header for CpuPageTableLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_PAGE_TABLE_H_
#define CPU_PAGE_TABLE_H_

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuPageTableLib.h>

#define IA32_PE_BASE_ADDRESS_MASK_40  0xFFFFFFFFFF000ull
#define IA32_PE_BASE_ADDRESS_MASK_39  0xFFFFFFFFFE000ull

#define REGION_LENGTH(l)  LShiftU64 (1, (l) * 9 + 3)

typedef enum {
  Pte   = 1,
  Pde   = 2,
  Pdpte = 3,
  Pml4  = 4,
  Pml5  = 5
} IA32_PAGE_LEVEL;

typedef struct {
  UINT64    Present        : 1;       // 0 = Not present in memory, 1 = Present in memory
  UINT64    ReadWrite      : 1;       // 0 = Read-Only, 1= Read/Write
  UINT64    UserSupervisor : 1;       // 0 = Supervisor, 1=User
  UINT64    Reserved       : 58;
  UINT64    Nx             : 1;        // No Execute bit
} IA32_PAGE_COMMON_ENTRY;

///
/// Format of a non-leaf entry that references a page table entry
///
typedef union {
  struct {
    UINT64    Present              : 1; // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1; // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1; // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1; // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1; // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1; // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Available0           : 1; // Ignored
    UINT64    MustBeZero           : 1; // Must Be Zero

    UINT64    Available2           : 4; // Ignored

    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    Available3           : 11; // Ignored
    UINT64    Nx                   : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} IA32_PAGE_NON_LEAF_ENTRY;

#define IA32_PNLE_PAGE_TABLE_BASE_ADDRESS(pa)  ((pa)->Uint64 & IA32_PE_BASE_ADDRESS_MASK_40)

///
/// Format of a PML5 Entry (PML5E) that References a PML4 Table
///
typedef IA32_PAGE_NON_LEAF_ENTRY IA32_PML5E;

///
/// Format of a PML4 Entry (PML4E) that References a Page-Directory-Pointer Table
///
typedef IA32_PAGE_NON_LEAF_ENTRY IA32_PML4E;

///
/// Format of a Page-Directory-Pointer-Table Entry (PDPTE) that References a Page Directory
///
typedef IA32_PAGE_NON_LEAF_ENTRY IA32_PDPTE;

///
/// Format of a Page-Directory Entry that References a Page Table
///
typedef IA32_PAGE_NON_LEAF_ENTRY IA32_PDE;

///
/// Format of a leaf entry that Maps a 1-Gbyte or 2-MByte Page
///
typedef union {
  struct {
    UINT64    Present              : 1; // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1; // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1; // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1; // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1; // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1; // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1; // 0 = Not dirty, 1 = Dirty (set by CPU)
    UINT64    MustBeOne            : 1; // Page Size. Must Be One

    UINT64    Global               : 1; // 0 = Not global, 1 = Global (if CR4.PGE = 1)
    UINT64    Available1           : 3; // Ignored
    UINT64    Pat                  : 1; // PAT

    UINT64    PageTableBaseAddress : 39; // Page Table Base Address
    UINT64    Available3           : 7;  // Ignored
    UINT64    ProtectionKey        : 4;  // Protection key
    UINT64    Nx                   : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE;
#define IA32_PLEB_PAGE_TABLE_BASE_ADDRESS(pa)  ((pa)->Uint64 & IA32_PE_BASE_ADDRESS_MASK_39)

///
/// Format of a Page-Directory Entry that Maps a 2-MByte Page
///
typedef IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE IA32_PDE_2M;

///
/// Format of a Page-Directory-Pointer-Table Entry (PDPTE) that Maps a 1-GByte Page
///
typedef IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE IA32_PDPTE_1G;

///
/// Format of a Page-Table Entry that Maps a 4-KByte Page
///
typedef union {
  struct {
    UINT64    Present              : 1; // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1; // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1; // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1; // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1; // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1; // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1; // 0 = Not dirty, 1 = Dirty (set by CPU)
    UINT64    Pat                  : 1; // PAT

    UINT64    Global               : 1; // 0 = Not global, 1 = Global (if CR4.PGE = 1)
    UINT64    Available1           : 3; // Ignored

    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    Available3           : 7;  // Ignored
    UINT64    ProtectionKey        : 4;  // Protection key
    UINT64    Nx                   : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} IA32_PTE_4K;
#define IA32_PTE4K_PAGE_TABLE_BASE_ADDRESS(pa)  ((pa)->Uint64 & IA32_PE_BASE_ADDRESS_MASK_40)

///
/// Format of a Page-Directory-Pointer-Table Entry (PDPTE) that References a Page Directory (32bit PAE specific)
///
typedef union {
  struct {
    UINT64    Present              : 1; // 0 = Not present in memory, 1 = Present in memory
    UINT64    MustBeZero           : 2; // Must Be Zero
    UINT64    WriteThrough         : 1; // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1; // 0 = Cached, 1=Non-Cached
    UINT64    MustBeZero2          : 4; // Must Be Zero

    UINT64    Available            : 3; // Ignored

    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    MustBeZero3          : 12; // Must Be Zero
  } Bits;
  UINT64    Uint64;
} IA32_PDPTE_PAE;

typedef union {
  IA32_PAGE_NON_LEAF_ENTRY             Pnle; // To access Pml5, Pml4, Pdpte and Pde.
  IA32_PML5E                           Pml5;
  IA32_PML4E                           Pml4;
  IA32_PDPTE                           Pdpte;
  IA32_PDE                             Pde;

  IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE    PleB; // to access Pdpte1G and Pde2M.
  IA32_PDPTE_1G                        Pdpte1G;
  IA32_PDE_2M                          Pde2M;

  IA32_PTE_4K                          Pte4K;

  IA32_PDPTE_PAE                       PdptePae;
  IA32_PAGE_COMMON_ENTRY               Pce; // To access all common bits in above entries.

  UINT64                               Uint64;
  UINTN                                Uintn;
} IA32_PAGING_ENTRY;

/**
  Return TRUE when the page table entry is a leaf entry that points to the physical address memory.
  Return FALSE when the page table entry is a non-leaf entry that points to the page table entries.

  @param[in] PagingEntry Pointer to the page table entry.
  @param[in] Level       Page level where the page table entry resides in.

  @retval TRUE  It's a leaf entry.
  @retval FALSE It's a non-leaf entry.
**/
BOOLEAN
IsPle (
  IN     IA32_PAGING_ENTRY  *PagingEntry,
  IN     UINTN              Level
  );

/**
  Return the attribute of a 2M/1G page table entry.

  @param[in] PleB               Pointer to a 2M/1G page table entry.
  @param[in] ParentMapAttribute Pointer to the parent attribute.

  @return Attribute of the 2M/1G page table entry.
**/
UINT64
PageTableLibGetPleBMapAttribute (
  IN IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE  *PleB,
  IN IA32_MAP_ATTRIBUTE                 *ParentMapAttribute
  );

/**
  Return the attribute of a non-leaf page table entry.

  @param[in] Pnle               Pointer to a non-leaf page table entry.
  @param[in] ParentMapAttribute Pointer to the parent attribute.

  @return Attribute of the non-leaf page table entry.
**/
UINT64
PageTableLibGetPnleMapAttribute (
  IN IA32_PAGE_NON_LEAF_ENTRY  *Pnle,
  IN IA32_MAP_ATTRIBUTE        *ParentMapAttribute
  );

#endif
