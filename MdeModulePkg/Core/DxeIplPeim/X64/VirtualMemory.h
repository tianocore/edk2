/** @file
  x64 Long Mode Virtual Memory Management Definitions

  References:
    1) IA-32 Intel(R) Architecture Software Developer's Manual Volume 1:Basic Architecture, Intel
    2) IA-32 Intel(R) Architecture Software Developer's Manual Volume 2:Instruction Set Reference, Intel
    3) IA-32 Intel(R) Architecture Software Developer's Manual Volume 3:System Programmer's Guide, Intel
    4) AMD64 Architecture Programmer's Manual Volume 2: System Programming

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTUAL_MEMORY_H_
#define _VIRTUAL_MEMORY_H_

#define SYS_CODE64_SEL  0x38

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
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
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
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    PAT                  : 1;  //
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    AvabilableHigh       : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_4K_ENTRY;

//
// Page Table Entry 2MB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    MustBe1              : 1;  // Must be 1
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PAT                  : 1;  //
    UINT64    MustBeZero           : 8;  // Must be zero;
    UINT64    PageTableBaseAddress : 31; // Page Table Base Address
    UINT64    AvabilableHigh       : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_ENTRY;

//
// Page Table Entry 1GB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    MustBe1              : 1;  // Must be 1
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PAT                  : 1;  //
    UINT64    MustBeZero           : 17; // Must be zero;
    UINT64    PageTableBaseAddress : 22; // Page Table Base Address
    UINT64    AvabilableHigh       : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code, 1 = No Code Execution
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

#define PAGE_TABLE_POOL_ALIGNMENT   BASE_2MB
#define PAGE_TABLE_POOL_UNIT_SIZE   SIZE_2MB
#define PAGE_TABLE_POOL_UNIT_PAGES  EFI_SIZE_TO_PAGES (PAGE_TABLE_POOL_UNIT_SIZE)
#define PAGE_TABLE_POOL_ALIGN_MASK  \
  (~(EFI_PHYSICAL_ADDRESS)(PAGE_TABLE_POOL_ALIGNMENT - 1))

typedef struct {
  VOID     *NextPool;
  UINTN    Offset;
  UINTN    FreePages;
} PAGE_TABLE_POOL;

/**
  Check if Execute Disable Bit (IA32_EFER.NXE) should be enabled or not.

  @retval TRUE    IA32_EFER.NXE should be enabled.
  @retval FALSE   IA32_EFER.NXE should not be enabled.

**/
BOOLEAN
IsEnableNonExecNeeded (
  VOID
  );

/**
  Enable Execute Disable Bit.

**/
VOID
EnableExecuteDisableBit (
  VOID
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
Split2MPageTo4K (
  IN EFI_PHYSICAL_ADDRESS  PhysicalAddress,
  IN OUT UINT64            *PageEntry2M,
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
  @param[in] GhcbBase   GHCB page area base address.
  @param[in] GhcbSize   GHCB page area size.

  @return The address of 4 level page map.

**/
UINTN
CreateIdentityMappingPageTables (
  IN EFI_PHYSICAL_ADDRESS  StackBase,
  IN UINTN                 StackSize,
  IN EFI_PHYSICAL_ADDRESS  GhcbBase,
  IN UINTN                 GhcbkSize
  );

/**

  Fix up the vector number in the vector code.

  @param VectorBase   Base address of the vector handler.
  @param VectorNum    Index of vector.

**/
VOID
EFIAPI
AsmVectorFixup (
  VOID   *VectorBase,
  UINT8  VectorNum
  );

/**

  Get the information of vector template.

  @param TemplateBase   Base address of the template code.

  @return               Size of the Template code.

**/
UINTN
EFIAPI
AsmGetVectorTemplatInfo (
  OUT   VOID  **TemplateBase
  );

/**
  Clear legacy memory located at the first 4K-page.

  This function traverses the whole HOB list to check if memory from 0 to 4095
  exists and has not been allocated, and then clear it if so.

  @param HobStart         The start of HobList passed to DxeCore.

**/
VOID
ClearFirst4KPage (
  IN  VOID  *HobStart
  );

/**
  Return configure status of NULL pointer detection feature.

  @return TRUE   NULL pointer detection feature is enabled
  @return FALSE  NULL pointer detection feature is disabled
**/
BOOLEAN
IsNullDetectionEnabled (
  VOID
  );

/**
  Prevent the memory pages used for page table from been overwritten.

  @param[in] PageTableBase    Base address of page table (CR3).
  @param[in] Level4Paging     Level 4 paging flag.

**/
VOID
EnablePageTableProtection (
  IN  UINTN    PageTableBase,
  IN  BOOLEAN  Level4Paging
  );

/**
  This API provides a way to allocate memory for page table.

  This API can be called more than once to allocate memory for page tables.

  Allocates the number of 4KB pages and returns a pointer to the allocated
  buffer. The buffer returned is aligned on a 4KB boundary.

  If Pages is 0, then NULL is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
AllocatePageTableMemory (
  IN UINTN  Pages
  );

#endif
