/** @file

  The protocol provides support to allocate, free, map and umap a DMA buffer
  for bus master (e.g PciHostBridge). When SEV or TDX is enabled, the DMA
  operations must be performed on unencrypted buffer hence we use a bounce
  buffer to map the guest buffer into an unencrypted DMA buffer.

  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PcdLib.h>
#include <ConfidentialComputingGuestAttr.h>
#include "AmdSevIoMmu.h"

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

//
// List of the MAP_INFO structures that have been set up by IoMmuMap() and not
// yet torn down by IoMmuUnmap(). The list represents the full set of mappings
// currently in effect.
//
STATIC LIST_ENTRY  mMapInfos = INITIALIZE_LIST_HEAD_VARIABLE (mMapInfos);

#define COMMON_BUFFER_SIG  SIGNATURE_64 ('C', 'M', 'N', 'B', 'U', 'F', 'F', 'R')

//
// ASCII names for EDKII_IOMMU_OPERATION constants, for debug logging.
//
STATIC CONST CHAR8 *CONST
mBusMasterOperationName[EdkiiIoMmuOperationMaximum] = {
  "Read",
  "Write",
  "CommonBuffer",
  "Read64",
  "Write64",
  "CommonBuffer64"
};

//
// The following structure enables Map() and Unmap() to perform in-place
// decryption and encryption, respectively, for BusMasterCommonBuffer[64]
// operations, without dynamic memory allocation or release.
//
// Both COMMON_BUFFER_HEADER and COMMON_BUFFER_HEADER.StashBuffer are allocated
// by AllocateBuffer() and released by FreeBuffer().
//
#pragma pack (1)
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
// Please refer to InitReservedSharedMem() for detailed information.
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
} RESERVED_MEM_RANGE;

#pragma pack ()

#define SIZE_OF_MEM_RANGE(MemRange)  (MemRange->HeaderSize + MemRange->DataSize)

#define RESERVED_MEM_BITMAP_4K_MASK    0xf
#define RESERVED_MEM_BITMAP_32K_MASK   0xff0
#define RESERVED_MEM_BITMAP_128K_MASK  0x3000
#define RESERVED_MEM_BITMAP_1M_MASK    0x40000
#define RESERVED_MEM_BITMAP_2M_MASK    0x180000
#define RESERVED_MEM_BITMAP_MASK       0x1fffff

STATIC RESERVED_MEM_RANGE  mReservedMemRanges[] = {
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
// Indicate if the feature of reserved memory is supported in DMA operation.
//
STATIC BOOLEAN  mReservedSharedMemSupported = FALSE;

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
 * @return UINT32   Size of the reserved memory
 */
STATIC
UINT32
CalcuateReservedMemSize (
  VOID
  )
{
  UINT32              Index;
  RESERVED_MEM_RANGE  *MemRange;

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
 * @return EFI_SUCCESS Successfully initialize the reserved memory.
 */
STATIC
EFI_STATUS
InitReservedSharedMem (
  VOID
  )
{
  EFI_STATUS            Status;
  UINT32                Index1, Index2;
  UINTN                 TotalPages;
  RESERVED_MEM_RANGE    *MemRange;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

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
      Status = MemEncryptTdxSetPageSharedBit (
                 0,
                 (UINT64)(UINTN)(MemRange->StartAddressOfMemRange + Index2 * SIZE_OF_MEM_RANGE (MemRange) + MemRange->HeaderSize),
                 EFI_SIZE_TO_PAGES (MemRange->DataSize)
                 );
      ASSERT (!EFI_ERROR (Status));
    }

    PhysicalAddress += (MemRange->Slots * SIZE_OF_MEM_RANGE (MemRange));
  }

  return EFI_SUCCESS;
}

/**
 * Release the pre-alloc shared memory.
 *
 * @return EFI_SUCCESS  Successfully release the shared memory
 */
