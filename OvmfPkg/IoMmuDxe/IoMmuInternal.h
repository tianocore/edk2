/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IOMMU_INTERNAL_H_
#define IOMMU_INTERNAL_H_

#include <Base.h>
#include <Protocol/IoMmu.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#define MAP_INFO_SIG  SIGNATURE_64 ('M', 'A', 'P', '_', 'I', 'N', 'F', 'O')

typedef struct {
  UINT64                   Signature;
  LIST_ENTRY               Link;
  EDKII_IOMMU_OPERATION    Operation;
  UINTN                    NumberOfBytes;
  UINTN                    NumberOfPages;
  EFI_PHYSICAL_ADDRESS     CryptedAddress;
  EFI_PHYSICAL_ADDRESS     PlainTextAddress;
  UINT32                   ReservedMemBitmap;
} MAP_INFO;

#define COMMON_BUFFER_SIG  SIGNATURE_64 ('C', 'M', 'N', 'B', 'U', 'F', 'F', 'R')

#pragma pack (1)
//
// The following structure enables Map() and Unmap() to perform in-place
// decryption and encryption, respectively, for BusMasterCommonBuffer[64]
// operations, without dynamic memory allocation or release.
//
// Both COMMON_BUFFER_HEADER and COMMON_BUFFER_HEADER.StashBuffer are allocated
// by AllocateBuffer() and released by FreeBuffer().
//
typedef struct {
  UINT64    Signature;

  //
  // Always allocated from EfiBootServicesData type memory, and always
  // encrypted.
  //
  VOID      *StashBuffer;

  //
  // Bitmap of reserved memory
  //
  UINT32    ReservedMemBitmap;

  //
  // Followed by the actual common buffer, starting at the next page.
  //
} COMMON_BUFFER_HEADER;

//
// This data structure defines a memory range in the reserved memory region.
// Please refer to IoMmuInitReservedSharedMem() for detailed information.
//
// The memory region looks like:
//     |------------|----------------------------|
//     | Header     |    Data                    |
//     | 4k, private| 4k/32k/128k/etc, shared    |
//     |-----------------------------------------|
//
typedef struct {
  UINT32                  BitmapMask;
  UINT32                  Shift;
  UINT32                  Slots;
  UINT32                  DataSize;
  UINT32                  HeaderSize;
  EFI_PHYSICAL_ADDRESS    StartAddressOfMemRange;
} IOMMU_RESERVED_MEM_RANGE;
#pragma pack()

/**
 * Allocate a memory region and convert it to be shared. This memory region will be
 * used in the DMA operation.
 *
 * The pre-alloc memory contains pieces of memory regions with different size. The
 * allocation of the shared memory regions are indicated by a 32-bit bitmap (mReservedMemBitmap).
 *
 * The memory regions are consumed by IoMmuAllocateBuffer (in which CommonBuffer is allocated) and
 * IoMmuMap (in which bounce buffer is allocated).
 *
 * The CommonBuffer contains 2 parts, one page for CommonBufferHeader which is private memory,
 * the other part is shared memory. So the layout of a piece of memory region after initialization
 * looks like:
 *
 *     |------------|----------------------------|
 *     | Header     |    Data                    |  <-- a piece of pre-alloc memory region
 *     | 4k, private| 4k/32k/128k/etc, shared    |
 *     |-----------------------------------------|
 *
 * @retval EFI_SUCCESS      Successfully initialize the reserved memory.
 * @retval EFI_UNSUPPORTED  This feature is not supported.
 */
EFI_STATUS
IoMmuInitReservedSharedMem (
  VOID
  );

/**
 * Release the pre-alloc shared memory.
 *
 * @retval EFI_SUCCESS  Successfully release the shared memory
 */
EFI_STATUS
IoMmuReleaseReservedSharedMem (
  BOOLEAN  MemoryMapLocked
  );

/**
 * Allocate reserved shared memory for bounce buffer.
 *
 * @param Type        Allocate type
 * @param MemoryType  The memory type to be allocated
 * @param MapInfo     Pointer to the MAP_INFO
 *
 * @retval EFI_SUCCESS        Successfully allocate the bounce buffer
 * @retval Other              As the error code indicates
 */
EFI_STATUS
IoMmuAllocateBounceBuffer (
  IN     EFI_ALLOCATE_TYPE  Type,
  IN     EFI_MEMORY_TYPE    MemoryType,
  IN OUT MAP_INFO           *MapInfo
  );

/**
 * Free the bounce buffer allocated in IoMmuAllocateBounceBuffer.
 *
 * @param MapInfo       Pointer to the MAP_INFO
 * @return EFI_SUCCESS  Successfully free the bounce buffer.
 */
EFI_STATUS
IoMmuFreeBounceBuffer (
  IN OUT     MAP_INFO  *MapInfo
  );

/**
 * Allocate CommonBuffer from pre-allocated shared memory.
 *
 * @param MemoryType          Memory type
 * @param CommonBufferPages   Pages of CommonBuffer
 * @param PhysicalAddress     Allocated physical address
 * @param ReservedMemBitmap   Bitmap which indicates the allocation of reserved memory
 *
 * @retval EFI_SUCCESS        Successfully allocate the common buffer
 * @retval Other              As the error code indicates
 */
EFI_STATUS
IoMmuAllocateCommonBuffer (
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  CommonBufferPages,
  OUT EFI_PHYSICAL_ADDRESS  *PhysicalAddress,
  OUT UINT32                *ReservedMemBitmap
  );

/**
 * Free CommonBuffer which is allocated by IoMmuAllocateCommonBuffer().
 *
 * @param CommonBufferHeader  Pointer to the CommonBufferHeader
 * @param CommonBufferPages   Pages of CommonBuffer
 *
 * @retval EFI_SUCCESS        Successfully free the common buffer
 * @retval Other              As the error code indicates
 */
EFI_STATUS
IoMmuFreeCommonBuffer (
  IN COMMON_BUFFER_HEADER  *CommonBufferHeader,
  IN UINTN                 CommonBufferPages
  );

#endif
