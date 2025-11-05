/** @file

  Virtual Memory Management Services to set or clear the memory encryption bit

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Code is derived from MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.h

**/

#ifndef TDX_VIRTUAL_MEMORY_
#define TDX_VIRTUAL_MEMORY_

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi.h>

#define SYS_CODE64_SEL  0x38

#pragma pack(1)

//
// Page-Map Level-4 Offset (PML4) and
// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
//

typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory,
                                         //   1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching,
                                         //   1 = Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed,
                                         //   1 = Accessed (set by CPU)
    UINT64    Reserved             : 1;  // Reserved
    UINT64    MustBeZero           : 2;  // Must Be Zero
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    AvabilableHigh       : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} PAGE_MAP_AND_DIRECTORY_POINTER;

//
// Page Table Entry 4KB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory,
                                         //   1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching,
                                         //   1 = Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed,
                                         //   1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by
                                         //   processor on access to page
    UINT64    PAT                  : 1;  //
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page
                                         //   TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    AvabilableHigh       : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code,
                                         //   1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_4K_ENTRY;

//
// Page Table Entry 2MB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory,
                                         //   1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching,
                                         //   1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed,
                                         //   1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by
                                         //   processor on access to page
    UINT64    MustBe1              : 1;  // Must be 1
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page
                                         //   TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PAT                  : 1;  //
    UINT64    MustBeZero           : 8;  // Must be zero;
    UINT64    PageTableBaseAddress : 31; // Page Table Base Address
    UINT64    AvabilableHigh       : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code,
                                         //   1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_ENTRY;

//
// Page Table Entry 1GB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory,
                                         //   1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching,
                                         //   1 = Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed,
                                         //   1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by
                                         //   processor on access to page
    UINT64    MustBe1              : 1;  // Must be 1
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page
                                         //   TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PAT                  : 1;  //
    UINT64    MustBeZero           : 17; // Must be zero;
    UINT64    PageTableBaseAddress : 22; // Page Table Base Address
    UINT64    AvabilableHigh       : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code,
                                         //   1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_1G_ENTRY;

#pragma pack()

#define IA32_PG_P   BIT0
#define IA32_PG_RW  BIT1
#define IA32_PG_PS  BIT7

#define PAGING_PAE_INDEX_MASK  0x1FF

#define PAGING_4K_ADDRESS_MASK_64  0x000FFFFFFFFFF000ull
#define PAGING_2M_ADDRESS_MASK_64  0x000FFFFFFFE00000ull
#define PAGING_1G_ADDRESS_MASK_64  0x000FFFFFC0000000ull

#define PAGING_L1_ADDRESS_SHIFT  12
#define PAGING_L2_ADDRESS_SHIFT  21
#define PAGING_L3_ADDRESS_SHIFT  30
#define PAGING_L4_ADDRESS_SHIFT  39

#define PAGING_PML4E_NUMBER  4

#define PAGETABLE_ENTRY_MASK  ((1UL << 9) - 1)
#define PML4_OFFSET(x)  ( (x >> 39) & PAGETABLE_ENTRY_MASK)
#define PDP_OFFSET(x)   ( (x >> 30) & PAGETABLE_ENTRY_MASK)
#define PDE_OFFSET(x)   ( (x >> 21) & PAGETABLE_ENTRY_MASK)
#define PTE_OFFSET(x)   ( (x >> 12) & PAGETABLE_ENTRY_MASK)
#define PAGING_1G_ADDRESS_MASK_64  0x000FFFFFC0000000ull

#define PAGE_TABLE_POOL_ALIGNMENT  BASE_2MB
#define PAGE_TABLE_POOL_UNIT_SIZE  SIZE_2MB
#define PAGE_TABLE_POOL_UNIT_PAGES  \
  EFI_SIZE_TO_PAGES (PAGE_TABLE_POOL_UNIT_SIZE)
#define PAGE_TABLE_POOL_ALIGN_MASK  \
  (~(EFI_PHYSICAL_ADDRESS)(PAGE_TABLE_POOL_ALIGNMENT - 1))

typedef struct {
  VOID     *NextPool;
  UINTN    Offset;
  UINTN    FreePages;
} PAGE_TABLE_POOL;

#endif
