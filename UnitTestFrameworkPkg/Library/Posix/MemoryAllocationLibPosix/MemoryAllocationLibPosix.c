/** @file
  Instance of Memory Allocation Library based on POSIX APIs

  Uses POSIX APIs malloc() and free() to allocate and free memory.

  Copyright (c) 2018 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdlib.h>
#include <string.h>

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

///
/// Signature for PAGE_HEAD structure
/// Used to verify that buffer being freed was allocated by this library.
///
#define PAGE_HEAD_PRIVATE_SIGNATURE  SIGNATURE_32 ('P', 'H', 'D', 'R')

///
/// Structure placed immediately before an aligned allocation to store the
/// information required to free the entire buffer allocated to support then
/// aligned allocation.
///
typedef struct {
  UINT32  Signature;
  VOID    *AllocatedBufffer;
  UINTN   TotalPages;
  VOID    *AlignedBuffer;
  UINTN   AlignedPages;
} PAGE_HEAD;

/**
  Allocates one or more 4KB pages of type EfiBootServicesData.

  Allocates the number of 4KB pages of type EfiBootServicesData and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages  The number of 4 KB pages to allocate.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  return AllocateAlignedPages (Pages, SIZE_4KB);
}

/**
  Allocates one or more 4KB pages of type EfiRuntimeServicesData.

  Allocates the number of 4KB pages of type EfiRuntimeServicesData and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages  The number of 4 KB pages to allocate.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateRuntimePages (
  IN UINTN  Pages
  )
{
  return AllocatePages (Pages);
}

/**
  Allocates one or more 4KB pages of type EfiReservedMemoryType.

  Allocates the number of 4KB pages of type EfiReservedMemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages  The number of 4 KB pages to allocate.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateReservedPages (
  IN UINTN  Pages
  )
{
  return AllocatePages (Pages);
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

  @param  Buffer  The pointer to the buffer of pages to free.
  @param  Pages   The number of 4 KB pages to free.

**/
VOID
EFIAPI
FreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  FreeAlignedPages (Buffer, Pages);
}

/**
  Allocates one or more 4KB pages of type EfiBootServicesData at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiBootServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

  @param  Pages      The number of 4 KB pages to allocate.
  @param  Alignment  The requested alignment of the allocation.  Must be a power of two.
                     If Alignment is zero, then byte alignment is used.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/VOID *
EFIAPI
AllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  PAGE_HEAD  PageHead;
  PAGE_HEAD  *PageHeadPtr;
  UINTN      AlignmentMask;

  ASSERT ((Alignment & (Alignment - 1)) == 0);

  if (Alignment < SIZE_4KB) {
    Alignment = SIZE_4KB;
  }
  AlignmentMask  = Alignment - 1;

  //
  // We need reserve Alignment pages for PAGE_HEAD, as meta data.
  //
  PageHead.Signature = PAGE_HEAD_PRIVATE_SIGNATURE;
  PageHead.TotalPages = Pages + EFI_SIZE_TO_PAGES (Alignment) * 2;
  PageHead.AlignedPages = Pages;
  PageHead.AllocatedBufffer = malloc (EFI_PAGES_TO_SIZE (PageHead.TotalPages));
  if (PageHead.AllocatedBufffer == NULL) {
    return NULL;
  }
  PageHead.AlignedBuffer = (VOID *)(((UINTN) PageHead.AllocatedBufffer + AlignmentMask) & ~AlignmentMask);
  if ((UINTN)PageHead.AlignedBuffer - (UINTN)PageHead.AllocatedBufffer < sizeof(PAGE_HEAD)) {
    PageHead.AlignedBuffer = (VOID *)((UINTN)PageHead.AlignedBuffer + Alignment);
  }

  PageHeadPtr = (VOID *)((UINTN)PageHead.AlignedBuffer - sizeof(PAGE_HEAD));
  memcpy (PageHeadPtr, &PageHead, sizeof(PAGE_HEAD));

  return PageHead.AlignedBuffer;
}

/**
  Allocates one or more 4KB pages of type EfiRuntimeServicesData at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiRuntimeServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

  @param  Pages      The number of 4 KB pages to allocate.
  @param  Alignment  The requested alignment of the allocation.  Must be a power of two.
                     If Alignment is zero, then byte alignment is used.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedRuntimePages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return AllocateAlignedPages (Pages, Alignment);
}

/**
  Allocates one or more 4KB pages of type EfiReservedMemoryType at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiReservedMemoryType with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

  @param  Pages      The number of 4 KB pages to allocate.
  @param  Alignment  The requested alignment of the allocation.  Must be a power of two.
                     If Alignment is zero, then byte alignment is used.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return AllocateAlignedPages (Pages, Alignment);
}

/**
  Frees one or more 4KB pages that were previously allocated with one of the aligned page
  allocation functions in the Memory Allocation Library.

  Frees the number of 4KB pages specified by Pages from the buffer specified by Buffer.  Buffer
  must have been allocated on a previous call to the aligned page allocation services of the Memory
  Allocation Library.  If it is not possible to free allocated pages, then this function will
  perform no actions.

  If Buffer was not allocated with an aligned page allocation function in the Memory Allocation
  Library, then ASSERT().
  If Pages is zero, then ASSERT().

  @param  Buffer  The pointer to the buffer of pages to free.
  @param  Pages   The number of 4 KB pages to free.

**/
VOID
EFIAPI
FreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  PAGE_HEAD  *PageHeadPtr;

  //
  // NOTE: Partial free is not supported. Just keep it.
  //
  PageHeadPtr = (VOID *)((UINTN)Buffer - sizeof(PAGE_HEAD));
  if (PageHeadPtr->Signature != PAGE_HEAD_PRIVATE_SIGNATURE) {
    return;
  }
  if (PageHeadPtr->AlignedPages != Pages) {
    return;
  }

  PageHeadPtr->Signature = 0;
  free (PageHeadPtr->AllocatedBufffer);
}