STATIC
EFI_STATUS
ReleaseReservedSharedMem (
  BOOLEAN  MemoryMapLocked
  )
{
  EFI_STATUS          Status;
  UINT32              Index1, Index2;
  RESERVED_MEM_RANGE  *MemRange;

  for (Index1 = 0; Index1 < ARRAY_SIZE (mReservedMemRanges); Index1++) {
    MemRange = &mReservedMemRanges[Index1];
    for (Index2 = 0; Index2 < MemRange->Slots; Index2++) {
      Status = MemEncryptTdxClearPageSharedBit (
                 0,
                 (UINT64)(UINTN)(MemRange->StartAddressOfMemRange + Index2 * SIZE_OF_MEM_RANGE (MemRange) + MemRange->HeaderSize),
                 EFI_SIZE_TO_PAGES (MemRange->DataSize)
                 );
      ASSERT (!EFI_ERROR (Status));
    }
  }

  if (!MemoryMapLocked) {
    FreePages ((VOID *)(UINTN)mReservedSharedMemAddress, EFI_SIZE_TO_PAGES (CalcuateReservedMemSize ()));
    mReservedSharedMemAddress = 0;
    mReservedMemBitmap        = 0;
  }

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
 * @return EFI_SUCCESS        Successfully allocate the buffer
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
  UINT32              MemBitmap;
  UINT8               Index;
  RESERVED_MEM_RANGE  *MemRange;
  UINTN               PagesOfLastMemRange;

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
 * @return EFI_SUCCESS  Successfully allocate the bounce buffer.
 */
STATIC
EFI_STATUS
AllocateBounceBuffer (
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
 * Free the bounce buffer allocated in AllocateBounceBuffer.
 *
 * @param MapInfo       Pointer to the MAP_INFO
 * @return EFI_SUCCESS  Successfully free the bounce buffer.
 */
STATIC
EFI_STATUS
FreeBounceBuffer (
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
  Provides the controller-specific addresses required to access system memory
  from a DMA bus master. On SEV/TDX guest, the DMA operations must be performed on
  shared buffer hence we allocate a bounce buffer to map the HostAddress to a
  DeviceAddress. The Encryption attribute is removed from the DeviceAddress
  buffer.

  @param  This                  The protocol instance pointer.
  @param  Operation             Indicates if the bus master is going to read or
                                write to system memory.
  @param  HostAddress           The system memory address to map to the PCI
                                controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output
                                the number of bytes that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master
                                PCI controller to use to access the hosts
                                HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned
                                NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common
                                buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested
                                address.

**/
EFI_STATUS
EFIAPI
IoMmuMap (
  IN     EDKII_IOMMU_PROTOCOL   *This,
  IN     EDKII_IOMMU_OPERATION  Operation,
  IN     VOID                   *HostAddress,
  IN OUT UINTN                  *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID                   **Mapping
  )
{
  EFI_STATUS            Status;
  MAP_INFO              *MapInfo;
  EFI_ALLOCATE_TYPE     AllocateType;
  COMMON_BUFFER_HEADER  *CommonBufferHeader;
  VOID                  *DecryptionSource;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Operation=%a Host=0x%p Bytes=0x%Lx\n",
    __FUNCTION__,
    ((Operation >= 0 &&
      Operation < ARRAY_SIZE (mBusMasterOperationName)) ?
     mBusMasterOperationName[Operation] :
     "Invalid"),
    HostAddress,
    (UINT64)((NumberOfBytes == NULL) ? 0 : *NumberOfBytes)
    ));

  if ((HostAddress == NULL) || (NumberOfBytes == NULL) || (DeviceAddress == NULL) ||
      (Mapping == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  //
  // Allocate a MAP_INFO structure to remember the mapping when Unmap() is
  // called later.
  //
  MapInfo = AllocatePool (sizeof (MAP_INFO));
  if (MapInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Failed;
  }

  //
  // Initialize the MAP_INFO structure, except the PlainTextAddress field
  //
  ZeroMem (&MapInfo->Link, sizeof MapInfo->Link);
  MapInfo->Signature         = MAP_INFO_SIG;
  MapInfo->Operation         = Operation;
  MapInfo->NumberOfBytes     = *NumberOfBytes;
  MapInfo->NumberOfPages     = EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes);
  MapInfo->CryptedAddress    = (UINTN)HostAddress;
  MapInfo->ReservedMemBitmap = 0;

  //
  // In the switch statement below, we point "MapInfo->PlainTextAddress" to the
  // plaintext buffer, according to Operation. We also set "DecryptionSource".
  //
  MapInfo->PlainTextAddress = MAX_ADDRESS;
  AllocateType              = AllocateAnyPages;
  DecryptionSource          = (VOID *)(UINTN)MapInfo->CryptedAddress;
  switch (Operation) {
    //
    // For BusMasterRead[64] and BusMasterWrite[64] operations, a bounce buffer
    // is necessary regardless of whether the original (crypted) buffer crosses
    // the 4GB limit or not -- we have to allocate a separate plaintext buffer.
    // The only variable is whether the plaintext buffer should be under 4GB.
    //
    case EdkiiIoMmuOperationBusMasterRead:
    case EdkiiIoMmuOperationBusMasterWrite:
      MapInfo->PlainTextAddress = BASE_4GB - 1;
      AllocateType              = AllocateMaxAddress;
    //
    // fall through
    //
    case EdkiiIoMmuOperationBusMasterRead64:
    case EdkiiIoMmuOperationBusMasterWrite64:
      //
      // Allocate the implicit plaintext bounce buffer.
      //
      Status = AllocateBounceBuffer (
                 AllocateType,
                 EfiBootServicesData,
                 MapInfo
                 );
      if (EFI_ERROR (Status)) {
        goto FreeMapInfo;
      }

      break;

    //
    // For BusMasterCommonBuffer[64] operations, a to-be-plaintext buffer and a
    // stash buffer (for in-place decryption) have been allocated already, with
    // AllocateBuffer(). We only check whether the address of the to-be-plaintext
    // buffer is low enough for the requested operation.
    //
    case EdkiiIoMmuOperationBusMasterCommonBuffer:
      if ((MapInfo->CryptedAddress > BASE_4GB) ||
          (EFI_PAGES_TO_SIZE (MapInfo->NumberOfPages) >
           BASE_4GB - MapInfo->CryptedAddress))
      {
        //
        // CommonBuffer operations cannot be remapped. If the common buffer is
        // above 4GB, then it is not possible to generate a mapping, so return an
        // error.
        //
        Status = EFI_UNSUPPORTED;
        goto FreeMapInfo;
      }

    //
    // fall through
    //
    case EdkiiIoMmuOperationBusMasterCommonBuffer64:
      //
      // The buffer at MapInfo->CryptedAddress comes from AllocateBuffer().
      //
      MapInfo->PlainTextAddress = MapInfo->CryptedAddress;
      //
      // Stash the crypted data.
      //
      CommonBufferHeader = (COMMON_BUFFER_HEADER *)(
                                                    (UINTN)MapInfo->CryptedAddress - EFI_PAGE_SIZE
                                                    );
      ASSERT (CommonBufferHeader->Signature == COMMON_BUFFER_SIG);
      CopyMem (
        CommonBufferHeader->StashBuffer,
        (VOID *)(UINTN)MapInfo->CryptedAddress,
        MapInfo->NumberOfBytes
        );
      //
      // Point "DecryptionSource" to the stash buffer so that we decrypt
      // it to the original location, after the switch statement.
      //
      DecryptionSource           = CommonBufferHeader->StashBuffer;
      MapInfo->ReservedMemBitmap = CommonBufferHeader->ReservedMemBitmap;
      break;

    default:
      //
      // Operation is invalid
      //
      Status = EFI_INVALID_PARAMETER;
      goto FreeMapInfo;
  }

  if (CC_GUEST_IS_SEV (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
    //
    // Clear the memory encryption mask on the plaintext buffer.
    //
    Status = MemEncryptSevClearPageEncMask (
               0,
               MapInfo->PlainTextAddress,
               MapInfo->NumberOfPages
               );
  } else if (CC_GUEST_IS_TDX (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
    //
    // Set the memory shared bit.
    // If MapInfo->ReservedMemBitmap is 0, it means the bounce buffer is not allocated
    // from the pre-allocated shared memory, so it must be converted to shared memory here.
    //
    if (MapInfo->ReservedMemBitmap == 0) {
      Status = MemEncryptTdxSetPageSharedBit (
                 0,
                 MapInfo->PlainTextAddress,
                 MapInfo->NumberOfPages
                 );
    }
  } else {
    ASSERT (FALSE);
  }

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    CpuDeadLoop ();
  }

  //
  // If this is a read operation from the Bus Master's point of view,
  // then copy the contents of the real buffer into the mapped buffer
  // so the Bus Master can read the contents of the real buffer.
  //
  // For BusMasterCommonBuffer[64] operations, the CopyMem() below will decrypt
  // the original data (from the stash buffer) back to the original location.
  //
  if ((Operation == EdkiiIoMmuOperationBusMasterRead) ||
      (Operation == EdkiiIoMmuOperationBusMasterRead64) ||
      (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer) ||
      (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64))
  {
    CopyMem (
      (VOID *)(UINTN)MapInfo->PlainTextAddress,
      DecryptionSource,
      MapInfo->NumberOfBytes
      );
  }

  //
  // Track all MAP_INFO structures.
  //
  InsertHeadList (&mMapInfos, &MapInfo->Link);
  //
  // Populate output parameters.
  //
  *DeviceAddress = MapInfo->PlainTextAddress;
  *Mapping       = MapInfo;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Mapping=0x%p Device(PlainText)=0x%Lx Crypted=0x%Lx Pages=0x%Lx, ReservedMemBitmap=0x%Lx\n",
    __FUNCTION__,
    MapInfo,
    MapInfo->PlainTextAddress,
    MapInfo->CryptedAddress,
    (UINT64)MapInfo->NumberOfPages,
    MapInfo->ReservedMemBitmap
    ));

  return EFI_SUCCESS;

FreeMapInfo:
  FreePool (MapInfo);

Failed:
  *NumberOfBytes = 0;
  return Status;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  This is an internal worker function that only extends the Map() API with
  the MemoryMapLocked parameter.

  @param  This                  The protocol instance pointer.
  @param  Mapping               The mapping value returned from Map().
  @param  MemoryMapLocked       The function is executing on the stack of
                                gBS->ExitBootServices(); changes to the UEFI
                                memory map are forbidden.

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by
                                Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system
                                memory.
**/
STATIC
EFI_STATUS
EFIAPI
IoMmuUnmapWorker (
  IN  EDKII_IOMMU_PROTOCOL  *This,
  IN  VOID                  *Mapping,
  IN  BOOLEAN               MemoryMapLocked
  )
{
  MAP_INFO              *MapInfo;
  EFI_STATUS            Status;
  COMMON_BUFFER_HEADER  *CommonBufferHeader;
  VOID                  *EncryptionTarget;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Mapping=0x%p MemoryMapLocked=%d\n",
    __FUNCTION__,
    Mapping,
    MemoryMapLocked
    ));

  if (Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MapInfo = (MAP_INFO *)Mapping;
  Status  = EFI_SUCCESS;
  //
  // set CommonBufferHeader to suppress incorrect compiler/analyzer warnings
  //
  CommonBufferHeader = NULL;

  //
  // For BusMasterWrite[64] operations and BusMasterCommonBuffer[64] operations
  // we have to encrypt the results, ultimately to the original place (i.e.,
  // "MapInfo->CryptedAddress").
  //
  // For BusMasterCommonBuffer[64] operations however, this encryption has to
  // land in-place, so divert the encryption to the stash buffer first.
  //
  EncryptionTarget = (VOID *)(UINTN)MapInfo->CryptedAddress;

  switch (MapInfo->Operation) {
    case EdkiiIoMmuOperationBusMasterCommonBuffer:
    case EdkiiIoMmuOperationBusMasterCommonBuffer64:
      ASSERT (MapInfo->PlainTextAddress == MapInfo->CryptedAddress);

      CommonBufferHeader = (COMMON_BUFFER_HEADER *)(
                                                    (UINTN)MapInfo->PlainTextAddress - EFI_PAGE_SIZE
                                                    );
      ASSERT (CommonBufferHeader->Signature == COMMON_BUFFER_SIG);
      EncryptionTarget = CommonBufferHeader->StashBuffer;
    //
    // fall through
    //

    case EdkiiIoMmuOperationBusMasterWrite:
    case EdkiiIoMmuOperationBusMasterWrite64:
      CopyMem (
        EncryptionTarget,
        (VOID *)(UINTN)MapInfo->PlainTextAddress,
        MapInfo->NumberOfBytes
        );
      break;

    default:
      //
      // nothing to encrypt after BusMasterRead[64] operations
      //
      break;
  }

  if (CC_GUEST_IS_SEV (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
    //
    // Restore the memory encryption mask on the area we used to hold the
    // plaintext.
    //
    Status = MemEncryptSevSetPageEncMask (
               0,
               MapInfo->PlainTextAddress,
               MapInfo->NumberOfPages
               );
  } else if (CC_GUEST_IS_TDX (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
    //
    // Restore the memory shared bit mask on the area we used to hold the
    // plaintext.
    //
    if (MapInfo->ReservedMemBitmap == 0) {
      Status = MemEncryptTdxClearPageSharedBit (
                 0,
                 MapInfo->PlainTextAddress,
                 MapInfo->NumberOfPages
                 );
    }
  } else {
    ASSERT (FALSE);
  }

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    CpuDeadLoop ();
  }

  //
  // For BusMasterCommonBuffer[64] operations, copy the stashed data to the
  // original (now encrypted) location.
  //
  // For all other operations, fill the late bounce buffer (which existed as
  // plaintext at some point) with zeros, and then release it (unless the UEFI
  // memory map is locked).
  //
  if ((MapInfo->Operation == EdkiiIoMmuOperationBusMasterCommonBuffer) ||
      (MapInfo->Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64))
  {
    CopyMem (
      (VOID *)(UINTN)MapInfo->CryptedAddress,
      CommonBufferHeader->StashBuffer,
      MapInfo->NumberOfBytes
      );
  } else {
    ZeroMem (
      (VOID *)(UINTN)MapInfo->PlainTextAddress,
      EFI_PAGES_TO_SIZE (MapInfo->NumberOfPages)
      );

    if (!MemoryMapLocked) {
      FreeBounceBuffer (MapInfo);
    }
  }

  //
  // Forget the MAP_INFO structure, then free it (unless the UEFI memory map is
  // locked).
  //
  RemoveEntryList (&MapInfo->Link);
  if (!MemoryMapLocked) {
    FreePool (MapInfo);
  }

  return EFI_SUCCESS;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  The protocol instance pointer.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by
                                Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system
                                memory.
**/
EFI_STATUS
EFIAPI
IoMmuUnmap (
  IN  EDKII_IOMMU_PROTOCOL  *This,
  IN  VOID                  *Mapping
  )
{
  return IoMmuUnmapWorker (
           This,
           Mapping,
           FALSE    // MemoryMapLocked
           );
}

/**
 * Allocate CommonBuffer from pre-allocated shared memory.
 *
 * @param MemoryType          Memory type
 * @param CommonBufferPages   Pages of CommonBuffer
 * @param PhysicalAddress     Allocated physical address
 * @param ReservedMemBitmap   Bitmap which indicates the allocation of reserved memory
 * @return EFI_SUCCESS  Successfully allocate CommonBuffer
 */
STATIC
EFI_STATUS
AllocateCommonBuffer (
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
 * Free CommonBuffer which is allocated by AllocateCommonBuffer().
 *
 * @param CommonBufferHeader  Pointer to the CommonBufferHeader
 * @param CommonBufferPages   Pages of CommonBuffer
 * @return EFI_SUCCESS        Successfully free the CommonBuffer
 */
STATIC
EFI_STATUS
FreeCommonBufer (
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

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  This                  The protocol instance pointer.
  @param  Type                  This parameter is not used and must be ignored.
  @param  MemoryType            The type of memory to allocate,
                                EfiBootServicesData or EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory
                                address of the allocated range.
  @param  Attributes            The requested bit mask of attributes for the
                                allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal
                                attribute bits are MEMORY_WRITE_COMBINE and
                                MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
EFIAPI
IoMmuAllocateBuffer (
  IN     EDKII_IOMMU_PROTOCOL  *This,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN     EFI_MEMORY_TYPE       MemoryType,
  IN     UINTN                 Pages,
  IN OUT VOID                  **HostAddress,
  IN     UINT64                Attributes
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  VOID                  *StashBuffer;
  UINTN                 CommonBufferPages;
  COMMON_BUFFER_HEADER  *CommonBufferHeader;
  UINT32                ReservedMemBitmap;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: MemoryType=%u Pages=0x%Lx Attributes=0x%Lx\n",
    __FUNCTION__,
    (UINT32)MemoryType,
    (UINT64)Pages,
    Attributes
    ));

  ReservedMemBitmap = 0;

  //
  // Validate Attributes
  //
  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check for invalid inputs
  //
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The only valid memory types are EfiBootServicesData and
  // EfiRuntimeServicesData
  //
  if ((MemoryType != EfiBootServicesData) &&
      (MemoryType != EfiRuntimeServicesData))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We'll need a header page for the COMMON_BUFFER_HEADER structure.
  //
  if (Pages > MAX_UINTN - 1) {
    return EFI_OUT_OF_RESOURCES;
  }

  CommonBufferPages = Pages + 1;

  //
  // Allocate the stash in EfiBootServicesData type memory.
  //
  // Map() will temporarily save encrypted data in the stash for
  // BusMasterCommonBuffer[64] operations, so the data can be decrypted to the
  // original location.
  //
  // Unmap() will temporarily save plaintext data in the stash for
  // BusMasterCommonBuffer[64] operations, so the data can be encrypted to the
  // original location.
  //
  // StashBuffer always resides in encrypted memory.
  //
  StashBuffer = AllocatePages (Pages);
  if (StashBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PhysicalAddress = (UINTN)-1;
  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0) {
    //
    // Limit allocations to memory below 4GB
    //
    PhysicalAddress = SIZE_4GB - 1;
  }

  Status = AllocateCommonBuffer (
             MemoryType,
             CommonBufferPages,
             &PhysicalAddress,
             &ReservedMemBitmap
             );

  if (EFI_ERROR (Status)) {
    goto FreeStashBuffer;
  }

  CommonBufferHeader = (VOID *)(UINTN)PhysicalAddress;
  PhysicalAddress   += EFI_PAGE_SIZE;

  CommonBufferHeader->Signature         = COMMON_BUFFER_SIG;
  CommonBufferHeader->StashBuffer       = StashBuffer;
  CommonBufferHeader->ReservedMemBitmap = ReservedMemBitmap;

  *HostAddress = (VOID *)(UINTN)PhysicalAddress;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Host=0x%Lx Stash=0x%p\n",
    __FUNCTION__,
    PhysicalAddress,
    StashBuffer
    ));

  return EFI_SUCCESS;

FreeStashBuffer:
  FreePages (StashBuffer, Pages);
  return Status;
}

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  The protocol instance pointer.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated
                                range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and
                                Pages was not allocated with AllocateBuffer().

**/
EFI_STATUS
EFIAPI
IoMmuFreeBuffer (
  IN  EDKII_IOMMU_PROTOCOL  *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  )
{
  UINTN                 CommonBufferPages;
  COMMON_BUFFER_HEADER  *CommonBufferHeader;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Host=0x%p Pages=0x%Lx\n",
    __FUNCTION__,
    HostAddress,
    (UINT64)Pages
    ));

  CommonBufferPages  = Pages + 1;
  CommonBufferHeader = (COMMON_BUFFER_HEADER *)(
                                                (UINTN)HostAddress - EFI_PAGE_SIZE
                                                );

  //
  // Check the signature.
  //
  ASSERT (CommonBufferHeader->Signature == COMMON_BUFFER_SIG);
  if (CommonBufferHeader->Signature != COMMON_BUFFER_SIG) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Free the stash buffer. This buffer was always encrypted, so no need to
  // zero it.
  //
  FreePages (CommonBufferHeader->StashBuffer, Pages);

  //
  // Release the common buffer itself. Unmap() has re-encrypted it in-place, so
  // no need to zero it.
  //
  return FreeCommonBufer (CommonBufferHeader, CommonBufferPages);
}

