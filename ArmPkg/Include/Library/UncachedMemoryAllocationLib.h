/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UNCACHED_MEMORY_ALLOCATION_LIB_H__
#define __UNCACHED_MEMORY_ALLOCATION_LIB_H__

/**
  Converts a cached or uncached address to a physical address suitable for use in SoC registers.

  @param  VirtualAddress                 The pointer to convert.

  @return The physical address of the supplied virtual pointer.

**/
EFI_PHYSICAL_ADDRESS
ConvertToPhysicalAddress (
  IN VOID *VirtualAddress
  );

/**
  Converts a cached or uncached address to a cached address.

  @param  Address                 The pointer to convert.

  @return The address of the cached memory location corresponding to the input address.

**/
VOID *
ConvertToCachedAddress (
  IN VOID *Address
  );

/**
  Converts a cached or uncached address to an uncached address.

  @param  Address                 The pointer to convert.

  @return The address of the uncached memory location corresponding to the input address.

**/
VOID *
ConvertToUncachedAddress (
  IN VOID *Address
  );

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
UncachedAllocatePages (
  IN UINTN  Pages
  );

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
UncachedAllocateRuntimePages (
  IN UINTN  Pages
  );

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
UncachedAllocateReservedPages (
  IN UINTN  Pages
  );

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
UncachedFreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  );

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
UncachedAllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  );

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
UncachedAllocateAlignedRuntimePages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  );

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
UncachedAllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  );

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
UncachedFreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  );

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
UncachedAllocatePool (
  IN UINTN  AllocationSize
  );

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
UncachedAllocateRuntimePool (
  IN UINTN  AllocationSize
  );

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
UncachedAllocateReservedPool (
  IN UINTN  AllocationSize
  );

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
UncachedAllocateZeroPool (
  IN UINTN  AllocationSize
  );

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
UncachedAllocateRuntimeZeroPool (
  IN UINTN  AllocationSize
  );

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
UncachedAllocateReservedZeroPool (
  IN UINTN  AllocationSize
  );

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
UncachedAllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  );

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
UncachedAllocateRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  );

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
UncachedAllocateReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  );

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
UncachedFreePool (
  IN VOID   *Buffer
  );

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
UncachedAllocateAlignedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  );

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
UncachedAllocateAlignedRuntimePool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  );

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
UncachedAllocateAlignedReservedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  );

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
UncachedAllocateAlignedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  );

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
UncachedAllocateAlignedRuntimeZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  );

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
UncachedAllocateAlignedReservedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  );

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
UncachedAllocateAlignedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  );

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
UncachedAllocateAlignedRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  );

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
UncachedAllocateAlignedReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  );

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
UncachedFreeAlignedPool (
  IN VOID   *Buffer
  );

VOID
EFIAPI
UncachedSafeFreePool (
  IN VOID   *Buffer
  );

#endif // __UNCACHED_MEMORY_ALLOCATION_LIB_H__
