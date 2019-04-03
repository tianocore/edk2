/** @file
  Intel VTd driver.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DmaProtection.h"

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Convert the DeviceHandle to SourceId and Segment.

  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[out] Segment           The Segment used to identify a VTd engine.
  @param[out] SourceId          The SourceId used to identify a VTd engine and table entry.

  @retval EFI_SUCCESS            The Segment and SourceId are returned.
  @retval EFI_INVALID_PARAMETER  DeviceHandle is an invalid handle.
  @retval EFI_UNSUPPORTED        DeviceHandle is unknown by the IOMMU.
**/
EFI_STATUS
DeviceHandleToSourceId (
  IN EFI_HANDLE            DeviceHandle,
  OUT UINT16               *Segment,
  OUT VTD_SOURCE_ID        *SourceId
  )
{
  EFI_PCI_IO_PROTOCOL                      *PciIo;
  UINTN                                    Seg;
  UINTN                                    Bus;
  UINTN                                    Dev;
  UINTN                                    Func;
  EFI_STATUS                               Status;
  EDKII_PLATFORM_VTD_DEVICE_INFO           DeviceInfo;

  Status = EFI_NOT_FOUND;
  if (mPlatformVTdPolicy != NULL) {
    Status = mPlatformVTdPolicy->GetDeviceId (mPlatformVTdPolicy, DeviceHandle, &DeviceInfo);
    if (!EFI_ERROR(Status)) {
      *Segment  = DeviceInfo.Segment;
      *SourceId = DeviceInfo.SourceId;
      return EFI_SUCCESS;
    }
  }

  Status = gBS->HandleProtocol (DeviceHandle, &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }
  Status = PciIo->GetLocation (PciIo, &Seg, &Bus, &Dev, &Func);
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }
  *Segment = (UINT16)Seg;
  SourceId->Bits.Bus = (UINT8)Bus;
  SourceId->Bits.Device = (UINT8)Dev;
  SourceId->Bits.Function = (UINT8)Func;

  return EFI_SUCCESS;
}

/**
  Set IOMMU attribute for a system memory.

  If the IOMMU protocol exists, the system memory cannot be used
  for DMA by default.

  When a device requests a DMA access for a system memory,
  the device driver need use SetAttribute() to update the IOMMU
  attribute to request DMA access (read and/or write).

  The DeviceHandle is used to identify which device submits the request.
  The IOMMU implementation need translate the device path to an IOMMU device ID,
  and set IOMMU hardware register accordingly.
  1) DeviceHandle can be a standard PCI device.
     The memory for BusMasterRead need set EDKII_IOMMU_ACCESS_READ.
     The memory for BusMasterWrite need set EDKII_IOMMU_ACCESS_WRITE.
     The memory for BusMasterCommonBuffer need set EDKII_IOMMU_ACCESS_READ|EDKII_IOMMU_ACCESS_WRITE.
     After the memory is used, the memory need set 0 to keep it being protected.
  2) DeviceHandle can be an ACPI device (ISA, I2C, SPI, etc).
     The memory for DMA access need set EDKII_IOMMU_ACCESS_READ and/or EDKII_IOMMU_ACCESS_WRITE.

  @param[in]  This              The protocol instance pointer.
  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[in]  DeviceAddress     The base of device memory address to be used as the DMA memory.
  @param[in]  Length            The length of device memory address to be used as the DMA memory.
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by DeviceAddress and Length.
  @retval EFI_INVALID_PARAMETER  DeviceHandle is an invalid handle.
  @retval EFI_INVALID_PARAMETER  DeviceAddress is not IoMmu Page size aligned.
  @retval EFI_INVALID_PARAMETER  Length is not IoMmu Page size aligned.
  @retval EFI_INVALID_PARAMETER  Length is 0.
  @retval EFI_INVALID_PARAMETER  IoMmuAccess specified an illegal combination of access.
  @retval EFI_UNSUPPORTED        DeviceHandle is unknown by the IOMMU.
  @retval EFI_UNSUPPORTED        The bit mask of IoMmuAccess is not supported by the IOMMU.
  @retval EFI_UNSUPPORTED        The IOMMU does not support the memory range specified by DeviceAddress and Length.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to modify the IOMMU access.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.

**/
EFI_STATUS
VTdSetAttribute (
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN EFI_HANDLE            DeviceHandle,
  IN EFI_PHYSICAL_ADDRESS  DeviceAddress,
  IN UINT64                Length,
  IN UINT64                IoMmuAccess
  )
{
  EFI_STATUS           Status;
  UINT16               Segment;
  VTD_SOURCE_ID        SourceId;
  CHAR8                PerfToken[sizeof("VTD(S0000.B00.D00.F00)")];
  UINT32               Identifier;

  DumpVtdIfError ();

  Status = DeviceHandleToSourceId (DeviceHandle, &Segment, &SourceId);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "IoMmuSetAttribute: "));
  DEBUG ((DEBUG_VERBOSE, "PCI(S%x.B%x.D%x.F%x) ", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));
  DEBUG ((DEBUG_VERBOSE, "(0x%lx~0x%lx) - %lx\n", DeviceAddress, Length, IoMmuAccess));

  if (mAcpiDmarTable == NULL) {
    //
    // Record the entry to driver global variable.
    // As such once VTd is activated, the setting can be adopted.
    //
    if ((PcdGet8 (PcdVTdPolicyPropertyMask) & BIT2) != 0) {
      //
      // Force no IOMMU access attribute request recording before DMAR table is installed.
      //
      ASSERT_EFI_ERROR (EFI_NOT_READY);
      return EFI_NOT_READY;
    }
    Status = RequestAccessAttribute (Segment, SourceId, DeviceAddress, Length, IoMmuAccess);
  } else {
    PERF_CODE (
      AsciiSPrint (PerfToken, sizeof(PerfToken), "S%04xB%02xD%02xF%01x", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function);
      Identifier = (Segment << 16) | SourceId.Uint16;
      PERF_START_EX (gImageHandle, PerfToken, "IntelVTD", 0, Identifier);
    );

    Status = SetAccessAttribute (Segment, SourceId, DeviceAddress, Length, IoMmuAccess);

    PERF_CODE (
      Identifier = (Segment << 16) | SourceId.Uint16;
      PERF_END_EX (gImageHandle, PerfToken, "IntelVTD", 0, Identifier);
    );
  }

  if (!EFI_ERROR(Status)) {
    SyncDeviceHandleToMapInfo (
      DeviceHandle,
      DeviceAddress,
      Length,
      IoMmuAccess
      );
  }

  return Status;
}