/**
  Set IOMMU attribute for a system memory.

  If the IOMMU protocol exists, the system memory cannot be used
  for DMA by default.

  When a device requests a DMA access for a system memory,
  the device driver need use SetAttribute() to update the IOMMU
  attribute to request DMA access (read and/or write).

  The DeviceHandle is used to identify which device submits the request.
  The IOMMU implementation need translate the device path to an IOMMU device
  ID, and set IOMMU hardware register accordingly.
  1) DeviceHandle can be a standard PCI device.
     The memory for BusMasterRead need set EDKII_IOMMU_ACCESS_READ.
     The memory for BusMasterWrite need set EDKII_IOMMU_ACCESS_WRITE.
     The memory for BusMasterCommonBuffer need set
     EDKII_IOMMU_ACCESS_READ|EDKII_IOMMU_ACCESS_WRITE.
     After the memory is used, the memory need set 0 to keep it being
     protected.
  2) DeviceHandle can be an ACPI device (ISA, I2C, SPI, etc).
     The memory for DMA access need set EDKII_IOMMU_ACCESS_READ and/or
     EDKII_IOMMU_ACCESS_WRITE.

  @param[in]  This              The protocol instance pointer.
  @param[in]  DeviceHandle      The device who initiates the DMA access
                                request.
  @param[in]  Mapping           The mapping value returned from Map().
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range
                                 specified by DeviceAddress and Length.
  @retval EFI_INVALID_PARAMETER  DeviceHandle is an invalid handle.
  @retval EFI_INVALID_PARAMETER  Mapping is not a value that was returned by
                                 Map().
  @retval EFI_INVALID_PARAMETER  IoMmuAccess specified an illegal combination
                                 of access.
  @retval EFI_UNSUPPORTED        DeviceHandle is unknown by the IOMMU.
  @retval EFI_UNSUPPORTED        The bit mask of IoMmuAccess is not supported
                                 by the IOMMU.
  @retval EFI_UNSUPPORTED        The IOMMU does not support the memory range
                                 specified by Mapping.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to
                                 modify the IOMMU access.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while
                                 attempting the operation.

**/
EFI_STATUS
EFIAPI
IoMmuSetAttribute (
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN EFI_HANDLE            DeviceHandle,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  )
{
  return EFI_UNSUPPORTED;
}

