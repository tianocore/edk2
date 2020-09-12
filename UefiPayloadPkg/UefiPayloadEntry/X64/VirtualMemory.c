/** @file
  x64 Virtual Memory Management Services in the form of an IA-32 driver.
  Used to establish a 1:1 Virtual to Physical Mapping that is required to
  enter Long Mode (x64 64-bit mode).

  While we make a 1:1 mapping (identity mapping) for all physical pages
  we still need to use the MTRR's to ensure that the cachability attributes
  for all memory regions is correct.

  The basic idea is to use 2MB page table entries where ever possible. If
  more granularity of cachability is required then 4K page tables are used.

  References:
    1) IA-32 Intel(R) Architecture Software Developer's Manual Volume 1:Basic Architecture, Intel
    2) IA-32 Intel(R) Architecture Software Developer's Manual Volume 2:Instruction Set Reference, Intel
    3) IA-32 Intel(R) Architecture Software Developer's Manual Volume 3:System Programmer's Guide, Intel

Copyright (c) 2006 - 2020, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Register/Intel/Cpuid.h>
#include "VirtualMemory.h"

//
// Global variable to keep track current available memory used as page table.
//
PAGE_TABLE_POOL   *mPageTablePool = NULL;

/**
  Clear legacy memory located at the first 4K-page, if available.

  This function traverses the whole HOB list to check if memory from 0 to 4095
  exists and has not been allocated, and then clear it if so.

  @param HobStart                  The start of HobList passed to DxeCore.

**/
VOID
ClearFirst4KPage (
  IN  VOID *HobStart
  )
{
  EFI_PEI_HOB_POINTERS          RscHob;
  EFI_PEI_HOB_POINTERS          MemHob;
  BOOLEAN                       DoClear;

  RscHob.Raw = HobStart;
  MemHob.Raw = HobStart;
  DoClear = FALSE;

  //
  // Check if page 0 exists and free
  //
  while ((RscHob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,
                                   RscHob.Raw)) != NULL) {
    if (RscHob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY &&
        RscHob.ResourceDescriptor->PhysicalStart == 0) {
      DoClear = TRUE;
      //
      // Make sure memory at 0-4095 has not been allocated.
      //
      while ((MemHob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION,
                                       MemHob.Raw)) != NULL) {
        if (MemHob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
            < EFI_PAGE_SIZE) {
          DoClear = FALSE;
          break;
        }
        MemHob.Raw = GET_NEXT_HOB (MemHob);
      }
      break;
    }
    RscHob.Raw = GET_NEXT_HOB (RscHob);
  }

  if (DoClear) {
    DEBUG ((DEBUG_INFO, "Clearing first 4K-page!\r\n"));
    SetMem (NULL, EFI_PAGE_SIZE, 0);
  }

  return;
}

/**
  Return configure status of NULL pointer detection feature.

  @return TRUE   NULL pointer detection feature is enabled
  @return FALSE  NULL pointer detection feature is disabled

**/
BOOLEAN
IsNullDetectionEnabled (
  VOID
  )
{
  return ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT0) != 0);
}

/**
  The function will check if Execute Disable Bit is available.

  @retval TRUE      Execute Disable Bit is available.
  @retval FALSE     Execute Disable Bit is not available.

**/
BOOLEAN
IsExecuteDisableBitAvailable (
  VOID
  )
{
  UINT32            RegEax;
  UINT32            RegEdx;
  BOOLEAN           Available;

  Available = FALSE;
  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000001) {
    AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
    if ((RegEdx & BIT20) != 0) {
      //
      // Bit 20: Execute Disable Bit available.
      //
      Available = TRUE;
    }
  }

  return Available;
}

