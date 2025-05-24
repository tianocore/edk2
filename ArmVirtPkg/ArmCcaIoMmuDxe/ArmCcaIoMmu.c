/** @file
  The protocol provides support to allocate, free, map and umap a DMA buffer
  for bus master (e.g PciHostBridge). When the execution context is a Realm,
  the DMA operations must be performed on buffers that are shared with the Host.
  Hence the RAMP protocol is used to manage the sharing of the DMA buffers or
  in some cases to bounce the buffers.

  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/UefiLib.h>
#include <Protocol/ResetNotification.h>

#include "ArmCcaIoMmu.h"

/** List of the MAP_INFO structures that have been set up by IoMmuMap() and not
    yet torn down by IoMmuUnmap(). The list represents the full set of mappings
    currently in effect.
*/
STATIC LIST_ENTRY  mMapInfos = INITIALIZE_LIST_HEAD_VARIABLE (mMapInfos);

/** ASCII names for EDKII_IOMMU_OPERATION constants, for debug logging.
*/
STATIC CONST CHAR8 *CONST
mBusMasterOperationName[EdkiiIoMmuOperationMaximum] = {
  "Read",
  "Write",
  "CommonBuffer",
  "Read64",
  "Write64",
  "CommonBuffer64"
};

/** Pointer to the Realm Aperture Management Protocol
*/
extern EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL  *mRamp;

/**
  Given the host address find a mapping node in the linked list.

  @param [in] HostAddress Host address.

  @return Pointer to the MapInfo node if found, otherwise NULL.
**/
STATIC
MAP_INFO *
EFIAPI
FindMappingByHostAddress (
  IN    VOID  *HostAddress
  )
{
  LIST_ENTRY  *Node;
  LIST_ENTRY  *NextNode;
  MAP_INFO    *MapInfo;

  for (Node = GetFirstNode (&mMapInfos); Node != &mMapInfos; Node = NextNode) {
    NextNode = GetNextNode (&mMapInfos, Node);
    MapInfo  = CR (Node, MAP_INFO, Link, MAP_INFO_SIG);
    if (MapInfo->HostAddress == HostAddress) {
      return MapInfo;
    }
  }

  return NULL;
}

/**
  Map a shared buffer

  @param [in]   Operation       IoMMU operation to perform.
  @param [in]   HostAddress     Pointer to the Host buffer.
  @param [in]   NumberOfBytes   Number of bytes to map.
  @param [in]   BbAddress       Bounce buffer address.
  @param [in]   BbPages         Number of pages covering the bounce buffer.
  @param [out]  Mapping         Pointer to the MapInfo node.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES      Failed to allocate memory.
**/
STATIC
EFI_STATUS
MapSharedBuffer (
  IN    EDKII_IOMMU_OPERATION  Operation,
  IN    VOID                   *HostAddress,
  IN    UINTN                  NumberOfBytes,
  IN    EFI_PHYSICAL_ADDRESS   BbAddress,
  IN    UINTN                  BbPages,
  OUT   MAP_INFO               **Mapping
  )
{
  EFI_STATUS  Status;
  MAP_INFO    *MapInfo;

  if (BbPages != EFI_SIZE_TO_PAGES (NumberOfBytes)) {
    return EFI_INVALID_PARAMETER;
  }

  // Allocate a MAP_INFO structure to remember the mapping when Unmap() is
  // called later.
  MapInfo = AllocateZeroPool (sizeof (MAP_INFO));
  if (MapInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&MapInfo->Link);

  // Initialize the MAP_INFO structure, except the NonParAddress field
  MapInfo->Signature     = MAP_INFO_SIG;
  MapInfo->Operation     = Operation;
  MapInfo->NumberOfBytes = NumberOfBytes;
  MapInfo->NumberOfPages = BbPages;
  MapInfo->HostAddress   = HostAddress;
  MapInfo->BbAddress     = BbAddress;

  // Open aperture here
  Status = mRamp->OpenAperture (
                    BbAddress,
                    BbPages,
                    &MapInfo->ApertureRef
                    );
  if (EFI_ERROR (Status)) {
    goto FreeMapInfo;
  }

  // Track all MAP_INFO structures.
  InsertHeadList (&mMapInfos, &MapInfo->Link);
  *Mapping = MapInfo;
  return Status;

FreeMapInfo:
  FreePool (MapInfo);
  return Status;
}