EDKII_IOMMU_PROTOCOL  mIoMmu = {
  EDKII_IOMMU_PROTOCOL_REVISION,
  IoMmuSetAttribute,
  IoMmuMap,
  IoMmuUnmap,
  IoMmuAllocateBuffer,
  IoMmuFreeBuffer,
};

/**
  Notification function that is queued when gBS->ExitBootServices() signals the
  EFI_EVENT_GROUP_EXIT_BOOT_SERVICES event group. This function signals another
  event, received as Context, and returns.

  Signaling an event in this context is safe. The UEFI spec allows
  gBS->SignalEvent() to return EFI_SUCCESS only; EFI_OUT_OF_RESOURCES is not
  listed, hence memory is not allocated. The edk2 implementation also does not
  release memory (and we only have to care about the edk2 implementation
  because EDKII_IOMMU_PROTOCOL is edk2-specific anyway).

  @param[in] Event          Event whose notification function is being invoked.
                            Event is permitted to request the queueing of this
                            function at TPL_CALLBACK or TPL_NOTIFY task
                            priority level.

  @param[in] EventToSignal  Identifies the EFI_EVENT to signal. EventToSignal
                            is permitted to request the queueing of its
                            notification function only at TPL_CALLBACK level.
**/
STATIC
VOID
EFIAPI
IoMmuExitBoot (
  IN EFI_EVENT  Event,
  IN VOID       *EventToSignal
  )
{
  //
  // (1) The NotifyFunctions of all the events in
  //     EFI_EVENT_GROUP_EXIT_BOOT_SERVICES will have been queued before
  //     IoMmuExitBoot() is entered.
  //
  // (2) IoMmuExitBoot() is executing minimally at TPL_CALLBACK.
  //
  // (3) IoMmuExitBoot() has been queued in unspecified order relative to the
  //     NotifyFunctions of all the other events in
  //     EFI_EVENT_GROUP_EXIT_BOOT_SERVICES whose NotifyTpl is the same as
  //     Event's.
  //
  // Consequences:
  //
  // - If Event's NotifyTpl is TPL_CALLBACK, then some other NotifyFunctions
  //   queued at TPL_CALLBACK may be invoked after IoMmuExitBoot() returns.
  //
  // - If Event's NotifyTpl is TPL_NOTIFY, then some other NotifyFunctions
  //   queued at TPL_NOTIFY may be invoked after IoMmuExitBoot() returns; plus
  //   *all* NotifyFunctions queued at TPL_CALLBACK will be invoked strictly
  //   after all NotifyFunctions queued at TPL_NOTIFY, including
  //   IoMmuExitBoot(), have been invoked.
  //
  // - By signaling EventToSignal here, whose NotifyTpl is TPL_CALLBACK, we
  //   queue EventToSignal's NotifyFunction after the NotifyFunctions of *all*
  //   events in EFI_EVENT_GROUP_EXIT_BOOT_SERVICES.
  //
  DEBUG ((DEBUG_VERBOSE, "%a\n", __FUNCTION__));
  gBS->SignalEvent (EventToSignal);
}