/**
  Check if Execute Disable Bit (IA32_EFER.NXE) should be enabled or not.

  @retval TRUE    IA32_EFER.NXE should be enabled.
  @retval FALSE   IA32_EFER.NXE should not be enabled.

**/
BOOLEAN
IsEnableNonExecNeeded (
  VOID
  )
{
  if (!IsExecuteDisableBitAvailable ()) {
    return FALSE;
  }

  //
  // XD flag (BIT63) in page table entry is only valid if IA32_EFER.NXE is set.
  // Features controlled by Following PCDs need this feature to be enabled.
  //
  return (PcdGetBool (PcdSetNxForStack) ||
          PcdGet64 (PcdDxeNxMemoryProtectionPolicy) != 0 ||
          PcdGet32 (PcdImageProtectionPolicy) != 0);
}

/**
  Enable Execute Disable Bit.

**/
VOID
EnableExecuteDisableBit (
  VOID
  )
{
  UINT64           MsrRegisters;

  MsrRegisters = AsmReadMsr64 (0xC0000080);
  MsrRegisters |= BIT11;
  AsmWriteMsr64 (0xC0000080, MsrRegisters);
}

/**
  The function will check if page table entry should be splitted to smaller
  granularity.

  @param Address      Physical memory address.
  @param Size         Size of the given physical memory.
  @param StackBase    Base address of stack.
  @param StackSize    Size of stack.
  @param GhcbBase     Base address of GHCB pages.
  @param GhcbSize     Size of GHCB area.

  @retval TRUE      Page table should be split.
  @retval FALSE     Page table should not be split.
**/
BOOLEAN
ToSplitPageTable (
  IN EFI_PHYSICAL_ADDRESS               Address,
  IN UINTN                              Size,
  IN EFI_PHYSICAL_ADDRESS               StackBase,
  IN UINTN                              StackSize,
  IN EFI_PHYSICAL_ADDRESS               GhcbBase,
  IN UINTN                              GhcbSize
  )
{
  if (IsNullDetectionEnabled () && Address == 0) {
    return TRUE;
  }

  if (PcdGetBool (PcdCpuStackGuard)) {
    if (StackBase >= Address && StackBase < (Address + Size)) {
      return TRUE;
    }
  }

  if (PcdGetBool (PcdSetNxForStack)) {
    if ((Address < StackBase + StackSize) && ((Address + Size) > StackBase)) {
      return TRUE;
    }
  }

  if (GhcbBase != 0) {
    if ((Address < GhcbBase + GhcbSize) && ((Address + Size) > GhcbBase)) {
      return TRUE;
    }
  }

  return FALSE;
}
/**
  Initialize a buffer pool for page table use only.

  To reduce the potential split operation on page table, the pages reserved for
  page table should be allocated in the times of PAGE_TABLE_POOL_UNIT_PAGES and
  at the boundary of PAGE_TABLE_POOL_ALIGNMENT. So the page pool is always
  initialized with number of pages greater than or equal to the given PoolPages.

  Once the pages in the pool are used up, this method should be called again to
  reserve at least another PAGE_TABLE_POOL_UNIT_PAGES. But usually this won't
  happen in practice.

  @param PoolPages  The least page number of the pool to be created.

  @retval TRUE    The pool is initialized successfully.
  @retval FALSE   The memory is out of resource.
**/
BOOLEAN
InitializePageTablePool (
  IN UINTN           PoolPages
  )
{
  VOID          *Buffer;

  //
  // Always reserve at least PAGE_TABLE_POOL_UNIT_PAGES, including one page for
  // header.
  //
  PoolPages += 1;   // Add one page for header.
  PoolPages = ((PoolPages - 1) / PAGE_TABLE_POOL_UNIT_PAGES + 1) *
              PAGE_TABLE_POOL_UNIT_PAGES;
  Buffer = AllocateAlignedPages (PoolPages, PAGE_TABLE_POOL_ALIGNMENT);
  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Out of aligned pages\r\n"));
    return FALSE;
  }

  //
  // Link all pools into a list for easier track later.
  //
  if (mPageTablePool == NULL) {
    mPageTablePool = Buffer;
    mPageTablePool->NextPool = mPageTablePool;
  } else {
    ((PAGE_TABLE_POOL *)Buffer)->NextPool = mPageTablePool->NextPool;
    mPageTablePool->NextPool = Buffer;
    mPageTablePool = Buffer;
  }

  //
  // Reserve one page for pool header.
  //
  mPageTablePool->FreePages  = PoolPages - 1;
  mPageTablePool->Offset = EFI_PAGES_TO_SIZE (1);

  return TRUE;
}

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
  IN UINTN           Pages
  )
{
  VOID          *Buffer;

  if (Pages == 0) {
    return NULL;
  }

  //
  // Renew the pool if necessary.
  //
  if (mPageTablePool == NULL ||
      Pages > mPageTablePool->FreePages) {
    if (!InitializePageTablePool (Pages)) {
      return NULL;
    }
  }

  Buffer = (UINT8 *)mPageTablePool + mPageTablePool->Offset;

  mPageTablePool->Offset     += EFI_PAGES_TO_SIZE (Pages);
  mPageTablePool->FreePages  -= Pages;

  return Buffer;
}

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
  IN EFI_PHYSICAL_ADDRESS               PhysicalAddress,
  IN OUT UINT64                         *PageEntry2M,
  IN EFI_PHYSICAL_ADDRESS               StackBase,
  IN UINTN                              StackSize,
  IN EFI_PHYSICAL_ADDRESS               GhcbBase,
  IN UINTN                              GhcbSize
  )
{
  EFI_PHYSICAL_ADDRESS                  PhysicalAddress4K;
  UINTN                                 IndexOfPageTableEntries;
  PAGE_TABLE_4K_ENTRY                   *PageTableEntry;
  UINT64                                AddressEncMask;

  //
  // Make sure AddressEncMask is contained to smallest supported address field
  //
  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;

  PageTableEntry = AllocatePageTableMemory (1);
  ASSERT (PageTableEntry != NULL);

  //
  // Fill in 2M page entry.
  //
  *PageEntry2M = (UINT64) (UINTN) PageTableEntry | AddressEncMask | IA32_PG_P | IA32_PG_RW;

  PhysicalAddress4K = PhysicalAddress;
  for (IndexOfPageTableEntries = 0; IndexOfPageTableEntries < 512; IndexOfPageTableEntries++, PageTableEntry++, PhysicalAddress4K += SIZE_4KB) {
    //
    // Fill in the Page Table entries
    //
    PageTableEntry->Uint64 = (UINT64) PhysicalAddress4K;

    //
    // The GHCB range consists of two pages per CPU, the GHCB and a
    // per-CPU variable page. The GHCB page needs to be mapped as an
    // unencrypted page while the per-CPU variable page needs to be
    // mapped encrypted. These pages alternate in assignment.
    //
    if ((GhcbBase == 0)
        || (PhysicalAddress4K < GhcbBase)
        || (PhysicalAddress4K >= GhcbBase + GhcbSize)
        || (((PhysicalAddress4K - GhcbBase) & SIZE_4KB) != 0)) {
      PageTableEntry->Uint64 |= AddressEncMask;
    }
    PageTableEntry->Bits.ReadWrite = 1;

    if ((IsNullDetectionEnabled () && PhysicalAddress4K == 0) ||
        (PcdGetBool (PcdCpuStackGuard) && PhysicalAddress4K == StackBase)) {
      PageTableEntry->Bits.Present = 0;
    } else {
      PageTableEntry->Bits.Present = 1;
    }

    if (PcdGetBool (PcdSetNxForStack)
        && (PhysicalAddress4K >= StackBase)
        && (PhysicalAddress4K < StackBase + StackSize)) {
      //
      // Set Nx bit for stack.
      //
      PageTableEntry->Bits.Nx = 1;
    }
  }
}

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
Split1GPageTo2M (
  IN EFI_PHYSICAL_ADDRESS               PhysicalAddress,
  IN OUT UINT64                         *PageEntry1G,
  IN EFI_PHYSICAL_ADDRESS               StackBase,
  IN UINTN                              StackSize,
  IN EFI_PHYSICAL_ADDRESS               GhcbBase,
  IN UINTN                              GhcbSize
  )
{
  EFI_PHYSICAL_ADDRESS                  PhysicalAddress2M;
  UINTN                                 IndexOfPageDirectoryEntries;
  PAGE_TABLE_ENTRY                      *PageDirectoryEntry;
  UINT64                                AddressEncMask;

  //
  // Make sure AddressEncMask is contained to smallest supported address field
  //
  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;

  PageDirectoryEntry = AllocatePageTableMemory (1);
  ASSERT (PageDirectoryEntry != NULL);

  //
  // Fill in 1G page entry.
  //
  *PageEntry1G = (UINT64) (UINTN) PageDirectoryEntry | AddressEncMask | IA32_PG_P | IA32_PG_RW;

  PhysicalAddress2M = PhysicalAddress;
  for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PhysicalAddress2M += SIZE_2MB) {
    if (ToSplitPageTable (PhysicalAddress2M, SIZE_2MB, StackBase, StackSize, GhcbBase, GhcbSize)) {
      //
      // Need to split this 2M page that covers NULL or stack range.
      //
      Split2MPageTo4K (PhysicalAddress2M, (UINT64 *) PageDirectoryEntry, StackBase, StackSize, GhcbBase, GhcbSize);
    } else {
      //
      // Fill in the Page Directory entries
      //
      PageDirectoryEntry->Uint64 = (UINT64) PhysicalAddress2M | AddressEncMask;
      PageDirectoryEntry->Bits.ReadWrite = 1;
      PageDirectoryEntry->Bits.Present = 1;
      PageDirectoryEntry->Bits.MustBe1 = 1;
    }
  }
}

