/** @file
  Support routines for memory allocation routines based on SMM Core internal functions,
  with memory profile support.

  The PI System Management Mode Core Interface Specification only allows the use
  of EfiRuntimeServicesCode and EfiRuntimeServicesData memory types for memory
  allocations as the SMRAM space should be reserved after BDS phase.  The functions
  in the Memory Allocation Library use EfiBootServicesData as the default memory
  allocation type.  For this SMM specific instance of the Memory Allocation Library,
  EfiRuntimeServicesData is used as the default memory type for all allocations.
  In addition, allocation for the Reserved memory types are not supported and will
  always return NULL.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "PiSmmCoreMemoryAllocationServices.h"

#include <Library/MemoryProfileLib.h>

EFI_SMRAM_DESCRIPTOR  *mSmmCoreMemoryAllocLibSmramRanges    = NULL;
UINTN                 mSmmCoreMemoryAllocLibSmramRangeCount = 0;

/**
  Check whether the start address of buffer is within any of the SMRAM ranges.

  @param[in]  Buffer   The pointer to the buffer to be checked.

  @retval     TRUE     The buffer is in SMRAM ranges.
  @retval     FALSE    The buffer is out of SMRAM ranges.
**/
BOOLEAN
EFIAPI
BufferInSmram (
  IN VOID  *Buffer
  )
{
  UINTN  Index;

  for (Index = 0; Index < mSmmCoreMemoryAllocLibSmramRangeCount; Index++) {
    if (((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer >= mSmmCoreMemoryAllocLibSmramRanges[Index].CpuStart) &&
        ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer < (mSmmCoreMemoryAllocLibSmramRanges[Index].CpuStart + mSmmCoreMemoryAllocLibSmramRanges[Index].PhysicalSize)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Allocates one or more 4KB pages of a certain memory type.

  Allocates the number of 4KB pages of a certain memory type and returns a pointer to the allocated
  buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  MemoryType            The type of memory to allocate.
  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalAllocatePages (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;

  if (Pages == 0) {
    return NULL;
  }

  Status = SmmAllocatePages (AllocateAnyPages, MemoryType, Pages, &Memory);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return (VOID *)(UINTN)Memory;
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
AllocatePages (
  IN UINTN  Pages
  )
{
  VOID  *Buffer;

  Buffer = InternalAllocatePages (EfiRuntimeServicesData, Pages);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_PAGES,
      EfiRuntimeServicesData,
      Buffer,
      EFI_PAGES_TO_SIZE (Pages),
      NULL
      );
  }

  return Buffer;
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
  VOID  *Buffer;

  Buffer = InternalAllocatePages (EfiRuntimeServicesData, Pages);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_PAGES,
      EfiRuntimeServicesData,
      Buffer,
      EFI_PAGES_TO_SIZE (Pages),
      NULL
      );
  }

  return Buffer;
}

/**
  Allocates one or more 4KB pages of type EfiReservedMemoryType.

  Allocates the number of 4KB pages of type EfiReservedMemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateReservedPages (
  IN UINTN  Pages
  )
{
  return NULL;
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
  EFI_STATUS  Status;

  ASSERT (Pages != 0);
  if (BufferInSmram (Buffer)) {
    //
    // When Buffer is in SMRAM range, it should be allocated by SmmAllocatePages() service.
    // So, SmmFreePages() service is used to free it.
    //
    Status = SmmFreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, Pages);
  } else {
    //
    // When Buffer is out of SMRAM range, it should be allocated by gBS->AllocatePages() service.
    // So, gBS->FreePages() service is used to free it.
    //
    Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, Pages);
  }

  ASSERT_EFI_ERROR (Status);
}

/**
  Allocates one or more 4KB pages of a certain memory type at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of a certain memory type with an alignment
  specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is returned.
  If there is not enough memory at the specified alignment remaining to satisfy the request, then
  NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

  @param  MemoryType            The type of memory to allocate.
  @param  Pages                 The number of 4 KB pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalAllocateAlignedPages (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages,
  IN UINTN            Alignment
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;
  UINTN                 AlignedMemory;
  UINTN                 AlignmentMask;
  UINTN                 UnalignedPages;
  UINTN                 RealPages;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  if (Pages == 0) {
    return NULL;
  }

  if (Alignment > EFI_PAGE_SIZE) {
    //
    // Calculate the total number of pages since alignment is larger than page size.
    //
    AlignmentMask = Alignment - 1;
    RealPages     = Pages + EFI_SIZE_TO_PAGES (Alignment);
    //
    // Make sure that Pages plus EFI_SIZE_TO_PAGES (Alignment) does not overflow.
    //
    ASSERT (RealPages > Pages);

    Status = SmmAllocatePages (AllocateAnyPages, MemoryType, RealPages, &Memory);
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    AlignedMemory  = ((UINTN)Memory + AlignmentMask) & ~AlignmentMask;
    UnalignedPages = EFI_SIZE_TO_PAGES (AlignedMemory - (UINTN)Memory);
    if (UnalignedPages > 0) {
      //
      // Free first unaligned page(s).
      //
      Status = SmmFreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }

    Memory         = AlignedMemory + EFI_PAGES_TO_SIZE (Pages);
    UnalignedPages = RealPages - Pages - UnalignedPages;
    if (UnalignedPages > 0) {
      //
      // Free last unaligned page(s).
      //
      Status = SmmFreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    //
    // Do not over-allocate pages in this case.
    //
    Status = SmmAllocatePages (AllocateAnyPages, MemoryType, Pages, &Memory);
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    AlignedMemory = (UINTN)Memory;
  }

  return (VOID *)AlignedMemory;
}

/**
  Allocates one or more 4KB pages of type EfiRuntimeServicesData at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiRuntimeServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

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
  VOID  *Buffer;

  Buffer = InternalAllocateAlignedPages (EfiRuntimeServicesData, Pages, Alignment);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_PAGES,
      EfiRuntimeServicesData,
      Buffer,
      EFI_PAGES_TO_SIZE (Pages),
      NULL
      );
  }

  return Buffer;
}

/**
  Allocates one or more 4KB pages of type EfiRuntimeServicesData at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiRuntimeServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

  @param  Pages                 The number of 4 KB pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedRuntimePages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  VOID  *Buffer;

  Buffer = InternalAllocateAlignedPages (EfiRuntimeServicesData, Pages, Alignment);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_RUNTIME_PAGES,
      EfiRuntimeServicesData,
      Buffer,
      EFI_PAGES_TO_SIZE (Pages),
      NULL
      );
  }

  return Buffer;
}

/**
  Allocates one or more 4KB pages of type EfiReservedMemoryType at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiReservedMemoryType with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

  @param  Pages                 The number of 4 KB pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return NULL;
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

  @param  Buffer                Pointer to the buffer of pages to free.
  @param  Pages                 The number of 4 KB pages to free.

**/
VOID
EFIAPI
FreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  EFI_STATUS  Status;

  ASSERT (Pages != 0);
  if (BufferInSmram (Buffer)) {
    //
    // When Buffer is in SMRAM range, it should be allocated by SmmAllocatePages() service.
    // So, SmmFreePages() service is used to free it.
    //
    Status = SmmFreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, Pages);
  } else {
    //
    // When Buffer is out of SMRAM range, it should be allocated by gBS->AllocatePages() service.
    // So, gBS->FreePages() service is used to free it.
    //
    Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, Pages);
  }

  ASSERT_EFI_ERROR (Status);
}

