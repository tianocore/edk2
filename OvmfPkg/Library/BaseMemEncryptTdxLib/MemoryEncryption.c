/** @file

  Virtual Memory Management Services to set or clear the memory encryption.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Code is derived from MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.c

  Note:
  There a lot of duplicated codes for Page Table operations. These
  codes should be moved to a common library (PageTablesLib) so that it is
  more friendly for review and maintain. There is a new feature requirement
  https://bugzilla.tianocore.org/show_bug.cgi?id=847 which is to implement
  the library. After the lib is introduced, this file will be refactored.

**/

#include <Uefi.h>
#include <Uefi/UefiBaseType.h>
#include <Library/CpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptTdxLib.h>
#include "VirtualMemory.h"
#include <IndustryStandard/Tdx.h>
#include <Library/TdxLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MemoryAccept.h>
#include <ConfidentialComputingGuestAttr.h>

typedef enum {
  SetSharedBit,
  ClearSharedBit
} TDX_PAGETABLE_MODE;

STATIC PAGE_TABLE_POOL  *mPageTablePool = NULL;

#define MAX_RETRIES_PER_PAGE  3

/**
  Returns boolean to indicate whether to indicate which, if any, memory encryption is enabled

  @param[in]  Type          Bitmask of encryption technologies to check is enabled

  @retval TRUE              The encryption type(s) are enabled
  @retval FALSE             The encryption type(s) are not enabled
**/
BOOLEAN
EFIAPI
MemEncryptTdxIsEnabled (
  VOID
  )
{
  return CC_GUEST_IS_TDX (PcdGet64 (PcdConfidentialComputingGuestAttr));
}

