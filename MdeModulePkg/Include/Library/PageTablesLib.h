/** @file
  Define PageTables library helper function
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PAGETABLES_LIB_H_
#define PAGETABLES_LIB_H_

#include <Base.h>

#pragma pack(1)

typedef union {
  struct {
    UINT32    LimitLow    : 16;
    UINT32    BaseLow     : 16;
    UINT32    BaseMid     : 8;
    UINT32    Type        : 4;
    UINT32    System      : 1;
    UINT32    Dpl         : 2;
    UINT32    Present     : 1;
    UINT32    LimitHigh   : 4;
    UINT32    Software    : 1;
    UINT32    Reserved    : 1;
    UINT32    DefaultSize : 1;
    UINT32    Granularity : 1;
    UINT32    BaseHigh    : 8;
  } Bits;
  UINT64    Uint64;
} IA32_GDT;

typedef struct {
  IA32_IDT_GATE_DESCRIPTOR    Ia32IdtEntry;
  UINT32                      Offset32To63;
  UINT32                      Reserved;
} X64_IDT_GATE_DESCRIPTOR;

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
    UINT64    Pat                  : 1;  //
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
    UINT64    Pat                  : 1;  //
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
    UINT64    Pat                  : 1;  //
    UINT64    MustBeZero           : 17; // Must be zero;
    UINT64    PageTableBaseAddress : 22; // Page Table Base Address
    UINT64    AvabilableHigh       : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code,
                                         //   1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_1G_ENTRY;

#pragma pack()

#define CR0_WP  BIT16

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

typedef
EFI_STATUS
(EFIAPI *CC_BIT_SET_CLEAR_HOOK)(
  IN  UINT64  Param0,
  IN  UINT64  Param1,
  IN  UINT64  Param2,
  IN  UINT64  Param3
  );

typedef struct {
  BOOLEAN    PcdSetNxForStack;
  BOOLEAN    PcdIa32EferChangeAllowed;
  BOOLEAN    PcdCpuStackGuard;
  BOOLEAN    PcdUse1GPageTable;
  BOOLEAN    PcdUse5LevelPageTable;
  UINT8      PcdNullPointerDetectionPropertyMask;
  UINT32     PcdImageProtectionPolicy;
  UINT64     PcdDxeNxMemoryProtectionPolicy;
  UINT64     PcdPteMemoryEncryptionAddressOrMask;
  UINT64     PgTableMask;
} PAGE_TABLES_PCD_SETTINGS;

/**
  This function calls the Hook for the memory region specified by PhysicalAddress
  and Length from the current page table context. The Hook (provided by the caller)
  set / clear the Confidential Computing bit, for example, SEV sets or clears
  memory encryption bit, TDX sets or clears memory shared bit.

  The function iterates through the PhysicalAddress one page at a time, and call
  the Hook in the page table. If it encounters that a given physical
  address range is part of large page then it attempts to change the attribute at
  one go (based on size), otherwise it splits the large pages into smaller
  (e.g 2M page into 4K pages) and then try to call the Hook on the smallest
  page size.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3).
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region.
  @param[in]  Hook                    Hook function provided by caller to set / clear
                                      Confidential Computing (CC) bit for the memory region.

  @retval EFI_SUCCESS                 The Encryption related bit were successfully
                                      set / cleared for the memory region.
  @retval EFI_NO_MAPPING              The physical address not present in memory.

 **/
EFI_STATUS
EFIAPI
SetClearCcBits (
  IN    PHYSICAL_ADDRESS       Cr3BaseAddress,
  IN    PHYSICAL_ADDRESS       PhysicalAddress,
  IN    UINTN                  Length,
  IN    CC_BIT_SET_CLEAR_HOOK  Hook
  );

/**
  Set the Page Tables PCD Settings.

  @param   Settings                Pointer to the Settings
  @return  EFI_SUCCESS             Successfully set the PCDs
  @return  EFI_INVALID_PARAMETER   Invalid input parameter
**/
EFI_STATUS
EFIAPI
SetPageTablesPcdSettings (
  PAGE_TABLES_PCD_SETTINGS  *Settings
  );

