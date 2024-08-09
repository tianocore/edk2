/** @file


  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

/**
  Allocates one or more pages of type EfiBootServicesData.

  Allocates the number of pages of MemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.
  If Pages is 0, then NULL is returned.
  If there is not enough memory availble to satisfy the request, then NULL
  is returned.

  @param   Pages                 The number of 4 KB pages to allocate.
  @param   MemoryType            The MemoryType
  @return  A pointer to the allocated buffer or NULL if allocation fails.
**/
VOID *
EFIAPI
PayloadAllocatePages (
  IN UINTN            Pages,
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PHYSICAL_ADDRESS        Offset;
  EFI_HOB_HANDOFF_INFO_TABLE  *HobTable;

  Hob.Raw  = GetHobList ();
  HobTable = Hob.HandoffInformationTable;

  if (Pages == 0) {
    return NULL;
  }

  // Make sure allocation address is page alligned.
  Offset = HobTable->EfiFreeMemoryTop & EFI_PAGE_MASK;
  if (Offset != 0) {
    HobTable->EfiFreeMemoryTop -= Offset;
  }

  //
  // Check available memory for the allocation
  //
  if (HobTable->EfiFreeMemoryTop - ((Pages * EFI_PAGE_SIZE) + sizeof (EFI_HOB_MEMORY_ALLOCATION)) < HobTable->EfiFreeMemoryBottom) {
    return NULL;
  }

  HobTable->EfiFreeMemoryTop -= Pages * EFI_PAGE_SIZE;
  BuildMemoryAllocationHob (HobTable->EfiFreeMemoryTop, Pages * EFI_PAGE_SIZE, MemoryType);

  return (VOID *)(UINTN)HobTable->EfiFreeMemoryTop;
}

/**
  Allocates one or more pages of type EfiBootServicesData.

  Allocates the number of pages of MemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.
  If Pages is 0, then NULL is returned.
  If there is not enough memory availble to satisfy the request, then NULL
  is returned.

  @param   Pages                 The number of 4 KB pages to allocate.
  @return  A pointer to the allocated buffer or NULL if allocation fails.
**/
VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PHYSICAL_ADDRESS        Offset;
  EFI_HOB_HANDOFF_INFO_TABLE  *HobTable;

  Hob.Raw  = GetHobList ();
  HobTable = Hob.HandoffInformationTable;

  if (Pages == 0) {
    return NULL;
  }

  // Make sure allocation address is page aligned.
  Offset = HobTable->EfiFreeMemoryTop & EFI_PAGE_MASK;
  if (Offset != 0) {
    HobTable->EfiFreeMemoryTop -= Offset;
  }

  //
  // Check available memory for the allocation
  //
  if (HobTable->EfiFreeMemoryTop - ((Pages * EFI_PAGE_SIZE) + sizeof (EFI_HOB_MEMORY_ALLOCATION)) < HobTable->EfiFreeMemoryBottom) {
    return NULL;
  }

  HobTable->EfiFreeMemoryTop -= Pages * EFI_PAGE_SIZE;
  BuildMemoryAllocationHob (HobTable->EfiFreeMemoryTop, Pages * EFI_PAGE_SIZE, EfiBootServicesData);

  return (VOID *)(UINTN)HobTable->EfiFreeMemoryTop;
}

/**
  Frees one or more 4KB pages that were previously allocated with one of the page allocation
  functions in the Memory Allocation Library.

  Frees the number of 4KB pages specified by Pages from the buffer specified by Buffer.  Buffer
  must have been allocated on a previous call to the page allocation services of the Memory
  Allocation Library.  If it is not possible to free allocated pages, then this function will
  perform no actions.

  If Buffer was not allocated with a page allocation function in the Memory Allocation Library,
  then ASSERT().
  If Pages is zero, then ASSERT().

  @param  Buffer                Pointer to the buffer of pages to free.
  @param  Pages                 The number of 4 KB pages to free.

**/
VOID
EFIAPI
FreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
}

/**
  Allocates one or more pages of type EfiBootServicesData at a specified alignment.

  Allocates the number of pages specified by Pages of type EfiBootServicesData with an
  alignment specified by Alignment.
  If Pages is 0, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If there is no enough memory at the specified alignment available to satisfy the
  request, then NULL is returned.

  @param  Pages          The number of 4 KB pages to allocate.
  @param  Alignment      The requested alignment of the allocation.

  @return A pointer to the allocated buffer or NULL if allocation fails.
**/
VOID *
EFIAPI
AllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  VOID   *Memory;
  UINTN  AlignmentMask;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  if (Pages == 0) {
    return NULL;
  }

  //
  // Check overflow.
  //
  ASSERT (Pages <= (MAX_ADDRESS - EFI_SIZE_TO_PAGES (Alignment)));

  Memory = (VOID *)(UINTN)AllocatePages (Pages + EFI_SIZE_TO_PAGES (Alignment));
  if (Memory == NULL) {
    return NULL;
  }

  if (Alignment == 0) {
    AlignmentMask = Alignment;
  } else {
    AlignmentMask = Alignment - 1;
  }

  return (VOID *)(UINTN)(((UINTN)Memory + AlignmentMask) & ~AlignmentMask);
}

/**
  Allocates a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData and returns a
  pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocatePool (
  IN UINTN  AllocationSize
  )
{
  EFI_HOB_MEMORY_POOL  *Hob;

  if (AllocationSize > 0x4000) {
    // Please use AllocatePages for big allocations
    return NULL;
  }

  Hob = (EFI_HOB_MEMORY_POOL *)CreateHob (EFI_HOB_TYPE_MEMORY_POOL, (UINT16)(sizeof (EFI_HOB_MEMORY_POOL) + AllocationSize));
  return (VOID *)(Hob + 1);
}

/**
  Allocates and zeros a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
  VOID  *Buffer;

  Buffer = AllocatePool (AllocationSize);
  if (Buffer == NULL) {
    return NULL;
  }

  ZeroMem (Buffer, AllocationSize);

  return Buffer;
}