/**
  Set one page of page table pool memory to be read-only.

  @param[in] PageTableBase    Base address of page table (CR3).
  @param[in] Address          Start address of a page to be set as read-only.
  @param[in] Level4Paging     Level 4 paging flag.

**/
VOID
SetPageTablePoolReadOnly (
  IN  UINTN                             PageTableBase,
  IN  EFI_PHYSICAL_ADDRESS              Address,
  IN  BOOLEAN                           Level4Paging
  )
{
  UINTN                 Index;
  UINTN                 EntryIndex;
  UINT64                AddressEncMask;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  UINT64                *PageTable;
  UINT64                *NewPageTable;
  UINT64                PageAttr;
  UINT64                LevelSize[5];
  UINT64                LevelMask[5];
  UINTN                 LevelShift[5];
  UINTN                 Level;
  UINT64                PoolUnitSize;

  ASSERT (PageTableBase != 0);

  //
  // Since the page table is always from page table pool, which is always
  // located at the boundary of PcdPageTablePoolAlignment, we just need to
  // set the whole pool unit to be read-only.
  //
  Address = Address & PAGE_TABLE_POOL_ALIGN_MASK;

  LevelShift[1] = PAGING_L1_ADDRESS_SHIFT;
  LevelShift[2] = PAGING_L2_ADDRESS_SHIFT;
  LevelShift[3] = PAGING_L3_ADDRESS_SHIFT;
  LevelShift[4] = PAGING_L4_ADDRESS_SHIFT;

  LevelMask[1] = PAGING_4K_ADDRESS_MASK_64;
  LevelMask[2] = PAGING_2M_ADDRESS_MASK_64;
  LevelMask[3] = PAGING_1G_ADDRESS_MASK_64;
  LevelMask[4] = PAGING_1G_ADDRESS_MASK_64;

  LevelSize[1] = SIZE_4KB;
  LevelSize[2] = SIZE_2MB;
  LevelSize[3] = SIZE_1GB;
  LevelSize[4] = SIZE_512GB;

  AddressEncMask  = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) &
                    PAGING_1G_ADDRESS_MASK_64;
  PageTable       = (UINT64 *)(UINTN)PageTableBase;
  PoolUnitSize    = PAGE_TABLE_POOL_UNIT_SIZE;

  for (Level = (Level4Paging) ? 4 : 3; Level > 0; --Level) {
    Index = ((UINTN)RShiftU64 (Address, LevelShift[Level]));
    Index &= PAGING_PAE_INDEX_MASK;

    PageAttr = PageTable[Index];
    if ((PageAttr & IA32_PG_PS) == 0) {
      //
      // Go to next level of table.
      //
      PageTable = (UINT64 *)(UINTN)(PageAttr & ~AddressEncMask &
                                    PAGING_4K_ADDRESS_MASK_64);
      continue;
    }

    if (PoolUnitSize >= LevelSize[Level]) {
      //
      // Clear R/W bit if current page granularity is not larger than pool unit
      // size.
      //
      if ((PageAttr & IA32_PG_RW) != 0) {
        while (PoolUnitSize > 0) {
          //
          // PAGE_TABLE_POOL_UNIT_SIZE and PAGE_TABLE_POOL_ALIGNMENT are fit in
          // one page (2MB). Then we don't need to update attributes for pages
          // crossing page directory. ASSERT below is for that purpose.
          //
          ASSERT (Index < EFI_PAGE_SIZE/sizeof (UINT64));

          PageTable[Index] &= ~(UINT64)IA32_PG_RW;
          PoolUnitSize    -= LevelSize[Level];

          ++Index;
        }
      }

      break;

    } else {
      //
      // The smaller granularity of page must be needed.
      //
      ASSERT (Level > 1);

      NewPageTable = AllocatePageTableMemory (1);
      ASSERT (NewPageTable != NULL);

      PhysicalAddress = PageAttr & LevelMask[Level];
      for (EntryIndex = 0;
            EntryIndex < EFI_PAGE_SIZE/sizeof (UINT64);
            ++EntryIndex) {
        NewPageTable[EntryIndex] = PhysicalAddress  | AddressEncMask |
                                   IA32_PG_P | IA32_PG_RW;
        if (Level > 2) {
          NewPageTable[EntryIndex] |= IA32_PG_PS;
        }
        PhysicalAddress += LevelSize[Level - 1];
      }

      PageTable[Index] = (UINT64)(UINTN)NewPageTable | AddressEncMask |
                                        IA32_PG_P | IA32_PG_RW;
      PageTable = NewPageTable;
    }
  }
}

