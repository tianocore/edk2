/*++ @file

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Base.h"
#include "Library/BaseMemoryLib.h"
#include "Library/MemoryAllocationLib.h"

#include <stdlib.h>

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
  return (VOID *)malloc (AllocationSize);
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
  Reallocates a buffer of type EfiBootServicesData.

  Allocates and zeros the number bytes specified by NewSize from memory of type
  EfiBootServicesData.  If OldBuffer is not NULL, then the smaller of OldSize and
  NewSize bytes are copied from OldBuffer to the newly allocated buffer, and
  OldBuffer is freed.  A pointer to the newly allocated buffer is returned.
  If NewSize is 0, then a valid buffer of 0 size is  returned.  If there is not
  enough memory remaining to satisfy the request, then NULL is returned.

  If the allocation of the new buffer is successful and the smaller of NewSize and OldSize
  is greater than (MAX_ADDRESS - OldBuffer + 1), then ASSERT().

  @param  OldSize        The size, in bytes, of OldBuffer.
  @param  NewSize        The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer      The buffer to copy to the allocated buffer.  This is an optional
                         parameter that may be NULL.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
ReallocatePool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer  OPTIONAL
  )
{
  VOID  *NewBuffer;

  NewBuffer = AllocatePool (NewSize);
  if (NewBuffer == NULL) {
    return NULL;
  }

  if (OldBuffer != NULL) {
    if (OldSize > 0) {
      CopyMem (NewBuffer, OldBuffer, OldSize);
    }

    FreePool (OldBuffer);
  }

  return NewBuffer;
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
  free ((void *)Buffer);
}