/**
  Set IOMMU attribute for a system memory.

  If the IOMMU protocol exists, the system memory cannot be used
  for DMA by default.

  When a device requests a DMA access for a system memory,
  the device driver need use SetAttribute() to update the IOMMU
  attribute to request DMA access (read and/or write).

  The DeviceHandle is used to identify which device submits the request.
  The IOMMU implementation need translate the device path to an IOMMU device ID,
  and set IOMMU hardware register accordingly.
  1) DeviceHandle can be a standard PCI device.
     The memory for BusMasterRead need set EDKII_IOMMU_ACCESS_READ.
     The memory for BusMasterWrite need set EDKII_IOMMU_ACCESS_WRITE.
     The memory for BusMasterCommonBuffer need set EDKII_IOMMU_ACCESS_READ|EDKII_IOMMU_ACCESS_WRITE.
     After the memory is used, the memory need set 0 to keep it being protected.
  2) DeviceHandle can be an ACPI device (ISA, I2C, SPI, etc).
     The memory for DMA access need set EDKII_IOMMU_ACCESS_READ and/or EDKII_IOMMU_ACCESS_WRITE.

  @param[in]  This              The protocol instance pointer.
  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[in]  Mapping           The mapping value returned from Map().
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by DeviceAddress and Length.
  @retval EFI_INVALID_PARAMETER  DeviceHandle is an invalid handle.
  @retval EFI_INVALID_PARAMETER  Mapping is not a value that was returned by Map().
  @retval EFI_INVALID_PARAMETER  IoMmuAccess specified an illegal combination of access.
  @retval EFI_UNSUPPORTED        DeviceHandle is unknown by the IOMMU.
  @retval EFI_UNSUPPORTED        The bit mask of IoMmuAccess is not supported by the IOMMU.
  @retval EFI_UNSUPPORTED        The IOMMU does not support the memory range specified by Mapping.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to modify the IOMMU access.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.

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
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  DeviceAddress;
  UINTN                 NumberOfPages;
  EFI_TPL               OriginalTpl;

  OriginalTpl = gBS->RaiseTPL (VTD_TPL_LEVEL);

  Status = GetDeviceInfoFromMapping (Mapping, &DeviceAddress, &NumberOfPages);
  if (!EFI_ERROR(Status)) {
    Status = VTdSetAttribute (
               This,
               DeviceHandle,
               DeviceAddress,
               EFI_PAGES_TO_SIZE(NumberOfPages),
               IoMmuAccess
               );
  }

  gBS->RestoreTPL (OriginalTpl);

  return Status;
}

EDKII_IOMMU_PROTOCOL  mIntelVTd = {
  EDKII_IOMMU_PROTOCOL_REVISION,
  IoMmuSetAttribute,
  IoMmuMap,
  IoMmuUnmap,
  IoMmuAllocateBuffer,
  IoMmuFreeBuffer,
};

/**
  Initialize the VTd driver.

  @param[in]  ImageHandle  ImageHandle of the loaded driver
  @param[in]  SystemTable  Pointer to the System Table

  @retval  EFI_SUCCESS           The Protocol is installed.
  @retval  EFI_OUT_OF_RESOURCES  Not enough resources available to initialize driver.
  @retval  EFI_DEVICE_ERROR      A device error occurred attempting to initialize the driver.

**/
EFI_STATUS
EFIAPI
IntelVTdInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  if ((PcdGet8(PcdVTdPolicyPropertyMask) & BIT0) == 0) {
    return EFI_UNSUPPORTED;
  }

  InitializeDmaProtection ();

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiIoMmuProtocolGuid, &mIntelVTd,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
