/** @file

  Virtual Memory Management Services to set or clear the memory encryption bit

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Code is derived from MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.c

**/

#include <Library/CpuLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Cpuid.h>

#include "VirtualMemory.h"

STATIC BOOLEAN mAddressEncMaskChecked = FALSE;
STATIC UINT64  mAddressEncMask;
STATIC PAGE_TABLE_POOL   *mPageTablePool = NULL;

typedef enum {
   SetCBit,
   ClearCBit
} MAP_RANGE_MODE;

/**
  Return the pagetable memory encryption mask.

  @return  The pagetable memory encryption mask.

**/
UINT64
EFIAPI
InternalGetMemEncryptionAddressMask (
  VOID
  )
{
  UINT64                            EncryptionMask;

  if (mAddressEncMaskChecked) {
    return mAddressEncMask;
  }

  EncryptionMask = MemEncryptSevGetEncryptionMask ();

  mAddressEncMask = EncryptionMask & PAGING_1G_ADDRESS_MASK_64;
  mAddressEncMaskChecked = TRUE;

  return mAddressEncMask;
}

/**
  Initialize a buffer pool for page table use only.

  To reduce the potential split operation on page table, the pages reserved for
  page table should be allocated in the times of PAGE_TABLE_POOL_UNIT_PAGES and
  at the boundary of PAGE_TABLE_POOL_ALIGNMENT. So the page pool is always
  initialized with number of pages greater than or equal to the given
  PoolPages.

  Once the pages in the pool are used up, this method should be called again to
  reserve at least another PAGE_TABLE_POOL_UNIT_PAGES. Usually this won't
  happen often in practice.

  @param[in] PoolPages      The least page number of the pool to be created.

  @retval TRUE    The pool is initialized successfully.
  @retval FALSE   The memory is out of resource.
**/
STATIC
BOOLEAN
InitializePageTablePool (
  IN  UINTN                           PoolPages
  )
{
  VOID                      *Buffer;

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
STATIC
VOID *
EFIAPI
AllocatePageTableMemory (
  IN UINTN           Pages
  )
{
  VOID                            *Buffer;

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

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: Buffer=0x%Lx Pages=%ld\n",
    gEfiCallerBaseName,
    __FUNCTION__,
    Buffer,
    Pages
    ));

  return Buffer;
}


/**
  Split 2M page to 4K.

  @param[in]      PhysicalAddress       Start physical address the 2M page
                                        covered.
  @param[in, out] PageEntry2M           Pointer to 2M page entry.
  @param[in]      StackBase             Stack base address.
  @param[in]      StackSize             Stack size.

**/
STATIC
VOID
Split2MPageTo4K (
  IN        PHYSICAL_ADDRESS               PhysicalAddress,
  IN  OUT   UINT64                        *PageEntry2M,
  IN        PHYSICAL_ADDRESS               StackBase,
  IN        UINTN                          StackSize
  )
{
  PHYSICAL_ADDRESS                  PhysicalAddress4K;
  UINTN                             IndexOfPageTableEntries;
  PAGE_TABLE_4K_ENTRY               *PageTableEntry;
  PAGE_TABLE_4K_ENTRY               *PageTableEntry1;
  UINT64                            AddressEncMask;

  PageTableEntry = AllocatePageTableMemory(1);

  PageTableEntry1 = PageTableEntry;

  AddressEncMask = InternalGetMemEncryptionAddressMask ();

  ASSERT (PageTableEntry != NULL);
  ASSERT (*PageEntry2M & AddressEncMask);

  PhysicalAddress4K = PhysicalAddress;
  for (IndexOfPageTableEntries = 0;
       IndexOfPageTableEntries < 512;
       (IndexOfPageTableEntries++,
        PageTableEntry++,
        PhysicalAddress4K += SIZE_4KB)) {
    //
    // Fill in the Page Table entries
    //
    PageTableEntry->Uint64 = (UINT64) PhysicalAddress4K | AddressEncMask;
    PageTableEntry->Bits.ReadWrite = 1;
    PageTableEntry->Bits.Present = 1;
    if ((PhysicalAddress4K >= StackBase) &&
        (PhysicalAddress4K < StackBase + StackSize)) {
      //
      // Set Nx bit for stack.
      //
      PageTableEntry->Bits.Nx = 1;
    }
  }

  //
  // Fill in 2M page entry.
  //
  *PageEntry2M = ((UINT64)(UINTN)PageTableEntry1 |
                  IA32_PG_P | IA32_PG_RW | AddressEncMask);
}

