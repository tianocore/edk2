/** @file
  RISC-V IOMMU protocol.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/IoMmu.h>
#include <Protocol/PciIo.h>
#include <Register/RiscV64/RiscVImpl.h>
#include "RiscVIoMmu.h"

#define MAP_INFO_SIGNATURE  SIGNATURE_32 ('D', 'M', 'A', 'P')
typedef struct {
  UINT32                                    Signature;
#if 0
  LIST_ENTRY                                Link;
#endif
  EDKII_IOMMU_OPERATION                     Operation;
  EFI_PHYSICAL_ADDRESS                      HostAddress;
  UINTN                                     NumberOfBytes;
  EFI_PHYSICAL_ADDRESS                      DeviceAddress;
#if 0
  LIST_ENTRY                                HandleList;
#endif
} MAP_INFO;

EDKII_IOMMU_PROTOCOL  mRiscVIoMmuProtocol = {
  EDKII_IOMMU_PROTOCOL_REVISION,
  IoMmuSetAttribute,
  IoMmuMap,
  IoMmuUnmap,
  IoMmuAllocateBuffer,
  IoMmuFreeBuffer,
};

/**
  Returns the top of IOMMU addressable memory based on
  the HART's operating SATP mode and the IOMMU's GXL bit.

**/
STATIC
UINT64
RiscVGetIoMmuMemoryTop (
  VOID
  )
{
  STATIC UINT64        IoMmuMemoryTop = 0;
  LIST_ENTRY           *Link;
  RISCV_IOMMU_CONTEXT  *IoMmuContext;
  RISCV_IOMMU_FCTL     FeatureControl;
  UINTN                HartSatpMode;

  if (IoMmuMemoryTop != 0) {
    return IoMmuMemoryTop;
  }

  //
  // This function can be called to allocate the common buffer,
  // which means that its result must be usable by all IOMMUs.
  //
  for (Link = GetFirstNode (&mRiscVIoMmuContexts)
     ; !IsNull (&mRiscVIoMmuContexts, Link)
     ; Link = GetNextNode (&mRiscVIoMmuContexts, Link)
     ) {
    IoMmuContext = RISCV_IOMMU_CONTEXT_FROM_LINK (Link);
    FeatureControl.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FCTL);
    if (FeatureControl.Bits.GXL) {
      DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "GXL bit is set, so buffer must be below 4G\n"));
      IoMmuMemoryTop = (1ULL << 32) - 1;
      return IoMmuMemoryTop;
    }
  }

  HartSatpMode = (RiscVGetSupervisorAddressTranslationRegister () & SATP64_MODE) >> SATP64_MODE_SHIFT;
  if (HartSatpMode == SATP_MODE_SV39) {
    IoMmuMemoryTop = (1ULL << 39) - 1;
    return IoMmuMemoryTop;
  } else if (HartSatpMode == SATP_MODE_SV48) {
    IoMmuMemoryTop = (1ULL << 48) - 1;
    return IoMmuMemoryTop;
  } else if (HartSatpMode == SATP_MODE_SV57) {
    IoMmuMemoryTop = (1ULL << 57) - 1;
    return IoMmuMemoryTop;
  }

  ASSERT (FALSE);
  return 0;
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
  EFI_TPL                   OriginalTpl;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  MAP_INFO                  *MapInfo;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_STATUS                Status;
  UINTN                     Seg;
  UINTN                     Bus;
  UINTN                     Dev;
  UINTN                     Func;
  RISCV_IOMMU_DEVICE_ID     IoMmuDeviceId;
  LIST_ENTRY                *Link;
  RISCV_IOMMU_CONTEXT       *IoMmuContext;
  LIST_ENTRY                *DownstreamLink;
  RISCV_IOMMU_DOWNSTREAMS   *IoMmuDownstreams;
  RISCV_IOMMU_DEVICE_ID     MappedIoMmuDeviceId;

  OriginalTpl = gBS->RaiseTPL (TPL_NOTIFY);

  DEBUG ((
    RISCV_IOMMU_DEBUG_LEVEL,
    "%a: DeviceHandle=0x%lx, Mapping=0x%lx, IoMmuAccess=0x%lx\n",
    __func__,
    DeviceHandle,
    Mapping,
    IoMmuAccess
    ));

  //
  // Validate input arguments
  //
  if ((DeviceHandle == NULL) || (Mapping == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  DevicePath = DevicePathFromHandle (DeviceHandle);
  if (DevicePath == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  MapInfo = Mapping;
  if (MapInfo->Signature != MAP_INFO_SIGNATURE) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // FIXME: Implement this for MMIO devices against FDT/ACPI.
  // - So, the remaining is PCI-specific.
  //
  if ((DevicePath->Type != HARDWARE_DEVICE_PATH) && (DevicePath->SubType != HW_PCI_DP)) {
    DEBUG ((DEBUG_ERROR, "%a: At this time, only PCI devices are supported by the IOMMU driver!\n", __func__));
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  //
  // Meta BUGBUG: This is called to erase mappings too. It doesn't need a complementary function.
  // - However, we may want to be able to unload this, such as at EndOfFirmware? Follow a secure convention.
  // - In reverting our mappings, see the comment in 4.1.2.
  //
  //if (IoMmuAccess == 0) {
    //Status = EFI_SUCCESS;
    //goto Exit;
  //}

  //
  // Get device_id for this request.
  //
  Status = gBS->HandleProtocol (DeviceHandle, &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  Status = PciIo->GetLocation (PciIo, &Seg, &Bus, &Dev, &Func);
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  IoMmuDeviceId.PciBdf.Padding  = 0;
  IoMmuDeviceId.PciBdf.Segment  = (UINT8)Seg;
  IoMmuDeviceId.PciBdf.Bus      = (UINT8)Bus;
  IoMmuDeviceId.PciBdf.Device   = (UINT8)Dev;
  IoMmuDeviceId.PciBdf.Function = (UINT8)Func;

  //
  // Discover which IOMMU this device_id is behind through its context's downstreams.
  // - TODO: MapMask handling.
  //
  for (Link = GetFirstNode (&mRiscVIoMmuContexts)
     ; !IsNull (&mRiscVIoMmuContexts, Link)
     ; Link = GetNextNode (&mRiscVIoMmuContexts, Link)
     ) {
    IoMmuContext = RISCV_IOMMU_CONTEXT_FROM_LINK (Link);
    for (DownstreamLink = GetFirstNode (&IoMmuContext->DownstreamDevices)
       ; !IsNull (&IoMmuContext->DownstreamDevices, DownstreamLink)
       ; DownstreamLink = GetNextNode (&IoMmuContext->DownstreamDevices, DownstreamLink)
       ) {
      IoMmuDownstreams = RISCV_IOMMU_DOWNSTREAMS_FROM_LINK (DownstreamLink);
      if ((IoMmuDeviceId.Uint32 >= IoMmuDownstreams->NodeMapping.SourceIdBase) && (IoMmuDeviceId.Uint32 < (IoMmuDownstreams->NodeMapping.SourceIdBase + IoMmuDownstreams->NodeMapping.NumberOfIds))) {
        MappedIoMmuDeviceId.Uint32 = IoMmuDownstreams->NodeMapping.DestinationDeviceIdBase + (IoMmuDeviceId.Uint32 - IoMmuDownstreams->NodeMapping.SourceIdBase);
        goto Work;
      }
    }
  }

  if (IsNull (&mRiscVIoMmuContexts, Link)) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

Work:
  Status = RiscVIoMmuSetAttributeWorker (IoMmuContext, &MappedIoMmuDeviceId, MapInfo->DeviceAddress, MapInfo->NumberOfBytes, IoMmuAccess);
  ASSERT_EFI_ERROR (Status);

  Status = IoMmuCommandQueueFence (IoMmuContext);
  ASSERT_EFI_ERROR (Status);

Exit:
  gBS->RestoreTPL (OriginalTpl);

  return Status;
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
  IN     EDKII_IOMMU_PROTOCOL   *This,
  IN     EDKII_IOMMU_OPERATION  Operation,
  IN     VOID                   *HostAddress,
  IN OUT UINTN                  *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID                   **Mapping
  )
{
  BOOLEAN               NeedRemap;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  EFI_PHYSICAL_ADDRESS  DmaMemoryTop;
  MAP_INFO              *MapInfo;
  EFI_STATUS            Status;
#if 0
  EFI_TPL               OriginalTpl;
#endif

  NeedRemap = FALSE;

  DEBUG ((
    RISCV_IOMMU_DEBUG_LEVEL,
    "%a: Operation=0x%lx, HostAddress=0x%lx, *NumberOfBytes=0x%lx\n",
    __func__,
    Operation,
    HostAddress,
    NumberOfBytes != NULL ? *NumberOfBytes : 0
    ));

  //
  // Validate input arguments
  //
  if (Operation >= EdkiiIoMmuOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if ((HostAddress == NULL) || (NumberOfBytes == NULL) || (DeviceAddress == NULL) || (Mapping == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If the caller goes through IoMmuAllocateBuffer first, then the host buffer
  // already satisfies the IOMMU's requirement. But that isn't guaranteed, so we check here too.
  //
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS)HostAddress;
  DmaMemoryTop = RiscVGetIoMmuMemoryTop ();

  if ((PhysicalAddress + *NumberOfBytes) >= DmaMemoryTop) {
    NeedRemap = TRUE;
  }

  //
  // If this is a 32-bit request (if the root bridge or device cannot handle 64-bit access)
  // and any part of the DMA transfer being mapped is above 4GB, then remap the DMA transfer.
  //
  if (((Operation != EdkiiIoMmuOperationBusMasterRead64) &&
       (Operation != EdkiiIoMmuOperationBusMasterWrite64) &&
       (Operation != EdkiiIoMmuOperationBusMasterCommonBuffer64)) &&
      ((PhysicalAddress + *NumberOfBytes) > SIZE_4GB)) {
    NeedRemap = TRUE;
    DmaMemoryTop = MIN (DmaMemoryTop, SIZE_4GB - 1);
  }

  //
  // If this is a dedicated buffer, check that it can fit on a page table.
  //
  if (((Operation != EdkiiIoMmuOperationBusMasterCommonBuffer) &&
       (Operation != EdkiiIoMmuOperationBusMasterCommonBuffer64)) &&
      ((PhysicalAddress != ALIGN_VALUE(PhysicalAddress, SIZE_4KB)) ||
       (*NumberOfBytes != ALIGN_VALUE(*NumberOfBytes, SIZE_4KB)))) {
    NeedRemap = TRUE;
  }

  //
  // Common Buffer operations can not be remapped. If the common buffer is above 4GB
  // then it is not possible to generate a mapping, so return an error.
  //
  if ((Operation == EdkiiIoMmuOperationBusMasterCommonBuffer) ||
      (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64)) {
    if (NeedRemap) {
      DEBUG ((DEBUG_ERROR, "%a: Common buffer operations cannot be remapped\n", __func__));
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Allocate a MAP_INFO structure to remember the mapping for later steps.
  //
  MapInfo = AllocatePool (sizeof (MAP_INFO));
  if (MapInfo == NULL) {
    *NumberOfBytes = 0;
    return EFI_OUT_OF_RESOURCES;
  }

  MapInfo->Signature         = MAP_INFO_SIGNATURE;
  MapInfo->Operation         = Operation;
  MapInfo->HostAddress       = PhysicalAddress;
  MapInfo->NumberOfBytes     = *NumberOfBytes;
  MapInfo->DeviceAddress     = DmaMemoryTop;
#if 0
  InitializeListHead(&MapInfo->HandleList);
#endif

  //
  // Allocate a buffer that fulfils the device's requirements.
  //
  if (NeedRemap) {
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes),
                    &MapInfo->DeviceAddress
                    );
    if (EFI_ERROR (Status)) {
      *NumberOfBytes = 0;
      FreePool (MapInfo);
      return Status;
    }

    //
    // If this is a read operation from the Bus Master's point of view,
    // then copy the contents of the real buffer into the mapped buffer
    // so that the Bus Master can read the contents of the real buffer.
    //
    if ((Operation == EdkiiIoMmuOperationBusMasterRead) ||
        (Operation == EdkiiIoMmuOperationBusMasterRead64)) {
      CopyMem (
        (VOID *)MapInfo->DeviceAddress,
        (VOID *)MapInfo->HostAddress,
        MapInfo->NumberOfBytes
        );
    }
  } else {
    MapInfo->DeviceAddress = MapInfo->HostAddress;
  }

#if 0
  OriginalTpl = gBS->RaiseTPL (VTD_TPL_LEVEL);
  InsertTailList (&gMaps, &MapInfo->Link);
  gBS->RestoreTPL (OriginalTpl);
#endif

  *DeviceAddress = MapInfo->DeviceAddress;
  *Mapping       = MapInfo;

  DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "%a: *DeviceAddress=0x%lx *Mapping=0x%x\n", __func__, *DeviceAddress, *Mapping));
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
  IN  EDKII_IOMMU_PROTOCOL  *This,
  IN  VOID                  *Mapping
  )
{
  MAP_INFO         *MapInfo;
#if 0
  EFI_TPL          OriginalTpl;
  LIST_ENTRY       *Link;
  MAP_HANDLE_INFO  *MapHandleInfo;
#endif

  DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "%a: Mapping=0x%lx\n", __func__, Mapping));

  //
  // Validate input arguments
  //
  if (Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the MAP_INFO structure.
  //
#if 0
  OriginalTpl = gBS->RaiseTPL (VTD_TPL_LEVEL);
  MapInfo     = NULL;
  for (Link = GetFirstNode (&gMaps)
       ; !IsNull (&gMaps, Link)
       ; Link = GetNextNode (&gMaps, Link)
       ) {
    MapInfo = MAP_INFO_FROM_LINK (Link);
    if (Mapping == MapInfo) {
      break;
    }
  }

  //
  // Mapping is not a valid value returned by Map().
  //
  if (Mapping != MapInfo) {
    gBS->RestoreTPL (OriginalTpl);
    return EFI_INVALID_PARAMETER;
  }

  RemoveEntryList (&MapInfo->Link);
  gBS->RestoreTPL (OriginalTpl);
#else
  MapInfo = Mapping;
  if (MapInfo->Signature != MAP_INFO_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }
#endif

#if 0
  //
  // remove all nodes in MapInfo->HandleList
  //
  while (!IsListEmpty (&MapInfo->HandleList)) {
    MapHandleInfo = MAP_HANDLE_INFO_FROM_LINK (MapInfo->HandleList.ForwardLink);
    RemoveEntryList (&MapHandleInfo->Link);
    FreePool (MapHandleInfo);
  }
#endif

  if (MapInfo->DeviceAddress != MapInfo->HostAddress) {
    //
    // If this is a write operation from the Bus Master's point of view,
    // then copy the contents of the mapped buffer into the real buffer
    // so that the processor can read the contents of the real buffer.
    //
    if ((MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite) ||
        (MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite64)) {
      CopyMem (
        (VOID *)MapInfo->HostAddress,
        (VOID *)MapInfo->DeviceAddress,
        MapInfo->NumberOfBytes
        );
    }

    //
    // Free the mapped buffer and the MAP_INFO structure.
    //
    gBS->FreePages (MapInfo->DeviceAddress, EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes));
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
  IN     EDKII_IOMMU_PROTOCOL  *This,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN     EFI_MEMORY_TYPE       MemoryType,
  IN     UINTN                 Pages,
  IN OUT VOID                  **HostAddress,
  IN     UINT64                Attributes
  )
{
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  EFI_STATUS            Status;

  DEBUG ((
    RISCV_IOMMU_DEBUG_LEVEL,
    "%a: MemoryType=0x%lx, Pages=0x%lx, Attributes=0x%lx\n",
    __func__,
    MemoryType,
    Pages,
    Attributes
    ));

  //
  // These data types are the only valid types for IOMMU memory
  //
  if ((MemoryType != EfiBootServicesData) && (MemoryType != EfiRuntimeServicesData)) {
    DEBUG ((DEBUG_ERROR, "%a: MemoryType 0x%x is not a valid type for IOMMU operations!\n", __func__, MemoryType));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Validate input arguments
  //
  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) != 0) {
    return EFI_UNSUPPORTED;
  }

  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the highest available address usable by the IOMMU for this mapping.
  // - This should probably be IOSATP's MODE, but we're at an earlier step in EDK2's IOMMU protocol flow,
  //   which means that the device context is unfindable because we aren't provided a device_id yet.
  //   Since we set the IOMMU to the same mode as the HART, and use GXL as an override (which forces SXL and SV32),
  //   this is fine.
  //
  PhysicalAddress = RiscVGetIoMmuMemoryTop ();
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
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *HostAddress = (VOID *)PhysicalAddress;

  DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "%a: *HostAddress=0x%x\n", __func__, *HostAddress));
  return EFI_SUCCESS;
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
  IN  EDKII_IOMMU_PROTOCOL  *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  )
{
  DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "%a: HostAddress=0x%lx, Pages=0x%lx\n", __func__, HostAddress, Pages));
  return gBS->FreePages ((EFI_PHYSICAL_ADDRESS)HostAddress, Pages);
}