/**
  Get the memory encryption mask

  @param[out]      EncryptionMask        contains the pte mask.

**/
STATIC
UINT64
GetMemEncryptionAddressMask (
  VOID
  )
{
  return TdSharedPageMask ();
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
  IN  UINTN  PoolPages
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
STATIC
VOID *
EFIAPI
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

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: Buffer=0x%Lx Pages=%ld\n",
    gEfiCallerBaseName,
    __func__,
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
  IN        PHYSICAL_ADDRESS  PhysicalAddress,
  IN  OUT   UINT64            *PageEntry2M,
  IN        PHYSICAL_ADDRESS  StackBase,
  IN        UINTN             StackSize,
  IN        UINT64            AddressEncMask
  )
{
  PHYSICAL_ADDRESS     PhysicalAddress4K;
  UINTN                IndexOfPageTableEntries;
  PAGE_TABLE_4K_ENTRY  *PageTableEntry, *PageTableEntry1;

  PageTableEntry = AllocatePageTableMemory (1);

  PageTableEntry1 = PageTableEntry;

  if (PageTableEntry == NULL) {
    ASSERT (FALSE);
    return;
  }

  PhysicalAddress4K = PhysicalAddress;
  for (IndexOfPageTableEntries = 0;
       IndexOfPageTableEntries < 512;
       (IndexOfPageTableEntries++,
        PageTableEntry++,
        PhysicalAddress4K += SIZE_4KB))
  {
    //
    // Fill in the Page Table entries
    //
    PageTableEntry->Uint64         = (UINT64)PhysicalAddress4K | AddressEncMask;
    PageTableEntry->Bits.ReadWrite = 1;
    PageTableEntry->Bits.Present   = 1;
    if ((PhysicalAddress4K >= StackBase) &&
        (PhysicalAddress4K < StackBase + StackSize))
    {
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
  IN  UINTN                 PageTableBase,
  IN  EFI_PHYSICAL_ADDRESS  Address,
  IN  BOOLEAN               Level4Paging
  )
{
  UINTN                 Index;
  UINTN                 EntryIndex;
  UINT64                AddressEncMask;
  UINT64                ActiveAddressEncMask;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  UINT64                *PageTable;
  UINT64                *NewPageTable;
  UINT64                PageAttr;
  UINT64                LevelSize[5];
  UINT64                LevelMask[5];
  UINTN                 LevelShift[5];
  UINTN                 Level;
  UINT64                PoolUnitSize;

  if (PageTableBase == 0) {
    ASSERT (FALSE);
    return;
  }

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

  AddressEncMask = GetMemEncryptionAddressMask () &
                   PAGING_1G_ADDRESS_MASK_64;
  PageTable    = (UINT64 *)(UINTN)PageTableBase;
  PoolUnitSize = PAGE_TABLE_POOL_UNIT_SIZE;

  for (Level = (Level4Paging) ? 4 : 3; Level > 0; --Level) {
    Index  = ((UINTN)RShiftU64 (Address, LevelShift[Level]));
    Index &= PAGING_PAE_INDEX_MASK;

    PageAttr             = PageTable[Index];
    ActiveAddressEncMask = GetMemEncryptionAddressMask () & PageAttr;

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
      if (NewPageTable == NULL) {
        ASSERT (FALSE);
        return;
      }

      PhysicalAddress = PageAttr & LevelMask[Level];
      for (EntryIndex = 0;
           EntryIndex < EFI_PAGE_SIZE/sizeof (UINT64);
           ++EntryIndex)
      {
        NewPageTable[EntryIndex] = PhysicalAddress  | ActiveAddressEncMask |
                                   IA32_PG_P | IA32_PG_RW;
        if (Level > 2) {
          NewPageTable[EntryIndex] |= IA32_PG_PS;
        }

        PhysicalAddress += LevelSize[Level - 1];
      }

      PageTable[Index] = (UINT64)(UINTN)NewPageTable | ActiveAddressEncMask |
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
  // SetPageTablePoolReadOnly might update mPageTablePool. It's safer to
  // remember original one in advance.
  //
  HeadPool = mPageTablePool;
  Pool     = HeadPool;
  do {
    Address  = (EFI_PHYSICAL_ADDRESS)(UINTN)Pool;
    PoolSize = Pool->Offset + EFI_PAGES_TO_SIZE (Pool->FreePages);

    //
    // The size of one pool must be multiple of PAGE_TABLE_POOL_UNIT_SIZE,
    // which is one of page size of the processor (2MB by default). Let's apply
    // the protection to them one by one.
    //
    while (PoolSize > 0) {
      SetPageTablePoolReadOnly (PageTableBase, Address, Level4Paging);
      Address  += PAGE_TABLE_POOL_UNIT_SIZE;
      PoolSize -= PAGE_TABLE_POOL_UNIT_SIZE;
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
  IN          PHYSICAL_ADDRESS  PhysicalAddress,
  IN  OUT     UINT64            *PageEntry1G,
  IN          PHYSICAL_ADDRESS  StackBase,
  IN          UINTN             StackSize
  )
{
  PHYSICAL_ADDRESS  PhysicalAddress2M;
  UINTN             IndexOfPageDirectoryEntries;
  PAGE_TABLE_ENTRY  *PageDirectoryEntry;
  UINT64            AddressEncMask;
  UINT64            ActiveAddressEncMask;

  PageDirectoryEntry = AllocatePageTableMemory (1);
  if (PageDirectoryEntry == NULL) {
    return;
  }

  AddressEncMask = GetMemEncryptionAddressMask ();
  ASSERT (PageDirectoryEntry != NULL);

  ActiveAddressEncMask = *PageEntry1G & AddressEncMask;
  //
  // Fill in 1G page entry.
  //
  *PageEntry1G = ((UINT64)(UINTN)PageDirectoryEntry |
                  IA32_PG_P | IA32_PG_RW | ActiveAddressEncMask);

  PhysicalAddress2M = PhysicalAddress;
  for (IndexOfPageDirectoryEntries = 0;
       IndexOfPageDirectoryEntries < 512;
       (IndexOfPageDirectoryEntries++,
        PageDirectoryEntry++,
        PhysicalAddress2M += SIZE_2MB))
  {
    if ((PhysicalAddress2M < StackBase + StackSize) &&
        ((PhysicalAddress2M + SIZE_2MB) > StackBase))
    {
      //
      // Need to split this 2M page that covers stack range.
      //
      Split2MPageTo4K (
        PhysicalAddress2M,
        (UINT64 *)PageDirectoryEntry,
        StackBase,
        StackSize,
        ActiveAddressEncMask
        );
    } else {
      //
      // Fill in the Page Directory entries
      //
      PageDirectoryEntry->Uint64         = (UINT64)PhysicalAddress2M | ActiveAddressEncMask;
      PageDirectoryEntry->Bits.ReadWrite = 1;
      PageDirectoryEntry->Bits.Present   = 1;
      PageDirectoryEntry->Bits.MustBe1   = 1;
    }
  }
}

/**
  Set or Clear the memory shared bit

  @param[in]      PagetablePoint        Page table entry pointer (PTE).
  @param[in]      Mode                  Set or Clear shared bit

  @retval         EFI_SUCCESS           Successfully set or clear the memory shared bit
  @retval         Others                Other error as indicated
**/
STATIC
EFI_STATUS
SetOrClearSharedBit (
  IN   OUT     UINT64              *PageTablePointer,
  IN           TDX_PAGETABLE_MODE  Mode,
  IN           PHYSICAL_ADDRESS    PhysicalAddress,
  IN           UINT64              Length
  )
{
  UINT64                        AddressEncMask;
  UINT64                        TdStatus;
  EFI_STATUS                    Status;
  EDKII_MEMORY_ACCEPT_PROTOCOL  *MemoryAcceptProtocol;

  UINT64  MapGpaRetryAddr;
  UINT32  RetryCount;
  UINT64  EndAddress;

  MapGpaRetryAddr = 0;
  RetryCount      = 0;

  AddressEncMask = GetMemEncryptionAddressMask ();

  //
  // Set or clear page table entry. Also, set shared bit in physical address, before calling MapGPA
  //
  if (Mode == SetSharedBit) {
    *PageTablePointer |= AddressEncMask;
    PhysicalAddress   |= AddressEncMask;
  } else {
    *PageTablePointer &= ~AddressEncMask;
    PhysicalAddress   &= ~AddressEncMask;
  }

  EndAddress = PhysicalAddress + Length;
  while (RetryCount < MAX_RETRIES_PER_PAGE) {
    TdStatus = TdVmCall (TDVMCALL_MAPGPA, PhysicalAddress, Length, 0, 0, &MapGpaRetryAddr);
    if (TdStatus != TDVMCALL_STATUS_RETRY) {
      break;
    }

    DEBUG ((DEBUG_VERBOSE, "%a: TdVmcall(MAPGPA) Retry PhysicalAddress is %llx, MapGpaRetryAddr is %llx\n", __func__, PhysicalAddress, MapGpaRetryAddr));

    if ((MapGpaRetryAddr < PhysicalAddress) || (MapGpaRetryAddr >= EndAddress)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: TdVmcall(MAPGPA) failed with MapGpaRetryAddr(%llx) less than PhysicalAddress(%llx) or more than or equal to EndAddress(%llx) \n",
        __func__,
        MapGpaRetryAddr,
        PhysicalAddress,
        EndAddress
        ));
      break;
    }

    if (MapGpaRetryAddr == PhysicalAddress) {
      RetryCount++;
      continue;
    }

    PhysicalAddress = MapGpaRetryAddr;
    Length          = EndAddress - PhysicalAddress;
    RetryCount      = 0;
  }

  if (TdStatus != 0) {
    DEBUG ((DEBUG_ERROR, "%a: TdVmcall(MAPGPA) failed with %llx\n", __func__, TdStatus));
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  Status = EFI_SUCCESS;

  //
  // If changing shared to private, must accept-page again
  //
  if (Mode == ClearSharedBit) {
    Status = gBS->LocateProtocol (&gEdkiiMemoryAcceptProtocolGuid, NULL, (VOID **)&MemoryAcceptProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to locate MemoryAcceptProtocol with %r\n", __func__, Status));
      ASSERT (FALSE);
      return Status;
    }

    Status = MemoryAcceptProtocol->AcceptMemory (MemoryAcceptProtocol, PhysicalAddress, Length);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to AcceptMemory with %r\n", __func__, Status));
      ASSERT (FALSE);
      return Status;
    }
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: pte=0x%Lx AddressEncMask=0x%Lx Mode=0x%x MapGPA Status=0x%x\n",
    gEfiCallerBaseName,
    __func__,
    *PageTablePointer,
    AddressEncMask,
    Mode,
    Status
    ));

  return EFI_SUCCESS;
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
  AsmWriteCr0 (AsmReadCr0 () & ~BIT16);
}

/**
 Enable Write Protect on pages marked as read-only.
**/
VOID
EnableReadOnlyPageWriteProtect (
  VOID
  )
{
  AsmWriteCr0 (AsmReadCr0 () | BIT16);
}

/**
  This function either sets or clears memory encryption for the memory
  region specified by PhysicalAddress and Length from the current page table
  context.

  The function iterates through the PhysicalAddress one page at a time, and set
  or clears the memory encryption in the page table. If it encounters
  that a given physical address range is part of large page then it attempts to
  change the attribute at one go (based on size), otherwise it splits the
  large pages into smaller (e.g 2M page into 4K pages) and then try to set or
  clear the shared bit on the smallest page size.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Mode                    Set or Clear mode

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute
                                      is not supported
**/
STATIC
RETURN_STATUS
EFIAPI
SetMemorySharedOrPrivate (
  IN    PHYSICAL_ADDRESS    Cr3BaseAddress,
  IN    PHYSICAL_ADDRESS    PhysicalAddress,
  IN    UINTN               Length,
  IN    TDX_PAGETABLE_MODE  Mode
  )
{
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageUpperDirectoryPointerEntry;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageDirectoryPointerEntry;
  PAGE_TABLE_1G_ENTRY             *PageDirectory1GEntry;
  PAGE_TABLE_ENTRY                *PageDirectory2MEntry;
  PAGE_TABLE_4K_ENTRY             *PageTableEntry;
  UINT64                          PgTableMask;
  UINT64                          AddressEncMask;
  UINT64                          ActiveEncMask;
  BOOLEAN                         IsWpEnabled;
  RETURN_STATUS                   Status;
  IA32_CR4                        Cr4;
  BOOLEAN                         Page5LevelSupport;

  //
  // Set PageMapLevel4Entry to suppress incorrect compiler/analyzer warnings.
  //
  PageMapLevel4Entry = NULL;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: Cr3Base=0x%Lx Physical=0x%Lx Length=0x%Lx Mode=%a\n",
    gEfiCallerBaseName,
    __func__,
    Cr3BaseAddress,
    PhysicalAddress,
    (UINT64)Length,
    (Mode == SetSharedBit) ? "Shared" : "Private"
    ));

  //
  // Check if we have a valid memory encryption mask
  //
  AddressEncMask = GetMemEncryptionAddressMask ();

  PgTableMask = AddressEncMask | EFI_PAGE_MASK;

  if (Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Make sure that the page table is changeable.
  //
  IsWpEnabled = IsReadOnlyPageWriteProtected ();
  if (IsWpEnabled) {
    DisableReadOnlyPageWriteProtect ();
  }

  //
  // If Cr3BaseAddress is not specified then read the current CR3
  //
  if (Cr3BaseAddress == 0) {
    Cr3BaseAddress = AsmReadCr3 ();
  }

  //
  // CPU will already have LA57 enabled so just check CR4
  //
  Cr4.UintN = AsmReadCr4 ();

  Page5LevelSupport = (Cr4.Bits.LA57 ? TRUE : FALSE);
  //
  // If 5-level pages, adjust Cr3BaseAddress to point to first 4-level page directory,
  // we will only have 1
  //
  if (Page5LevelSupport) {
    Cr3BaseAddress = *(UINT64 *)Cr3BaseAddress & ~PgTableMask;
  }

  Status = EFI_SUCCESS;

  while (Length) {
    PageMapLevel4Entry  = (VOID *)(Cr3BaseAddress & ~PgTableMask);
    PageMapLevel4Entry += PML4_OFFSET (PhysicalAddress);
    if (!PageMapLevel4Entry->Bits.Present) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%a: bad PML4 for Physical=0x%Lx\n",
        gEfiCallerBaseName,
        __func__,
        PhysicalAddress
        ));
      Status = RETURN_NO_MAPPING;
      goto Done;
    }

    PageDirectory1GEntry = (VOID *)(
                                    (PageMapLevel4Entry->Bits.PageTableBaseAddress <<
                                     12) & ~PgTableMask
                                    );
    PageDirectory1GEntry += PDP_OFFSET (PhysicalAddress);
    if (!PageDirectory1GEntry->Bits.Present) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%a: bad PDPE for Physical=0x%Lx\n",
        gEfiCallerBaseName,
        __func__,
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
      if (!(PhysicalAddress & (BIT30 - 1)) && (Length >= BIT30)) {
        Status = SetOrClearSharedBit (&PageDirectory1GEntry->Uint64, Mode, PhysicalAddress, BIT30);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        DEBUG ((
          DEBUG_VERBOSE,
          "%a:%a: updated 1GB entry for Physical=0x%Lx\n",
          gEfiCallerBaseName,
          __func__,
          PhysicalAddress
          ));
        PhysicalAddress += BIT30;
        Length          -= BIT30;
      } else {
        //
        // We must split the page
        //
        DEBUG ((
          DEBUG_VERBOSE,
          "%a:%a: splitting 1GB page for Physical=0x%Lx\n",
          gEfiCallerBaseName,
          __func__,
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
      PageDirectory2MEntry += PDE_OFFSET (PhysicalAddress);
      if (!PageDirectory2MEntry->Bits.Present) {
        DEBUG ((
          DEBUG_ERROR,
          "%a:%a: bad PDE for Physical=0x%Lx\n",
          gEfiCallerBaseName,
          __func__,
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
        if (!(PhysicalAddress & (BIT21-1)) && (Length >= BIT21)) {
          Status = SetOrClearSharedBit (&PageDirectory2MEntry->Uint64, Mode, PhysicalAddress, BIT21);
          if (EFI_ERROR (Status)) {
            goto Done;
          }

          PhysicalAddress += BIT21;
          Length          -= BIT21;
        } else {
          //
          // We must split up this page into 4K pages
          //
          DEBUG ((
            DEBUG_VERBOSE,
            "%a:%a: splitting 2MB page for Physical=0x%Lx\n",
            gEfiCallerBaseName,
            __func__,
            PhysicalAddress
            ));

          ActiveEncMask = PageDirectory2MEntry->Uint64 & AddressEncMask;

          Split2MPageTo4K (
            (UINT64)PageDirectory2MEntry->Bits.PageTableBaseAddress << 21,
            (UINT64 *)PageDirectory2MEntry,
            0,
            0,
            ActiveEncMask
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
        PageTableEntry += PTE_OFFSET (PhysicalAddress);
        if (!PageTableEntry->Bits.Present) {
          DEBUG ((
            DEBUG_ERROR,
            "%a:%a: bad PTE for Physical=0x%Lx\n",
            gEfiCallerBaseName,
            __func__,
            PhysicalAddress
            ));
          Status = RETURN_NO_MAPPING;
          goto Done;
        }

        Status = SetOrClearSharedBit (&PageTableEntry->Uint64, Mode, PhysicalAddress, EFI_PAGE_SIZE);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        PhysicalAddress += EFI_PAGE_SIZE;
        Length          -= EFI_PAGE_SIZE;
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
  CpuFlushTlb ();

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
  This function clears memory shared bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptTdxSetPageSharedBit (
  IN PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN             NumPages
  )
{
  return SetMemorySharedOrPrivate (
           Cr3BaseAddress,
           BaseAddress,
           EFI_PAGES_TO_SIZE (NumPages),
           SetSharedBit
           );
}

/**
  This function sets memory shared bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were set for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptTdxClearPageSharedBit (
  IN PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN             NumPages
  )
{
  return SetMemorySharedOrPrivate (
           Cr3BaseAddress,
           BaseAddress,
           EFI_PAGES_TO_SIZE (NumPages),
           ClearSharedBit
           );
}
