/** @file

  The protocol provides support to allocate, free, map and umap a DMA buffer
  for bus master (e.g PciHostBridge). When SEV is enabled, the DMA operations
  must be performed on unencrypted buffer hence we use a bounce buffer to map
  the guest buffer into an unencrypted DMA buffer.

  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AmdSevIoMmu.h"

typedef struct {
  EDKII_IOMMU_OPERATION                     Operation;
  UINTN                                     NumberOfBytes;
  UINTN                                     NumberOfPages;
  EFI_PHYSICAL_ADDRESS                      CryptedAddress;
  EFI_PHYSICAL_ADDRESS                      PlainTextAddress;
} MAP_INFO;

#define NO_MAPPING             (VOID *) (UINTN) -1

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
  EFI_ALLOCATE_TYPE                                 AllocateType;

  if (HostAddress == NULL || NumberOfBytes == NULL || DeviceAddress == NULL ||
      Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Make sure that Operation is valid
  //
  if ((UINT32) Operation >= EdkiiIoMmuOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress;

  DmaMemoryTop = (UINTN)-1;
  AllocateType = AllocateAnyPages;

  if (((Operation != EdkiiIoMmuOperationBusMasterRead64 &&
        Operation != EdkiiIoMmuOperationBusMasterWrite64 &&
        Operation != EdkiiIoMmuOperationBusMasterCommonBuffer64)) &&
      ((PhysicalAddress + *NumberOfBytes) > SIZE_4GB)) {
    //
    // If the root bridge or the device cannot handle performing DMA above
    // 4GB but any part of the DMA transfer being mapped is above 4GB, then
    // map the DMA transfer to a buffer below 4GB.
    //
    DmaMemoryTop = SIZE_4GB - 1;
    AllocateType = AllocateMaxAddress;

    if (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer ||
        Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64) {
        //
        // Common Buffer operations can not be remapped.  If the common buffer
        // if above 4GB, then it is not possible to generate a mapping, so
        // return an error.
        //
        return EFI_UNSUPPORTED;
    }
  }

  //
  // CommandBuffer was allocated by us (AllocateBuffer) and is already in
  // unencryted buffer so no need to create bounce buffer
  //
  if (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer ||
      Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64) {
    *Mapping = NO_MAPPING;
    *DeviceAddress = PhysicalAddress;

    return EFI_SUCCESS;
  }

  //
  // Allocate a MAP_INFO structure to remember the mapping when Unmap() is
  // called later.
  //
  MapInfo = AllocatePool (sizeof (MAP_INFO));
  if (MapInfo == NULL) {
    *NumberOfBytes = 0;
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the MAP_INFO structure
  //
  MapInfo->Operation         = Operation;
  MapInfo->NumberOfBytes     = *NumberOfBytes;
  MapInfo->NumberOfPages     = EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes);
  MapInfo->CryptedAddress    = PhysicalAddress;
  MapInfo->PlainTextAddress  = DmaMemoryTop;

  //
  // Allocate a buffer to map the transfer to.
  //
  Status = gBS->AllocatePages (
                  AllocateType,
                  EfiBootServicesData,
                  MapInfo->NumberOfPages,
                  &MapInfo->PlainTextAddress
                  );
  if (EFI_ERROR (Status)) {
    FreePool (MapInfo);
    *NumberOfBytes = 0;
    return Status;
  }

  //
  // Clear the memory encryption mask from the device buffer
  //
  Status = MemEncryptSevClearPageEncMask (
             0,
             MapInfo->PlainTextAddress,
             MapInfo->NumberOfPages,
             TRUE
             );
  ASSERT_EFI_ERROR(Status);

  //
  // If this is a read operation from the Bus Master's point of view,
  // then copy the contents of the real buffer into the mapped buffer
  // so the Bus Master can read the contents of the real buffer.
  //
  if (Operation == EdkiiIoMmuOperationBusMasterRead ||
      Operation == EdkiiIoMmuOperationBusMasterRead64) {
    CopyMem (
      (VOID *) (UINTN) MapInfo->PlainTextAddress,
      (VOID *) (UINTN) MapInfo->CryptedAddress,
      MapInfo->NumberOfBytes
      );
  }

  //
  // The DeviceAddress is the address of the maped buffer below 4GB
  //
  *DeviceAddress = MapInfo->PlainTextAddress;

  //
  // Return a pointer to the MAP_INFO structure in Mapping
  //
  *Mapping       = MapInfo;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a PlainText 0x%Lx Crypted 0x%Lx Pages 0x%Lx Bytes 0x%Lx\n",
    __FUNCTION__,
    MapInfo->PlainTextAddress,
    MapInfo->CryptedAddress,
    MapInfo->NumberOfPages,
    MapInfo->NumberOfBytes
    ));

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
  IN  EDKII_IOMMU_PROTOCOL                     *This,
  IN  VOID                                     *Mapping
  )
{
  MAP_INFO                 *MapInfo;
  EFI_STATUS               Status;

  if (Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // See if the Map() operation associated with this Unmap() required a mapping
  // buffer. If a mapping buffer was not required, then this function simply
  // buffer. If a mapping buffer was not required, then this function simply
  //
  if (Mapping == NO_MAPPING) {
    return EFI_SUCCESS;
  }

  MapInfo = (MAP_INFO *)Mapping;

  //
  // If this is a write operation from the Bus Master's point of view,
  // then copy the contents of the mapped buffer into the real buffer
  // so the processor can read the contents of the real buffer.
  //
  if (MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite ||
      MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite64) {
    CopyMem (
      (VOID *) (UINTN) MapInfo->CryptedAddress,
      (VOID *) (UINTN) MapInfo->PlainTextAddress,
      MapInfo->NumberOfBytes
      );
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a PlainText 0x%Lx Crypted 0x%Lx Pages 0x%Lx Bytes 0x%Lx\n",
    __FUNCTION__,
    MapInfo->PlainTextAddress,
    MapInfo->CryptedAddress,
    MapInfo->NumberOfPages,
    MapInfo->NumberOfBytes
    ));
  //
  // Restore the memory encryption mask
  //
  Status = MemEncryptSevSetPageEncMask (
             0,
             MapInfo->PlainTextAddress,
             MapInfo->NumberOfPages,
             TRUE
             );
  ASSERT_EFI_ERROR(Status);

  //
  // Free the mapped buffer and the MAP_INFO structure.
  //
  gBS->FreePages (MapInfo->PlainTextAddress, MapInfo->NumberOfPages);
  FreePool (Mapping);
  return EFI_SUCCESS;
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
  if (MemoryType != EfiBootServicesData &&
      MemoryType != EfiRuntimeServicesData) {
    return EFI_INVALID_PARAMETER;
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
                  Pages,
                  &PhysicalAddress
                  );
  if (!EFI_ERROR (Status)) {
    *HostAddress = (VOID *) (UINTN) PhysicalAddress;

    //
    // Clear memory encryption mask
    //
    Status = MemEncryptSevClearPageEncMask (0, PhysicalAddress, Pages, TRUE);
    ASSERT_EFI_ERROR(Status);
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a Address 0x%Lx Pages 0x%Lx\n",
    __FUNCTION__,
    PhysicalAddress,
    Pages
    ));
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
  IN  EDKII_IOMMU_PROTOCOL                     *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  )
{
  EFI_STATUS  Status;

  //
  // Set memory encryption mask
  //
  Status = MemEncryptSevSetPageEncMask (
             0,
             (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress,
             Pages,
             TRUE
             );
  ASSERT_EFI_ERROR(Status);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a Address 0x%Lx Pages 0x%Lx\n",
    __FUNCTION__,
    (UINTN)HostAddress,
    Pages
    ));
  return gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress, Pages);
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
  Initialize Iommu Protocol.

**/
VOID
EFIAPI
AmdSevInstallIoMmuProtocol (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiIoMmuProtocolGuid, &mAmdSev,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}