/**
   Unmap a shared buffer.

  @param [in] MapInfo           Pointer to the MapInfo node.
  @param [in] MemoryMapLocked   The function is executing on the stack of
                                gBS->ExitBootServices(); changes to the UEFI
                                memory map are forbidden.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
UnMapSharedBuffer (
  IN  MAP_INFO  *MapInfo,
  IN  BOOLEAN   MemoryMapLocked
  )
{
  EFI_STATUS  Status;

  if (MapInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsNodeInList (&mMapInfos, &MapInfo->Link)) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: HostAddress = 0x%p, BbAddress = 0x%p\n",
    __func__,
    MapInfo->HostAddress,
    MapInfo->BbAddress
    ));
  Status = mRamp->CloseAperture (MapInfo->ApertureRef, MemoryMapLocked);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to close aperture. Status = %r\n",
      Status
      ));
  }

  RemoveEntryList (&MapInfo->Link);

  if (!MemoryMapLocked) {
    ZeroMem (MapInfo, sizeof (MAP_INFO));
    FreePool (MapInfo);
  } else {
    // Changes to the UEFI memory map are forbidden.
    // So just zero out the memory.
    ZeroMem (MapInfo, sizeof (MAP_INFO));
  }

  return Status;
}

/**
  Provides the controller-specific addresses required to access system memory
  from a DMA bus master. On guest Realms, the DMA operations must be performed
  on shared buffer hence we allocate a bounce buffer to map the HostAddress to
  a DeviceAddress. The Realm Aperture Management protocol is then involved to
  open the aperture for sharing the buffer pages with the Host OS.

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
  EFI_PHYSICAL_ADDRESS  BbAddress;
  UINTN                 Pages;
  EFI_ALLOCATE_TYPE     AllocateType;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Operation=%a Host=0x%p Bytes=0x%lx\n",
    __func__,
    ((Operation >= 0 &&
      Operation < ARRAY_SIZE (mBusMasterOperationName)) ?
     mBusMasterOperationName[Operation] :
     "Invalid"),
    HostAddress,
    (UINT64)((NumberOfBytes == NULL) ? 0 : *NumberOfBytes)
    ));

  if ((HostAddress == NULL)                     ||
      (NumberOfBytes == NULL)                   ||
      (DeviceAddress == NULL)                   ||
      (Mapping == NULL)                         ||
      (Operation >= EdkiiIoMmuOperationMaximum) ||
      (Operation < EdkiiIoMmuOperationBusMasterRead))
  {
    return EFI_INVALID_PARAMETER;
  }

  BbAddress    = MAX_ADDRESS;
  Pages        = EFI_SIZE_TO_PAGES (*NumberOfBytes);
  AllocateType = AllocateAnyPages;
  switch (Operation) {
    // For BusMasterRead[64] and BusMasterWrite[64] operations, a bounce buffer
    // is necessary as the original buffer may not meet the page start/end and
    // page size alignment requirements. Also we need to consider the case where
    // the original buffer crosses the 4GB limit.
    case EdkiiIoMmuOperationBusMasterRead:
    case EdkiiIoMmuOperationBusMasterWrite:
      BbAddress    = BASE_4GB - 1;
      AllocateType = AllocateMaxAddress;
    // fall through
    case EdkiiIoMmuOperationBusMasterRead64:
    case EdkiiIoMmuOperationBusMasterWrite64:
      // Allocate a bounce buffer.
      Status = gBS->AllocatePages (
                      AllocateType,
                      EfiBootServicesData,
                      Pages,
                      &BbAddress
                      );
      if (EFI_ERROR (Status)) {
        goto Failed;
      }

      // Open aperture here
      Status = MapSharedBuffer (
                 Operation,
                 HostAddress,
                 *NumberOfBytes,
                 BbAddress,
                 Pages,
                 &MapInfo
                 );
      if (EFI_ERROR (Status)) {
        goto FreeBounceBuffer;
      }

      break;

    // For BusMasterCommonBuffer[64] operations, the buffer is already allocated
    // and mapped in a call to AllocateBuffer(). So, we only need to return the
    // device address and the mapping info
    case EdkiiIoMmuOperationBusMasterCommonBuffer:
    // fall through
    case EdkiiIoMmuOperationBusMasterCommonBuffer64:
      MapInfo = FindMappingByHostAddress (HostAddress);
      if (MapInfo == NULL) {
        ASSERT (MapInfo == NULL);
        goto Failed;
      }

      BbAddress = MapInfo->BbAddress;
      break;

    default:
      // Operation is invalid
      Status = EFI_INVALID_PARAMETER;
      goto Failed;
  } // switch

  // If this is a read operation from the Bus Master's point of view,
  // then copy the contents of the real buffer into the mapped buffer
  // so the Bus Master can read the contents of the real buffer.
  // No special action is needed for BusMasterCommonBuffer[64] operations.
  if ((Operation == EdkiiIoMmuOperationBusMasterRead) ||
      (Operation == EdkiiIoMmuOperationBusMasterRead64))
  {
    CopyMem (
      (VOID *)(UINTN)BbAddress,
      (VOID *)(UINTN)HostAddress,
      MapInfo->NumberOfBytes
      );
  }

  // Populate output parameters.
  *DeviceAddress = BbAddress;
  *Mapping       = MapInfo;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Mapping=0x%p HostAddress = 0x%p BBAddress = 0x%Lx Pages=0x%Lx\n",
    __func__,
    MapInfo,
    HostAddress,
    MapInfo->BbAddress,
    MapInfo->NumberOfPages
    ));

  return EFI_SUCCESS;

FreeBounceBuffer:
  gBS->FreePages (BbAddress, Pages);

Failed:
  *NumberOfBytes = 0;
  return Status;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  This is an internal worker function that only extends the Map() API with
  the MemoryMapLocked parameter.

  @param  This                  The protocol instance pointer.
  @param  MapInfo               The mapping value returned from Map().
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
  IN  MAP_INFO              *MapInfo,
  IN  BOOLEAN               MemoryMapLocked
  )
{
  EFI_STATUS        Status;
  PHYSICAL_ADDRESS  BbAddress;
  UINTN             Pages;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: MapInfo=0x%p MemoryMapLocked=%d\n",
    __func__,
    MapInfo,
    MemoryMapLocked
    ));

  if (MapInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BbAddress = MapInfo->BbAddress;
  Pages     = MapInfo->NumberOfPages;

  // For BusMasterWrite[64] operations and BusMasterCommonBuffer[64] operations
  // we have to copy the results, ultimately to the original place (i.e.,
  // "MapInfo->HostAddress").
  // No special operaton is needed for BusMasterCommonBuffer[64] operations.
  switch (MapInfo->Operation) {
    case EdkiiIoMmuOperationBusMasterCommonBuffer:
    case EdkiiIoMmuOperationBusMasterCommonBuffer64:
      ASSERT (BbAddress == (PHYSICAL_ADDRESS)MapInfo->HostAddress);
      break;
    case EdkiiIoMmuOperationBusMasterWrite:
    case EdkiiIoMmuOperationBusMasterWrite64:
      CopyMem (
        (VOID *)(UINTN)MapInfo->HostAddress,
        (VOID *)(UINTN)BbAddress,
        MapInfo->NumberOfBytes
        );
      break;

    default:
      // nothing to do for BusMasterRead[64] operations
      break;
  }

  // For all other operations, fill the late bounce buffer with zeros, and
  // then release it (unless the UEFI memory map is locked).
  if ((MapInfo->Operation != EdkiiIoMmuOperationBusMasterCommonBuffer) &&
      (MapInfo->Operation != EdkiiIoMmuOperationBusMasterCommonBuffer64))
  {
    ZeroMem (
      (VOID *)(UINTN)BbAddress,
      EFI_PAGES_TO_SIZE (Pages)
      );

    // UnMapSharedPages
    Status = UnMapSharedBuffer (MapInfo, MemoryMapLocked);
    ASSERT_EFI_ERROR (Status);

    if (!MemoryMapLocked) {
      gBS->FreePages (BbAddress, Pages);
    }
  }

  return Status;
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
           (MAP_INFO *)Mapping,
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
  EFI_PHYSICAL_ADDRESS  BbAddress;
  MAP_INFO              *MapInfo;

  // Validate Attributes
  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) != 0) {
    return EFI_UNSUPPORTED;
  }

  // Check for invalid inputs
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // The only valid memory types are EfiBootServicesData
  if (MemoryType != EfiBootServicesData) {
    return EFI_INVALID_PARAMETER;
  }

  BbAddress = (UINTN)-1;
  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0) {
    // Limit allocations to memory below 4GB
    BbAddress = BASE_4GB - 1;
  }

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  MemoryType,
                  Pages,
                  &BbAddress
                  );
  if (EFI_ERROR (Status)) {
    // Set the host address to NULL in case of error
    *HostAddress = NULL;
  } else {
    *HostAddress = (VOID *)(UINTN)BbAddress;
    Status       = MapSharedBuffer (
                     EdkiiIoMmuOperationBusMasterCommonBuffer,
                     *HostAddress,
                     EFI_PAGES_TO_SIZE (Pages),
                     BbAddress,
                     Pages,
                     &MapInfo
                     );
    ASSERT_EFI_ERROR (Status);
  }

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
  EFI_STATUS  Status;
  MAP_INFO    *MapInfo;

  // Release the common buffer itself. Unmap() has re-encrypted it in-place, so
  // no need to zero it.
  MapInfo = FindMappingByHostAddress (HostAddress);
  if (MapInfo == NULL) {
    ASSERT (0);
    return EFI_NOT_FOUND;
  } else {
    // UnMapSharedPages
    Status = UnMapSharedBuffer (MapInfo, FALSE);
    ASSERT_EFI_ERROR (Status);
  }

  return gBS->FreePages ((UINTN)HostAddress, Pages);
}

/**
  Set IOMMU attribute for a system memory.

  If the IOMMU protocol exists, the system memory cannot be used
  for DMA by default.

  When a device requests a DMA access to system memory,
  the device driver need use SetAttribute() to update the IOMMU
  attribute to request DMA access (read and/or write).

  The DeviceHandle is used to identify which device submits the request.
  The IOMMU implementation need to translate the device path to an IOMMU device
  ID, and set the IOMMU hardware register accordingly.
  1) DeviceHandle can be a standard PCI device.
     The memory for BusMasterRead needs EDKII_IOMMU_ACCESS_READ set.
     The memory for BusMasterWrite needs EDKII_IOMMU_ACCESS_WRITE set.
     The memory for BusMasterCommonBuffer needs
     EDKII_IOMMU_ACCESS_READ|EDKII_IOMMU_ACCESS_WRITE set.
     After the memory is used, the memory need set 0 to keep it being
     protected.
  2) DeviceHandle can be an ACPI device (ISA, I2C, SPI, etc).
     The memory for DMA access need set EDKII_IOMMU_ACCESS_READ and/or
     EDKII_IOMMU_ACCESS_WRITE.

  @param[in]  This              The protocol instance pointer.
  @param[in]  DeviceHandle      The device initiating the DMA access
                                request.
  @param[in]  Mapping           The mapping value returned from Map().
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_UNSUPPORTED         The requested operation is not supported.
  @retval EFI_SUCCESS             Success.

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
  EFI_STATUS  Status;
  MAP_INFO    *MapInfo;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Mapping=0x%p Access=%lu\n",
    __func__,
    Mapping,
    IoMmuAccess
    ));

  if (Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  // An IoMmuAccess value of 0 is always accepted,
  // validate any non-zero value.
  if (IoMmuAccess != 0) {
    MapInfo = (MAP_INFO *)Mapping;

    // The mapping operation already implied the access mode.
    // Validate that the supplied access mode matches operation
    // access mode.
    switch (MapInfo->Operation) {
      case EdkiiIoMmuOperationBusMasterRead:
      case EdkiiIoMmuOperationBusMasterRead64:
        if (IoMmuAccess != EDKII_IOMMU_ACCESS_READ) {
          Status = EFI_INVALID_PARAMETER;
        }

        break;

      case EdkiiIoMmuOperationBusMasterWrite:
      case EdkiiIoMmuOperationBusMasterWrite64:
        if (IoMmuAccess != EDKII_IOMMU_ACCESS_WRITE) {
          Status = EFI_INVALID_PARAMETER;
        }

        break;

      case EdkiiIoMmuOperationBusMasterCommonBuffer:
      case EdkiiIoMmuOperationBusMasterCommonBuffer64:
        if (IoMmuAccess !=
            (EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE))
        {
          Status = EFI_INVALID_PARAMETER;
        }

        break;

      default:
        Status = EFI_UNSUPPORTED;
    } // switch
  }

  return Status;
}

/** Arm CCA IoMMU protocol
*/
EDKII_IOMMU_PROTOCOL  mArmCcaIoMmu = {
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
ArmCcaIoMmuExitBoot (
  IN EFI_EVENT  Event,
  IN VOID       *EventToSignal
  )
{
  // (1) The NotifyFunctions of all the events in
  //     EFI_EVENT_GROUP_EXIT_BOOT_SERVICES will have been queued before
  //     ArmCcaIoMmuExitBoot() is entered.
  //
  // (2) ArmCcaIoMmuExitBoot() is executing minimally at TPL_CALLBACK.
  //
  // (3) ArmCcaIoMmuExitBoot() has been queued in unspecified order relative
  //      to the NotifyFunctions of all the other events in
  //     EFI_EVENT_GROUP_EXIT_BOOT_SERVICES whose NotifyTpl is the same as
  //     Event's.
  //
  // Consequences:
  //
  // - If Event's NotifyTpl is TPL_CALLBACK, then some other NotifyFunctions
  //   queued at TPL_CALLBACK may be invoked after ArmCcaIoMmuExitBoot()
  //   returns.
  //
  // - If Event's NotifyTpl is TPL_NOTIFY, then some other NotifyFunctions
  //   queued at TPL_NOTIFY may be invoked after ArmCcaIoMmuExitBoot() returns;
  //   plus *all* NotifyFunctions queued at TPL_CALLBACK will be invoked
  //   strictly after all NotifyFunctions queued at TPL_NOTIFY, including
  //   ArmCcaIoMmuExitBoot(), have been invoked.
  //
  // - By signaling EventToSignal here, whose NotifyTpl is TPL_CALLBACK, we
  //   queue EventToSignal's NotifyFunction after the NotifyFunctions of *all*
  //   events in EFI_EVENT_GROUP_EXIT_BOOT_SERVICES.
  gBS->SignalEvent (EventToSignal);
}

/**
  Unmap all currently existing IOMMU mappings.

**/
STATIC
VOID
EFIAPI
ArmCcaIoMmuUnmapAllMappings (
  VOID
  )
{
  LIST_ENTRY  *Node;
  LIST_ENTRY  *NextNode;
  MAP_INFO    *MapInfo;

  // All drivers that had set up IOMMU mappings have halted their respective
  // controllers by now; tear down the mappings.
  for (Node = GetFirstNode (&mMapInfos); Node != &mMapInfos; Node = NextNode) {
    NextNode = GetNextNode (&mMapInfos, Node);
    MapInfo  = CR (Node, MAP_INFO, Link, MAP_INFO_SIG);
    IoMmuUnmapWorker (
      &mArmCcaIoMmu, // This
      MapInfo,       // Mapping
      TRUE           // MemoryMapLocked
      );
  }
}

/**
  Notification function that is queued after the notification functions of all
  events in the EFI_EVENT_GROUP_EXIT_BOOT_SERVICES event group. The same memory
  map restrictions apply.

  @param[in] Event    Event whose notification function is being invoked. Event
                      is permitted to request the queueing of this function
                      only at TPL_CALLBACK task priority level.

  @param[in] Context  Ignored.
**/
STATIC
VOID
EFIAPI
ArmCcaIoMmuUnmapAllMappingsEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((
    DEBUG_INFO,
    "ArmCcaIoMmu: Unmapping all Mappings on ExitBootServices.\n"
    ));
  ArmCcaIoMmuUnmapAllMappings ();
}

