/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemEncryptTdxLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "IoMmuInternal.h"

extern BOOLEAN  mReservedSharedMemSupported;

#define SIZE_OF_MEM_RANGE(MemRange)  (MemRange->HeaderSize + MemRange->DataSize)

#define RESERVED_MEM_BITMAP_4K_MASK    0xf
#define RESERVED_MEM_BITMAP_32K_MASK   0xff0
#define RESERVED_MEM_BITMAP_128K_MASK  0x3000
#define RESERVED_MEM_BITMAP_1M_MASK    0x40000
#define RESERVED_MEM_BITMAP_2M_MASK    0x180000
#define RESERVED_MEM_BITMAP_MASK       0x1fffff

/**
 * mReservedMemRanges describes the layout of the reserved memory.
 * The reserved memory consists of disfferent size of memory region.
 * The pieces of memory with the same size are managed by one entry
 * in the mReservedMemRanges. All the pieces of memories are managed by
 * mReservedMemBitmap which is a UINT32. It means it can manage at most
 * 32 pieces of memory. Because of the layout of CommonBuffer
 * (1-page header + n-page data), a piece of reserved memory consists of
 * 2 parts: Header + Data.
 *
 * So put all these together, mReservedMemRanges and mReservedMemBitmap
 * are designed to manage the reserved memory.
 *
 * Use the second entry of mReservedMemRanges as an example.
 * { RESERVED_MEM_BITMAP_32K_MASK,  4,  8, SIZE_32KB,  SIZE_4KB, 0 },
 * - RESERVED_MEM_BITMAP_32K_MASK is 0xff0. It means bit4-11 in mReservedMemBitmap
 *   is reserved for 32K size memory.
 * - 4 is the shift of mReservedMemBitmap.
 * - 8 means there are 8 pieces of 32K size memory.
 * - SIZE_32KB indicates the size of Data part.
 * - SIZE_4KB is the size of Header part.
 * - 0 is the start address of this memory range which will be populated when
 *   the reserved memory is initialized.
 *
 * The size and count of the memory region are derived from the experience. For
 * a typical grub boot, there are about 5100 IoMmu/DMA operation. Most of these
 * DMA operation require the memory with size less than 32K (~5080). But we find
 * in grub boot there may be 2 DMA operation which require for the memory larger
 * than 1M. And these 2 DMA operation occur concurrently. So we reserve 2 pieces
 * of memory with size of SIZE_2MB. This is for the best boot performance.
 *
 * If all the reserved memory are exausted, then it will fall back to the legacy
 * memory allocation as before.
 */
STATIC IOMMU_RESERVED_MEM_RANGE  mReservedMemRanges[] = {
  { RESERVED_MEM_BITMAP_4K_MASK,   0,  4, SIZE_4KB,   SIZE_4KB, 0 },
  { RESERVED_MEM_BITMAP_32K_MASK,  4,  8, SIZE_32KB,  SIZE_4KB, 0 },
  { RESERVED_MEM_BITMAP_128K_MASK, 12, 2, SIZE_128KB, SIZE_4KB, 0 },
  { RESERVED_MEM_BITMAP_1M_MASK,   14, 1, SIZE_1MB,   SIZE_4KB, 0 },
  { RESERVED_MEM_BITMAP_2M_MASK,   15, 2, SIZE_2MB,   SIZE_4KB, 0 },
};

//
// Bitmap of the allocation of reserved memory.
//
STATIC UINT32  mReservedMemBitmap = 0;

//
// Start address of the reserved memory region.
//
STATIC EFI_PHYSICAL_ADDRESS  mReservedSharedMemAddress = 0;

//
// Total size of the reserved memory region.
//
STATIC UINT32  mReservedSharedMemSize = 0;

/**
 * Calculate the size of reserved memory.
 *
 * @retval UINT32   Size of the reserved memory
 */
