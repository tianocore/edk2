/** @file

  Virtual Memory Management Services to set or clear the memory encryption bit

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017 - 2024, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Code is derived from MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.c

**/

#include <Library/CpuLib.h>
#include <Library/CpuPageTableLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Cpuid.h>

#include "VirtualMemory.h"
#include "SnpPageStateChange.h"

STATIC BOOLEAN          mAddressEncMaskChecked = FALSE;
STATIC UINT64           mAddressEncMask;
STATIC PAGE_TABLE_POOL  *mPageTablePool = NULL;

STATIC VOID  *mPscBuffer = NULL;

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
  UINT64  EncryptionMask;

  if (mAddressEncMaskChecked) {
    return mAddressEncMask;
  }

  EncryptionMask = MemEncryptSevGetEncryptionMask ();

  mAddressEncMask        = EncryptionMask & PAGING_1G_ADDRESS_MASK_64;
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
  IN  UINTN  PoolPages
  )
{
  UINTN               AllocPages;
  VOID                *Buffer;
  UINTN               PageTable;
  UINT64              MapAddress;
  UINT64              MapLength;
  IA32_MAP_ATTRIBUTE  MapAttribute;
  IA32_MAP_ATTRIBUTE  MapMask;
  UINTN               BufferSize;
  BOOLEAN             Flush;
  RETURN_STATUS       Status;

  //
  // Always reserve at least PAGE_TABLE_POOL_UNIT_PAGES, including one page for
  // header.
  //
  AllocPages = (PoolPages / PAGE_TABLE_POOL_UNIT_PAGES + 1) * PAGE_TABLE_POOL_UNIT_PAGES;

  Buffer = AllocateAlignedPages (AllocPages, PAGE_TABLE_POOL_ALIGNMENT);
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
  mPageTablePool->FreePages = AllocPages - 1;
  mPageTablePool->Offset    = EFI_PAGES_TO_SIZE (1);

  //
  // Perform the change to readonly.
  //
  PageTable = AsmReadCr3 ();

  MapAddress = (UINT64)(UINTN)Buffer;
  MapLength  = EFI_PAGES_TO_SIZE (AllocPages);

  MapAttribute.Uint64    = 0;
  MapMask.Uint64         = 0;
  MapMask.Bits.ReadWrite = 1;

  BufferSize = 0;
  Flush      = FALSE;

  Status = PageTableMap (&PageTable, Paging4Level1GB, NULL, &BufferSize, MapAddress, MapLength, &MapAttribute, &MapMask, &Flush);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    //
    // Ensure there are enough pages in the pool for the new pagetable pages.
    //
    ASSERT (EFI_SIZE_TO_PAGES (BufferSize) <= mPageTablePool->FreePages);
    if (EFI_SIZE_TO_PAGES (BufferSize) <= mPageTablePool->FreePages) {
      Buffer = mPageTablePool + mPageTablePool->Offset;

      mPageTablePool->Offset    += BufferSize;
      mPageTablePool->FreePages -= EFI_SIZE_TO_PAGES (BufferSize);

      Status = PageTableMap (&PageTable, Paging4Level1GB, Buffer, &BufferSize, MapAddress, MapLength, &MapAttribute, &MapMask, &Flush);
    }
  }

  if (Status != RETURN_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a:%a failed to read-only map Address=0x%Lx Length=0x%x\n", gEfiCallerBaseName, __func__, MapAddress, MapLength));
    return FALSE;
  }

  //
  // Ensure there are enough pages remaining in the pool to satisfy the request.
  //
  ASSERT (PoolPages <= mPageTablePool->FreePages);
  if (PoolPages > mPageTablePool->FreePages) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a not enough pages remaining after read-only update: requested=%x, available=%x\n",
      gEfiCallerBaseName,
      __func__,
      PoolPages,
      mPageTablePool->FreePages
      ));
    return FALSE;
  }

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
STATIC
VOID
EnableReadOnlyPageWriteProtect (
  VOID
  )
{
  AsmWriteCr0 (AsmReadCr0 () | BIT16);
}

