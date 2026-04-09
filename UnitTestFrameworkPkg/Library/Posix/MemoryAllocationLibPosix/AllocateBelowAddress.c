/** @file
  Instance of Memory Below Address Allocation Library based on Windows APIs
  and Linux APIs.

  Uses Windows APIs VirtualAlloc() and VirtualFree() to allocate and free memory
  below a specified virtual address.

  Uses Linux APIs mmap() and munmap() to allocate and free memory below a
  specified virtual address.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#if defined (_WIN32) || defined (_WIN64)
  #include "WinInclude.h"
#elif defined (__linux__)
  #include <sys/mman.h>
  #include <unistd.h>
  #include <errno.h>
  #include <string.h>
#else
  #error Unsupported target
#endif

#include <Library/HostMemoryAllocationBelowAddressLib.h>
#include <Library/DebugLib.h>

///
/// Signature for PAGE_HEAD_BELOW_ADDRESS structure
/// Used to verify that buffer being freed was allocated by this library.
///
#define PAGE_HEAD_BELOW_ADDRESS_PRIVATE_SIGNATURE  SIGNATURE_64 ('P', 'A', 'H', 'B', 'e', 'l', 'A', 'd')

///
/// Structure placed immediately before an aligned allocation to store the
/// information required to free the entire allocated buffer.
///
typedef struct {
  UINT64    Signature;
  VOID      *AllocatedBuffer;
  UINTN     TotalPages;
  VOID      *AlignedBuffer;
  UINTN     AlignedPages;
} PAGE_HEAD_BELOW_ADDRESS;

///
/// Signature for POOL_HEAD_BELOW_ADDRESS structure
/// Used to verify that buffer being freed was allocated by this library.
///
#define POOL_HEAD_BELOW_ADDRESS_PRIVATE_SIGNATURE  SIGNATURE_64 ('P', 'O', 'H', 'B', 'e', 'l', 'A', 'd')

///
/// Structure placed immediately before an pool allocation to store the
/// information required to free the entire allocated buffer.
///
typedef struct {
  UINT64    Signature;
  UINT64    TotalSize;
} POOL_HEAD_BELOW_ADDRESS;

//
// Lowest address that can be allocated by this library
//
#define MINIMUM_ALLOCATION_ADDRESS  BASE_64KB

//
// The page size of the host
//
static UINTN  mPageSize = 0;

/**
  Use system services to get the host page size.

  @return  Host page size in bytes.
**/
static
UINTN
HostGetPageSize (
  VOID
  )
{
 #if defined (_WIN32) || defined (_WIN64)
  SYSTEM_INFO  SystemInfo;

  GetSystemInfo (&SystemInfo);
  return (UINTN)SystemInfo.dwPageSize;
 #elif defined (__linux__)
  return sysconf (_SC_PAGESIZE);
 #else
  return 0;
 #endif
}

/**
  Use system services to allocate a buffer between a minimum and maximum
  address aligned to the requested page size.

  @param[in] MaximumAddress  The address below which the memory allocation must
                             be performed.
  @param[in] Length          The size, in bytes, of the memory allocation.

  @retval  !NULL  Pointer to the allocated memory.
  @retval  NULL   The memory allocation failed.
**/
static
VOID *
HostAllocateBufferInRange (
  UINTN  MaximumAddress,
  UINTN  Length
  )
{
  UINTN  Address;
  VOID   *AllocatedAddress;

  if (mPageSize == 0) {
    mPageSize = HostGetPageSize ();
    if (mPageSize == 0) {
      return NULL;
    }
  }

  //
  // Round maximum address down to the nearest page boundary
  //
  MaximumAddress &= ~(mPageSize - 1);

  for (Address = MaximumAddress; Address >= MINIMUM_ALLOCATION_ADDRESS; Address -= mPageSize) {
 #if defined (_WIN32) || defined (_WIN64)
    AllocatedAddress = VirtualAlloc (
                         (VOID *)Address,
                         Length,
                         MEM_RESERVE | MEM_COMMIT,
                         PAGE_READWRITE
                         );
    if (AllocatedAddress != NULL) {
      return AllocatedAddress;
    }

 #elif defined (__linux__)
    AllocatedAddress = mmap (
                         (VOID *)Address,
                         Length,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                         -1,
                         0
                         );
    if (AllocatedAddress != MAP_FAILED) {
      return AllocatedAddress;
    }

 #else
    return NULL;
 #endif
  }

  return NULL;
}