/**
  Set one page of page table pool memory to be read-only.

  @param[in] PageTableBase    Base address of page table (CR3).
  @param[in] Address          Start address of a page to be set as read-only.
  @param[in] Level4Paging     Level 4 paging flag.

**/
STATIC
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

  AddressEncMask  = InternalGetMemEncryptionAddressMask();
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
STATIC
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
  // SetPageTablePoolReadOnly might update mPageTablePool. It's safer to
  // remember original one in advance.
  //
  HeadPool = mPageTablePool;
  Pool = HeadPool;
  do {
    Address  = (EFI_PHYSICAL_ADDRESS)(UINTN)Pool;
    PoolSize = Pool->Offset + EFI_PAGES_TO_SIZE (Pool->FreePages);

    //
    // The size of one pool must be multiple of PAGE_TABLE_POOL_UNIT_SIZE,
    // which is one of page size of the processor (2MB by default). Let's apply
    // the protection to them one by one.
    //
    while (PoolSize > 0) {
      SetPageTablePoolReadOnly(PageTableBase, Address, Level4Paging);
      Address   += PAGE_TABLE_POOL_UNIT_SIZE;
      PoolSize  -= PAGE_TABLE_POOL_UNIT_SIZE;
    }

    Pool = Pool->NextPool;
  } while (Pool != HeadPool);

}


