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

Copyright (c) 2006 - 2023, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Register/Intel/Cpuid.h>
#include "DxeIpl.h"
#include "VirtualMemory.h"

//
// Global variable to keep track current available memory used as page table.
//
PAGE_TABLE_POOL  *mPageTablePool = NULL;

/**
  Clear legacy memory located at the first 4K-page, if available.

  This function traverses the whole HOB list to check if memory from 0 to 4095
  exists and has not been allocated, and then clear it if so.

  @param HobStart                  The start of HobList passed to DxeCore.

**/
VOID
ClearFirst4KPage (
  IN  VOID  *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  RscHob;
  EFI_PEI_HOB_POINTERS  MemHob;
  BOOLEAN               DoClear;

  RscHob.Raw = HobStart;
  MemHob.Raw = HobStart;
  DoClear    = FALSE;

  //
  // Check if page 0 exists and free
  //
  while ((RscHob.Raw = GetNextHob (
                         EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,
                         RscHob.Raw
                         )) != NULL)
  {
    if ((RscHob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
        (RscHob.ResourceDescriptor->PhysicalStart == 0))
    {
      DoClear = TRUE;
      //
      // Make sure memory at 0-4095 has not been allocated.
      //
      while ((MemHob.Raw = GetNextHob (
                             EFI_HOB_TYPE_MEMORY_ALLOCATION,
                             MemHob.Raw
                             )) != NULL)
      {
        if (MemHob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
            < EFI_PAGE_SIZE)
        {
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
  UINT32   RegEax;
  UINT32   RegEdx;
  BOOLEAN  Available;

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
  UINT64  MsrRegisters;

  MsrRegisters = AsmReadMsr64 (0xC0000080);
  if ((MsrRegisters & BIT11) == 0) {
    MsrRegisters |= BIT11;
    AsmWriteMsr64 (0xC0000080, MsrRegisters);
  }
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
  IN UINTN  PoolPages
  )
{
  VOID  *Buffer;

  //
  // Always reserve at least PAGE_TABLE_POOL_UNIT_PAGES, including one page for
  // header.
  //
  PoolPages += 1;   // Add one page for header.
  PoolPages  = ((PoolPages - 1) / PAGE_TABLE_POOL_UNIT_PAGES + 1) *
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
    mPageTablePool           = Buffer;
    mPageTablePool->NextPool = mPageTablePool;
  } else {
    ((PAGE_TABLE_POOL *)Buffer)->NextPool = mPageTablePool->NextPool;
    mPageTablePool->NextPool              = Buffer;
    mPageTablePool                        = Buffer;
  }

  //
  // Reserve one page for pool header.
  //
  mPageTablePool->FreePages = PoolPages - 1;
  mPageTablePool->Offset    = EFI_PAGES_TO_SIZE (1);

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
  IN UINTN  Pages
  )
{
  VOID  *Buffer;

  if (Pages == 0) {
    return NULL;
  }

  //
  // Renew the pool if necessary.
  //
  if ((mPageTablePool == NULL) ||
      (Pages > mPageTablePool->FreePages))
  {
    if (!InitializePageTablePool (Pages)) {
      return NULL;
    }
  }

  Buffer = (UINT8 *)mPageTablePool + mPageTablePool->Offset;

  mPageTablePool->Offset    += EFI_PAGES_TO_SIZE (Pages);
  mPageTablePool->FreePages -= Pages;

  return Buffer;
}

/**
  This function creates a new page table or modifies the page MapAttribute for the memory region
  specified by BaseAddress and Length from their current attributes to the attributes specified
  by MapAttribute and Mask.

  @param[in] PageTable        Pointer to Page table address.
  @param[in] PagingMode       The paging mode.
  @param[in] BaseAddress      The start of the linear address range.
  @param[in] Length           The length of the linear address range.
  @param[in] MapAttribute     The attribute of the linear address range.
  @param[in] MapMask          The mask used for attribute.
**/
VOID
CreateOrUpdatePageTable (
  IN  UINTN               *PageTable,
  IN  PAGING_MODE         PagingMode,
  IN  PHYSICAL_ADDRESS    BaseAddress,
  IN  UINT64              Length,
  IN  IA32_MAP_ATTRIBUTE  *MapAttribute,
  IN  IA32_MAP_ATTRIBUTE  *MapMask
  )
{
  RETURN_STATUS  Status;
  UINTN          PageTableBufferSize;
  VOID           *PageTableBuffer;

  PageTableBufferSize = 0;
  Status              = PageTableMap (PageTable, PagingMode, NULL, &PageTableBufferSize, BaseAddress, Length, MapAttribute, MapMask, NULL);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    PageTableBuffer = AllocatePageTableMemory (EFI_SIZE_TO_PAGES (PageTableBufferSize));
    DEBUG ((DEBUG_INFO, "DxeIpl: 0x%x bytes needed for page table\n", PageTableBufferSize));
    ASSERT (PageTableBuffer != NULL);
    Status = PageTableMap (PageTable, PagingMode, PageTableBuffer, &PageTableBufferSize, BaseAddress, Length, MapAttribute, MapMask, NULL);
  }

  ASSERT_RETURN_ERROR (Status);
  ASSERT (PageTableBufferSize == 0);
}

/**
  Set one page of page table pool memory to be read-only.

  @param[in] PageTableBase    Base address of page table (CR3).
  @param[in] Address          Start address of a page to be set as read-only.
  @param[in] Level4Paging     Level 4 paging flag.

**/
VOID
SetPageTablePoolReadOnly (
  IN  UINTN                 PageTableBase,
  IN  EFI_PHYSICAL_ADDRESS  Address,
  IN  BOOLEAN               Level4Paging
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

  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) &
                   PAGING_1G_ADDRESS_MASK_64;
  PageTable    = (UINT64 *)(UINTN)PageTableBase;
  PoolUnitSize = PAGE_TABLE_POOL_UNIT_SIZE;

  for (Level = (Level4Paging) ? 4 : 3; Level > 0; --Level) {
    Index  = ((UINTN)RShiftU64 (Address, LevelShift[Level]));
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
          PoolUnitSize     -= LevelSize[Level];

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
           ++EntryIndex)
      {
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
  IN  UINTN    PageTableBase,
  IN  BOOLEAN  Level4Paging
  )
{
  PAGE_TABLE_POOL       *HeadPool;
  PAGE_TABLE_POOL       *Pool;
  UINT64                PoolSize;
  EFI_PHYSICAL_ADDRESS  Address;

  if (mPageTablePool == NULL) {
    return;
  }

  //
  // No need to clear CR0.WP since PageTableBase has't been written to CR3 yet.
  // SetPageTablePoolReadOnly might update mPageTablePool. It's safer to
  // remember original one in advance.
  //
  HeadPool = mPageTablePool;
  Pool     = HeadPool;
  do {
    Address  = (EFI_PHYSICAL_ADDRESS)(UINTN)Pool;
    PoolSize = Pool->Offset + EFI_PAGES_TO_SIZE (Pool->FreePages);

    //
    // The size of one pool must be multiple of PAGE_TABLE_POOL_UNIT_SIZE, which
    // is one of page size of the processor (2MB by default). Let's apply the
    // protection to them one by one.
    //
    while (PoolSize > 0) {
      SetPageTablePoolReadOnly (PageTableBase, Address, Level4Paging);
      Address  += PAGE_TABLE_POOL_UNIT_SIZE;
      PoolSize -= PAGE_TABLE_POOL_UNIT_SIZE;
    }

    Pool = Pool->NextPool;
  } while (Pool != HeadPool);

  //
  // Enable write protection, after page table attribute updated.
  //
  AsmWriteCr0 (AsmReadCr0 () | CR0_WP);
}

/**
  Create IA32 PAE paging or 4-level/5-level paging for long mode to
  establish a 1:1 Virtual to Physical mapping.

  @param[in] Is64BitPageTable     Whether to create 64-bit page table.
  @param[in] StackBase            Stack base address.
  @param[in] StackSize            Stack size.
  @param[in] GhcbBase             GHCB base address.
  @param[in] GhcbSize             GHCB size.

  @return  PageTable Address
**/
UINTN
CreateIdentityMappingPageTables (
  IN BOOLEAN               Is64BitPageTable,
  IN EFI_PHYSICAL_ADDRESS  StackBase,
  IN UINTN                 StackSize,
  IN EFI_PHYSICAL_ADDRESS  GhcbBase,
  IN UINTN                 GhcbSize
  )
{
  UINT32                                       RegEax;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX  EcxFlags;
  UINT32                                       RegEdx;
  UINT8                                        PhysicalAddressBits;
  VOID                                         *Hob;
  BOOLEAN                                      Page5LevelSupport;
  BOOLEAN                                      Page1GSupport;
  UINT64                                       AddressEncMask;
  IA32_CR4                                     Cr4;
  PAGING_MODE                                  PagingMode;
  UINTN                                        PageTable;
  IA32_MAP_ATTRIBUTE                           MapAttribute;
  IA32_MAP_ATTRIBUTE                           MapMask;
  EFI_PHYSICAL_ADDRESS                         GhcbBase4K;
  EFI_PHYSICAL_ADDRESS                         GhcbBaseEnd;

  //
  // Make sure AddressEncMask is contained to smallest supported address field
  //
  AddressEncMask    = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;
  Page5LevelSupport = FALSE;
  Page1GSupport     = FALSE;

  if (!Is64BitPageTable) {
    PagingMode          = PagingPae;
    PhysicalAddressBits = 32;
  } else {
    if (PcdGetBool (PcdUse1GPageTable)) {
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
      PhysicalAddressBits = ((EFI_HOB_CPU *)Hob)->SizeOfMemorySpace;
    } else {
      AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
      if (RegEax >= 0x80000008) {
        AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
        PhysicalAddressBits = (UINT8)RegEax;
      } else {
        PhysicalAddressBits = 36;
      }
    }

    if (PcdGetBool (PcdUse5LevelPageTable)) {
      AsmCpuidEx (
        CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
        CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO,
        NULL,
        NULL,
        &EcxFlags.Uint32,
        NULL
        );
      if (EcxFlags.Bits.FiveLevelPage != 0) {
        Page5LevelSupport = TRUE;
      }
    }

    if (Page5LevelSupport) {
      if (Page1GSupport) {
        PagingMode = Paging5Level1GB;
      } else {
        PagingMode = Paging5Level;
      }
    } else {
      if (Page1GSupport) {
        PagingMode = Paging4Level1GB;
      } else {
        PagingMode = Paging4Level;
      }
    }

    DEBUG ((DEBUG_INFO, "AddressBits=%u 5LevelPaging=%u 1GPage=%u\n", PhysicalAddressBits, Page5LevelSupport, Page1GSupport));
    //
    // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses
    // when 5-Level Paging is disabled, due to either unsupported by HW, or disabled by PCD.
    //
    ASSERT (PhysicalAddressBits <= 52);
    if (!Page5LevelSupport && (PhysicalAddressBits > 48)) {
      PhysicalAddressBits = 48;
    }
  }

  PageTable                   = 0;
  MapAttribute.Uint64         = AddressEncMask;
  MapAttribute.Bits.Present   = 1;
  MapAttribute.Bits.ReadWrite = 1;
  MapMask.Uint64              = MAX_UINT64;
  CreateOrUpdatePageTable (&PageTable, PagingMode, 0, LShiftU64 (1, PhysicalAddressBits), &MapAttribute, &MapMask);

  if ((GhcbBase > 0) && (GhcbSize > 0) && (AddressEncMask != 0)) {
    //
    // The GHCB range consists of two pages per CPU, the GHCB and a
    // per-CPU variable page. The GHCB page needs to be mapped as an
    // unencrypted page while the per-CPU variable page needs to be
    // mapped encrypted. These pages alternate in assignment.
    //
    ASSERT (Is64BitPageTable == TRUE);
    GhcbBase4K                           = ALIGN_VALUE (GhcbBase, SIZE_4KB);
    GhcbBaseEnd                          = ALIGN_VALUE (GhcbBase + GhcbSize, SIZE_4KB);
    MapMask.Uint64                       = 0;
    MapMask.Bits.PageTableBaseAddressLow = 1;
    //
    // Loop through the GHCB range, remapping the GHCB page unencrypted
    // and skipping over the per-CPU variable page.
    //
    while (GhcbBase4K < GhcbBaseEnd) {
      MapAttribute.Uint64 = GhcbBase4K;
      CreateOrUpdatePageTable (&PageTable, PagingMode, GhcbBase4K, SIZE_4KB, &MapAttribute, &MapMask);
      GhcbBase4K += (SIZE_4KB * 2);
    }
  }

  if (PcdGetBool (PcdSetNxForStack)) {
    //
    // Set the stack as Nx in page table.
    //
    MapAttribute.Uint64  = 0;
    MapAttribute.Bits.Nx = 1;
    MapMask.Uint64       = 0;
    MapMask.Bits.Nx      = 1;
    CreateOrUpdatePageTable (&PageTable, PagingMode, StackBase, StackSize, &MapAttribute, &MapMask);
  }

  MapAttribute.Uint64       = 0;
  MapAttribute.Bits.Present = 0;
  MapMask.Uint64            = 0;
  MapMask.Bits.Present      = 1;
  if (IsNullDetectionEnabled ()) {
    //
    // Set [0, 4KB] as non-present in page table.
    //
    CreateOrUpdatePageTable (&PageTable, PagingMode, 0, SIZE_4KB, &MapAttribute, &MapMask);
  }

  if (PcdGetBool (PcdCpuStackGuard)) {
    //
    // Set the the last 4KB of stack as non-present in page table.
    //
    CreateOrUpdatePageTable (&PageTable, PagingMode, StackBase, SIZE_4KB, &MapAttribute, &MapMask);
  }

  if (Page5LevelSupport) {
    Cr4.UintN     = AsmReadCr4 ();
    Cr4.Bits.LA57 = 1;
    AsmWriteCr4 (Cr4.UintN);
  }

  //
  // Protect the page table by marking the memory used for page table to be
  // read-only.
  //
  EnablePageTableProtection ((UINTN)PageTable, TRUE);

  //
  // Set IA32_EFER.NXE if necessary.
  //
  if (IsEnableNonExecNeeded ()) {
    EnableExecuteDisableBit ();
  }

  return PageTable;
}