/**
  Allocates a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData and returns a
  pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  AllocationSize  The number of bytes to allocate.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/VOID *
EFIAPI
AllocatePool (
  IN UINTN  AllocationSize
  )
{
  return malloc (AllocationSize);
}

/**
  Allocates a buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData and returns
  a pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  AllocationSize  The number of bytes to allocate.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateRuntimePool (
  IN UINTN  AllocationSize
  )
{
  return AllocatePool (AllocationSize);
}

/**
  Allocates a buffer of type EfiReservedMemoryType.

  Allocates the number bytes specified by AllocationSize of type EfiReservedMemoryType and returns
  a pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  AllocationSize  The number of bytes to allocate.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateReservedPool (
  IN UINTN  AllocationSize
  )
{
  return AllocatePool (AllocationSize);
}

/**
  Allocates and zeros a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize  The number of bytes to allocate and zero.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
  VOID  *Buffer;

  Buffer = malloc (AllocationSize);
  if (Buffer == NULL) {
    return NULL;
  }
  memset (Buffer, 0, AllocationSize);
  return Buffer;
}

/**
  Allocates and zeros a buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize  The number of bytes to allocate and zero.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateRuntimeZeroPool (
  IN UINTN  AllocationSize
  )
{
  return AllocateZeroPool (AllocationSize);
}

/**
  Allocates and zeros a buffer of type EfiReservedMemoryType.

  Allocates the number bytes specified by AllocationSize of type EfiReservedMemoryType, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize  The number of bytes to allocate and zero.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateReservedZeroPool (
  IN UINTN  AllocationSize
  )
{
  return AllocateZeroPool (AllocationSize);
}

/**
  Copies a buffer to an allocated buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.

  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  AllocationSize  The number of bytes to allocate and zero.
  @param  Buffer          The buffer to copy to the allocated buffer.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  VOID  *Memory;

  Memory = malloc (AllocationSize);
  if (Memory == NULL) {
    return NULL;
  }
  memcpy (Memory, Buffer, AllocationSize);
  return Memory;
}

/**
  Copies a buffer to an allocated buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.

  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  AllocationSize  The number of bytes to allocate and zero.
  @param  Buffer          The buffer to copy to the allocated buffer.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return AllocateCopyPool (AllocationSize, Buffer);
}

/**
  Copies a buffer to an allocated buffer of type EfiReservedMemoryType.

  Allocates the number bytes specified by AllocationSize of type EfiReservedMemoryType, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.

  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  AllocationSize  The number of bytes to allocate and zero.
  @param  Buffer          The buffer to copy to the allocated buffer.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return AllocateCopyPool (AllocationSize, Buffer);
}

/**
  Reallocates a buffer of type EfiBootServicesData.

  Allocates and zeros the number bytes specified by NewSize from memory of type
  EfiBootServicesData.  If OldBuffer is not NULL, then the smaller of OldSize and
  NewSize bytes are copied from OldBuffer to the newly allocated buffer, and
  OldBuffer is freed.  A pointer to the newly allocated buffer is returned.
  If NewSize is 0, then a valid buffer of 0 size is  returned.  If there is not
  enough memory remaining to satisfy the request, then NULL is returned.

  If the allocation of the new buffer is successful and the smaller of NewSize and OldSize
  is greater than (MAX_ADDRESS - OldBuffer + 1), then ASSERT().

  @param  OldSize    The size, in bytes, of OldBuffer.
  @param  NewSize    The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer  The buffer to copy to the allocated buffer.  This is an optional
                     parameter that may be NULL.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
ReallocatePool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer   OPTIONAL
  )
{
  VOID  *NewBuffer;

  NewBuffer = malloc (NewSize);
  if (NewBuffer != NULL && OldBuffer != NULL) {
    memcpy (NewBuffer, OldBuffer, MIN (OldSize, NewSize));
  }
  if (OldBuffer != NULL) {
    FreePool(OldBuffer);
  }
  return NewBuffer;
}

/**
  Reallocates a buffer of type EfiRuntimeServicesData.

  Allocates and zeros the number bytes specified by NewSize from memory of type
  EfiRuntimeServicesData.  If OldBuffer is not NULL, then the smaller of OldSize and
  NewSize bytes are copied from OldBuffer to the newly allocated buffer, and
  OldBuffer is freed.  A pointer to the newly allocated buffer is returned.
  If NewSize is 0, then a valid buffer of 0 size is  returned.  If there is not
  enough memory remaining to satisfy the request, then NULL is returned.

  If the allocation of the new buffer is successful and the smaller of NewSize and OldSize
  is greater than (MAX_ADDRESS - OldBuffer + 1), then ASSERT().

  @param  OldSize    The size, in bytes, of OldBuffer.
  @param  NewSize    The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer  The buffer to copy to the allocated buffer.  This is an optional
                     parameter that may be NULL.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
ReallocateRuntimePool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer   OPTIONAL
  )
{
  return ReallocatePool (OldSize, NewSize, OldBuffer);
}

/**
  Reallocates a buffer of type EfiReservedMemoryType.

  Allocates and zeros the number bytes specified by NewSize from memory of type
  EfiReservedMemoryType.  If OldBuffer is not NULL, then the smaller of OldSize and
  NewSize bytes are copied from OldBuffer to the newly allocated buffer, and
  OldBuffer is freed.  A pointer to the newly allocated buffer is returned.
  If NewSize is 0, then a valid buffer of 0 size is  returned.  If there is not
  enough memory remaining to satisfy the request, then NULL is returned.

  If the allocation of the new buffer is successful and the smaller of NewSize and OldSize
  is greater than (MAX_ADDRESS - OldBuffer + 1), then ASSERT().

  @param  OldSize    The size, in bytes, of OldBuffer.
  @param  NewSize    The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer  The buffer to copy to the allocated buffer.  This is an optional
                     parameter that may be NULL.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
ReallocateReservedPool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer   OPTIONAL
  )
{
  return ReallocatePool (OldSize, NewSize, OldBuffer);
}

/**
  Frees a buffer that was previously allocated with one of the pool allocation functions in the
  Memory Allocation Library.

  Frees the buffer specified by Buffer.  Buffer must have been allocated on a previous call to the
  pool allocation services of the Memory Allocation Library.  If it is not possible to free pool
  resources, then this function will perform no actions.

  If Buffer was not allocated with a pool allocation function in the Memory Allocation Library,
  then ASSERT().

  @param  Buffer  The pointer to the buffer to free.

**/
VOID
EFIAPI
FreePool (
  IN VOID  *Buffer
  )
{
  free (Buffer);
}