RETURN_STATUS
EFIAPI
InternalMemEncryptSevCreateIdentityMap1G (
  IN    PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN    PHYSICAL_ADDRESS  PhysicalAddress,
  IN    UINTN             Length
  )
{
  UINTN               PageTable;
  UINT64              AddressEncMask;
  BOOLEAN             IsWpEnabled;
  UINT64              MapAddress;
  UINT64              MapLength;
  IA32_MAP_ATTRIBUTE  MapAttribute;
  IA32_MAP_ATTRIBUTE  MapMask;
  VOID                *Buffer;
  UINTN               BufferSize;
  BOOLEAN             Flush;
  RETURN_STATUS       Status;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: Cr3Base=0x%Lx Physical=0x%Lx Length=0x%Lx\n",
    gEfiCallerBaseName,
    __func__,
    Cr3BaseAddress,
    PhysicalAddress,
    (UINT64)Length
    ));

  if (Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  if (Cr3BaseAddress == 0) {
    PageTable = AsmReadCr3 ();
  } else {
    PageTable = (UINTN)Cr3BaseAddress;
  }

  //
  // Check if we have a valid memory encryption mask
  //
  AddressEncMask = InternalGetMemEncryptionAddressMask ();
  if (!AddressEncMask) {
    return RETURN_ACCESS_DENIED;
  }

  //
  // Make sure that the page table is changeable.
  //
  IsWpEnabled = IsReadOnlyPageWriteProtected ();
  if (IsWpEnabled) {
    DisableReadOnlyPageWriteProtect ();
  }

  MapAddress = PhysicalAddress & ~((UINT64)SIZE_1GB - 1);
  if (MapAddress != PhysicalAddress) {
    DEBUG ((DEBUG_VERBOSE, "%a:%a: Aligning address to 1GB: 0x%Lx => 0x%Lx\n", gEfiCallerBaseName, __func__, PhysicalAddress, MapAddress));
  }

  MapLength  = ALIGN_VALUE (PhysicalAddress + Length, SIZE_1GB);
  MapLength -= MapAddress;
  if (MapLength != Length) {
    DEBUG ((DEBUG_VERBOSE, "%a:%a: Aligning length to 1GB: 0x%Lx => 0x%Lx\n", gEfiCallerBaseName, __func__, Length, MapLength));
  }

  MapAttribute.Uint64         = AddressEncMask | MapAddress;
  MapAttribute.Bits.Present   = 1;
  MapAttribute.Bits.ReadWrite = 1;
  MapMask.Uint64              = MAX_UINT64;

  BufferSize = 0;
  Flush      = FALSE;

  Status = PageTableMap (&PageTable, Paging4Level1GB, NULL, &BufferSize, MapAddress, MapLength, &MapAttribute, &MapMask, &Flush);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    Buffer = AllocatePageTableMemory (EFI_SIZE_TO_PAGES (BufferSize));
    if (Buffer != NULL) {
      Status = PageTableMap (&PageTable, Paging4Level1GB, Buffer, &BufferSize, PhysicalAddress, Length, &MapAttribute, &MapMask, &Flush);
    }
  }

  if (IsWpEnabled) {
    EnableReadOnlyPageWriteProtect ();
  }

  if (Flush) {
    CpuFlushTlb ();
  }

  if (Status != RETURN_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a:%a failed to map Address=0x%Lx Length=0x%x\n", gEfiCallerBaseName, __func__, PhysicalAddress, Length));
  }

  return Status;
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
  @param[in]  Mmio                    The physical address specified is Mmio

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
  IN    PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN    PHYSICAL_ADDRESS  PhysicalAddress,
  IN    UINTN             Length,
  IN    MAP_RANGE_MODE    Mode,
  IN    BOOLEAN           CacheFlush,
  IN    BOOLEAN           Mmio
  )
{
  UINTN               PageTable;
  UINT64              AddressEncMask;
  BOOLEAN             IsWpEnabled;
  UINT64              MapAddress;
  UINT64              MapLength;
  IA32_MAP_ATTRIBUTE  MapAttribute;
  IA32_MAP_ATTRIBUTE  MapMask;
  VOID                *Buffer;
  UINTN               BufferSize;
  BOOLEAN             Flush;
  RETURN_STATUS       Status;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: Cr3Base=0x%Lx Physical=0x%Lx Length=0x%Lx Mode=%a CacheFlush=%u Mmio=%u\n",
    gEfiCallerBaseName,
    __func__,
    Cr3BaseAddress,
    PhysicalAddress,
    (UINT64)Length,
    (Mode == SetCBit) ? "Encrypt" : "Decrypt",
    (UINT32)CacheFlush,
    (UINT32)Mmio
    ));

  //
  // Check if we have a valid memory encryption mask
  //
  AddressEncMask = InternalGetMemEncryptionAddressMask ();
  if (!AddressEncMask) {
    return RETURN_ACCESS_DENIED;
  }

  if (Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  MapAddress = PhysicalAddress & ~((UINT64)SIZE_4KB - 1);
  if (MapAddress != PhysicalAddress) {
    DEBUG ((DEBUG_VERBOSE, "%a:%a: Aligning address to 4KB: 0x%Lx => 0x%Lx\n", gEfiCallerBaseName, __func__, PhysicalAddress, MapAddress));
  }

  MapLength  = ALIGN_VALUE (PhysicalAddress + Length, SIZE_4KB);
  MapLength -= MapAddress;
  if (MapLength != Length) {
    DEBUG ((DEBUG_VERBOSE, "%a:%a: Aligning length to 4KB: 0x%Lx => 0x%Lx\n", gEfiCallerBaseName, __func__, Length, MapLength));
  }

  //
  // We are going to change the memory encryption attribute from C=0 -> C=1 or
  // vice versa Flush the caches to ensure that data is written into memory
  // with correct C-bit
  //
  if (CacheFlush) {
    WriteBackInvalidateDataCacheRange ((VOID *)(UINTN)MapAddress, MapLength);
  }

  //
  // Make sure that the page table is changeable.
  //
  IsWpEnabled = IsReadOnlyPageWriteProtected ();
  if (IsWpEnabled) {
    DisableReadOnlyPageWriteProtect ();
  }

  //
  // To maintain the security gurantees we must set the page to shared in the RMP
  // table before clearing the memory encryption mask from the current page table.
  //
  // The InternalSetPageState() is used for setting the page state in the RMP table.
  //
  if (!Mmio && (Mode == ClearCBit) && MemEncryptSevSnpIsEnabled ()) {
    if (mPscBuffer == NULL) {
      mPscBuffer = AllocateReservedPages (1);
      ASSERT (mPscBuffer != NULL);
    }

    InternalSetPageState (
      MapAddress,
      EFI_SIZE_TO_PAGES (MapLength),
      SevSnpPageShared,
      FALSE,
      mPscBuffer,
      EFI_PAGE_SIZE
      );
  }

  PageTable = (Cr3BaseAddress != 0) ? (UINTN)Cr3BaseAddress : AsmReadCr3 ();

  MapAttribute.Uint64 = MapAddress;
  if (Mode == SetCBit) {
    MapAttribute.Uint64 |= AddressEncMask;
  }

  MapMask.Uint64 = IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK;

  BufferSize = 0;
  Flush      = FALSE;

  Status = PageTableMap (&PageTable, Paging4Level1GB, NULL, &BufferSize, MapAddress, MapLength, &MapAttribute, &MapMask, &Flush);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    Buffer = AllocatePageTableMemory (EFI_SIZE_TO_PAGES (BufferSize));
    if (Buffer != NULL) {
      Status = PageTableMap (&PageTable, Paging4Level1GB, Buffer, &BufferSize, MapAddress, MapLength, &MapAttribute, &MapMask, &Flush);
    }
  }

  if (IsWpEnabled) {
    EnableReadOnlyPageWriteProtect ();
  }

  if (Flush) {
    CpuFlushTlb ();
  }

  if (Status != RETURN_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a:%a failed to map Address=0x%Lx Length=0x%x\n", gEfiCallerBaseName, __func__, PhysicalAddress, Length));
    return Status;
  }

  //
  // SEV-SNP requires that all the private pages (i.e pages mapped encrypted) must be
  // added in the RMP table before the access.
  //
  // The InternalSetPageState() is used for setting the page state in the RMP table.
  //
  if ((Mode == SetCBit) && MemEncryptSevSnpIsEnabled ()) {
    if (mPscBuffer == NULL) {
      mPscBuffer = AllocateReservedPages (1);
      ASSERT (mPscBuffer != NULL);
    }

    InternalSetPageState (
      MapAddress,
      EFI_SIZE_TO_PAGES (MapLength),
      SevSnpPagePrivate,
      FALSE,
      mPscBuffer,
      EFI_PAGE_SIZE
      );
  }

  return RETURN_SUCCESS;
}

/**
  This function clears memory encryption bit for the memory region specified by
  PhysicalAddress and Length from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevSetMemoryDecrypted (
  IN  PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS  PhysicalAddress,
  IN  UINTN             Length
  )
{
  return SetMemoryEncDec (
           Cr3BaseAddress,
           PhysicalAddress,
           Length,
           ClearCBit,
           TRUE,
           FALSE
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

  @retval RETURN_SUCCESS              The attributes were set for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevSetMemoryEncrypted (
  IN  PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS  PhysicalAddress,
  IN  UINTN             Length
  )
{
  return SetMemoryEncDec (
           Cr3BaseAddress,
           PhysicalAddress,
           Length,
           SetCBit,
           TRUE,
           FALSE
           );
}

/**
  This function clears memory encryption bit for the MMIO region specified by
  PhysicalAddress and Length.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a MMIO region.
  @param[in]  Length                  The length of memory region

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Length is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevClearMmioPageEncMask (
  IN  PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS  PhysicalAddress,
  IN  UINTN             Length
  )
{
  return SetMemoryEncDec (
           Cr3BaseAddress,
           PhysicalAddress,
           Length,
           ClearCBit,
           FALSE,
           TRUE
           );
}