/**
  This routine is called to unmap all mappings before system reset.

  @param[in]  ResetType    The type of reset to perform.
  @param[in]  ResetStatus  The status code for the reset.
  @param[in]  DataSize     The size, in bytes, of ResetData.
  @param[in]  ResetData    For a ResetType of EfiResetCold, EfiResetWarm, or
                           EfiResetShutdown the data buffer starts with a Null-
                           terminated string, optionally followed by additional
                           binary data. The string is a description that the
                           caller may use to further indicate the reason for
                           the system reset. ResetData is only valid if
                           ResetStatus is something other than EFI_SUCCESS
                           unless the ResetType is EfiResetPlatformSpecific
                           where a minimum amount of ResetData is always
                           required.
                           For a ResetType of EfiResetPlatformSpecific the data
                           buffer also starts with a Null-terminated string
                           that is followed by an EFI_GUID that describes the
                           specific type of reset to perform.
**/
STATIC
VOID
EFIAPI
OnResetEvent (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  DEBUG ((DEBUG_INFO, "ArmCcaIoMmu: Unmapping all Mappings on Reset.\n"));
  ArmCcaIoMmuUnmapAllMappings ();
}

/**
  Hook the system reset to unmap all mappings.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
STATIC
VOID
EFIAPI
OnResetNotificationInstall (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                       Status;
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotify;

  Status = gBS->LocateProtocol (
                  &gEfiResetNotificationProtocolGuid,
                  NULL,
                  (VOID **)&ResetNotify
                  );
  if (!EFI_ERROR (Status)) {
    Status = ResetNotify->RegisterResetNotify (ResetNotify, OnResetEvent);
    ASSERT_EFI_ERROR (Status);
    DEBUG ((
      DEBUG_INFO,
      "ArmCcaIoMmu: Hook system reset to unmap all mappings.\n"
      ));
    gBS->CloseEvent (Event);
  }
}

/**
  Initialize and install the ArmCca IoMmu Protocol.

  @return RETURN_SUCCESS if successful, otherwise any other error.
**/
EFI_STATUS
EFIAPI
ArmCcaInstallIoMmuProtocol (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   UnmapAllMappingsEvent;
  EFI_EVENT   ExitBootEvent;
  EFI_HANDLE  Handle;
  VOID        *Registration;

  // Create the "late" event whose notification function will tear down all
  // left-over IOMMU mappings.
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,                  // Type
                  TPL_CALLBACK,                       // NotifyTpl
                  ArmCcaIoMmuUnmapAllMappingsEvent,   // NotifyFunction
                  NULL,                               // NotifyContext
                  &UnmapAllMappingsEvent              // Event
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Create the event whose notification function will be queued by
  // gBS->ExitBootServices() and will signal the event created above.
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES, // Type
                  TPL_CALLBACK,                  // NotifyTpl
                  ArmCcaIoMmuExitBoot,           // NotifyFunction
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
                  &mArmCcaIoMmu,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    // Hook the system reset to tear down all mappings on reset.
    EfiCreateProtocolNotifyEvent (
      &gEfiResetNotificationProtocolGuid,
      TPL_CALLBACK,
      OnResetNotificationInstall,
      NULL,
      &Registration
      );
    return Status;
  }

  // cleanup on error
  gBS->CloseEvent (ExitBootEvent);

CloseUnmapAllMappingsEvent:
  gBS->CloseEvent (UnmapAllMappingsEvent);

  return Status;
}
