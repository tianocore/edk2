/** @file
  Implementation of the 6 PEI Ffs (FV) APIs in library form.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrePiLib.h>
#include <Library/DebugLib.h>

STATIC
VOID *
EFIAPI
InternalAllocatePages (
  IN UINTN            Pages,
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  NewTop;

  Hob.Raw = GetHobList ();

  NewTop  = Hob.HandoffInformationTable->EfiFreeMemoryTop & ~(EFI_PHYSICAL_ADDRESS)EFI_PAGE_MASK;
  NewTop -= Pages * EFI_PAGE_SIZE;

  //
  // Verify that there is sufficient memory to satisfy the allocation
  //
  if (NewTop < (Hob.HandoffInformationTable->EfiFreeMemoryBottom + sizeof (EFI_HOB_MEMORY_ALLOCATION))) {
    return NULL;
  }

  //
  // Update the PHIT to reflect the memory usage
  //
  Hob.HandoffInformationTable->EfiFreeMemoryTop = NewTop;

  //
  // Create a memory allocation HOB.
  //
  BuildMemoryAllocationHob (
    Hob.HandoffInformationTable->EfiFreeMemoryTop,
    Pages * EFI_PAGE_SIZE,
    MemoryType
    );

  return (VOID *)(UINTN)Hob.HandoffInformationTable->EfiFreeMemoryTop;
}

/**
  Allocates one or more 4KB pages of type EfiBootServicesData.

  Allocates the number of 4KB pages of MemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  return InternalAllocatePages (Pages, EfiBootServicesData);
}

/**
  Allocates one or more 4KB pages of type EfiRuntimeServicesData.

  Allocates the number of 4KB pages of type EfiRuntimeServicesData and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateRuntimePages (
  IN UINTN  Pages
  )
{
  return InternalAllocatePages (Pages, EfiRuntimeServicesData);
}

/**
  Allocates one or more 4KB pages of type EfiBootServicesData at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiBootServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  Pages                 The number of 4 KB pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

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
  // Make sure that Pages plus EFI_SIZE_TO_PAGES (Alignment) does not overflow.
  //
  ASSERT (Pages <= (MAX_ADDRESS - EFI_SIZE_TO_PAGES (Alignment)));
  //
  // We would rather waste some memory to save PEI code size.
  //
  Memory = (VOID *)(UINTN)AllocatePages (Pages + EFI_SIZE_TO_PAGES (Alignment));
  if (Alignment == 0) {
    AlignmentMask = Alignment;
  } else {
    AlignmentMask = Alignment - 1;
  }

  return (VOID *)(UINTN)(((UINTN)Memory + AlignmentMask) & ~AlignmentMask);
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
  // For now, we do not support the ability to free pages in the PrePei Memory Allocator.
  // The allocated memory is lost.
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

  Hob = GetHobList ();

  //
  // Verify that there is sufficient memory to satisfy the allocation
  //
  if (AllocationSize > 0x10000) {
    // Please call AllocatePages for big allocations
    return 0;
  } else {
    Hob = (EFI_HOB_MEMORY_POOL *)CreateHob (
                                   EFI_HOB_TYPE_MEMORY_POOL,
                                   (UINT16)(sizeof (EFI_HOB_MEMORY_POOL) +
                                            AllocationSize)
                                   );
    return (VOID *)(Hob + 1);
  }
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

/**
  Frees a buffer that was previously allocated with one of the pool allocation functions in the
  Memory Allocation Library.

  Frees the buffer specified by Buffer.  Buffer must have been allocated on a previous call to the
  pool allocation services of the Memory Allocation Library.  If it is not possible to free pool
  resources, then this function will perform no actions.

  If Buffer was not allocated with a pool allocation function in the Memory Allocation Library,
  then ASSERT().

  @param  Buffer                Pointer to the buffer to free.

**/
VOID
EFIAPI
FreePool (
  IN VOID  *Buffer
  )
{
  // Not implemented yet
}
