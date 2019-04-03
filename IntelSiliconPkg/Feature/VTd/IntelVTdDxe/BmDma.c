/** @file
  BmDma related function

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DmaProtection.h"

// TBD: May make it a policy
#define DMA_MEMORY_TOP          MAX_UINTN
//#define DMA_MEMORY_TOP          0x0000000001FFFFFFULL

#define MAP_HANDLE_INFO_SIGNATURE  SIGNATURE_32 ('H', 'M', 'A', 'P')
typedef struct {
  UINT32                                    Signature;
  LIST_ENTRY                                Link;
  EFI_HANDLE                                DeviceHandle;
  UINT64                                    IoMmuAccess;
} MAP_HANDLE_INFO;
#define MAP_HANDLE_INFO_FROM_LINK(a) CR (a, MAP_HANDLE_INFO, Link, MAP_HANDLE_INFO_SIGNATURE)

#define MAP_INFO_SIGNATURE  SIGNATURE_32 ('D', 'M', 'A', 'P')
typedef struct {
  UINT32                                    Signature;
  LIST_ENTRY                                Link;
  EDKII_IOMMU_OPERATION                     Operation;
  UINTN                                     NumberOfBytes;
  UINTN                                     NumberOfPages;
  EFI_PHYSICAL_ADDRESS                      HostAddress;
  EFI_PHYSICAL_ADDRESS                      DeviceAddress;
  LIST_ENTRY                                HandleList;
} MAP_INFO;
#define MAP_INFO_FROM_LINK(a) CR (a, MAP_INFO, Link, MAP_INFO_SIGNATURE)

LIST_ENTRY                        gMaps = INITIALIZE_LIST_HEAD_VARIABLE(gMaps);

/**
  This function fills DeviceHandle/IoMmuAccess to the MAP_HANDLE_INFO,
  based upon the DeviceAddress.

  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[in]  DeviceAddress     The base of device memory address to be used as the DMA memory.
  @param[in]  Length            The length of device memory address to be used as the DMA memory.
  @param[in]  IoMmuAccess       The IOMMU access.

**/
VOID
SyncDeviceHandleToMapInfo (
  IN EFI_HANDLE            DeviceHandle,
  IN EFI_PHYSICAL_ADDRESS  DeviceAddress,
  IN UINT64                Length,
  IN UINT64                IoMmuAccess
  )
{
  MAP_INFO                 *MapInfo;
  MAP_HANDLE_INFO          *MapHandleInfo;
  LIST_ENTRY               *Link;
  EFI_TPL                  OriginalTpl;

  //
  // Find MapInfo according to DeviceAddress
  //
  OriginalTpl = gBS->RaiseTPL (VTD_TPL_LEVEL);
  MapInfo = NULL;
  for (Link = GetFirstNode (&gMaps)
       ; !IsNull (&gMaps, Link)
       ; Link = GetNextNode (&gMaps, Link)
       ) {
    MapInfo = MAP_INFO_FROM_LINK (Link);
    if (MapInfo->DeviceAddress == DeviceAddress) {
      break;
    }
  }
  if ((MapInfo == NULL) || (MapInfo->DeviceAddress != DeviceAddress)) {
    DEBUG ((DEBUG_ERROR, "SyncDeviceHandleToMapInfo: DeviceAddress(0x%lx) - not found\n", DeviceAddress));
    gBS->RestoreTPL (OriginalTpl);
    return ;
  }

  //
  // Find MapHandleInfo according to DeviceHandle
  //
  MapHandleInfo = NULL;
  for (Link = GetFirstNode (&MapInfo->HandleList)
       ; !IsNull (&MapInfo->HandleList, Link)
       ; Link = GetNextNode (&MapInfo->HandleList, Link)
       ) {
    MapHandleInfo = MAP_HANDLE_INFO_FROM_LINK (Link);
    if (MapHandleInfo->DeviceHandle == DeviceHandle) {
      break;
    }
  }
  if ((MapHandleInfo != NULL) && (MapHandleInfo->DeviceHandle == DeviceHandle)) {
    MapHandleInfo->IoMmuAccess       = IoMmuAccess;
    gBS->RestoreTPL (OriginalTpl);
    return ;
  }

  //
  // No DeviceHandle
  // Initialize and insert the MAP_HANDLE_INFO structure
  //
  MapHandleInfo = AllocatePool (sizeof (MAP_HANDLE_INFO));
  if (MapHandleInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "SyncDeviceHandleToMapInfo: %r\n", EFI_OUT_OF_RESOURCES));
    gBS->RestoreTPL (OriginalTpl);
    return ;
  }

  MapHandleInfo->Signature         = MAP_HANDLE_INFO_SIGNATURE;
  MapHandleInfo->DeviceHandle      = DeviceHandle;
  MapHandleInfo->IoMmuAccess       = IoMmuAccess;

  InsertTailList (&MapInfo->HandleList, &MapHandleInfo->Link);
  gBS->RestoreTPL (OriginalTpl);

  return ;
}

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param  This                  The protocol instance pointer.
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
EFIAPI
IoMmuMap (
  IN     EDKII_IOMMU_PROTOCOL                       *This,
  IN     EDKII_IOMMU_OPERATION                      Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  )
{
  EFI_STATUS                                        Status;
  EFI_PHYSICAL_ADDRESS                              PhysicalAddress;
  MAP_INFO                                          *MapInfo;
  EFI_PHYSICAL_ADDRESS                              DmaMemoryTop;
  BOOLEAN                                           NeedRemap;
  EFI_TPL                                           OriginalTpl;

  if (NumberOfBytes == NULL || DeviceAddress == NULL ||
      Mapping == NULL) {
    DEBUG ((DEBUG_ERROR, "IoMmuMap: %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "IoMmuMap: ==> 0x%08x - 0x%08x (%x)\n", HostAddress, *NumberOfBytes, Operation));

  //
  // Make sure that Operation is valid
  //
  if ((UINT32) Operation >= EdkiiIoMmuOperationMaximum) {
    DEBUG ((DEBUG_ERROR, "IoMmuMap: %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }
  NeedRemap = FALSE;
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress;

  DmaMemoryTop = DMA_MEMORY_TOP;

  //
  // Alignment check
  //
  if ((*NumberOfBytes != ALIGN_VALUE(*NumberOfBytes, SIZE_4KB)) ||
      (PhysicalAddress != ALIGN_VALUE(PhysicalAddress, SIZE_4KB))) {
    if ((Operation == EdkiiIoMmuOperationBusMasterCommonBuffer) ||
        (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64)) {
      //
      // The input buffer might be a subset from IoMmuAllocateBuffer.
      // Skip the check.
      //
    } else {
      NeedRemap = TRUE;
    }
  }

  if ((PhysicalAddress + *NumberOfBytes) >= DMA_MEMORY_TOP) {
    NeedRemap = TRUE;
  }

  if (((Operation != EdkiiIoMmuOperationBusMasterRead64 &&
        Operation != EdkiiIoMmuOperationBusMasterWrite64 &&
        Operation != EdkiiIoMmuOperationBusMasterCommonBuffer64)) &&
      ((PhysicalAddress + *NumberOfBytes) > SIZE_4GB)) {
    //
    // If the root bridge or the device cannot handle performing DMA above
    // 4GB but any part of the DMA transfer being mapped is above 4GB, then
    // map the DMA transfer to a buffer below 4GB.
    //
    NeedRemap = TRUE;
    DmaMemoryTop = MIN (DmaMemoryTop, SIZE_4GB - 1);
  }

  if (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer ||
      Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64) {
    if (NeedRemap) {
      //
      // Common Buffer operations can not be remapped.  If the common buffer
      // is above 4GB, then it is not possible to generate a mapping, so return
      // an error.
      //
      DEBUG ((DEBUG_ERROR, "IoMmuMap: %r\n", EFI_UNSUPPORTED));
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Allocate a MAP_INFO structure to remember the mapping when Unmap() is
  // called later.
  //
  MapInfo = AllocatePool (sizeof (MAP_INFO));
  if (MapInfo == NULL) {
    *NumberOfBytes = 0;
    DEBUG ((DEBUG_ERROR, "IoMmuMap: %r\n", EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the MAP_INFO structure
  //
  MapInfo->Signature         = MAP_INFO_SIGNATURE;
  MapInfo->Operation         = Operation;
  MapInfo->NumberOfBytes     = *NumberOfBytes;
  MapInfo->NumberOfPages     = EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes);
  MapInfo->HostAddress       = PhysicalAddress;
  MapInfo->DeviceAddress     = DmaMemoryTop;
  InitializeListHead(&MapInfo->HandleList);

  //
  // Allocate a buffer below 4GB to map the transfer to.
  //
  if (NeedRemap) {
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    MapInfo->NumberOfPages,
                    &MapInfo->DeviceAddress
                    );
    if (EFI_ERROR (Status)) {
      FreePool (MapInfo);
      *NumberOfBytes = 0;
      DEBUG ((DEBUG_ERROR, "IoMmuMap: %r\n", Status));
      return Status;
    }

    //
    // If this is a read operation from the Bus Master's point of view,
    // then copy the contents of the real buffer into the mapped buffer
    // so the Bus Master can read the contents of the real buffer.
    //
    if (Operation == EdkiiIoMmuOperationBusMasterRead ||
        Operation == EdkiiIoMmuOperationBusMasterRead64) {
      CopyMem (
        (VOID *) (UINTN) MapInfo->DeviceAddress,
        (VOID *) (UINTN) MapInfo->HostAddress,
        MapInfo->NumberOfBytes
        );
    }
  } else {
    MapInfo->DeviceAddress = MapInfo->HostAddress;
  }

  OriginalTpl = gBS->RaiseTPL (VTD_TPL_LEVEL);
  InsertTailList (&gMaps, &MapInfo->Link);
  gBS->RestoreTPL (OriginalTpl);

  //
  // The DeviceAddress is the address of the maped buffer below 4GB
  //
  *DeviceAddress = MapInfo->DeviceAddress;
  //
  // Return a pointer to the MAP_INFO structure in Mapping
  //
  *Mapping       = MapInfo;

  DEBUG ((DEBUG_VERBOSE, "IoMmuMap: 0x%08x - 0x%08x <==\n", *DeviceAddress, *Mapping));

  return EFI_SUCCESS;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  The protocol instance pointer.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
**/
EFI_STATUS
EFIAPI
IoMmuUnmap (
  IN  EDKII_IOMMU_PROTOCOL                     *This,
  IN  VOID                                     *Mapping
  )
{
  MAP_INFO                 *MapInfo;
  MAP_HANDLE_INFO          *MapHandleInfo;
  LIST_ENTRY               *Link;
  EFI_TPL                  OriginalTpl;

  DEBUG ((DEBUG_VERBOSE, "IoMmuUnmap: 0x%08x\n", Mapping));

  if (Mapping == NULL) {
    DEBUG ((DEBUG_ERROR, "IoMmuUnmap: %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  OriginalTpl = gBS->RaiseTPL (VTD_TPL_LEVEL);
  MapInfo = NULL;
  for (Link = GetFirstNode (&gMaps)
       ; !IsNull (&gMaps, Link)
       ; Link = GetNextNode (&gMaps, Link)
       ) {
    MapInfo = MAP_INFO_FROM_LINK (Link);
    if (MapInfo == Mapping) {
      break;
    }
  }
  //
  // Mapping is not a valid value returned by Map()
  //
  if (MapInfo != Mapping) {
    gBS->RestoreTPL (OriginalTpl);
    DEBUG ((DEBUG_ERROR, "IoMmuUnmap: %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }
  RemoveEntryList (&MapInfo->Link);
  gBS->RestoreTPL (OriginalTpl);

  //
  // remove all nodes in MapInfo->HandleList
  //
  while (!IsListEmpty (&MapInfo->HandleList)) {
    MapHandleInfo = MAP_HANDLE_INFO_FROM_LINK (MapInfo->HandleList.ForwardLink);
    RemoveEntryList (&MapHandleInfo->Link);
    FreePool (MapHandleInfo);
  }

  if (MapInfo->DeviceAddress != MapInfo->HostAddress) {
    //
    // If this is a write operation from the Bus Master's point of view,
    // then copy the contents of the mapped buffer into the real buffer
    // so the processor can read the contents of the real buffer.
    //
    if (MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite ||
        MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite64) {
      CopyMem (
        (VOID *) (UINTN) MapInfo->HostAddress,
        (VOID *) (UINTN) MapInfo->DeviceAddress,
        MapInfo->NumberOfBytes
        );
    }

    //
    // Free the mapped buffer and the MAP_INFO structure.
    //
    gBS->FreePages (MapInfo->DeviceAddress, MapInfo->NumberOfPages);
  }

  FreePool (Mapping);
  return EFI_SUCCESS;
}

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  This                  The protocol instance pointer.
  @param  Type                  This parameter is not used and must be ignored.
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  Attributes            The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE, MEMORY_CACHED and DUAL_ADDRESS_CYCLE.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
EFIAPI
IoMmuAllocateBuffer (
  IN     EDKII_IOMMU_PROTOCOL                     *This,
  IN     EFI_ALLOCATE_TYPE                        Type,
  IN     EFI_MEMORY_TYPE                          MemoryType,
  IN     UINTN                                    Pages,
  IN OUT VOID                                     **HostAddress,
  IN     UINT64                                   Attributes
  )
{
  EFI_STATUS                Status;
  EFI_PHYSICAL_ADDRESS      PhysicalAddress;

  DEBUG ((DEBUG_VERBOSE, "IoMmuAllocateBuffer: ==> 0x%08x\n", Pages));

  //
  // Validate Attributes
  //
  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) != 0) {
    DEBUG ((DEBUG_ERROR, "IoMmuAllocateBuffer: %r\n", EFI_UNSUPPORTED));
    return EFI_UNSUPPORTED;
  }

  //
  // Check for invalid inputs
  //
  if (HostAddress == NULL) {
    DEBUG ((DEBUG_ERROR, "IoMmuAllocateBuffer: %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  //
  // The only valid memory types are EfiBootServicesData and
  // EfiRuntimeServicesData
  //
  if (MemoryType != EfiBootServicesData &&
      MemoryType != EfiRuntimeServicesData) {
    DEBUG ((DEBUG_ERROR, "IoMmuAllocateBuffer: %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  PhysicalAddress = DMA_MEMORY_TOP;
  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0) {
    //
    // Limit allocations to memory below 4GB
    //
    PhysicalAddress = MIN (PhysicalAddress, SIZE_4GB - 1);
  }
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  MemoryType,
                  Pages,
                  &PhysicalAddress
                  );
  if (!EFI_ERROR (Status)) {
    *HostAddress = (VOID *) (UINTN) PhysicalAddress;
  }

  DEBUG ((DEBUG_VERBOSE, "IoMmuAllocateBuffer: 0x%08x <==\n", *HostAddress));

  return Status;
}

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  The protocol instance pointer.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
EFI_STATUS
EFIAPI
IoMmuFreeBuffer (
  IN  EDKII_IOMMU_PROTOCOL                     *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  )
{
  DEBUG ((DEBUG_VERBOSE, "IoMmuFreeBuffer: 0x%\n", Pages));
  return gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress, Pages);
}

/**
  Get device information from mapping.

  @param[in]  Mapping        The mapping.
  @param[out] DeviceAddress  The device address of the mapping.
  @param[out] NumberOfPages  The number of pages of the mapping.

  @retval EFI_SUCCESS            The device information is returned.
  @retval EFI_INVALID_PARAMETER  The mapping is invalid.
**/
EFI_STATUS
GetDeviceInfoFromMapping (
  IN  VOID                                     *Mapping,
  OUT EFI_PHYSICAL_ADDRESS                     *DeviceAddress,
  OUT UINTN                                    *NumberOfPages
  )
{
  MAP_INFO                 *MapInfo;
  LIST_ENTRY               *Link;

  if (Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MapInfo = NULL;
  for (Link = GetFirstNode (&gMaps)
       ; !IsNull (&gMaps, Link)
       ; Link = GetNextNode (&gMaps, Link)
       ) {
    MapInfo = MAP_INFO_FROM_LINK (Link);
    if (MapInfo == Mapping) {
      break;
    }
  }
  //
  // Mapping is not a valid value returned by Map()
  //
  if (MapInfo != Mapping) {
    return EFI_INVALID_PARAMETER;
  }

  *DeviceAddress = MapInfo->DeviceAddress;
  *NumberOfPages = MapInfo->NumberOfPages;
  return EFI_SUCCESS;
}

