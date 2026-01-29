/** @file
  HostMemoryAllocationBelowAddressLib class

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOST_MEMORY_ALLOCATION_BELOW_ADDRESS_LIB_H_

/**
  Allocate memory below a specifies address.

  @param[in] MaximumAddress  The address below which the memory allocation must
                             be performed.
  @param[in] Length          The size, in bytes, of the memory allocation.

  @retval  !NULL  Pointer to the allocated memory.
  @retval  NULL   The memory allocation failed.
**/
VOID *
EFIAPI
HostAllocatePoolBelowAddress (
  IN UINT64  MaximumAddress,
  IN UINT64  Length
  );

/**
  Free memory allocated with AllocateMemoryHostAllocatePoolBelowAddress().

  @param[in] Address  Pointer to buffer previously allocated with
                      HostAllocatePoolBelowAddress().
**/
VOID
EFIAPI
HostFreePoolBelowAddress (
  IN VOID  *Address
  );

/**
  Allocates one or more 4KB pages below a specified address at a specified
  alignment.

  Allocates the number of 4KB pages specified by Pages below MaximumAddress with
  an alignment specified by Alignment. The allocated buffer is returned.  If
  Pages is 0, then NULL is returned.  If there is not enough memory below the
  requested address at the specified alignment remaining to satisfy the request,
  then NULL is returned.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

  @param[in] MaximumAddress  The address below which the memory allocation must
  @param[in] Pages           The number of 4 KB pages to allocate.
  @param[in] Alignment       The requested alignment of the allocation. Must be
                             a power of two. If Alignment is zero, then byte
                             alignment is used.

  @return  A pointer to the allocated buffer or NULL if allocation fails.
**/
VOID *
EFIAPI
HostAllocateAlignedPagesBelowAddress (
  IN UINT64  MaximumAddress,
  IN UINTN   Pages,
  IN UINT64  Alignment
  );

/**
  Frees one or more 4KB pages that were previously allocated with
  HostAllocateAlignedPagesBelowAddress().

  Frees the number of 4KB pages specified by Pages from the buffer specified by
  Buffer. Buffer must have been allocated with HostAllocateAlignedPagesBelowAddress().
  If it is not possible to free allocated pages, then this function will perform
  no actions.

  If Buffer was not allocated with HostAllocateAlignedPagesBelowAddress(), then
  ASSERT(). If Pages is zero, then ASSERT().

  @param[in] Buffer  The pointer to the buffer of pages to free.
  @param[in] Pages   The number of 4 KB pages to free.
**/
VOID
EFIAPI
HostFreeAlignedPagesBelowAddress (
  IN VOID   *Buffer,
  IN UINTN  Pages
  );

#endif
