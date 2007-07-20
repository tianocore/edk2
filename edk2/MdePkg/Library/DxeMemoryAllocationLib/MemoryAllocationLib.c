/** @file
  Support routines for memory allocation routines for use with drivers.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include <PiDxe.h>


#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "MemoryAllocationLibInternals.h"

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

  Status = gBS->AllocatePages (AllocateAnyPages, MemoryType, Pages, &Memory);
  if (EFI_ERROR (Status)) {
    Memory = 0;
  }
  return (VOID *) (UINTN) Memory;
}

/**
  Allocates one or more 4KB pages of type EfiBootServicesData.

  Allocates the number of 4KB pages of type EfiBootServicesData and returns a pointer to the
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
  return InternalAllocatePages (EfiBootServicesData, Pages);
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
  return InternalAllocatePages (EfiRuntimeServicesData, Pages);
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
  return InternalAllocatePages (EfiReservedMemoryType, Pages);
}

/**
  Frees one or more 4KB pages that were previously allocated with one of the page allocation
  functions in the Memory Allocation Library.

  Frees the number of 4KB pages specified by Pages from the buffer specified by Buffer.  Buffer
  must have been allocated on a previous call to the page allocation services of the Memory
  Allocation Library.
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
  Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) Buffer, Pages);
  ASSERT_EFI_ERROR (Status);
}

/**
  Allocates one or more 4KB pages of a certain memory type at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of a certain memory type with an alignment
  specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is returned.
  If there is not enough memory at the specified alignment remaining to satisfy the request, then
  NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

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
    // Caculate the total number of pages since alignment is larger than page size.
    //
    AlignmentMask  = Alignment - 1;
    RealPages      = Pages + EFI_SIZE_TO_PAGES (Alignment);
    //
    // Make sure that Pages plus EFI_SIZE_TO_PAGES (Alignment) does not overflow.
    //
    ASSERT (RealPages > Pages);
 
    Status         = gBS->AllocatePages (AllocateAnyPages, MemoryType, RealPages, &Memory);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
    AlignedMemory  = ((UINTN) Memory + AlignmentMask) & ~AlignmentMask;
    UnalignedPages = EFI_SIZE_TO_PAGES ((UINTN) Memory - AlignedMemory);
    if (UnalignedPages > 0) {
      //
      // Free first unaligned page(s).
      //
      Status = gBS->FreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }
    Memory         = (EFI_PHYSICAL_ADDRESS) (AlignedMemory + EFI_PAGES_TO_SIZE (Pages));
    UnalignedPages = RealPages - Pages - UnalignedPages;
    if (UnalignedPages > 0) {
      //
      // Free last unaligned page(s).
      //
      Status = gBS->FreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    //
    // Do not over-allocate pages in this case.
    //
    Status = gBS->AllocatePages (AllocateAnyPages, MemoryType, Pages, &Memory);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
    AlignedMemory  = (UINTN) Memory;
  }
  return (VOID *) AlignedMemory;
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
  return InternalAllocateAlignedPages (EfiBootServicesData, Pages, Alignment);
}

/**
  Allocates one or more 4KB pages of type EfiRuntimeServicesData at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiRuntimeServicesData with an
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
AllocateAlignedRuntimePages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPages (EfiRuntimeServicesData, Pages, Alignment);
}

/**
  Allocates one or more 4KB pages of type EfiReservedMemoryType at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiReservedMemoryType with an
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
AllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPages (EfiReservedMemoryType, Pages, Alignment);
}

/**
  Frees one or more 4KB pages that were previously allocated with one of the aligned page
  allocation functions in the Memory Allocation Library.

  Frees the number of 4KB pages specified by Pages from the buffer specified by Buffer.  Buffer
  must have been allocated on a previous call to the aligned page allocation services of the Memory
  Allocation Library.
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
  Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) Buffer, Pages);
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

  Status = gBS->AllocatePool (MemoryType, AllocationSize, &Memory);
  if (EFI_ERROR (Status)) {
    Memory = NULL;
  }
  return Memory;
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
  return InternalAllocatePool (EfiBootServicesData, AllocationSize);
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
  return InternalAllocatePool (EfiRuntimeServicesData, AllocationSize);
}

/**
  Allocates a buffer of type EfieservedMemoryType.

  Allocates the number bytes specified by AllocationSize of type EfieservedMemoryType and returns
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
  return InternalAllocatePool (EfiReservedMemoryType, AllocationSize);
}

/**
  Allocates and zeros a buffer of a certian pool type.

  Allocates the number bytes specified by AllocationSize of a certian pool type, clears the buffer
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
  return InternalAllocateZeroPool (EfiBootServicesData, AllocationSize);
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
  return InternalAllocateZeroPool (EfiRuntimeServicesData, AllocationSize);
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
  return InternalAllocateZeroPool (EfiReservedMemoryType, AllocationSize);
}

/**
  Copies a buffer to an allocated buffer of a certian pool type.

  Allocates the number bytes specified by AllocationSize of a certian pool type, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.
  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

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
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = InternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
     Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
} 

/**
  Copies a buffer to an allocated buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.
  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

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
  return InternalAllocateCopyPool (EfiBootServicesData, AllocationSize, Buffer);
}

/**
  Copies a buffer to an allocated buffer of type EfiRuntimeServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.
  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

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
  return InternalAllocateCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer);
}

/**
  Copies a buffer to an allocated buffer of type EfiReservedMemoryType.

  Allocates the number bytes specified by AllocationSize of type EfiReservedMemoryType, copies
  AllocationSize bytes from Buffer to the newly allocated buffer, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.
  If Buffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

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
  return InternalAllocateCopyPool (EfiReservedMemoryType, AllocationSize, Buffer);
}

/**
  Frees a buffer that was previously allocated with one of the pool allocation functions in the
  Memory Allocation Library.

  Frees the buffer specified by Buffer.  Buffer must have been allocated on a previous call to the
  pool allocation services of the Memory Allocation Library.
  If Buffer was not allocated with a pool allocation function in the Memory Allocation Library,
  then ASSERT().

  @param  Buffer                Pointer to the buffer to free.

**/
VOID
EFIAPI
FreePool (
  IN VOID   *Buffer
  )
{
  EFI_STATUS    Status;

  Status = gBS->FreePool (Buffer);
  ASSERT_EFI_ERROR (Status);
}