STATIC
UINT32
CalcuateReservedMemSize (
  VOID
  )
{
  UINT32                    Index;
  IOMMU_RESERVED_MEM_RANGE  *MemRange;

  if (mReservedSharedMemSize != 0) {
    return mReservedSharedMemSize;
  }

  for (Index = 0; Index < ARRAY_SIZE (mReservedMemRanges); Index++) {
    MemRange                = &mReservedMemRanges[Index];
    mReservedSharedMemSize += (SIZE_OF_MEM_RANGE (MemRange) * MemRange->Slots);
  }

  return mReservedSharedMemSize;
}

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
  )
{
  EFI_STATUS                Status;
  UINT32                    Index1, Index2;
  UINTN                     TotalPages;
  IOMMU_RESERVED_MEM_RANGE  *MemRange;
  EFI_PHYSICAL_ADDRESS      PhysicalAddress;
  UINT64                    SharedAddress;

  if (!mReservedSharedMemSupported) {
    return EFI_UNSUPPORTED;
  }

  TotalPages = EFI_SIZE_TO_PAGES (CalcuateReservedMemSize ());

  PhysicalAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePages (TotalPages);
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: ReservedMem (%d pages) address = 0x%llx\n",
    __FUNCTION__,
    TotalPages,
    PhysicalAddress
    ));

  mReservedMemBitmap        = 0;
  mReservedSharedMemAddress = PhysicalAddress;

  for (Index1 = 0; Index1 < ARRAY_SIZE (mReservedMemRanges); Index1++) {
    MemRange                         = &mReservedMemRanges[Index1];
    MemRange->StartAddressOfMemRange = PhysicalAddress;

    for (Index2 = 0; Index2 < MemRange->Slots; Index2++) {
      SharedAddress = (UINT64)(UINTN)(MemRange->StartAddressOfMemRange + Index2 * SIZE_OF_MEM_RANGE (MemRange) + MemRange->HeaderSize);

      if (CC_GUEST_IS_SEV (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
        Status = MemEncryptSevClearPageEncMask (
                   0,
                   SharedAddress,
                   EFI_SIZE_TO_PAGES (MemRange->DataSize)
                   );
        ASSERT (!EFI_ERROR (Status));
      } else if (CC_GUEST_IS_TDX (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
        Status = MemEncryptTdxSetPageSharedBit (
                   0,
                   SharedAddress,
                   EFI_SIZE_TO_PAGES (MemRange->DataSize)
                   );
        ASSERT (!EFI_ERROR (Status));
      } else {
        ASSERT (FALSE);
      }
    }

    PhysicalAddress += (MemRange->Slots * SIZE_OF_MEM_RANGE (MemRange));
  }

  return EFI_SUCCESS;
}

/**
 * Release the pre-alloc shared memory.
 *
 * @retval EFI_SUCCESS  Successfully release the shared memory
 */
EFI_STATUS
IoMmuReleaseReservedSharedMem (
  BOOLEAN  MemoryMapLocked
  )
{
  EFI_STATUS                Status;
  UINT32                    Index1, Index2;
  IOMMU_RESERVED_MEM_RANGE  *MemRange;
  UINT64                    SharedAddress;

  if (!mReservedSharedMemSupported) {
    return EFI_SUCCESS;
  }

  for (Index1 = 0; Index1 < ARRAY_SIZE (mReservedMemRanges); Index1++) {
    MemRange = &mReservedMemRanges[Index1];
    for (Index2 = 0; Index2 < MemRange->Slots; Index2++) {
      SharedAddress = (UINT64)(UINTN)(MemRange->StartAddressOfMemRange + Index2 * SIZE_OF_MEM_RANGE (MemRange) + MemRange->HeaderSize);

      if (CC_GUEST_IS_SEV (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
        Status = MemEncryptSevSetPageEncMask (
                   0,
                   SharedAddress,
                   EFI_SIZE_TO_PAGES (MemRange->DataSize)
                   );
        ASSERT (!EFI_ERROR (Status));
      } else if (CC_GUEST_IS_TDX (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
        Status = MemEncryptTdxClearPageSharedBit (
                   0,
                   SharedAddress,
                   EFI_SIZE_TO_PAGES (MemRange->DataSize)
                   );
        ASSERT (!EFI_ERROR (Status));
      } else {
        ASSERT (FALSE);
      }
    }
  }

  if (!MemoryMapLocked) {
    FreePages ((VOID *)(UINTN)mReservedSharedMemAddress, EFI_SIZE_TO_PAGES (CalcuateReservedMemSize ()));
    mReservedSharedMemAddress = 0;
    mReservedMemBitmap        = 0;
  }

  mReservedSharedMemSupported = FALSE;

  return EFI_SUCCESS;
}

/**
 * Allocate from the reserved memory pool.
 * If the reserved shared memory is exausted or there is no suitalbe size, it turns
 * to the LegacyAllocateBuffer.
 *
 * @param Type                Allocate type
 * @param MemoryType          The memory type to be allocated
 * @param Pages               Pages to be allocated.
 * @param ReservedMemBitmap   Bitmap of the allocated memory region
 * @param PhysicalAddress     Pointer to the data part of allocated memory region
 *
 * @retval EFI_SUCCESS        Successfully allocate the buffer
 * @retval Other              As the error code indicates
 */
STATIC
EFI_STATUS
InternalAllocateBuffer (
  IN  EFI_ALLOCATE_TYPE        Type,
  IN  EFI_MEMORY_TYPE          MemoryType,
  IN  UINTN                    Pages,
  IN OUT UINT32                *ReservedMemBitmap,
  IN OUT EFI_PHYSICAL_ADDRESS  *PhysicalAddress
  )
{
  UINT32                    MemBitmap;
  UINT8                     Index;
  IOMMU_RESERVED_MEM_RANGE  *MemRange;
  UINTN                     PagesOfLastMemRange;

  *ReservedMemBitmap = 0;

  if (Pages == 0) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (!mReservedSharedMemSupported) {
    goto LegacyAllocateBuffer;
  }

  if (mReservedSharedMemAddress == 0) {
    goto LegacyAllocateBuffer;
  }

  PagesOfLastMemRange = 0;

  for (Index = 0; Index < ARRAY_SIZE (mReservedMemRanges); Index++) {
    if ((Pages > PagesOfLastMemRange) && (Pages <= EFI_SIZE_TO_PAGES (mReservedMemRanges[Index].DataSize))) {
      break;
    }

    PagesOfLastMemRange = EFI_SIZE_TO_PAGES (mReservedMemRanges[Index].DataSize);
  }

  if (Index == ARRAY_SIZE (mReservedMemRanges)) {
    // There is no suitable size of reserved memory. Turn to legacy allocate.
    goto LegacyAllocateBuffer;
  }

  MemRange = &mReservedMemRanges[Index];

  if ((mReservedMemBitmap & MemRange->BitmapMask) == MemRange->BitmapMask) {
    // The reserved memory is exausted. Turn to legacy allocate.
    goto LegacyAllocateBuffer;
  }

  MemBitmap = (mReservedMemBitmap & MemRange->BitmapMask) >> MemRange->Shift;

  for (Index = 0; Index < MemRange->Slots; Index++) {
    if ((MemBitmap & (UINT8)(1<<Index)) == 0) {
      break;
    }
  }

  ASSERT (Index != MemRange->Slots);

  *PhysicalAddress   = MemRange->StartAddressOfMemRange + Index * SIZE_OF_MEM_RANGE (MemRange) + MemRange->HeaderSize;
  *ReservedMemBitmap = (UINT32)(1 << (Index + MemRange->Shift));

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: range-size: %lx, start-address=0x%llx, pages=0x%llx, bits=0x%lx, bitmap: %lx => %lx\n",
    __FUNCTION__,
    MemRange->DataSize,
    *PhysicalAddress,
    Pages,
    *ReservedMemBitmap,
    mReservedMemBitmap,
    mReservedMemBitmap | *ReservedMemBitmap
    ));

  return EFI_SUCCESS;

LegacyAllocateBuffer:

  *ReservedMemBitmap = 0;
  return gBS->AllocatePages (Type, MemoryType, Pages, PhysicalAddress);
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      ReservedMemBitmap;

  ReservedMemBitmap = 0;
  Status            = InternalAllocateBuffer (
                        Type,
                        MemoryType,
                        MapInfo->NumberOfPages,
                        &ReservedMemBitmap,
                        &MapInfo->PlainTextAddress
                        );
  MapInfo->ReservedMemBitmap = ReservedMemBitmap;
  mReservedMemBitmap        |= ReservedMemBitmap;

  ASSERT (Status == EFI_SUCCESS);

  return Status;
}

/**
 * Free the bounce buffer allocated in IoMmuAllocateBounceBuffer.
 *
 * @param MapInfo       Pointer to the MAP_INFO
 * @return EFI_SUCCESS  Successfully free the bounce buffer.
 */
EFI_STATUS
IoMmuFreeBounceBuffer (
  IN OUT     MAP_INFO  *MapInfo
  )
{
  if (MapInfo->ReservedMemBitmap == 0) {
    gBS->FreePages (MapInfo->PlainTextAddress, MapInfo->NumberOfPages);
  } else {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: PlainTextAddress=0x%Lx, bits=0x%Lx, bitmap: %Lx => %Lx\n",
      __FUNCTION__,
      MapInfo->PlainTextAddress,
      MapInfo->ReservedMemBitmap,
      mReservedMemBitmap,
      mReservedMemBitmap & ((UINT32)(~MapInfo->ReservedMemBitmap))
      ));
    MapInfo->PlainTextAddress  = 0;
    mReservedMemBitmap        &= (UINT32)(~MapInfo->ReservedMemBitmap);
    MapInfo->ReservedMemBitmap = 0;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS  Status;

  Status = InternalAllocateBuffer (
             AllocateMaxAddress,
             MemoryType,
             CommonBufferPages,
             ReservedMemBitmap,
             PhysicalAddress
             );
  ASSERT (Status == EFI_SUCCESS);

  mReservedMemBitmap |= *ReservedMemBitmap;

  if (*ReservedMemBitmap != 0) {
    *PhysicalAddress -= SIZE_4KB;
  }

  return Status;
}

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
  )
{
  if (!mReservedSharedMemSupported) {
    goto LegacyFreeCommonBuffer;
  }

  if (CommonBufferHeader->ReservedMemBitmap == 0) {
    goto LegacyFreeCommonBuffer;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: CommonBuffer=0x%Lx, bits=0x%Lx, bitmap: %Lx => %Lx\n",
    __FUNCTION__,
    (UINT64)(UINTN)CommonBufferHeader + SIZE_4KB,
    CommonBufferHeader->ReservedMemBitmap,
    mReservedMemBitmap,
    mReservedMemBitmap & ((UINT32)(~CommonBufferHeader->ReservedMemBitmap))
    ));

  mReservedMemBitmap &= (UINT32)(~CommonBufferHeader->ReservedMemBitmap);
  return EFI_SUCCESS;

LegacyFreeCommonBuffer:
  return gBS->FreePages ((UINTN)CommonBufferHeader, CommonBufferPages);
}
