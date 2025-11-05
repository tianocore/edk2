/** @file

Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

BOOLEAN      mIsShadowStack      = FALSE;
BOOLEAN      m5LevelPagingNeeded = FALSE;
PAGING_MODE  mPagingMode         = PagingModeMax;

//
// Global variable to keep track current available memory used as page table.
//
PAGE_TABLE_POOL  *mPageTablePool = NULL;

//
// If memory used by SMM page table has been mareked as ReadOnly.
//
BOOLEAN  mIsReadOnlyPageTable = FALSE;

/**
  Write unprotect read-only pages if Cr0.Bits.WP is 1.

  @param[out]  WriteProtect      If Cr0.Bits.WP is enabled.

**/
VOID
SmmWriteUnprotectReadOnlyPage (
  OUT BOOLEAN  *WriteProtect
  )
{
  IA32_CR0  Cr0;

  Cr0.UintN     = AsmReadCr0 ();
  *WriteProtect = (Cr0.Bits.WP != 0);
  if (*WriteProtect) {
    Cr0.Bits.WP = 0;
    AsmWriteCr0 (Cr0.UintN);
  }
}

/**
  Write protect read-only pages.

  @param[in]  WriteProtect      If Cr0.Bits.WP should be enabled.

**/
VOID
SmmWriteProtectReadOnlyPage (
  IN  BOOLEAN  WriteProtect
  )
{
  IA32_CR0  Cr0;

  if (WriteProtect) {
    Cr0.UintN   = AsmReadCr0 ();
    Cr0.Bits.WP = 1;
    AsmWriteCr0 (Cr0.UintN);
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
  VOID     *Buffer;
  BOOLEAN  WriteProtect;
  BOOLEAN  CetEnabled;

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

  //
  // If page table memory has been marked as RO, mark the new pool pages as read-only.
  //
  if (mIsReadOnlyPageTable) {
    WRITE_UNPROTECT_RO_PAGES (WriteProtect, CetEnabled);

    SmmSetMemoryAttributes ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, EFI_PAGES_TO_SIZE (PoolPages), EFI_MEMORY_RO);

    WRITE_PROTECT_RO_PAGES (WriteProtect, CetEnabled);
  }

  return TRUE;
}

/**
  This API provides a way to allocate memory for page table.

  This API can be called more once to allocate memory for page tables.

  Allocates the number of 4KB pages of type EfiRuntimeServicesData and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
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
  Return page table entry to match the address.

  @param[in]   PageTableBase      The page table base.
  @param[in]   Enable5LevelPaging If PML5 paging is enabled.
  @param[in]   Address            The address to be checked.
  @param[out]  PageAttributes     The page attribute of the page entry.

  @return The page entry.
**/
VOID *
GetPageTableEntry (
  IN  UINTN             PageTableBase,
  IN  BOOLEAN           Enable5LevelPaging,
  IN  PHYSICAL_ADDRESS  Address,
  OUT PAGE_ATTRIBUTE    *PageAttribute
  )
{
  UINTN   Index1;
  UINTN   Index2;
  UINTN   Index3;
  UINTN   Index4;
  UINTN   Index5;
  UINT64  *L1PageTable;
  UINT64  *L2PageTable;
  UINT64  *L3PageTable;
  UINT64  *L4PageTable;
  UINT64  *L5PageTable;

  Index5 = ((UINTN)RShiftU64 (Address, 48)) & PAGING_PAE_INDEX_MASK;
  Index4 = ((UINTN)RShiftU64 (Address, 39)) & PAGING_PAE_INDEX_MASK;
  Index3 = ((UINTN)Address >> 30) & PAGING_PAE_INDEX_MASK;
  Index2 = ((UINTN)Address >> 21) & PAGING_PAE_INDEX_MASK;
  Index1 = ((UINTN)Address >> 12) & PAGING_PAE_INDEX_MASK;

  if (sizeof (UINTN) == sizeof (UINT64)) {
    if (Enable5LevelPaging) {
      L5PageTable = (UINT64 *)PageTableBase;
      if (L5PageTable[Index5] == 0) {
        *PageAttribute = PageNone;
        return NULL;
      }

      L4PageTable = (UINT64 *)(UINTN)(L5PageTable[Index5] & ~mAddressEncMask & PAGING_4K_ADDRESS_MASK_64);
    } else {
      L4PageTable = (UINT64 *)PageTableBase;
    }

    if (L4PageTable[Index4] == 0) {
      *PageAttribute = PageNone;
      return NULL;
    }

    L3PageTable = (UINT64 *)(UINTN)(L4PageTable[Index4] & ~mAddressEncMask & PAGING_4K_ADDRESS_MASK_64);
  } else {
    L3PageTable = (UINT64 *)PageTableBase;
  }

  if (L3PageTable[Index3] == 0) {
    *PageAttribute = PageNone;
    return NULL;
  }

  if ((L3PageTable[Index3] & IA32_PG_PS) != 0) {
    // 1G
    *PageAttribute = Page1G;
    return &L3PageTable[Index3];
  }

  L2PageTable = (UINT64 *)(UINTN)(L3PageTable[Index3] & ~mAddressEncMask & PAGING_4K_ADDRESS_MASK_64);
  if (L2PageTable[Index2] == 0) {
    *PageAttribute = PageNone;
    return NULL;
  }

  if ((L2PageTable[Index2] & IA32_PG_PS) != 0) {
    // 2M
    *PageAttribute = Page2M;
    return &L2PageTable[Index2];
  }

  // 4k
  L1PageTable = (UINT64 *)(UINTN)(L2PageTable[Index2] & ~mAddressEncMask & PAGING_4K_ADDRESS_MASK_64);
  if ((L1PageTable[Index1] == 0) && (Address != 0)) {
    *PageAttribute = PageNone;
    return NULL;
  }

  *PageAttribute = Page4K;
  return &L1PageTable[Index1];
}

