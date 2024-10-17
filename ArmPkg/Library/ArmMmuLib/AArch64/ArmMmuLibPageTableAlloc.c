/** @file ArmMmuLibPageTableAlloc.c

  Logic facilitating the allocation of page table memory from a reserved pool.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/ArmPageTableMemoryAllocation.h>

PAGE_TABLE_MEM_ALLOC_PROTOCOL  *mPageTableMemAllocProtocol = NULL;

/**
  Allocates pages for the page table from a reserved pool.

  @param[in]  Pages  The number of pages to allocate

  @return A pointer to the allocated buffer or NULL if allocation fails
**/
VOID *
AllocatePageTableMemory (
  IN UINTN  Pages
  )
{
  if (mPageTableMemAllocProtocol == NULL) {
    gBS->LocateProtocol (
           &gArmPageTableMemoryAllocationProtocolGuid,
           NULL,
           (VOID **)&mPageTableMemAllocProtocol
           );
  }

  if (mPageTableMemAllocProtocol != NULL) {
    return mPageTableMemAllocProtocol->AllocatePageTableMemory (Pages);
  }

  return AllocatePages (Pages);
}