/**
  Prevent the memory pages used for page table from been overwritten.

  @param[in] PageTableBase    Base address of page table (CR3).
  @param[in] Level4Paging     Level 4 paging flag.

**/
VOID
EnablePageTableProtection (
  IN  UINTN     PageTableBase,
  IN  BOOLEAN   Level4Paging
  )
{
  PAGE_TABLE_POOL         *HeadPool;
  PAGE_TABLE_POOL         *Pool;
  UINT64                  PoolSize;
  EFI_PHYSICAL_ADDRESS    Address;

  if (mPageTablePool == NULL) {
    return;
  }

  //
  // Disable write protection, because we need to mark page table to be write
  // protected.
  //
  AsmWriteCr0 (AsmReadCr0() & ~CR0_WP);

  //
  // SetPageTablePoolReadOnly might update mPageTablePool. It's safer to
  // remember original one in advance.
  //
  HeadPool = mPageTablePool;
  Pool = HeadPool;
  do {
    Address  = (EFI_PHYSICAL_ADDRESS)(UINTN)Pool;
    PoolSize = Pool->Offset + EFI_PAGES_TO_SIZE (Pool->FreePages);

    //
    // The size of one pool must be multiple of PAGE_TABLE_POOL_UNIT_SIZE, which
    // is one of page size of the processor (2MB by default). Let's apply the
    // protection to them one by one.
    //
    while (PoolSize > 0) {
      SetPageTablePoolReadOnly(PageTableBase, Address, Level4Paging);
      Address   += PAGE_TABLE_POOL_UNIT_SIZE;
      PoolSize  -= PAGE_TABLE_POOL_UNIT_SIZE;
    }

    Pool = Pool->NextPool;
  } while (Pool != HeadPool);

  //
  // Enable write protection, after page table attribute updated.
  //
  AsmWriteCr0 (AsmReadCr0() | CR0_WP);
}

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
CreateIdentityMappingPageTables (
  IN EFI_PHYSICAL_ADDRESS   StackBase,
  IN UINTN                  StackSize,
  IN EFI_PHYSICAL_ADDRESS   GhcbBase,
  IN UINTN                  GhcbSize
  )
{
  UINT32                                        RegEax;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX   EcxFlags;
  UINT32                                        RegEdx;
  UINT8                                         PhysicalAddressBits;
  EFI_PHYSICAL_ADDRESS                          PageAddress;
  UINTN                                         IndexOfPml5Entries;
  UINTN                                         IndexOfPml4Entries;
  UINTN                                         IndexOfPdpEntries;
  UINTN                                         IndexOfPageDirectoryEntries;
  UINT32                                        NumberOfPml5EntriesNeeded;
  UINT32                                        NumberOfPml4EntriesNeeded;
  UINT32                                        NumberOfPdpEntriesNeeded;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageMapLevel5Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageMap;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageDirectoryPointerEntry;
  PAGE_TABLE_ENTRY                              *PageDirectoryEntry;
  UINTN                                         TotalPagesNum;
  UINTN                                         BigPageAddress;
  VOID                                          *Hob;
  BOOLEAN                                       Page5LevelSupport;
  BOOLEAN                                       Page1GSupport;
  PAGE_TABLE_1G_ENTRY                           *PageDirectory1GEntry;
  UINT64                                        AddressEncMask;
  IA32_CR4                                      Cr4;

  //
  // Set PageMapLevel5Entry to suppress incorrect compiler/analyzer warnings
  //
  PageMapLevel5Entry = NULL;

  //
  // Make sure AddressEncMask is contained to smallest supported address field
  //
  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;

  Page1GSupport = FALSE;
  if (PcdGetBool(PcdUse1GPageTable)) {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000001) {
      AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
      if ((RegEdx & BIT26) != 0) {
        Page1GSupport = TRUE;
      }
    }
  }

  //
  // Get physical address bits supported.
  //
  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    PhysicalAddressBits = ((EFI_HOB_CPU *) Hob)->SizeOfMemorySpace;
  } else {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000008) {
      AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
      PhysicalAddressBits = (UINT8) RegEax;
    } else {
      PhysicalAddressBits = 36;
    }
  }

  Page5LevelSupport = FALSE;
  if (PcdGetBool (PcdUse5LevelPageTable)) {
    AsmCpuidEx (
      CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS, CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO, NULL,
      &EcxFlags.Uint32, NULL, NULL
      );
    if (EcxFlags.Bits.FiveLevelPage != 0) {
      Page5LevelSupport = TRUE;
    }
  }

  DEBUG ((DEBUG_INFO, "AddressBits=%u 5LevelPaging=%u 1GPage=%u\n", PhysicalAddressBits, Page5LevelSupport, Page1GSupport));

  //
  // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses
  //  when 5-Level Paging is disabled,
  //  due to either unsupported by HW, or disabled by PCD.
  //
  ASSERT (PhysicalAddressBits <= 52);
  if (!Page5LevelSupport && PhysicalAddressBits > 48) {
    PhysicalAddressBits = 48;
  }

  //
  // Calculate the table entries needed.
  //
  NumberOfPml5EntriesNeeded = 1;
  if (PhysicalAddressBits > 48) {
    NumberOfPml5EntriesNeeded = (UINT32) LShiftU64 (1, PhysicalAddressBits - 48);
    PhysicalAddressBits = 48;
  }

  NumberOfPml4EntriesNeeded = 1;
  if (PhysicalAddressBits > 39) {
    NumberOfPml4EntriesNeeded = (UINT32) LShiftU64 (1, PhysicalAddressBits - 39);
    PhysicalAddressBits = 39;
  }

  NumberOfPdpEntriesNeeded = 1;
  ASSERT (PhysicalAddressBits > 30);
  NumberOfPdpEntriesNeeded = (UINT32) LShiftU64 (1, PhysicalAddressBits - 30);

  //
  // Pre-allocate big pages to avoid later allocations.
  //
  if (!Page1GSupport) {
    TotalPagesNum = ((NumberOfPdpEntriesNeeded + 1) * NumberOfPml4EntriesNeeded + 1) * NumberOfPml5EntriesNeeded + 1;
  } else {
    TotalPagesNum = (NumberOfPml4EntriesNeeded + 1) * NumberOfPml5EntriesNeeded + 1;
  }

  //
  // Substract the one page occupied by PML5 entries if 5-Level Paging is disabled.
  //
  if (!Page5LevelSupport) {
    TotalPagesNum--;
  }

  DEBUG ((DEBUG_INFO, "Pml5=%u Pml4=%u Pdp=%u TotalPage=%Lu\n",
    NumberOfPml5EntriesNeeded, NumberOfPml4EntriesNeeded,
    NumberOfPdpEntriesNeeded, (UINT64)TotalPagesNum));

  BigPageAddress = (UINTN) AllocatePageTableMemory (TotalPagesNum);
  ASSERT (BigPageAddress != 0);

  //
  // By architecture only one PageMapLevel4 exists - so lets allocate storage for it.
  //
  PageMap         = (VOID *) BigPageAddress;
  if (Page5LevelSupport) {
    //
    // By architecture only one PageMapLevel5 exists - so lets allocate storage for it.
    //
    PageMapLevel5Entry = PageMap;
    BigPageAddress    += SIZE_4KB;
  }
  PageAddress        = 0;

  for ( IndexOfPml5Entries = 0
      ; IndexOfPml5Entries < NumberOfPml5EntriesNeeded
      ; IndexOfPml5Entries++) {
    //
    // Each PML5 entry points to a page of PML4 entires.
    // So lets allocate space for them and fill them in in the IndexOfPml4Entries loop.
    // When 5-Level Paging is disabled, below allocation happens only once.
    //
    PageMapLevel4Entry = (VOID *) BigPageAddress;
    BigPageAddress    += SIZE_4KB;

    if (Page5LevelSupport) {
      //
      // Make a PML5 Entry
      //
      PageMapLevel5Entry->Uint64 = (UINT64) (UINTN) PageMapLevel4Entry | AddressEncMask;
      PageMapLevel5Entry->Bits.ReadWrite = 1;
      PageMapLevel5Entry->Bits.Present   = 1;
      PageMapLevel5Entry++;
    }

    for ( IndexOfPml4Entries = 0
        ; IndexOfPml4Entries < (NumberOfPml5EntriesNeeded == 1 ? NumberOfPml4EntriesNeeded : 512)
        ; IndexOfPml4Entries++, PageMapLevel4Entry++) {
      //
      // Each PML4 entry points to a page of Page Directory Pointer entires.
      // So lets allocate space for them and fill them in in the IndexOfPdpEntries loop.
      //
      PageDirectoryPointerEntry = (VOID *) BigPageAddress;
      BigPageAddress += SIZE_4KB;

      //
      // Make a PML4 Entry
      //
      PageMapLevel4Entry->Uint64 = (UINT64)(UINTN)PageDirectoryPointerEntry | AddressEncMask;
      PageMapLevel4Entry->Bits.ReadWrite = 1;
      PageMapLevel4Entry->Bits.Present = 1;

      if (Page1GSupport) {
        PageDirectory1GEntry = (VOID *) PageDirectoryPointerEntry;

        for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectory1GEntry++, PageAddress += SIZE_1GB) {
          if (ToSplitPageTable (PageAddress, SIZE_1GB, StackBase, StackSize, GhcbBase, GhcbSize)) {
            Split1GPageTo2M (PageAddress, (UINT64 *) PageDirectory1GEntry, StackBase, StackSize, GhcbBase, GhcbSize);
          } else {
            //
            // Fill in the Page Directory entries
            //
            PageDirectory1GEntry->Uint64 = (UINT64)PageAddress | AddressEncMask;
            PageDirectory1GEntry->Bits.ReadWrite = 1;
            PageDirectory1GEntry->Bits.Present = 1;
            PageDirectory1GEntry->Bits.MustBe1 = 1;
          }
        }
      } else {
        for ( IndexOfPdpEntries = 0
            ; IndexOfPdpEntries < (NumberOfPml4EntriesNeeded == 1 ? NumberOfPdpEntriesNeeded : 512)
            ; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
          //
          // Each Directory Pointer entries points to a page of Page Directory entires.
          // So allocate space for them and fill them in in the IndexOfPageDirectoryEntries loop.
          //
          PageDirectoryEntry = (VOID *) BigPageAddress;
          BigPageAddress += SIZE_4KB;

          //
          // Fill in a Page Directory Pointer Entries
          //
          PageDirectoryPointerEntry->Uint64 = (UINT64)(UINTN)PageDirectoryEntry | AddressEncMask;
          PageDirectoryPointerEntry->Bits.ReadWrite = 1;
          PageDirectoryPointerEntry->Bits.Present = 1;

          for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PageAddress += SIZE_2MB) {
            if (ToSplitPageTable (PageAddress, SIZE_2MB, StackBase, StackSize, GhcbBase, GhcbSize)) {
              //
              // Need to split this 2M page that covers NULL or stack range.
              //
              Split2MPageTo4K (PageAddress, (UINT64 *) PageDirectoryEntry, StackBase, StackSize, GhcbBase, GhcbSize);
            } else {
              //
              // Fill in the Page Directory entries
              //
              PageDirectoryEntry->Uint64 = (UINT64)PageAddress | AddressEncMask;
              PageDirectoryEntry->Bits.ReadWrite = 1;
              PageDirectoryEntry->Bits.Present = 1;
              PageDirectoryEntry->Bits.MustBe1 = 1;
            }
          }
        }

        //
        // Fill with null entry for unused PDPTE
        //
        ZeroMem (PageDirectoryPointerEntry, (512 - IndexOfPdpEntries) * sizeof(PAGE_MAP_AND_DIRECTORY_POINTER));
      }
    }

    //
    // For the PML4 entries we are not using fill in a null entry.
    //
    ZeroMem (PageMapLevel4Entry, (512 - IndexOfPml4Entries) * sizeof (PAGE_MAP_AND_DIRECTORY_POINTER));
  }

  if (Page5LevelSupport) {
    Cr4.UintN = AsmReadCr4 ();
    Cr4.Bits.LA57 = 1;
    AsmWriteCr4 (Cr4.UintN);
    //
    // For the PML5 entries we are not using fill in a null entry.
    //
    ZeroMem (PageMapLevel5Entry, (512 - IndexOfPml5Entries) * sizeof (PAGE_MAP_AND_DIRECTORY_POINTER));
  }

  //
  // Protect the page table by marking the memory used for page table to be
  // read-only.
  //
  EnablePageTableProtection ((UINTN)PageMap, TRUE);

  //
  // Set IA32_EFER.NXE if necessary.
  //
  if (IsEnableNonExecNeeded ()) {
    EnableExecuteDisableBit ();
  }

  return (UINTN)PageMap;
}