/**
  Return memory attributes of page entry.

  @param[in]  PageEntry        The page entry.

  @return Memory attributes of page entry.
**/
UINT64
GetAttributesFromPageEntry (
  IN  UINT64  *PageEntry
  )
{
  UINT64  Attributes;

  Attributes = 0;
  if ((*PageEntry & IA32_PG_P) == 0) {
    Attributes |= EFI_MEMORY_RP;
  }

  if ((*PageEntry & IA32_PG_RW) == 0) {
    Attributes |= EFI_MEMORY_RO;
  }

  if ((*PageEntry & IA32_PG_NX) != 0) {
    Attributes |= EFI_MEMORY_XP;
  }

  return Attributes;
}

/**
  This function modifies the page attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  Caller should make sure BaseAddress and Length is at page boundary.

  @param[in]   PageTableBase    The page table base.
  @param[in]   PagingMode       The paging mode.
  @param[in]   BaseAddress      The physical address that is the start address of a memory region.
  @param[in]   Length           The size in bytes of the memory region.
  @param[in]   Attributes       The bit mask of attributes to modify for the memory region.
  @param[in]   IsSet            TRUE means to set attributes. FALSE means to clear attributes.
  @param[out]  IsModified       TRUE means page table modified. FALSE means page table not modified.

  @retval RETURN_SUCCESS           The attributes were modified for the memory region.
  @retval RETURN_ACCESS_DENIED     The attributes for the memory resource range specified by
                                   BaseAddress and Length cannot be modified.
  @retval RETURN_INVALID_PARAMETER Length is zero.
                                   Attributes specified an illegal combination of attributes that
                                   cannot be set together.
  @retval RETURN_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                   the memory resource range.
  @retval RETURN_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                   resource range specified by BaseAddress and Length.
                                   The bit mask of attributes is not support for the memory resource
                                   range specified by BaseAddress and Length.
**/
RETURN_STATUS
ConvertMemoryPageAttributes (
  IN  UINTN             PageTableBase,
  IN  PAGING_MODE       PagingMode,
  IN  PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64            Length,
  IN  UINT64            Attributes,
  IN  BOOLEAN           IsSet,
  OUT BOOLEAN           *IsModified   OPTIONAL
  )
{
  RETURN_STATUS         Status;
  IA32_MAP_ATTRIBUTE    PagingAttribute;
  IA32_MAP_ATTRIBUTE    PagingAttrMask;
  UINTN                 PageTableBufferSize;
  VOID                  *PageTableBuffer;
  EFI_PHYSICAL_ADDRESS  MaximumSupportMemAddress;
  IA32_MAP_ENTRY        *Map;
  UINTN                 Count;
  UINTN                 Index;
  UINT64                OverlappedRangeBase;
  UINT64                OverlappedRangeLimit;

  ASSERT (Attributes != 0);
  ASSERT ((Attributes & ~EFI_MEMORY_ATTRIBUTE_MASK) == 0);

  ASSERT ((BaseAddress & (SIZE_4KB - 1)) == 0);
  ASSERT ((Length & (SIZE_4KB - 1)) == 0);
  ASSERT (PageTableBase != 0);

  if (Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  MaximumSupportMemAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)(LShiftU64 (1, mPhysicalAddressBits) - 1);
  if (BaseAddress > MaximumSupportMemAddress) {
    return RETURN_UNSUPPORTED;
  }

  if (Length > MaximumSupportMemAddress) {
    return RETURN_UNSUPPORTED;
  }

  if ((Length != 0) && (BaseAddress > MaximumSupportMemAddress - (Length - 1))) {
    return RETURN_UNSUPPORTED;
  }

  if (IsModified != NULL) {
    *IsModified = FALSE;
  }

  PagingAttribute.Uint64 = 0;
  PagingAttribute.Uint64 = mAddressEncMask | BaseAddress;
  PagingAttrMask.Uint64  = 0;

  if ((Attributes & EFI_MEMORY_RO) != 0) {
    PagingAttrMask.Bits.ReadWrite = 1;
    if (IsSet) {
      PagingAttribute.Bits.ReadWrite = 0;
      PagingAttrMask.Bits.Dirty      = 1;
      if (mIsShadowStack) {
        // Environment setup
        // ReadOnly page need set Dirty bit for shadow stack
        PagingAttribute.Bits.Dirty = 1;
        // Clear user bit for supervisor shadow stack
        PagingAttribute.Bits.UserSupervisor = 0;
        PagingAttrMask.Bits.UserSupervisor  = 1;
      } else {
        // Runtime update
        // Clear dirty bit for non shadow stack, to protect RO page.
        PagingAttribute.Bits.Dirty = 0;
      }
    } else {
      PagingAttribute.Bits.ReadWrite = 1;
    }
  }

  if ((Attributes & EFI_MEMORY_XP) != 0) {
    if (mXdSupported) {
      PagingAttribute.Bits.Nx = IsSet ? 1 : 0;
      PagingAttrMask.Bits.Nx  = 1;
    }
  }

  if ((Attributes & EFI_MEMORY_RP) != 0) {
    if (IsSet) {
      PagingAttribute.Bits.Present = 0;
      //
      // When map a range to non-present, all attributes except Present should not be provided.
      //
      PagingAttrMask.Uint64       = 0;
      PagingAttrMask.Bits.Present = 1;
    } else {
      //
      // When map range to present range, provide all attributes.
      //
      PagingAttribute.Bits.Present = 1;
      PagingAttrMask.Uint64        = MAX_UINT64;

      //
      // By default memory is Ring 3 accessble.
      //
      PagingAttribute.Bits.UserSupervisor = 1;

      DEBUG_CODE_BEGIN ();
      if (((Attributes & EFI_MEMORY_RO) == 0) || (((Attributes & EFI_MEMORY_XP) == 0) && (mXdSupported))) {
        //
        // When mapping a range to present and EFI_MEMORY_RO or EFI_MEMORY_XP is not specificed,
        // check if [BaseAddress, BaseAddress + Length] contains present range.
        // Existing Present range in [BaseAddress, BaseAddress + Length] is set to NX disable or ReadOnly.
        //
        Count  = 0;
        Map    = NULL;
        Status = PageTableParse (PageTableBase, mPagingMode, NULL, &Count);

        while (Status == RETURN_BUFFER_TOO_SMALL) {
          if (Map != NULL) {
            FreePool (Map);
          }

          Map = AllocatePool (Count * sizeof (IA32_MAP_ENTRY));
          ASSERT (Map != NULL);
          Status = PageTableParse (PageTableBase, mPagingMode, Map, &Count);
        }

        ASSERT_RETURN_ERROR (Status);
        for (Index = 0; Index < Count; Index++) {
          if (Map[Index].LinearAddress >= BaseAddress + Length) {
            break;
          }

          if ((BaseAddress < Map[Index].LinearAddress + Map[Index].Length) && (BaseAddress + Length > Map[Index].LinearAddress)) {
            OverlappedRangeBase  = MAX (BaseAddress, Map[Index].LinearAddress);
            OverlappedRangeLimit = MIN (BaseAddress + Length, Map[Index].LinearAddress + Map[Index].Length);

            if (((Attributes & EFI_MEMORY_RO) == 0) && (Map[Index].Attribute.Bits.ReadWrite == 1)) {
              DEBUG ((DEBUG_ERROR, "SMM ConvertMemoryPageAttributes: [0x%lx, 0x%lx] is set from ReadWrite to ReadOnly\n", OverlappedRangeBase, OverlappedRangeLimit));
            }

            if (((Attributes & EFI_MEMORY_XP) == 0) && (mXdSupported) && (Map[Index].Attribute.Bits.Nx == 1)) {
              DEBUG ((DEBUG_ERROR, "SMM ConvertMemoryPageAttributes: [0x%lx, 0x%lx] is set from NX enabled to NX disabled\n", OverlappedRangeBase, OverlappedRangeLimit));
            }
          }
        }

        FreePool (Map);
      }

      DEBUG_CODE_END ();
    }
  }

  if (PagingAttrMask.Uint64 == 0) {
    return RETURN_SUCCESS;
  }

  PageTableBufferSize = 0;
  Status              = PageTableMap (&PageTableBase, PagingMode, NULL, &PageTableBufferSize, BaseAddress, Length, &PagingAttribute, &PagingAttrMask, IsModified);

  if (Status == RETURN_BUFFER_TOO_SMALL) {
    PageTableBuffer = AllocatePageTableMemory (EFI_SIZE_TO_PAGES (PageTableBufferSize));
    ASSERT (PageTableBuffer != NULL);
    Status = PageTableMap (&PageTableBase, PagingMode, PageTableBuffer, &PageTableBufferSize, BaseAddress, Length, &PagingAttribute, &PagingAttrMask, IsModified);
  }

  if (Status == RETURN_INVALID_PARAMETER) {
    //
    // The only reason that PageTableMap returns RETURN_INVALID_PARAMETER here is to modify other attributes
    // of a non-present range but remains the non-present range still as non-present.
    //
    DEBUG ((DEBUG_ERROR, "SMM ConvertMemoryPageAttributes: Only change EFI_MEMORY_XP/EFI_MEMORY_RO for non-present range in [0x%lx, 0x%lx] is not permitted\n", BaseAddress, BaseAddress + Length));
  }

  ASSERT_RETURN_ERROR (Status);
  ASSERT (PageTableBufferSize == 0);

  return RETURN_SUCCESS;
}