/**
  Allocates a buffer of a certain pool type.

  Allocates the number bytes specified by AllocationSize of a certain pool type and returns a
  pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  MemoryType            The type of memory to allocate.
  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalAllocatePool (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            AllocationSize
  )
{
  EFI_STATUS  Status;
  VOID        *Memory;

  Memory = NULL;

  Status = SmmAllocatePool (MemoryType, AllocationSize, &Memory);
  if (EFI_ERROR (Status)) {
    Memory = NULL;
  }

  return Memory;
}

/**
  Allocates a buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData and returns a
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
  VOID  *Buffer;

  Buffer = InternalAllocatePool (EfiRuntimeServicesData, AllocationSize);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_POOL,
      EfiRuntimeServicesData,
      Buffer,
      AllocationSize,
      NULL
      );
  }

  return Buffer;
}

/**
  Allocates a buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData and returns
  a pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateRuntimePool (
  IN UINTN  AllocationSize
  )
{
  VOID  *Buffer;

  Buffer = InternalAllocatePool (EfiRuntimeServicesData, AllocationSize);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_POOL,
      EfiRuntimeServicesData,
      Buffer,
      AllocationSize,
      NULL
      );
  }

  return Buffer;
}

/**
  Allocates a buffer of type EfiReservedMemoryType.

  Allocates the number bytes specified by AllocationSize of type EfiReservedMemoryType and returns
  a pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateReservedPool (
  IN UINTN  AllocationSize
  )
{
  return NULL;
}

/**
  Allocates and zeros a buffer of a certain pool type.

  Allocates the number bytes specified by AllocationSize of a certain pool type, clears the buffer
  with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a valid
  buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the request,
  then NULL is returned.

  @param  PoolType              The type of memory to allocate.
  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalAllocateZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize
  )
{
  VOID  *Memory;

  Memory = InternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }

  return Memory;
}

/**
  Allocates and zeros a buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData, clears the
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

  Buffer = InternalAllocateZeroPool (EfiRuntimeServicesData, AllocationSize);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ZERO_POOL,
      EfiRuntimeServicesData,
      Buffer,
      AllocationSize,
      NULL
      );
  }

  return Buffer;
}

/**
  Allocates and zeros a buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateRuntimeZeroPool (
  IN UINTN  AllocationSize
  )
{
  VOID  *Buffer;

  Buffer = InternalAllocateZeroPool (EfiRuntimeServicesData, AllocationSize);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_ZERO_POOL,
      EfiRuntimeServicesData,
      Buffer,
      AllocationSize,
      NULL
      );
  }

  return Buffer;
}

/**
  Allocates and zeros a buffer of type EfiReservedMemoryType.

  Allocates the number bytes specified by AllocationSize of type EfiReservedMemoryType, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateReservedZeroPool (
  IN UINTN  AllocationSize
  )
{
  return NULL;
}

/**
  Copies a buffer to an allocated buffer of a certain pool type.

  Allocates the number bytes specified by AllocationSize of a certain pool type, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.
  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  PoolType              The type of pool to allocate.
  @param  AllocationSize        The number of bytes to allocate and zero.
  @param  Buffer                The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalAllocateCopyPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN CONST VOID       *Buffer
  )
{
  VOID  *Memory;

  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN)Buffer + 1));

  Memory = InternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
    Memory = CopyMem (Memory, Buffer, AllocationSize);
  }

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

  @param  AllocationSize        The number of bytes to allocate and zero.
  @param  Buffer                The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  VOID  *NewBuffer;

  NewBuffer = InternalAllocateCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer);
  if (NewBuffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_COPY_POOL,
      EfiRuntimeServicesData,
      NewBuffer,
      AllocationSize,
      NULL
      );
  }

  return NewBuffer;
}

/**
  Copies a buffer to an allocated buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.

  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  AllocationSize        The number of bytes to allocate and zero.
  @param  Buffer                The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  VOID  *NewBuffer;

  NewBuffer = InternalAllocateCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer);
  if (NewBuffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_COPY_POOL,
      EfiRuntimeServicesData,
      NewBuffer,
      AllocationSize,
      NULL
      );
  }

  return NewBuffer;
}

/**
  Copies a buffer to an allocated buffer of type EfiReservedMemoryType.

  Allocates the number bytes specified by AllocationSize of type EfiReservedMemoryType, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.

  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  AllocationSize        The number of bytes to allocate and zero.
  @param  Buffer                The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return NULL;
}

/**
  Reallocates a buffer of a specified memory type.

  Allocates and zeros the number bytes specified by NewSize from memory of the type
  specified by PoolType.  If OldBuffer is not NULL, then the smaller of OldSize and
  NewSize bytes are copied from OldBuffer to the newly allocated buffer, and
  OldBuffer is freed.  A pointer to the newly allocated buffer is returned.
  If NewSize is 0, then a valid buffer of 0 size is  returned.  If there is not
  enough memory remaining to satisfy the request, then NULL is returned.

  If the allocation of the new buffer is successful and the smaller of NewSize and OldSize
  is greater than (MAX_ADDRESS - OldBuffer + 1), then ASSERT().

  @param  PoolType       The type of pool to allocate.
  @param  OldSize        The size, in bytes, of OldBuffer.
  @param  NewSize        The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer      The buffer to copy to the allocated buffer.  This is an optional
                         parameter that may be NULL.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalReallocatePool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            OldSize,
  IN UINTN            NewSize,
  IN VOID             *OldBuffer  OPTIONAL
  )
{
  VOID  *NewBuffer;

  NewBuffer = InternalAllocateZeroPool (PoolType, NewSize);
  if ((NewBuffer != NULL) && (OldBuffer != NULL)) {
    CopyMem (NewBuffer, OldBuffer, MIN (OldSize, NewSize));
    FreePool (OldBuffer);
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
  VOID  *Buffer;

  Buffer = InternalReallocatePool (EfiRuntimeServicesData, OldSize, NewSize, OldBuffer);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_REALLOCATE_POOL,
      EfiRuntimeServicesData,
      Buffer,
      NewSize,
      NULL
      );
  }

  return Buffer;
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

  @param  OldSize        The size, in bytes, of OldBuffer.
  @param  NewSize        The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer      The buffer to copy to the allocated buffer.  This is an optional
                         parameter that may be NULL.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
ReallocateRuntimePool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer  OPTIONAL
  )
{
  VOID  *Buffer;

  Buffer = InternalReallocatePool (EfiRuntimeServicesData, OldSize, NewSize, OldBuffer);
  if (Buffer != NULL) {
    MemoryProfileLibRecord (
      (PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MEMORY_PROFILE_ACTION_LIB_REALLOCATE_RUNTIME_POOL,
      EfiRuntimeServicesData,
      Buffer,
      NewSize,
      NULL
      );
  }

  return Buffer;
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

  @param  OldSize        The size, in bytes, of OldBuffer.
  @param  NewSize        The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer      The buffer to copy to the allocated buffer.  This is an optional
                         parameter that may be NULL.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
ReallocateReservedPool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer  OPTIONAL
  )
{
  return NULL;
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
  EFI_STATUS  Status;

  if (BufferInSmram (Buffer)) {
    //
    // When Buffer is in SMRAM range, it should be allocated by SmmAllocatePool() service.
    // So, SmmFreePool() service is used to free it.
    //
    Status = SmmFreePool (Buffer);
  } else {
    //
    // When Buffer is out of SMRAM range, it should be allocated by gBS->AllocatePool() service.
    // So, gBS->FreePool() service is used to free it.
    //
    Status = gBS->FreePool (Buffer);
  }

  ASSERT_EFI_ERROR (Status);
}

/**
  The constructor function calls SmmInitializeMemoryServices to initialize memory in SMRAM.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PiSmmCoreMemoryAllocationLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS             Status;
  SMM_CORE_PRIVATE_DATA  *SmmCorePrivate;
  UINTN                  Size;
  VOID                   *BootServicesData;

  SmmCorePrivate = (SMM_CORE_PRIVATE_DATA *)ImageHandle;

  //
  // The FreePool()/FreePages() will need use SmramRanges data to know whether
  // the buffer to free is in SMRAM range or not. And there may be FreePool()/
  // FreePages() indrectly during calling SmmInitializeMemoryServices(), but
  // no SMRAM could be allocated before calling SmmInitializeMemoryServices(),
  // so temporarily use BootServicesData to hold the SmramRanges data.
  //
  mSmmCoreMemoryAllocLibSmramRangeCount = SmmCorePrivate->SmramRangeCount;
  Size                                  = mSmmCoreMemoryAllocLibSmramRangeCount * sizeof (EFI_SMRAM_DESCRIPTOR);
  Status                                = gBS->AllocatePool (EfiBootServicesData, Size, (VOID **)&mSmmCoreMemoryAllocLibSmramRanges);
  ASSERT_EFI_ERROR (Status);
  ASSERT (mSmmCoreMemoryAllocLibSmramRanges != NULL);
  CopyMem (mSmmCoreMemoryAllocLibSmramRanges, SmmCorePrivate->SmramRanges, Size);

  //
  // Initialize memory service using free SMRAM
  //
  SmmInitializeMemoryServices (SmmCorePrivate->SmramRangeCount, SmmCorePrivate->SmramRanges);

  //
  // Move the SmramRanges data from BootServicesData to SMRAM.
  //
  BootServicesData                  = mSmmCoreMemoryAllocLibSmramRanges;
  mSmmCoreMemoryAllocLibSmramRanges = (EFI_SMRAM_DESCRIPTOR *)AllocateCopyPool (Size, (VOID *)BootServicesData);
  ASSERT (mSmmCoreMemoryAllocLibSmramRanges != NULL);

  //
  // Free the temporarily used BootServicesData.
  //
  Status = gBS->FreePool (BootServicesData);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