/**
  Allocates a buffer of a certain pool type at a specified alignment.

  Allocates the number bytes specified by AllocationSize of a certain pool type with an alignment
  specified by Alignment.  The allocated buffer is returned.  If AllocationSize is 0, then a valid
  buffer of 0 size is returned.  If there is not enough memory at the specified alignment remaining
  to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  PoolType              The type of pool to allocate.
  @param  AllocationSize        The number of bytes to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.                            If Alignment is zero, then byte alignment is used.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalAllocateAlignedPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN UINTN            Alignment
  )
{
  VOID        *RawAddress;
  UINTN       AlignedAddress;
  UINTN       AlignmentMask;
  UINTN       OverAllocationSize;
  UINTN       RealAllocationSize;
  VOID        **FreePointer;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);
  
  if (Alignment == 0) {
    AlignmentMask = Alignment;
  } else {
    AlignmentMask = Alignment - 1;
  }
  //
  // Calculate the extra memory size, over-allocate memory pool and get the aligned memory address. 
  //
  OverAllocationSize  = sizeof (RawAddress) + AlignmentMask;
  RealAllocationSize  = AllocationSize + OverAllocationSize;
  //
  // Make sure that AllocationSize plus OverAllocationSize does not overflow. 
  //
  ASSERT (RealAllocationSize > AllocationSize); 

  RawAddress = InternalAllocatePool (PoolType, RealAllocationSize);
  if (RawAddress == NULL) {
    return NULL;
  }
  AlignedAddress      = ((UINTN) RawAddress + OverAllocationSize) & ~AlignmentMask;
  //
  // Save the original memory address just before the aligned address.
  //
  FreePointer         = (VOID **)(AlignedAddress - sizeof (RawAddress));
  *FreePointer        = RawAddress;

  return (VOID *) AlignedAddress;
}

/**
  Allocates a buffer of type EfiBootServicesData at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If AllocationSize is 0,
  then a valid buffer of 0 size is returned.  If there is not enough memory at the specified
  alignment remaining to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPool (EfiBootServicesData, AllocationSize, Alignment);
}

/**
  Allocates a buffer of type EfiRuntimeServicesData at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If AllocationSize is 0,
  then a valid buffer of 0 size is returned.  If there is not enough memory at the specified
  alignment remaining to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedRuntimePool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPool (EfiRuntimeServicesData, AllocationSize, Alignment);
}

/**
  Allocates a buffer of type EfieservedMemoryType at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfieservedMemoryType with an
  alignment specified by Alignment.  The allocated buffer is returned.  If AllocationSize is 0,
  then a valid buffer of 0 size is returned.  If there is not enough memory at the specified
  alignment remaining to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedReservedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPool (EfiReservedMemoryType, AllocationSize, Alignment);
}

/**
  Allocates and zeros a buffer of a certain pool type at a specified alignment.

  Allocates the number bytes specified by AllocationSize of a certain pool type with an alignment
  specified by Alignment, clears the buffer with zeros, and returns a pointer to the allocated
  buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there is not
  enough memory at the specified alignment remaining to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  PoolType              The type of pool to allocate.
  @param  AllocationSize        The number of bytes to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalAllocateAlignedZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN UINTN            Alignment
  )
{
  VOID    *Memory;
  Memory = InternalAllocateAlignedPool (PoolType, AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

/**
  Allocates and zeros a buffer of type EfiBootServicesData at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData with an
  alignment specified by Alignment, clears the buffer with zeros, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory at the specified alignment remaining to satisfy the request, then NULL is
  returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedZeroPool (EfiBootServicesData, AllocationSize, Alignment);
}

/**
  Allocates and zeros a buffer of type EfiRuntimeServicesData at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData with an
  alignment specified by Alignment, clears the buffer with zeros, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory at the specified alignment remaining to satisfy the request, then NULL is
  returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedRuntimeZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedZeroPool (EfiRuntimeServicesData, AllocationSize, Alignment);
}

/**
  Allocates and zeros a buffer of type EfieservedMemoryType at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfieservedMemoryType with an
  alignment specified by Alignment, clears the buffer with zeros, and returns a pointer to the
  allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory at the specified alignment remaining to satisfy the request, then NULL is
  returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedReservedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedZeroPool (EfiReservedMemoryType, AllocationSize, Alignment);
}

/**
  Copies a buffer to an allocated buffer of a certain pool type at a specified alignment.

  Allocates the number bytes specified by AllocationSize of a certain pool type with an alignment
  specified by Alignment.  The allocated buffer is returned.  If AllocationSize is 0, then a valid
  buffer of 0 size is returned.  If there is not enough memory at the specified alignment remaining
  to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  PoolType              The type of pool to allocate.
  @param  AllocationSize        The number of bytes to allocate.
  @param  Buffer                The buffer to copy to the allocated buffer.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalAllocateAlignedCopyPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN CONST VOID       *Buffer,
  IN UINTN            Alignment
  )
{
  VOID  *Memory;
  
  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = InternalAllocateAlignedPool (PoolType, AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

/**
  Copies a buffer to an allocated buffer of type EfiBootServicesData at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData type with an
  alignment specified by Alignment.  The allocated buffer is returned.  If AllocationSize is 0,
  then a valid buffer of 0 size is returned.  If there is not enough memory at the specified
  alignment remaining to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Buffer                The buffer to copy to the allocated buffer.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return InternalAllocateAlignedCopyPool (EfiBootServicesData, AllocationSize, Buffer, Alignment);
}

/**
  Copies a buffer to an allocated buffer of type EfiRuntimeServicesData at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfiRuntimeServicesData type with an
  alignment specified by Alignment.  The allocated buffer is returned.  If AllocationSize is 0,
  then a valid buffer of 0 size is returned.  If there is not enough memory at the specified
  alignment remaining to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Buffer                The buffer to copy to the allocated buffer.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return InternalAllocateAlignedCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer, Alignment);
}

/**
  Copies a buffer to an allocated buffer of type EfiReservedMemoryType at a specified alignment.

  Allocates the number bytes specified by AllocationSize of type EfiReservedMemoryType type with an
  alignment specified by Alignment.  The allocated buffer is returned.  If AllocationSize is 0,
  then a valid buffer of 0 size is returned.  If there is not enough memory at the specified
  alignment remaining to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate.
  @param  Buffer                The buffer to copy to the allocated buffer.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return InternalAllocateAlignedCopyPool (EfiReservedMemoryType, AllocationSize, Buffer, Alignment);
}

/**
  Frees a buffer that was previously allocated with one of the aligned pool allocation functions 
  in the Memory Allocation Library.

  Frees the buffer specified by Buffer.  Buffer must have been allocated on a previous call to the
  aligned pool allocation services of the Memory Allocation Library.
  If Buffer was not allocated with an aligned pool allocation function in the Memory Allocation
  Library, then ASSERT().

  @param  Buffer                Pointer to the buffer to free.

**/
VOID
EFIAPI
FreeAlignedPool (
  IN VOID   *Buffer
  )
{
  VOID        *RawAddress;
  VOID        **FreePointer;
  EFI_STATUS  Status;

  //
  // Get the pre-saved original address in the over-allocate pool.
  //
  FreePointer = (VOID **)((UINTN) Buffer - sizeof (RawAddress));
  RawAddress  = *FreePointer;

  Status = gBS->FreePool (RawAddress);
  ASSERT_EFI_ERROR (Status);
}