/**
  FlushTlb on current processor.

  @param[in,out] Buffer  Pointer to private data buffer.
**/
VOID
EFIAPI
FlushTlbOnCurrentProcessor (
  IN OUT VOID  *Buffer
  )
{
  CpuFlushTlb ();
}

/**
  FlushTlb for all processors.
**/
VOID
FlushTlbForAll (
  VOID
  )
{
  FlushTlbOnCurrentProcessor (NULL);
  InternalSmmStartupAllAPs (
    (EFI_AP_PROCEDURE2)FlushTlbOnCurrentProcessor,
    0,
    NULL,
    NULL,
    NULL
    );
}

/**
  This function sets the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]   PageTableBase    The page table base.
  @param[in]   PagingMode       The paging mode.
  @param[in]   BaseAddress      The physical address that is the start address of a memory region.
  @param[in]   Length           The size in bytes of the memory region.
  @param[in]   Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be set together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
SmmSetMemoryAttributesEx (
  IN  UINTN                 PageTableBase,
  IN  PAGING_MODE           PagingMode,
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsModified;

  Status = ConvertMemoryPageAttributes (PageTableBase, PagingMode, BaseAddress, Length, Attributes, TRUE, &IsModified);
  if (!EFI_ERROR (Status)) {
    if (IsModified) {
      //
      // Flush TLB as last step
      //
      FlushTlbForAll ();
    }
  }

  return Status;
}

/**
  This function clears the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]   PageTableBase    The page table base.
  @param[in]   PagingMode       The paging mode.
  @param[in]   BaseAddress      The physical address that is the start address of a memory region.
  @param[in]   Length           The size in bytes of the memory region.
  @param[in]   Attributes       The bit mask of attributes to clear for the memory region.

  @retval EFI_SUCCESS           The attributes were cleared for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be cleared together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not supported for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
SmmClearMemoryAttributesEx (
  IN  UINTN                 PageTableBase,
  IN  PAGING_MODE           PagingMode,
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsModified;

  Status = ConvertMemoryPageAttributes (PageTableBase, PagingMode, BaseAddress, Length, Attributes, FALSE, &IsModified);
  if (!EFI_ERROR (Status)) {
    if (IsModified) {
      //
      // Flush TLB as last step
      //
      FlushTlbForAll ();
    }
  }

  return Status;
}

/**
  This function sets the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]  BaseAddress      The physical address that is the start address of a memory region.
  @param[in]  Length           The size in bytes of the memory region.
  @param[in]  Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be set together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not supported for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
SmmSetMemoryAttributes (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
  )
{
  UINTN  PageTableBase;

  PageTableBase = AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64;
  return SmmSetMemoryAttributesEx (PageTableBase, mPagingMode, BaseAddress, Length, Attributes);
}

/**
  This function clears the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]  BaseAddress      The physical address that is the start address of a memory region.
  @param[in]  Length           The size in bytes of the memory region.
  @param[in]  Attributes       The bit mask of attributes to clear for the memory region.

  @retval EFI_SUCCESS           The attributes were cleared for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be cleared together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not supported for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
SmmClearMemoryAttributes (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
  )
{
  UINTN  PageTableBase;

  PageTableBase = AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64;
  return SmmClearMemoryAttributesEx (PageTableBase, mPagingMode, BaseAddress, Length, Attributes);
}

/**
  Set ShadowStack memory.

  @param[in]  Cr3              The page table base address.
  @param[in]  BaseAddress      The physical address that is the start address of a memory region.
  @param[in]  Length           The size in bytes of the memory region.

  @retval EFI_SUCCESS           The shadow stack memory is set.
**/
EFI_STATUS
SetShadowStack (
  IN  UINTN                 Cr3,
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  EFI_STATUS  Status;

  mIsShadowStack = TRUE;
  Status         = SmmSetMemoryAttributesEx (Cr3, mPagingMode, BaseAddress, Length, EFI_MEMORY_RO);
  mIsShadowStack = FALSE;

  return Status;
}

