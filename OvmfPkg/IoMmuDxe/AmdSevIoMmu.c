/** @file

  The protocol provides support to allocate, free, map and umap a DMA buffer
  for bus master (e.g PciHostBridge). When SEV is enabled, the DMA operations
  must be performed on unencrypted buffer hence we use a bounce buffer to map
  the guest buffer into an unencrypted DMA buffer.

  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

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
  // Followed by the actual common buffer, starting at the next page.
  //
} COMMON_BUFFER_HEADER;
#pragma pack ()

/**
  Provides the controller-specific addresses required to access system memory
  from a DMA bus master. On SEV guest, the DMA operations must be performed on
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
  MapInfo->Signature      = MAP_INFO_SIG;
  MapInfo->Operation      = Operation;
  MapInfo->NumberOfBytes  = *NumberOfBytes;
  MapInfo->NumberOfPages  = EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes);
  MapInfo->CryptedAddress = (UINTN)HostAddress;

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
      Status = gBS->AllocatePages (
                      AllocateType,
                      EfiBootServicesData,
                      MapInfo->NumberOfPages,
                      &MapInfo->PlainTextAddress
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
      DecryptionSource = CommonBufferHeader->StashBuffer;
      break;

    default:
      //
      // Operation is invalid
      //
      Status = EFI_INVALID_PARAMETER;
      goto FreeMapInfo;
  }

  //
  // Clear the memory encryption mask on the plaintext buffer.
  //
  Status = MemEncryptSevClearPageEncMask (
             0,
             MapInfo->PlainTextAddress,
             MapInfo->NumberOfPages
             );
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
    "%a: Mapping=0x%p Device(PlainText)=0x%Lx Crypted=0x%Lx Pages=0x%Lx\n",
    __FUNCTION__,
    MapInfo,
    MapInfo->PlainTextAddress,
    MapInfo->CryptedAddress,
    (UINT64)MapInfo->NumberOfPages
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

  //
  // Restore the memory encryption mask on the area we used to hold the
  // plaintext.
  //
  Status = MemEncryptSevSetPageEncMask (
             0,
             MapInfo->PlainTextAddress,
             MapInfo->NumberOfPages
             );
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
      gBS->FreePages (MapInfo->PlainTextAddress, MapInfo->NumberOfPages);
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

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: MemoryType=%u Pages=0x%Lx Attributes=0x%Lx\n",
    __FUNCTION__,
    (UINT32)MemoryType,
    (UINT64)Pages,
    Attributes
    ));

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

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  MemoryType,
                  CommonBufferPages,
                  &PhysicalAddress
                  );
  if (EFI_ERROR (Status)) {
    goto FreeStashBuffer;
  }

  CommonBufferHeader = (VOID *)(UINTN)PhysicalAddress;
  PhysicalAddress   += EFI_PAGE_SIZE;

  CommonBufferHeader->Signature   = COMMON_BUFFER_SIG;
  CommonBufferHeader->StashBuffer = StashBuffer;

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
  return gBS->FreePages ((UINTN)CommonBufferHeader, CommonBufferPages);
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

EDKII_IOMMU_PROTOCOL  mAmdSev = {
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
AmdSevExitBoot (
  IN EFI_EVENT  Event,
  IN VOID       *EventToSignal
  )
{
  //
  // (1) The NotifyFunctions of all the events in
  //     EFI_EVENT_GROUP_EXIT_BOOT_SERVICES will have been queued before
  //     AmdSevExitBoot() is entered.
  //
  // (2) AmdSevExitBoot() is executing minimally at TPL_CALLBACK.
  //
  // (3) AmdSevExitBoot() has been queued in unspecified order relative to the
  //     NotifyFunctions of all the other events in
  //     EFI_EVENT_GROUP_EXIT_BOOT_SERVICES whose NotifyTpl is the same as
  //     Event's.
  //
  // Consequences:
  //
  // - If Event's NotifyTpl is TPL_CALLBACK, then some other NotifyFunctions
  //   queued at TPL_CALLBACK may be invoked after AmdSevExitBoot() returns.
  //
  // - If Event's NotifyTpl is TPL_NOTIFY, then some other NotifyFunctions
  //   queued at TPL_NOTIFY may be invoked after AmdSevExitBoot() returns; plus
  //   *all* NotifyFunctions queued at TPL_CALLBACK will be invoked strictly
  //   after all NotifyFunctions queued at TPL_NOTIFY, including
  //   AmdSevExitBoot(), have been invoked.
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
AmdSevUnmapAllMappings (
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
      &mAmdSev, // This
      MapInfo,  // Mapping
      TRUE      // MemoryMapLocked
      );
  }
}

/**
  Initialize Iommu Protocol.

**/
EFI_STATUS
EFIAPI
AmdSevInstallIoMmuProtocol (
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
                  AmdSevUnmapAllMappings, // NotifyFunction
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
                  AmdSevExitBoot,                // NotifyFunction
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
                  &mAmdSev,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto CloseExitBootEvent;
  }

  return EFI_SUCCESS;

CloseExitBootEvent:
  gBS->CloseEvent (ExitBootEvent);

CloseUnmapAllMappingsEvent:
  gBS->CloseEvent (UnmapAllMappingsEvent);

  return Status;
}
