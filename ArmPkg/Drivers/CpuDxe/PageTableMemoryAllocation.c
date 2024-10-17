/** @file PageTableAlloc.c

Logic facilitating the pre-allocation of page table memory.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ArmPageTableMemoryAllocation.h>

#pragma pack(1)

typedef struct {
  UINT32        Signature;
  UINTN         Offset;
  UINTN         FreePages;
  LIST_ENTRY    NextPool;
} PAGE_TABLE_POOL;

#pragma pack()

#define MIN_PAGES_AVAILABLE        5
#define PAGE_TABLE_POOL_SIGNATURE  SIGNATURE_32 ('P','T','P','L')

UINTN       TotalFreePages     = 0;
BOOLEAN     mPageTablePoolLock = FALSE;
LIST_ENTRY  mPageTablePoolList = INITIALIZE_LIST_HEAD_VARIABLE (mPageTablePoolList);

/**
  Allocates pages for page table memory and adds them to the pool list.

  @param[in] PoolPages The number of pages to allocate. The actual number of pages
                       reserved will be at least 512 - the number of pages required to describe a
                       fully split 2MB region.

  @retval EFI_SUCCESS             More pages successfully added to the pool list
  @retval EFI_INVALID_PARAMETER   PoolPages is zero
  @retval EFI_ABORTED             Software lock is held by another call to this function
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate pages
**/
EFI_STATUS
GetMorePages (
  IN  UINTN  PoolPages
  )
{
  PAGE_TABLE_POOL  *Buffer;

  if (PoolPages == 0) {
    return EFI_INVALID_PARAMETER;
  }

  PoolPages++;
  PoolPages = ALIGN_VALUE (PoolPages, EFI_SIZE_TO_PAGES (SIZE_2MB)); // Add one page for the header
  Buffer    = (PAGE_TABLE_POOL *)AllocateAlignedPages (PoolPages, SIZE_2MB);
  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Out of aligned pages\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // Reserve one page for pool header.
  Buffer->FreePages = PoolPages - 1;
  Buffer->Offset    = EFI_PAGE_SIZE;
  Buffer->Signature = PAGE_TABLE_POOL_SIGNATURE;
  TotalFreePages   += Buffer->FreePages;
  InsertHeadList (&mPageTablePoolList, &Buffer->NextPool);

  return EFI_SUCCESS;
}

/**
  Walks the list to find a pool with enough free pages to allocate from.

  @param[in] Pages The number of pages to allocate

  @retval A pointer to the pool with enough free pages to allocate
          from or NULL if no pool has enough free pages.
**/
PAGE_TABLE_POOL *
FindPoolToAllocateFrom (
  IN UINTN  Pages
  )
{
  PAGE_TABLE_POOL  *PoolToAllocateFrom = NULL;
  LIST_ENTRY       *CurrentEntry       = mPageTablePoolList.ForwardLink;

  while (CurrentEntry != &mPageTablePoolList) {
    PoolToAllocateFrom = CR (CurrentEntry, PAGE_TABLE_POOL, NextPool, PAGE_TABLE_POOL_SIGNATURE);
    if (Pages <= PoolToAllocateFrom->FreePages) {
      break;
    }

    CurrentEntry = CurrentEntry->ForwardLink;
  }

  return PoolToAllocateFrom;
}

/**
  Allocate memory for the page table.

  @param[in]  NumPages          Number of pages to allocate.

  @retval A pointer to the allocated page table memory or NULL if the
          allocation failed.
**/
VOID *
AllocatePageTableMemory (
  IN UINTN  Pages
  )
{
  VOID             *Buffer;
  PAGE_TABLE_POOL  *PoolToAllocateFrom = NULL;
  EFI_STATUS       Status;

  if (Pages == 0) {
    return NULL;
  }

  PoolToAllocateFrom = FindPoolToAllocateFrom (Pages);

  // If we are running low on free pages, allocate more while we
  // still have enough pages to accommodate a potential split. Hold
  // a lock during this because the AllocatePages() call in GetMorePages()
  // may call this function recursively. In that recursive call, it should
  // pull from the existing pool rather than allocating more pages.
  if (((PoolToAllocateFrom == NULL) ||
       (TotalFreePages < Pages) ||
       ((TotalFreePages - Pages) <= MIN_PAGES_AVAILABLE)) &&
      !mPageTablePoolLock)
  {
    DEBUG ((DEBUG_INFO, "%a - The reserve of translation table memory is being refilled!\n", __func__));
    mPageTablePoolLock = TRUE;
    Status             = GetMorePages (1);
    mPageTablePoolLock = FALSE;
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    PoolToAllocateFrom = FindPoolToAllocateFrom (Pages);
  }

  if (PoolToAllocateFrom == NULL) {
    return NULL;
  }

  Buffer                         = (UINT8 *)PoolToAllocateFrom + PoolToAllocateFrom->Offset;
  PoolToAllocateFrom->Offset    += EFI_PAGES_TO_SIZE (Pages);
  PoolToAllocateFrom->FreePages -= Pages;
  TotalFreePages                -= Pages;

  return Buffer;
}

PAGE_TABLE_MEM_ALLOC_PROTOCOL  mPageTableMemAllocProtocol = {
  AllocatePageTableMemory
};

/**
  Initialize the page table memory pool and produce the page table memory allocation
  protocol.

  @param[in] ImageHandle  Handle on which to install the protocol.

  @retval EFI_SUCCESS           The page table pool was initialized and protocol produced.
  @retval Others                The driver returned an error while initializing.
**/
EFI_STATUS
EFIAPI
InitializePageTableMemory (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS       Status;
  PAGE_TABLE_POOL  *Pages;

  Status = GetMorePages (1);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Failed to allocate initial page table pool\n"));
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gArmPageTableMemoryAllocationProtocolGuid,
                  &mPageTableMemAllocProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Failed to install ARM page table memory allocation protocol!\n"));
    Pages = CR (mPageTablePoolList.ForwardLink, PAGE_TABLE_POOL, NextPool, PAGE_TABLE_POOL_SIGNATURE);
    FreePages (Pages, EFI_SIZE_TO_PAGES (Pages->FreePages) + 1);
  }

  return Status;
}