/**
  Retrieves a pointer to the system configuration table from the SMM System Table
  based on a specified GUID.

  @param[in]   TableGuid       The pointer to table's GUID type.
  @param[out]  Table           The pointer to the table associated with TableGuid in the EFI System Table.

  @retval EFI_SUCCESS     A configuration table matching TableGuid was found.
  @retval EFI_NOT_FOUND   A configuration table matching TableGuid could not be found.

**/
EFI_STATUS
EFIAPI
SmmGetSystemConfigurationTable (
  IN  EFI_GUID  *TableGuid,
  OUT VOID      **Table
  )
{
  UINTN  Index;

  ASSERT (TableGuid != NULL);
  ASSERT (Table != NULL);

  *Table = NULL;
  for (Index = 0; Index < gMmst->NumberOfTableEntries; Index++) {
    if (CompareGuid (TableGuid, &(gMmst->MmConfigurationTable[Index].VendorGuid))) {
      *Table = gMmst->MmConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This function sets SMM save state buffer to be RW and XP.
**/
VOID
PatchSmmSaveStateMap (
  VOID
  )
{
  UINTN  Index;
  UINTN  TileCodeSize;
  UINTN  TileDataSize;
  UINTN  TileSize;
  UINTN  PageTableBase;

  TileCodeSize  = GetSmiHandlerSize ();
  TileCodeSize  = ALIGN_VALUE (TileCodeSize, SIZE_4KB);
  TileDataSize  = (SMRAM_SAVE_STATE_MAP_OFFSET - SMM_PSD_OFFSET) + sizeof (SMRAM_SAVE_STATE_MAP);
  TileDataSize  = ALIGN_VALUE (TileDataSize, SIZE_4KB);
  TileSize      = TileDataSize + TileCodeSize - 1;
  TileSize      = 2 * GetPowerOfTwo32 ((UINT32)TileSize);
  PageTableBase = AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64;

  DEBUG ((DEBUG_INFO, "PatchSmmSaveStateMap:\n"));
  for (Index = 0; Index < mMaxNumberOfCpus - 1; Index++) {
    //
    // Code
    //
    ConvertMemoryPageAttributes (
      PageTableBase,
      mPagingMode,
      mCpuHotPlugData.SmBase[Index] + SMM_HANDLER_OFFSET,
      TileCodeSize,
      EFI_MEMORY_RO,
      TRUE,
      NULL
      );
    ConvertMemoryPageAttributes (
      PageTableBase,
      mPagingMode,
      mCpuHotPlugData.SmBase[Index] + SMM_HANDLER_OFFSET,
      TileCodeSize,
      EFI_MEMORY_XP,
      FALSE,
      NULL
      );

    //
    // Data
    //
    ConvertMemoryPageAttributes (
      PageTableBase,
      mPagingMode,
      mCpuHotPlugData.SmBase[Index] + SMM_HANDLER_OFFSET + TileCodeSize,
      TileSize - TileCodeSize,
      EFI_MEMORY_RO,
      FALSE,
      NULL
      );
    ConvertMemoryPageAttributes (
      PageTableBase,
      mPagingMode,
      mCpuHotPlugData.SmBase[Index] + SMM_HANDLER_OFFSET + TileCodeSize,
      TileSize - TileCodeSize,
      EFI_MEMORY_XP,
      TRUE,
      NULL
      );
  }

  //
  // Code
  //
  ConvertMemoryPageAttributes (
    PageTableBase,
    mPagingMode,
    mCpuHotPlugData.SmBase[mMaxNumberOfCpus - 1] + SMM_HANDLER_OFFSET,
    TileCodeSize,
    EFI_MEMORY_RO,
    TRUE,
    NULL
    );
  ConvertMemoryPageAttributes (
    PageTableBase,
    mPagingMode,
    mCpuHotPlugData.SmBase[mMaxNumberOfCpus - 1] + SMM_HANDLER_OFFSET,
    TileCodeSize,
    EFI_MEMORY_XP,
    FALSE,
    NULL
    );

  //
  // Data
  //
  ConvertMemoryPageAttributes (
    PageTableBase,
    mPagingMode,
    mCpuHotPlugData.SmBase[mMaxNumberOfCpus - 1] + SMM_HANDLER_OFFSET + TileCodeSize,
    SIZE_32KB - TileCodeSize,
    EFI_MEMORY_RO,
    FALSE,
    NULL
    );
  ConvertMemoryPageAttributes (
    PageTableBase,
    mPagingMode,
    mCpuHotPlugData.SmBase[mMaxNumberOfCpus - 1] + SMM_HANDLER_OFFSET + TileCodeSize,
    SIZE_32KB - TileCodeSize,
    EFI_MEMORY_XP,
    TRUE,
    NULL
    );

  FlushTlbForAll ();
}

/**
  This function sets GDT/IDT buffer to be RO and XP.
**/
VOID
PatchGdtIdtMap (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINTN                 Size;

  //
  // GDT
  //
  DEBUG ((DEBUG_INFO, "PatchGdtIdtMap - GDT:\n"));

  BaseAddress = mGdtBuffer;
  Size        = ALIGN_VALUE (mGdtBufferSize, SIZE_4KB);
  //
  // The range should have been set to RO
  // if it is allocated with EfiRuntimeServicesCode.
  //
  SmmSetMemoryAttributes (
    BaseAddress,
    Size,
    EFI_MEMORY_XP
    );

  //
  // IDT
  //
  DEBUG ((DEBUG_INFO, "PatchGdtIdtMap - IDT:\n"));

  BaseAddress = gcSmiIdtr.Base;
  Size        = ALIGN_VALUE (gcSmiIdtr.Limit + 1, SIZE_4KB);
  //
  // The range should have been set to RO
  // if it is allocated with EfiRuntimeServicesCode.
  //
  SmmSetMemoryAttributes (
    BaseAddress,
    Size,
    EFI_MEMORY_XP
    );
}

/**
  This function set [Base, Limit] to the input MemoryAttribute.

  @param  Base        Start address of range.
  @param  Limit       Limit address of range.
  @param  Attribute   The bit mask of attributes to modify for the memory region.
  @param  Map         Pointer to the array of Cr3 IA32_MAP_ENTRY.
  @param  Count       Count of IA32_MAP_ENTRY in Map.
**/
VOID
SetMemMapWithNonPresentRange (
  UINT64          Base,
  UINT64          Limit,
  UINT64          Attribute,
  IA32_MAP_ENTRY  *Map,
  UINTN           Count
  )
{
  UINTN   Index;
  UINT64  NonPresentRangeStart;

  NonPresentRangeStart = 0;
  for (Index = 0; Index < Count; Index++) {
    if ((Map[Index].LinearAddress > NonPresentRangeStart) &&
        (Base < Map[Index].LinearAddress) && (Limit > NonPresentRangeStart))
    {
      //
      // We should NOT set attributes for non-present ragne.
      //
      //
      // There is a non-present ( [NonPresentStart, Map[Index].LinearAddress] ) range before current Map[Index]
      // and it is overlapped with [Base, Limit].
      //
      if (Base < NonPresentRangeStart) {
        SmmSetMemoryAttributes (
          Base,
          NonPresentRangeStart - Base,
          Attribute
          );
      }

      Base = Map[Index].LinearAddress;
    }

    NonPresentRangeStart = Map[Index].LinearAddress + Map[Index].Length;
    if (NonPresentRangeStart >= Limit) {
      break;
    }
  }

  Limit = MIN (NonPresentRangeStart, Limit);

  if (Base < Limit) {
    //
    // There is no non-present range in current [Base, Limit] anymore.
    //
    SmmSetMemoryAttributes (
      Base,
      Limit - Base,
      Attribute
      );
  }
}

/**
  This function sets memory attribute according to MemoryAttributesTable.

  @param  MemoryAttributesTable  A pointer to the buffer of SmmMemoryAttributesTable.

**/
VOID
SetMemMapAttributes (
  EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapStart;
  UINTN                  MemoryMapEntryCount;
  UINTN                  DescriptorSize;
  UINTN                  Index;
  UINTN                  PageTable;
  EFI_STATUS             Status;
  IA32_MAP_ENTRY         *Map;
  UINTN                  Count;
  UINT64                 MemoryAttribute;
  BOOLEAN                WriteProtect;
  BOOLEAN                CetEnabled;

  ASSERT (MemoryAttributesTable != NULL);

  PERF_FUNCTION_BEGIN ();

  DEBUG ((DEBUG_INFO, "MemoryAttributesTable:\n"));
  DEBUG ((DEBUG_INFO, "  Version                   - 0x%08x\n", MemoryAttributesTable->Version));
  DEBUG ((DEBUG_INFO, "  NumberOfEntries           - 0x%08x\n", MemoryAttributesTable->NumberOfEntries));
  DEBUG ((DEBUG_INFO, "  DescriptorSize            - 0x%08x\n", MemoryAttributesTable->DescriptorSize));

  MemoryMapEntryCount = MemoryAttributesTable->NumberOfEntries;
  DescriptorSize      = MemoryAttributesTable->DescriptorSize;
  MemoryMapStart      = (EFI_MEMORY_DESCRIPTOR *)(MemoryAttributesTable + 1);
  MemoryMap           = MemoryMapStart;
  for (Index = 0; Index < MemoryMapEntryCount; Index++) {
    DEBUG ((DEBUG_INFO, "Entry (0x%x)\n", MemoryMap));
    DEBUG ((DEBUG_INFO, "  Type              - 0x%x\n", MemoryMap->Type));
    DEBUG ((DEBUG_INFO, "  PhysicalStart     - 0x%016lx\n", MemoryMap->PhysicalStart));
    DEBUG ((DEBUG_INFO, "  VirtualStart      - 0x%016lx\n", MemoryMap->VirtualStart));
    DEBUG ((DEBUG_INFO, "  NumberOfPages     - 0x%016lx\n", MemoryMap->NumberOfPages));
    DEBUG ((DEBUG_INFO, "  Attribute         - 0x%016lx\n", MemoryMap->Attribute));
    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }

  Count     = 0;
  Map       = NULL;
  PageTable = AsmReadCr3 ();
  Status    = PageTableParse (PageTable, mPagingMode, NULL, &Count);
  while (Status == RETURN_BUFFER_TOO_SMALL) {
    if (Map != NULL) {
      FreePool (Map);
    }

    Map = AllocatePool (Count * sizeof (IA32_MAP_ENTRY));
    ASSERT (Map != NULL);
    Status = PageTableParse (PageTable, mPagingMode, Map, &Count);
  }

  ASSERT_RETURN_ERROR (Status);

  WRITE_UNPROTECT_RO_PAGES (WriteProtect, CetEnabled);

  MemoryMap = MemoryMapStart;
  for (Index = 0; Index < MemoryMapEntryCount; Index++) {
    DEBUG ((DEBUG_VERBOSE, "SetAttribute: Memory Entry - 0x%lx, 0x%x\n", MemoryMap->PhysicalStart, MemoryMap->NumberOfPages));
    MemoryAttribute = MemoryMap->Attribute & EFI_MEMORY_ACCESS_MASK;
    if (MemoryAttribute == 0) {
      if (MemoryMap->Type == EfiRuntimeServicesCode) {
        MemoryAttribute = EFI_MEMORY_RO;
      } else {
        ASSERT ((MemoryMap->Type == EfiRuntimeServicesData) || (MemoryMap->Type == EfiConventionalMemory));
        //
        // Set other type memory as NX.
        //
        MemoryAttribute = EFI_MEMORY_XP;
      }
    }

    //
    // There may exist non-present range overlaps with the MemoryMap range.
    // Do not change other attributes of non-present range while still remaining it as non-present
    //
    SetMemMapWithNonPresentRange (
      MemoryMap->PhysicalStart,
      MemoryMap->PhysicalStart + EFI_PAGES_TO_SIZE ((UINTN)MemoryMap->NumberOfPages),
      MemoryAttribute,
      Map,
      Count
      );

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }

  WRITE_PROTECT_RO_PAGES (WriteProtect, CetEnabled);

  FreePool (Map);

  PatchSmmSaveStateMap ();
  PatchGdtIdtMap ();

  PERF_FUNCTION_END ();
}

/**
  This function set given attributes of the memory region specified by
  BaseAddress and Length.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        The bit mask of attributes to set for the memory
                            region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of
                                attributes that cannot be set together.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.
                                The bit mask of attributes is not supported for
                                the memory resource range specified by
                                BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
EdkiiSmmSetMemoryAttributes (
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS                 BaseAddress,
  IN  UINT64                               Length,
  IN  UINT64                               Attributes
  )
{
  return SmmSetMemoryAttributes (BaseAddress, Length, Attributes);
}

/**
  This function clears given attributes of the memory region specified by
  BaseAddress and Length.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        The bit mask of attributes to clear for the memory
                            region.

  @retval EFI_SUCCESS           The attributes were cleared for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of
                                attributes that cannot be cleared together.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.
                                The bit mask of attributes is not supported for
                                the memory resource range specified by
                                BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
EdkiiSmmClearMemoryAttributes (
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS                 BaseAddress,
  IN  UINT64                               Length,
  IN  UINT64                               Attributes
  )
{
  return SmmClearMemoryAttributes (BaseAddress, Length, Attributes);
}

/**
  Create page table based on input PagingMode, LinearAddress and Length.

  @param[in, out]  PageTable           The pointer to the page table.
  @param[in]       PagingMode          The paging mode.
  @param[in]       LinearAddress       The start of the linear address range.
  @param[in]       Length              The length of the linear address range.
  @param[in]       MapAttribute        The MapAttribute of the linear address range
  @param[in]       MapMask             The MapMask used for attribute. The corresponding field in Attribute is ignored if that in MapMask is 0.

**/
VOID
GenPageTable (
  IN OUT UINTN               *PageTable,
  IN     PAGING_MODE         PagingMode,
  IN     UINT64              LinearAddress,
  IN     UINT64              Length,
  IN     IA32_MAP_ATTRIBUTE  MapAttribute,
  IN     IA32_MAP_ATTRIBUTE  MapMask
  )
{
  RETURN_STATUS  Status;
  UINTN          PageTableBufferSize;
  VOID           *PageTableBuffer;

  PageTableBufferSize = 0;

  Status = PageTableMap (
             PageTable,
             PagingMode,
             NULL,
             &PageTableBufferSize,
             LinearAddress,
             Length,
             &MapAttribute,
             &MapMask,
             NULL
             );
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    PageTableBuffer = AllocatePageTableMemory (EFI_SIZE_TO_PAGES (PageTableBufferSize));
    ASSERT (PageTableBuffer != NULL);
    Status = PageTableMap (
               PageTable,
               PagingMode,
               PageTableBuffer,
               &PageTableBufferSize,
               LinearAddress,
               Length,
               &MapAttribute,
               &MapMask,
               NULL
               );
  }

  ASSERT (Status == RETURN_SUCCESS);
  ASSERT (PageTableBufferSize == 0);
}

/**
  Create page table based on input PagingMode and PhysicalAddressBits in smm.

  @param[in]      PagingMode           The paging mode.
  @param[in]      PhysicalAddressBits  The bits of physical address to map.

  @retval         PageTable Address

**/
UINTN
GenSmmPageTable (
  IN PAGING_MODE  PagingMode,
  IN UINT8        PhysicalAddressBits
  )
{
  UINTN                 PageTable;
  UINTN                 Index;
  MM_CPU_MEMORY_REGION  *MemoryRegion;
  UINTN                 MemoryRegionCount;
  IA32_MAP_ATTRIBUTE    MapAttribute;
  IA32_MAP_ATTRIBUTE    MapMask;
  RETURN_STATUS         Status;
  UINTN                 GuardPage;

  PageTable         = 0;
  MemoryRegion      = NULL;
  MemoryRegionCount = 0;
  MapMask.Uint64    = MAX_UINT64;

  //
  // 1. Create NonMmram MemoryRegion
  //
  CreateNonMmramMemMap (PhysicalAddressBits, &MemoryRegion, &MemoryRegionCount);
  ASSERT (MemoryRegion != NULL && MemoryRegionCount != 0);

  //
  // 2. Gen NonMmram MemoryRegion PageTable
  //
  for (Index = 0; Index < MemoryRegionCount; Index++) {
    ASSERT (MemoryRegion[Index].Base % SIZE_4KB == 0);
    ASSERT (MemoryRegion[Index].Length % EFI_PAGE_SIZE == 0);

    //
    // Set the MapAttribute
    //
    MapAttribute.Uint64              = mAddressEncMask|MemoryRegion[Index].Base;
    MapAttribute.Bits.Present        = 1;
    MapAttribute.Bits.ReadWrite      = 1;
    MapAttribute.Bits.UserSupervisor = 1;
    MapAttribute.Bits.Accessed       = 1;
    MapAttribute.Bits.Dirty          = 1;

    //
    // Update the MapAttribute according MemoryRegion[Index].Attribute
    //
    if ((MemoryRegion[Index].Attribute & EFI_MEMORY_RO) != 0) {
      MapAttribute.Bits.ReadWrite = 0;
    }

    if ((MemoryRegion[Index].Attribute & EFI_MEMORY_XP) != 0) {
      if (mXdSupported) {
        MapAttribute.Bits.Nx = 1;
      }
    }

    GenPageTable (&PageTable, PagingMode, MemoryRegion[Index].Base, (UINTN)MemoryRegion[Index].Length, MapAttribute, MapMask);
  }

  //
  // Free the MemoryRegion after usage
  //
  if (MemoryRegion != NULL) {
    FreePool (MemoryRegion);
  }

  //
  // 3. Gen MMRAM Range PageTable
  //
  for (Index = 0; Index < mSmmCpuSmramRangeCount; Index++) {
    ASSERT (mSmmCpuSmramRanges[Index].CpuStart % SIZE_4KB == 0);
    ASSERT (mSmmCpuSmramRanges[Index].PhysicalSize % EFI_PAGE_SIZE == 0);

    //
    // Set the MapAttribute
    //
    MapAttribute.Uint64              = mAddressEncMask|mSmmCpuSmramRanges[Index].CpuStart;
    MapAttribute.Bits.Present        = 1;
    MapAttribute.Bits.ReadWrite      = 1;
    MapAttribute.Bits.UserSupervisor = 1;
    MapAttribute.Bits.Accessed       = 1;
    MapAttribute.Bits.Dirty          = 1;

    GenPageTable (&PageTable, PagingMode, mSmmCpuSmramRanges[Index].CpuStart, mSmmCpuSmramRanges[Index].PhysicalSize, MapAttribute, MapMask);
  }

  if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
    //
    // Mark the 4KB guard page between known good stack and smm stack as non-present
    //
    for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
      GuardPage = mSmmStackArrayBase + EFI_PAGE_SIZE + Index * (mSmmStackSize + mSmmShadowStackSize);
      Status    = ConvertMemoryPageAttributes (PageTable, PagingMode, GuardPage, SIZE_4KB, EFI_MEMORY_RP, TRUE, NULL);
      ASSERT (Status == RETURN_SUCCESS);
    }
  }

  if ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT1) != 0) {
    //
    // Mark [0, 4k] as non-present
    //
    Status = ConvertMemoryPageAttributes (PageTable, PagingMode, 0, SIZE_4KB, EFI_MEMORY_RP, TRUE, NULL);
    ASSERT (Status == RETURN_SUCCESS);
  }

  return (UINTN)PageTable;
}