/**
  Notification function that is queued after the notification functions of all
  events in the EFI_EVENT_GROUP_EXIT_BOOT_SERVICES event group. The same memory
  map restrictions apply.

  This function unmaps all currently existing IOMMU mappings.

  @param[in] Event    Event whose notification function is being invoked. Event
                      is permitted to request the queueing of this function
                      only at TPL_CALLBACK task priority level.

  @param[in] Context  Ignored.
**/
STATIC
VOID
EFIAPI
IoMmuUnmapAllMappings (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  LIST_ENTRY  *Node;
  LIST_ENTRY  *NextNode;
  MAP_INFO    *MapInfo;

  DEBUG ((DEBUG_VERBOSE, "%a\n", __FUNCTION__));

  //
  // All drivers that had set up IOMMU mappings have halted their respective
  // controllers by now; tear down the mappings.
  //
  for (Node = GetFirstNode (&mMapInfos); Node != &mMapInfos; Node = NextNode) {
    NextNode = GetNextNode (&mMapInfos, Node);
    MapInfo  = CR (Node, MAP_INFO, Link, MAP_INFO_SIG);
    IoMmuUnmapWorker (
      &mIoMmu,  // This
      MapInfo,  // Mapping
      TRUE      // MemoryMapLocked
      );
  }

  ReleaseReservedSharedMem (TRUE);
}