/**
  Use system services to free memory allocated with HostAllocateBufferInRange().

  @param[in] Buffer  Pointer to buffer previously allocated with
                     HostAllocateBufferInRange().
  @param[in] Length  Length, in bytes, of buffer previously allocated with
                     HostAllocateBufferInRange().
**/
static
VOID
HostFreeBufferInRange (
  IN VOID   *Buffer,
  IN UINTN  Length
  )
{
 #if defined (_WIN32) || defined (_WIN64)
  if (!VirtualFree (Buffer, 0, MEM_RELEASE)) {
    ASSERT (FALSE);
  }

 #elif defined (__linux__)
  if (munmap (Buffer, Length) == -1) {
    ASSERT (FALSE);
  }

 #endif
}

/**
  Allocate memory below a specific address.

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
  )
{
  VOID                     *AllocatedAddress;
  POOL_HEAD_BELOW_ADDRESS  *PoolHead;

  if (Length == 0) {
    return NULL;
  }

  //
  // Limit maximum address to the largest supported virtual address
  //
  MaximumAddress = MIN (MaximumAddress, MAX_UINTN);

  //
  // Increase requested allocation length by the size of the pool header
  //
  Length += sizeof (POOL_HEAD_BELOW_ADDRESS);

  //
  // Make sure allocation length is smaller than maximum address
  //
  if (Length > MaximumAddress) {
    DEBUG ((DEBUG_ERROR, "HostAllocatePoolBelowAddress: Length > MaximumAddress\n"));
    return NULL;
  }

  //
  // Reduce maximum address by the requested allocation length
  //
  MaximumAddress -= Length;

  AllocatedAddress = HostAllocateBufferInRange (
                       (UINTN)MaximumAddress,
                       (UINTN)Length
                       );
  if (AllocatedAddress == NULL) {
    DEBUG ((DEBUG_ERROR, "HostAllocatePoolBelowAddress: HostAllocateBufferInRange failed\n"));
    return NULL;
  }

  DEBUG_CLEAR_MEMORY (AllocatedAddress, (UINTN)Length);
  PoolHead            = (POOL_HEAD_BELOW_ADDRESS *)AllocatedAddress;
  PoolHead->Signature = POOL_HEAD_BELOW_ADDRESS_PRIVATE_SIGNATURE;
  PoolHead->TotalSize = Length;
  return (VOID *)(PoolHead + 1);
}

/**
  Free memory allocated with HostAllocatePoolBelowAddress().

  @param[in] Buffer  Pointer to buffer previously allocated with
                     HostAllocatePoolBelowAddress().
**/
VOID
EFIAPI
HostFreePoolBelowAddress (
  IN VOID  *Buffer
  )
{
  POOL_HEAD_BELOW_ADDRESS  *PoolHead;
  UINTN                    Length;

  ASSERT (Buffer != NULL);

  PoolHead = ((POOL_HEAD_BELOW_ADDRESS *)Buffer) - 1;

  ASSERT (PoolHead != NULL);
  ASSERT (PoolHead->Signature == POOL_HEAD_BELOW_ADDRESS_PRIVATE_SIGNATURE);
  ASSERT (PoolHead->TotalSize >= sizeof (POOL_HEAD_BELOW_ADDRESS));
  ASSERT (PoolHead->TotalSize <= MAX_UINTN);

  Length = (UINTN)PoolHead->TotalSize;
  DEBUG_CLEAR_MEMORY (PoolHead, Length);

  HostFreeBufferInRange (PoolHead, Length);
}

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
  )
{
  PAGE_HEAD_BELOW_ADDRESS  PageHead;
  PAGE_HEAD_BELOW_ADDRESS  *PageHeadPtr;
  UINTN                    AlignmentMask;
  UINTN                    Length;

  if (Pages == 0) {
    return NULL;
  }

  //
  // Make sure alignment is a power of two
  //
  if ((Alignment & (Alignment - 1)) != 0) {
    DEBUG ((DEBUG_ERROR, "HostAllocateAlignedPagesBelowAddress: Alignment is not a power of two\n"));
    return NULL;
  }

  //
  // Make sure alignment is smaller than the largest supported virtual address
  //
  if (Alignment > MAX_UINTN) {
    DEBUG ((DEBUG_ERROR, "HostAllocateAlignedPagesBelowAddress: Alignment > MAX_UINTN\n"));
    return NULL;
  }

  //
  // Make sure alignment is at least 4KB
  //
  Alignment = MAX (Alignment, SIZE_4KB);

  //
  // Initialize local page head structure
  //
  PageHead.Signature    = PAGE_HEAD_BELOW_ADDRESS_PRIVATE_SIGNATURE;
  PageHead.AlignedPages = Pages;
  PageHead.TotalPages   = Pages + 2 * EFI_SIZE_TO_PAGES ((UINTN)Alignment);

  //
  // Limit maximum address to the largest supported virtual address
  //
  MaximumAddress = MIN (MaximumAddress, MAX_UINTN);

  //
  // Make sure total page allocation fits below maximum address
  //
  if (PageHead.TotalPages >= EFI_SIZE_TO_PAGES (MaximumAddress)) {
    DEBUG ((DEBUG_ERROR, "HostAllocateAlignedPagesBelowAddress: TotalPages >= MaximumAddress\n"));
    return NULL;
  }

  //
  // Determine the length of the allocation in bytes
  //
  Length = EFI_PAGES_TO_SIZE (PageHead.TotalPages);

  //
  // Reduce maximum address by the total allocation length
  //
  MaximumAddress -= Length;

  //
  // Allocate buffer large enough to support aligned page request
  //
  PageHead.AllocatedBuffer = HostAllocateBufferInRange (
                               (UINTN)MaximumAddress,
                               Length
                               );
  if (PageHead.AllocatedBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "HostAllocateAlignedPagesBelowAddress: HostAllocateBufferInRange failed\n"));
    return NULL;
  }

  DEBUG_CLEAR_MEMORY (PageHead.AllocatedBuffer, Length);

  AlignmentMask          = ((UINTN)Alignment - 1);
  PageHead.AlignedBuffer = (VOID *)(((UINTN)PageHead.AllocatedBuffer + AlignmentMask) & ~AlignmentMask);
  if ((UINTN)PageHead.AlignedBuffer - (UINTN)PageHead.AllocatedBuffer < sizeof (PAGE_HEAD_BELOW_ADDRESS)) {
    PageHead.AlignedBuffer = (VOID *)((UINTN)PageHead.AlignedBuffer + (UINTN)Alignment);
  }

  PageHeadPtr = (PAGE_HEAD_BELOW_ADDRESS *)((UINTN)PageHead.AlignedBuffer) - 1;
  memcpy (PageHeadPtr, &PageHead, sizeof (PageHead));

  return PageHead.AlignedBuffer;
}

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
  )
{
  PAGE_HEAD_BELOW_ADDRESS  *PageHeadPtr;
  VOID                     *AllocatedBuffer;
  UINTN                    Length;

  ASSERT (Buffer != NULL);

  PageHeadPtr = ((PAGE_HEAD_BELOW_ADDRESS *)Buffer) - 1;

  ASSERT (PageHeadPtr != NULL);
  ASSERT (PageHeadPtr->Signature == PAGE_HEAD_BELOW_ADDRESS_PRIVATE_SIGNATURE);
  ASSERT (PageHeadPtr->AlignedPages == Pages);
  ASSERT (PageHeadPtr->AllocatedBuffer != NULL);

  AllocatedBuffer = PageHeadPtr->AllocatedBuffer;
  Length          = EFI_PAGES_TO_SIZE (PageHeadPtr->TotalPages);

  DEBUG_CLEAR_MEMORY (AllocatedBuffer, Length);

  HostFreeBufferInRange (AllocatedBuffer, Length);
}