/**
  This function retrieves the attributes of the memory region specified by
  BaseAddress and Length. If different attributes are got from different part
  of the memory region, EFI_NO_MAPPING will be returned.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        Pointer to attributes returned.

  @retval EFI_SUCCESS           The attributes got for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes is NULL.
  @retval EFI_NO_MAPPING        Attributes are not consistent cross the memory
                                region.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
EdkiiSmmGetMemoryAttributes (
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS                 BaseAddress,
  IN  UINT64                               Length,
  OUT UINT64                               *Attributes
  )
{
  EFI_PHYSICAL_ADDRESS  Address;
  UINT64                *PageEntry;
  UINT64                MemAttr;
  PAGE_ATTRIBUTE        PageAttr;
  INT64                 Size;
  UINTN                 PageTableBase;
  BOOLEAN               EnablePML5Paging;
  IA32_CR4              Cr4;

  if ((Length < SIZE_4KB) || (Attributes == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Size    = (INT64)Length;
  MemAttr = (UINT64)-1;

  PageTableBase    = AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64;
  Cr4.UintN        = AsmReadCr4 ();
  EnablePML5Paging = (BOOLEAN)(Cr4.Bits.LA57 == 1);

  do {
    PageEntry = GetPageTableEntry (PageTableBase, EnablePML5Paging, BaseAddress, &PageAttr);
    if ((PageEntry == NULL) || (PageAttr == PageNone)) {
      return EFI_UNSUPPORTED;
    }

    //
    // If the memory range is cross page table boundary, make sure they
    // share the same attribute. Return EFI_NO_MAPPING if not.
    //
    *Attributes = GetAttributesFromPageEntry (PageEntry);
    if ((MemAttr != (UINT64)-1) && (*Attributes != MemAttr)) {
      return EFI_NO_MAPPING;
    }

    switch (PageAttr) {
      case Page4K:
        Address      = *PageEntry & ~mAddressEncMask & PAGING_4K_ADDRESS_MASK_64;
        Size        -= (SIZE_4KB - (BaseAddress - Address));
        BaseAddress += (SIZE_4KB - (BaseAddress - Address));
        break;

      case Page2M:
        Address      = *PageEntry & ~mAddressEncMask & PAGING_2M_ADDRESS_MASK_64;
        Size        -= SIZE_2MB - (BaseAddress - Address);
        BaseAddress += SIZE_2MB - (BaseAddress - Address);
        break;

      case Page1G:
        Address      = *PageEntry & ~mAddressEncMask & PAGING_1G_ADDRESS_MASK_64;
        Size        -= SIZE_1GB - (BaseAddress - Address);
        BaseAddress += SIZE_1GB - (BaseAddress - Address);
        break;

      default:
        return EFI_UNSUPPORTED;
    }

    MemAttr = *Attributes;
  } while (Size > 0);

  return EFI_SUCCESS;
}

/**
  Prevent the memory pages used for SMM page table from been overwritten.
**/
VOID
EnablePageTableProtection (
  VOID
  )
{
  PAGE_TABLE_POOL       *HeadPool;
  PAGE_TABLE_POOL       *Pool;
  UINT64                PoolSize;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 PageTableBase;

  if (mPageTablePool == NULL) {
    return;
  }

  PageTableBase = AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64;

  //
  // ConvertMemoryPageAttributes might update mPageTablePool. It's safer to
  // remember original one in advance.
  //
  HeadPool = mPageTablePool;
  Pool     = HeadPool;
  do {
    Address  = (EFI_PHYSICAL_ADDRESS)(UINTN)Pool;
    PoolSize = Pool->Offset + EFI_PAGES_TO_SIZE (Pool->FreePages);
    //
    // Set entire pool including header, used-memory and left free-memory as ReadOnly in SMM page table.
    //
    ConvertMemoryPageAttributes (PageTableBase, mPagingMode, Address, PoolSize, EFI_MEMORY_RO, TRUE, NULL);
    Pool = Pool->NextPool;
  } while (Pool != HeadPool);
}