/**
  Split 1G page to 2M.

  @param[in]      PhysicalAddress       Start physical address the 1G page
                                        covered.
  @param[in, out] PageEntry1G           Pointer to 1G page entry.
  @param[in]      StackBase             Stack base address.
  @param[in]      StackSize             Stack size.

**/
STATIC
VOID
Split1GPageTo2M (
  IN          PHYSICAL_ADDRESS               PhysicalAddress,
  IN  OUT     UINT64                         *PageEntry1G,
  IN          PHYSICAL_ADDRESS               StackBase,
  IN          UINTN                          StackSize
  )
{
  PHYSICAL_ADDRESS                  PhysicalAddress2M;
  UINTN                             IndexOfPageDirectoryEntries;
  PAGE_TABLE_ENTRY                  *PageDirectoryEntry;
  UINT64                            AddressEncMask;

  PageDirectoryEntry = AllocatePageTableMemory(1);

  AddressEncMask = InternalGetMemEncryptionAddressMask ();
  ASSERT (PageDirectoryEntry != NULL);
  ASSERT (*PageEntry1G & AddressEncMask);
  //
  // Fill in 1G page entry.
  //
  *PageEntry1G = ((UINT64)(UINTN)PageDirectoryEntry |
                  IA32_PG_P | IA32_PG_RW | AddressEncMask);

  PhysicalAddress2M = PhysicalAddress;
  for (IndexOfPageDirectoryEntries = 0;
       IndexOfPageDirectoryEntries < 512;
       (IndexOfPageDirectoryEntries++,
        PageDirectoryEntry++,
        PhysicalAddress2M += SIZE_2MB)) {
    if ((PhysicalAddress2M < StackBase + StackSize) &&
        ((PhysicalAddress2M + SIZE_2MB) > StackBase)) {
      //
      // Need to split this 2M page that covers stack range.
      //
      Split2MPageTo4K (
        PhysicalAddress2M,
        (UINT64 *)PageDirectoryEntry,
        StackBase,
        StackSize
        );
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
  Set or Clear the memory encryption bit

  @param[in, out] PageTablePointer      Page table entry pointer (PTE).
  @param[in]      Mode                  Set or Clear encryption bit

**/
STATIC VOID
SetOrClearCBit(
  IN   OUT     UINT64*            PageTablePointer,
  IN           MAP_RANGE_MODE     Mode
  )
{
  UINT64      AddressEncMask;

  AddressEncMask = InternalGetMemEncryptionAddressMask ();

  if (Mode == SetCBit) {
    *PageTablePointer |= AddressEncMask;
  } else {
    *PageTablePointer &= ~AddressEncMask;
  }

}

/**
 Check the WP status in CR0 register. This bit is used to lock or unlock write
 access to pages marked as read-only.

  @retval TRUE    Write protection is enabled.
  @retval FALSE   Write protection is disabled.
**/
STATIC
BOOLEAN
IsReadOnlyPageWriteProtected (
  VOID
  )
{
  return ((AsmReadCr0 () & BIT16) != 0);
}


/**
 Disable Write Protect on pages marked as read-only.
**/
STATIC
VOID
DisableReadOnlyPageWriteProtect (
  VOID
  )
{
  AsmWriteCr0 (AsmReadCr0() & ~BIT16);
}

/**
 Enable Write Protect on pages marked as read-only.
**/
STATIC
VOID
EnableReadOnlyPageWriteProtect (
  VOID
  )
{
  AsmWriteCr0 (AsmReadCr0() | BIT16);
}


/**
  This function either sets or clears memory encryption bit for the memory
  region specified by PhysicalAddress and Length from the current page table
  context.

  The function iterates through the PhysicalAddress one page at a time, and set
  or clears the memory encryption mask in the page table. If it encounters
  that a given physical address range is part of large page then it attempts to
  change the attribute at one go (based on size), otherwise it splits the
  large pages into smaller (e.g 2M page into 4K pages) and then try to set or
  clear the encryption bit on the smallest page size.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Mode                    Set or Clear mode
  @param[in]  CacheFlush              Flush the caches before applying the
                                      encryption mask

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute
                                      is not supported
**/
STATIC
RETURN_STATUS
EFIAPI
SetMemoryEncDec (
  IN    PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN    PHYSICAL_ADDRESS         PhysicalAddress,
  IN    UINTN                    Length,
  IN    MAP_RANGE_MODE           Mode,
  IN    BOOLEAN                  CacheFlush
  )
{
  PAGE_MAP_AND_DIRECTORY_POINTER *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER *PageUpperDirectoryPointerEntry;
  PAGE_MAP_AND_DIRECTORY_POINTER *PageDirectoryPointerEntry;
  PAGE_TABLE_1G_ENTRY            *PageDirectory1GEntry;
  PAGE_TABLE_ENTRY               *PageDirectory2MEntry;
  PAGE_TABLE_4K_ENTRY            *PageTableEntry;
  UINT64                         PgTableMask;
  UINT64                         AddressEncMask;
  BOOLEAN                        IsWpEnabled;
  RETURN_STATUS                  Status;

  //
  // Set PageMapLevel4Entry to suppress incorrect compiler/analyzer warnings.
  //
  PageMapLevel4Entry = NULL;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: Cr3Base=0x%Lx Physical=0x%Lx Length=0x%Lx Mode=%a CacheFlush=%u\n",
    gEfiCallerBaseName,
    __FUNCTION__,
    Cr3BaseAddress,
    PhysicalAddress,
    (UINT64)Length,
    (Mode == SetCBit) ? "Encrypt" : "Decrypt",
    (UINT32)CacheFlush
    ));

  //
  // Check if we have a valid memory encryption mask
  //
  AddressEncMask = InternalGetMemEncryptionAddressMask ();
  if (!AddressEncMask) {
    return RETURN_ACCESS_DENIED;
  }

  PgTableMask = AddressEncMask | EFI_PAGE_MASK;

  if (Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // We are going to change the memory encryption attribute from C=0 -> C=1 or
  // vice versa Flush the caches to ensure that data is written into memory
  // with correct C-bit
  //
  if (CacheFlush) {
    WriteBackInvalidateDataCacheRange((VOID*) (UINTN)PhysicalAddress, Length);
  }

  //
  // Make sure that the page table is changeable.
  //
  IsWpEnabled = IsReadOnlyPageWriteProtected ();
  if (IsWpEnabled) {
    DisableReadOnlyPageWriteProtect ();
  }

  Status = EFI_SUCCESS;

  while (Length != 0)
  {
    //
    // If Cr3BaseAddress is not specified then read the current CR3
    //
    if (Cr3BaseAddress == 0) {
      Cr3BaseAddress = AsmReadCr3();
    }

    PageMapLevel4Entry = (VOID*) (Cr3BaseAddress & ~PgTableMask);
    PageMapLevel4Entry += PML4_OFFSET(PhysicalAddress);
    if (!PageMapLevel4Entry->Bits.Present) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%a: bad PML4 for Physical=0x%Lx\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        PhysicalAddress
        ));
      Status = RETURN_NO_MAPPING;
      goto Done;
    }

    PageDirectory1GEntry = (VOID *)(
                             (PageMapLevel4Entry->Bits.PageTableBaseAddress <<
                              12) & ~PgTableMask
                             );
    PageDirectory1GEntry += PDP_OFFSET(PhysicalAddress);
    if (!PageDirectory1GEntry->Bits.Present) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%a: bad PDPE for Physical=0x%Lx\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        PhysicalAddress
        ));
      Status = RETURN_NO_MAPPING;
      goto Done;
    }

    //
    // If the MustBe1 bit is not 1, it's not actually a 1GB entry
    //
    if (PageDirectory1GEntry->Bits.MustBe1) {
      //
      // Valid 1GB page
      // If we have at least 1GB to go, we can just update this entry
      //
      if ((PhysicalAddress & (BIT30 - 1)) == 0 && Length >= BIT30) {
        SetOrClearCBit(&PageDirectory1GEntry->Uint64, Mode);
        DEBUG ((
          DEBUG_VERBOSE,
          "%a:%a: updated 1GB entry for Physical=0x%Lx\n",
          gEfiCallerBaseName,
          __FUNCTION__,
          PhysicalAddress
          ));
        PhysicalAddress += BIT30;
        Length -= BIT30;
      } else {
        //
        // We must split the page
        //
        DEBUG ((
          DEBUG_VERBOSE,
          "%a:%a: splitting 1GB page for Physical=0x%Lx\n",
          gEfiCallerBaseName,
          __FUNCTION__,
          PhysicalAddress
          ));
        Split1GPageTo2M (
          (UINT64)PageDirectory1GEntry->Bits.PageTableBaseAddress << 30,
          (UINT64 *)PageDirectory1GEntry,
          0,
          0
          );
        continue;
      }
    } else {
      //
      // Actually a PDP
      //
      PageUpperDirectoryPointerEntry =
        (PAGE_MAP_AND_DIRECTORY_POINTER *)PageDirectory1GEntry;
      PageDirectory2MEntry =
        (VOID *)(
          (PageUpperDirectoryPointerEntry->Bits.PageTableBaseAddress <<
           12) & ~PgTableMask
          );
      PageDirectory2MEntry += PDE_OFFSET(PhysicalAddress);
      if (!PageDirectory2MEntry->Bits.Present) {
        DEBUG ((
          DEBUG_ERROR,
          "%a:%a: bad PDE for Physical=0x%Lx\n",
          gEfiCallerBaseName,
          __FUNCTION__,
          PhysicalAddress
          ));
        Status = RETURN_NO_MAPPING;
        goto Done;
      }
      //
      // If the MustBe1 bit is not a 1, it's not a 2MB entry
      //
      if (PageDirectory2MEntry->Bits.MustBe1) {
        //
        // Valid 2MB page
        // If we have at least 2MB left to go, we can just update this entry
        //
        if ((PhysicalAddress & (BIT21-1)) == 0 && Length >= BIT21) {
          SetOrClearCBit (&PageDirectory2MEntry->Uint64, Mode);
          PhysicalAddress += BIT21;
          Length -= BIT21;
        } else {
          //
          // We must split up this page into 4K pages
          //
          DEBUG ((
            DEBUG_VERBOSE,
            "%a:%a: splitting 2MB page for Physical=0x%Lx\n",
            gEfiCallerBaseName,
            __FUNCTION__,
            PhysicalAddress
            ));
          Split2MPageTo4K (
            (UINT64)PageDirectory2MEntry->Bits.PageTableBaseAddress << 21,
            (UINT64 *)PageDirectory2MEntry,
            0,
            0
            );
          continue;
        }
      } else {
        PageDirectoryPointerEntry =
          (PAGE_MAP_AND_DIRECTORY_POINTER *)PageDirectory2MEntry;
        PageTableEntry =
          (VOID *)(
            (PageDirectoryPointerEntry->Bits.PageTableBaseAddress <<
             12) & ~PgTableMask
            );
        PageTableEntry += PTE_OFFSET(PhysicalAddress);
        if (!PageTableEntry->Bits.Present) {
          DEBUG ((
            DEBUG_ERROR,
            "%a:%a: bad PTE for Physical=0x%Lx\n",
            gEfiCallerBaseName,
            __FUNCTION__,
            PhysicalAddress
            ));
          Status = RETURN_NO_MAPPING;
          goto Done;
        }
        SetOrClearCBit (&PageTableEntry->Uint64, Mode);
        PhysicalAddress += EFI_PAGE_SIZE;
        Length -= EFI_PAGE_SIZE;
      }
    }
  }

  //
  // Protect the page table by marking the memory used for page table to be
  // read-only.
  //
  if (IsWpEnabled) {
    EnablePageTableProtection ((UINTN)PageMapLevel4Entry, TRUE);
  }

  //
  // Flush TLB
  //
  CpuFlushTlb();

Done:
  //
  // Restore page table write protection, if any.
  //
  if (IsWpEnabled) {
    EnableReadOnlyPageWriteProtect ();
  }

  return Status;
}

/**
  This function clears memory encryption bit for the memory region specified by
  PhysicalAddress and Length from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Flush                   Flush the caches before applying the
                                      encryption mask

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevSetMemoryDecrypted (
  IN  PHYSICAL_ADDRESS        Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS        PhysicalAddress,
  IN  UINTN                   Length,
  IN  BOOLEAN                 Flush
  )
{

  return SetMemoryEncDec (
           Cr3BaseAddress,
           PhysicalAddress,
           Length,
           ClearCBit,
           Flush
           );
}

/**
  This function sets memory encryption bit for the memory region specified by
  PhysicalAddress and Length from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Flush                   Flush the caches before applying the
                                      encryption mask

  @retval RETURN_SUCCESS              The attributes were set for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevSetMemoryEncrypted (
  IN  PHYSICAL_ADDRESS        Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS        PhysicalAddress,
  IN  UINTN                   Length,
  IN  BOOLEAN                 Flush
  )
{
  return SetMemoryEncDec (
           Cr3BaseAddress,
           PhysicalAddress,
           Length,
           SetCBit,
           Flush
           );
}