/**
  Initialize Iommu Protocol.

**/
EFI_STATUS
EFIAPI
InstallIoMmuProtocol (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   UnmapAllMappingsEvent;
  EFI_EVENT   ExitBootEvent;
  EFI_HANDLE  Handle;

  //
  // Create the "late" event whose notification function will tear down all
  // left-over IOMMU mappings.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,      // Type
                  TPL_CALLBACK,           // NotifyTpl
                  IoMmuUnmapAllMappings,  // NotifyFunction
                  NULL,                   // NotifyContext
                  &UnmapAllMappingsEvent  // Event
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create the event whose notification function will be queued by
  // gBS->ExitBootServices() and will signal the event created above.
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES, // Type
                  TPL_CALLBACK,                  // NotifyTpl
                  IoMmuExitBoot,                 // NotifyFunction
                  UnmapAllMappingsEvent,         // NotifyContext
                  &ExitBootEvent                 // Event
                  );
  if (EFI_ERROR (Status)) {
    goto CloseUnmapAllMappingsEvent;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiIoMmuProtocolGuid,
                  &mIoMmu,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto CloseExitBootEvent;
  }

  //
  // Currently only Tdx guest support Reserved shared memory for DMA operation.
  //
  if (TdIsEnabled ()) {
    mReservedSharedMemSupported = TRUE;
    Status                      = InitReservedSharedMem ();
    if (EFI_ERROR (Status)) {
      mReservedSharedMemSupported = FALSE;
    } else {
      DEBUG ((DEBUG_INFO, "%a: Feature of reserved memory for DMA is supported.\n", __FUNCTION__));
    }
  }

  return EFI_SUCCESS;

CloseExitBootEvent:
  gBS->CloseEvent (ExitBootEvent);

CloseUnmapAllMappingsEvent:
  gBS->CloseEvent (UnmapAllMappingsEvent);

  return Status;
}