/**
  Clear legacy memory located at the first 4K-page, if available.

  This function traverses the whole HOB list to check if memory from 0 to 4095
  exists and has not been allocated, and then clear it if so.

  @param HobStart                  The start of HobList passed to DxeCore.

**/
VOID
EFIAPI
ClearFirst4KPage (
  IN  VOID  *HobStart
  );

/**
 Check the WP status in CR0 register. This bit is used to lock or unlock write
 access to pages marked as read-only.

  @retval TRUE    Write protection is enabled.
  @retval FALSE   Write protection is disabled.
**/
BOOLEAN
EFIAPI
IsReadOnlyPageWriteProtected (
  VOID
  );

/**
 Disable Write Protect on pages marked as read-only.
**/
VOID
EFIAPI
DisableReadOnlyPageWriteProtect (
  VOID
  );

/**
 Enable Write Protect on pages marked as read-only.
**/
VOID
EFIAPI
EnableReadOnlyPageWriteProtect (
  VOID
  );

/**
  Prevent the memory pages used for page table from been overwritten.

  @param[in] PageTableBase    Base address of page table (CR3).
  @param[in] Level4Paging     Level 4 paging flag.

**/
VOID
EFIAPI
EnablePageTableProtection (
  IN  UINTN    PageTableBase,
  IN  BOOLEAN  Level4Paging
  );

/**
  Return configure status of NULL pointer detection feature.

  @return TRUE   NULL pointer detection feature is enabled
  @return FALSE  NULL pointer detection feature is disabled

**/
BOOLEAN
EFIAPI
IsNullDetectionEnabled (
  VOID
  );

/**
  Set one page of page table pool memory to be read-only.

  @param[in] PageTableBase    Base address of page table (CR3).
  @param[in] Address          Start address of a page to be set as read-only.
  @param[in] Level4Paging     Level 4 paging flag.

**/
VOID
EFIAPI
SetPageTablePoolReadOnly (
  IN  UINTN                 PageTableBase,
  IN  EFI_PHYSICAL_ADDRESS  Address,
  IN  BOOLEAN               Level4Paging
  );

/**
  Split 2M page to 4K.

  @param[in]      PhysicalAddress       Start physical address the 2M page covered.
  @param[in, out] PageEntry2M           Pointer to 2M page entry.
  @param[in]      StackBase             Stack base address.
  @param[in]      StackSize             Stack size.
  @param[in]      GhcbBase              GHCB page area base address.
  @param[in]      GhcbSize              GHCB page area size.

**/
VOID
EFIAPI
Split2MPageTo4K (
  IN EFI_PHYSICAL_ADDRESS  PhysicalAddress,
  IN OUT UINT64            *PageEntry2M,
  IN EFI_PHYSICAL_ADDRESS  StackBase,
  IN UINTN                 StackSize,
  IN EFI_PHYSICAL_ADDRESS  GhcbBase,
  IN UINTN                 GhcbSize
  );

/**
  Split 1G page to 2M.

  @param[in]      PhysicalAddress       Start physical address the 1G page covered.
  @param[in, out] PageEntry1G           Pointer to 1G page entry.
  @param[in]      StackBase             Stack base address.
  @param[in]      StackSize             Stack size.
  @param[in]      GhcbBase              GHCB page area base address.
  @param[in]      GhcbSize              GHCB page area size.

**/
VOID
EFIAPI
Split1GPageTo2M (
  IN EFI_PHYSICAL_ADDRESS  PhysicalAddress,
  IN OUT UINT64            *PageEntry1G,
  IN EFI_PHYSICAL_ADDRESS  StackBase,
  IN UINTN                 StackSize,
  IN EFI_PHYSICAL_ADDRESS  GhcbBase,
  IN UINTN                 GhcbSize
  );

/**
  Allocates and fills in the Page Directory and Page Table Entries to
  establish a 1:1 Virtual to Physical mapping.

  @param[in] StackBase  Stack base address.
  @param[in] StackSize  Stack size.
  @param[in] GhcbBase   GHCB base address.
  @param[in] GhcbSize   GHCB size.

  @return The address of 4 level page map.

**/
UINTN
EFIAPI
CreateIdentityMappingPageTables (
  IN EFI_PHYSICAL_ADDRESS  StackBase,
  IN UINTN                 StackSize,
  IN EFI_PHYSICAL_ADDRESS  GhcbBase,
  IN UINTN                 GhcbSize
  );

#endif
