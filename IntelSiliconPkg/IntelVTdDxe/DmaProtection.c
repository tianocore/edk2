/** @file

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DmaProtection.h"

EFI_ACPI_SDT_PROTOCOL                   *mAcpiSdt;
UINT64                                  mBelow4GMemoryLimit;
UINT64                                  mAbove4GMemoryLimit;

EDKII_PLATFORM_VTD_POLICY_PROTOCOL      *mPlatformVTdPolicy;

/**
  return the UEFI memory information.

  @param[out] Below4GMemoryLimit  The below 4GiB memory limit
  @param[out] Above4GMemoryLimit  The above 4GiB memory limit
**/
VOID
ReturnUefiMemoryMap (
  OUT UINT64   *Below4GMemoryLimit,
  OUT UINT64   *Above4GMemoryLimit
  )
{
  EFI_STATUS                  Status;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMap;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR       *EfiEntry;
  EFI_MEMORY_DESCRIPTOR       *NextEfiEntry;
  EFI_MEMORY_DESCRIPTOR       TempEfiEntry;
  UINTN                       EfiMemoryMapSize;
  UINTN                       EfiMapKey;
  UINTN                       EfiDescriptorSize;
  UINT32                      EfiDescriptorVersion;
  UINT64                      MemoryBlockLength;

  *Below4GMemoryLimit = 0;
  *Above4GMemoryLimit = 0;

  //
  // Get the EFI memory map.
  //
  EfiMemoryMapSize  = 0;
  EfiMemoryMap      = NULL;
  Status = gBS->GetMemoryMap (
                  &EfiMemoryMapSize,
                  EfiMemoryMap,
                  &EfiMapKey,
                  &EfiDescriptorSize,
                  &EfiDescriptorVersion
                  );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  do {
    //
    // Use size returned back plus 1 descriptor for the AllocatePool.
    // We don't just multiply by 2 since the "for" loop below terminates on
    // EfiMemoryMapEnd which is dependent upon EfiMemoryMapSize. Otherwize
    // we process bogus entries and create bogus E820 entries.
    //
    EfiMemoryMap = (EFI_MEMORY_DESCRIPTOR *) AllocatePool (EfiMemoryMapSize);
    ASSERT (EfiMemoryMap != NULL);
    Status = gBS->GetMemoryMap (
                    &EfiMemoryMapSize,
                    EfiMemoryMap,
                    &EfiMapKey,
                    &EfiDescriptorSize,
                    &EfiDescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (EfiMemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  ASSERT_EFI_ERROR (Status);

  //
  // Sort memory map from low to high
  //
  EfiEntry        = EfiMemoryMap;
  NextEfiEntry    = NEXT_MEMORY_DESCRIPTOR (EfiEntry, EfiDescriptorSize);
  EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) EfiMemoryMap + EfiMemoryMapSize);
  while (EfiEntry < EfiMemoryMapEnd) {
    while (NextEfiEntry < EfiMemoryMapEnd) {
      if (EfiEntry->PhysicalStart > NextEfiEntry->PhysicalStart) {
        CopyMem (&TempEfiEntry, EfiEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (EfiEntry, NextEfiEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (NextEfiEntry, &TempEfiEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
      }

      NextEfiEntry = NEXT_MEMORY_DESCRIPTOR (NextEfiEntry, EfiDescriptorSize);
    }

    EfiEntry      = NEXT_MEMORY_DESCRIPTOR (EfiEntry, EfiDescriptorSize);
    NextEfiEntry  = NEXT_MEMORY_DESCRIPTOR (EfiEntry, EfiDescriptorSize);
  }

  //
  //
  //
  DEBUG ((DEBUG_INFO, "MemoryMap:\n"));
  EfiEntry        = EfiMemoryMap;
  EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) EfiMemoryMap + EfiMemoryMapSize);
  while (EfiEntry < EfiMemoryMapEnd) {
    MemoryBlockLength = (UINT64) (LShiftU64 (EfiEntry->NumberOfPages, 12));
    DEBUG ((DEBUG_INFO, "Entry(0x%02x) 0x%016lx - 0x%016lx\n", EfiEntry->Type, EfiEntry->PhysicalStart, EfiEntry->PhysicalStart + MemoryBlockLength));
    switch (EfiEntry->Type) {
    case EfiLoaderCode:
    case EfiLoaderData:
    case EfiBootServicesCode:
    case EfiBootServicesData:
    case EfiConventionalMemory:
    case EfiRuntimeServicesCode:
    case EfiRuntimeServicesData:
    case EfiACPIReclaimMemory:
    case EfiACPIMemoryNVS:
    case EfiReservedMemoryType:
      if ((EfiEntry->PhysicalStart + MemoryBlockLength) <= BASE_1MB) {
        //
        // Skip the memory block is under 1MB
        //
      } else if (EfiEntry->PhysicalStart >= BASE_4GB) {
        if (*Above4GMemoryLimit < EfiEntry->PhysicalStart + MemoryBlockLength) {
          *Above4GMemoryLimit = EfiEntry->PhysicalStart + MemoryBlockLength;
        }
      } else {
        if (*Below4GMemoryLimit < EfiEntry->PhysicalStart + MemoryBlockLength) {
          *Below4GMemoryLimit = EfiEntry->PhysicalStart + MemoryBlockLength;
        }
      }
      break;
    }
    EfiEntry = NEXT_MEMORY_DESCRIPTOR (EfiEntry, EfiDescriptorSize);
  }

  FreePool (EfiMemoryMap);

  DEBUG ((DEBUG_INFO, "Result:\n"));
  DEBUG ((DEBUG_INFO, "Below4GMemoryLimit:  0x%016lx\n", *Below4GMemoryLimit));
  DEBUG ((DEBUG_INFO, "Above4GMemoryLimit:  0x%016lx\n", *Above4GMemoryLimit));

  return ;
}

/**
  The scan bus callback function to always enable page attribute.

  @param[in]  Context               The context of the callback.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Device                The device of the source.
  @param[in]  Function              The function of the source.

  @retval EFI_SUCCESS           The VTd entry is updated to always enable all DMA access for the specific device.
**/
EFI_STATUS
EFIAPI
ScanBusCallbackAlwaysEnablePageAttribute (
  IN VOID           *Context,
  IN UINT16         Segment,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Function
  )
{
  VTD_SOURCE_ID           SourceId;
  EFI_STATUS              Status;

  SourceId.Bits.Bus = Bus;
  SourceId.Bits.Device = Device;
  SourceId.Bits.Function = Function;
  Status = AlwaysEnablePageAttribute (Segment, SourceId);
  return Status;
}

/**
  Always enable the VTd page attribute for the device in the DeviceScope.

  @param[in]  DeviceScope  the input device scope data structure

  @retval EFI_SUCCESS           The VTd entry is updated to always enable all DMA access for the specific device in the device scope.
**/
EFI_STATUS
AlwaysEnablePageAttributeDeviceScope (
  IN  EDKII_PLATFORM_VTD_DEVICE_SCOPE   *DeviceScope
  )
{
  UINT8                             Bus;
  UINT8                             Device;
  UINT8                             Function;
  VTD_SOURCE_ID                     SourceId;
  UINT8                             SecondaryBusNumber;
  EFI_STATUS                        Status;

  Status = GetPciBusDeviceFunction (DeviceScope->SegmentNumber, &DeviceScope->DeviceScope, &Bus, &Device, &Function);

  if (DeviceScope->DeviceScope.Type == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE) {
    //
    // Need scan the bridge and add all devices.
    //
    SecondaryBusNumber = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(DeviceScope->SegmentNumber, Bus, Device, Function, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
    Status = ScanPciBus (NULL, DeviceScope->SegmentNumber, SecondaryBusNumber, ScanBusCallbackAlwaysEnablePageAttribute);
    return Status;
  } else {
    SourceId.Bits.Bus      = Bus;
    SourceId.Bits.Device   = Device;
    SourceId.Bits.Function = Function;
    Status = AlwaysEnablePageAttribute (DeviceScope->SegmentNumber, SourceId);
    return Status;
  }
}

/**
  Always enable the VTd page attribute for the device matching DeviceId.

  @param[in]  PciDeviceId  the input PCI device ID

  @retval EFI_SUCCESS           The VTd entry is updated to always enable all DMA access for the specific device matching DeviceId.
**/
EFI_STATUS
AlwaysEnablePageAttributePciDeviceId (
  IN  EDKII_PLATFORM_VTD_PCI_DEVICE_ID   *PciDeviceId
  )
{
  UINTN            VtdIndex;
  UINTN            PciIndex;
  PCI_DEVICE_DATA  *PciDeviceData;
  EFI_STATUS       Status;

  for (VtdIndex = 0; VtdIndex < mVtdUnitNumber; VtdIndex++) {
    for (PciIndex = 0; PciIndex < mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceDataNumber; PciIndex++) {
      PciDeviceData = &mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceData[PciIndex];

      if (((PciDeviceId->VendorId == 0xFFFF) || (PciDeviceId->VendorId == PciDeviceData->PciDeviceId.VendorId)) &&
          ((PciDeviceId->DeviceId == 0xFFFF) || (PciDeviceId->DeviceId == PciDeviceData->PciDeviceId.DeviceId)) &&
          ((PciDeviceId->RevisionId == 0xFF) || (PciDeviceId->RevisionId == PciDeviceData->PciDeviceId.RevisionId)) &&
          ((PciDeviceId->SubsystemVendorId == 0xFFFF) || (PciDeviceId->SubsystemVendorId == PciDeviceData->PciDeviceId.SubsystemVendorId)) &&
          ((PciDeviceId->SubsystemDeviceId == 0xFFFF) || (PciDeviceId->SubsystemDeviceId == PciDeviceData->PciDeviceId.SubsystemDeviceId)) ) {
        Status = AlwaysEnablePageAttribute (mVtdUnitInformation[VtdIndex].Segment, PciDeviceData->PciSourceId);
        if (EFI_ERROR(Status)) {
          continue;
        }
      }
    }
  }
  return EFI_SUCCESS;
}

/**
  Always enable the VTd page attribute for the device.

  @param[in]  DeviceInfo  the exception device information

  @retval EFI_SUCCESS           The VTd entry is updated to always enable all DMA access for the specific device in the device info.
**/
EFI_STATUS
AlwaysEnablePageAttributeExceptionDeviceInfo (
  IN  EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO   *DeviceInfo
  )
{
  switch (DeviceInfo->Type) {
  case EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO_TYPE_DEVICE_SCOPE:
    return AlwaysEnablePageAttributeDeviceScope ((VOID *)(DeviceInfo + 1));
  case EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO_TYPE_PCI_DEVICE_ID:
    return AlwaysEnablePageAttributePciDeviceId ((VOID *)(DeviceInfo + 1));
  default:
    return EFI_UNSUPPORTED;
  }
}

/**
  Initialize platform VTd policy.
**/
VOID
InitializePlatformVTdPolicy (
  VOID
  )
{
  EFI_STATUS                               Status;
  UINTN                                    DeviceInfoCount;
  VOID                                     *DeviceInfo;
  EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO *ThisDeviceInfo;
  UINTN                                    Index;

  //
  // It is optional.
  //
  Status = gBS->LocateProtocol (
                  &gEdkiiPlatformVTdPolicyProtocolGuid,
                  NULL,
                  (VOID **)&mPlatformVTdPolicy
                  );
  if (!EFI_ERROR(Status)) {
    DEBUG ((DEBUG_INFO, "InitializePlatformVTdPolicy\n"));
    Status = mPlatformVTdPolicy->GetExceptionDeviceList (mPlatformVTdPolicy, &DeviceInfoCount, &DeviceInfo);
    if (!EFI_ERROR(Status)) {
      ThisDeviceInfo = DeviceInfo;
      for (Index = 0; Index < DeviceInfoCount; Index++) {
        if (ThisDeviceInfo->Type == EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO_TYPE_END) {
          break;
        }
        AlwaysEnablePageAttributeExceptionDeviceInfo (ThisDeviceInfo);
        ThisDeviceInfo = (VOID *)((UINTN)ThisDeviceInfo + ThisDeviceInfo->Length);
      }
      FreePool (DeviceInfo);
    }
  }
}

/**
  Setup VTd engine.
**/
VOID
SetupVtd (
  VOID
  )
{
  EFI_STATUS      Status;
  VOID            *PciEnumerationComplete;
  UINTN           Index;
  UINT64          Below4GMemoryLimit;
  UINT64          Above4GMemoryLimit;

  //
  // PCI Enumeration must be done
  //
  Status = gBS->LocateProtocol (
                  &gEfiPciEnumerationCompleteProtocolGuid,
                  NULL,
                  &PciEnumerationComplete
                  );
  ASSERT_EFI_ERROR (Status);

  ReturnUefiMemoryMap (&Below4GMemoryLimit, &Above4GMemoryLimit);
  Below4GMemoryLimit = ALIGN_VALUE_UP(Below4GMemoryLimit, SIZE_256MB);
  DEBUG ((DEBUG_INFO, " Adjusted Below4GMemoryLimit: 0x%016lx\n", Below4GMemoryLimit));

  mBelow4GMemoryLimit = Below4GMemoryLimit;
  mAbove4GMemoryLimit = Above4GMemoryLimit;

  //
  // 1. setup
  //
  DEBUG ((DEBUG_INFO, "GetDmarAcpiTable\n"));
  Status = GetDmarAcpiTable ();
  if (EFI_ERROR (Status)) {
    return;
  }
  DEBUG ((DEBUG_INFO, "ParseDmarAcpiTable\n"));
  Status = ParseDmarAcpiTableDrhd ();
  if (EFI_ERROR (Status)) {
    return;
  }
  DEBUG ((DEBUG_INFO, "PrepareVtdConfig\n"));
  PrepareVtdConfig ();

  //
  // 2. initialization
  //
  DEBUG ((DEBUG_INFO, "SetupTranslationTable\n"));
  Status = SetupTranslationTable ();
  if (EFI_ERROR (Status)) {
    return;
  }

  InitializePlatformVTdPolicy ();

  ParseDmarAcpiTableRmrr ();

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    DEBUG ((DEBUG_INFO,"VTD Unit %d (Segment: %04x)\n", Index, mVtdUnitInformation[Index].Segment));
    if (mVtdUnitInformation[Index].ExtRootEntryTable != NULL) {
      DumpDmarExtContextEntryTable (mVtdUnitInformation[Index].ExtRootEntryTable);
    }
    if (mVtdUnitInformation[Index].RootEntryTable != NULL) {
      DumpDmarContextEntryTable (mVtdUnitInformation[Index].RootEntryTable);
    }
  }

  //
  // 3. enable
  //
  DEBUG ((DEBUG_INFO, "EnableDmar\n"));
  Status = EnableDmar ();
  if (EFI_ERROR (Status)) {
    return;
  }
  DEBUG ((DEBUG_INFO, "DumpVtdRegs\n"));
  DumpVtdRegsAll ();
}

/**
  ACPI notification function.

  @param[in] Table    A pointer to the ACPI table header.
  @param[in] Version  The ACPI table's version.
  @param[in] TableKey The table key for this ACPI table.

  @retval EFI_SUCCESS The notification function is executed.
**/
EFI_STATUS
EFIAPI
AcpiNotificationFunc (
  IN EFI_ACPI_SDT_HEADER    *Table,
  IN EFI_ACPI_TABLE_VERSION Version,
  IN UINTN                  TableKey
  )
{
  if (Table->Signature == EFI_ACPI_4_0_DMA_REMAPPING_TABLE_SIGNATURE) {
    DEBUG((DEBUG_INFO, "Vtd AcpiNotificationFunc\n"));
    SetupVtd ();
  }
  return EFI_SUCCESS;
}

/**
  Exit boot service callback function.

  @param[in]  Event    The event handle.
  @param[in]  Context  The event content.
**/
VOID
EFIAPI
OnExitBootServices (
  IN EFI_EVENT                               Event,
  IN VOID                                    *Context
  )
{
  DEBUG ((DEBUG_INFO, "Vtd OnExitBootServices\n"));
  DumpVtdRegsAll ();

  if ((PcdGet8(PcdVTdPolicyPropertyMask) & BIT1) == 0) {
    DisableDmar ();
    DumpVtdRegsAll ();
  }
}

/**
  Legacy boot callback function.

  @param[in]  Event    The event handle.
  @param[in]  Context  The event content.
**/
VOID
EFIAPI
OnLegacyBoot (
  EFI_EVENT                               Event,
  VOID                                    *Context
  )
{
  DEBUG ((DEBUG_INFO, "Vtd OnLegacyBoot\n"));
  DumpVtdRegsAll ();
  DisableDmar ();
  DumpVtdRegsAll ();
}

/**
  Initialize DMA protection.
**/
VOID
InitializeDmaProtection (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ExitBootServicesEvent;
  EFI_EVENT   LegacyBootEvent;

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **) &mAcpiSdt);
  ASSERT_EFI_ERROR (Status);

  Status = mAcpiSdt->RegisterNotify (TRUE, AcpiNotificationFunc);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  OnExitBootServices,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &ExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = EfiCreateEventLegacyBootEx (
             TPL_NOTIFY,
             OnLegacyBoot,
             NULL,
             &LegacyBootEvent
             );
  ASSERT_EFI_ERROR (Status);

  return ;
}