/**
  Return whether memory used by SMM page table need to be set as Read Only.

  @retval TRUE  Need to set SMM page table as Read Only.
  @retval FALSE Do not set SMM page table as Read Only.
**/
BOOLEAN
IfReadOnlyPageTableNeeded (
  VOID
  )
{
  //
  // Don't mark page table memory as read-only if
  //  - SMM heap guard feature enabled; or
  //      BIT2: SMM page guard enabled
  //      BIT3: SMM pool guard enabled
  //  - SMM profile feature enabled
  //
  if (((PcdGet8 (PcdHeapGuardPropertyMask) & (BIT3 | BIT2)) != 0) ||
      mSmmProfileEnabled)
  {
    return FALSE;
  }

  return TRUE;
}

/**
  This function sets memory attribute for page table.
**/
VOID
SetPageTableAttributes (
  VOID
  )
{
  BOOLEAN  WriteProtect;
  BOOLEAN  CetEnabled;

  if (!IfReadOnlyPageTableNeeded ()) {
    return;
  }

  PERF_FUNCTION_BEGIN ();
  DEBUG ((DEBUG_INFO, "SetPageTableAttributes\n"));

  //
  // Disable write protection, because we need mark page table to be write protected.
  // We need *write* page table memory, to mark itself to be *read only*.
  //
  WRITE_UNPROTECT_RO_PAGES (WriteProtect, CetEnabled);

  // Set memory used by page table as Read Only.
  DEBUG ((DEBUG_INFO, "Start...\n"));
  EnablePageTableProtection ();

  //
  // Enable write protection, after page table attribute updated.
  //
  WRITE_PROTECT_RO_PAGES (TRUE, CetEnabled);

  mIsReadOnlyPageTable = TRUE;

  //
  // Flush TLB after mark all page table pool as read only.
  //
  FlushTlbForAll ();
  PERF_FUNCTION_END ();
}
