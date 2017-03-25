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

/**
  Return the index of PCI descriptor.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
  @param[in]  Segment           The Segment used to identify a VTd engine.
  @param[in]  SourceId          The SourceId used to identify a VTd engine and table entry.

  @return The index of the PCI descriptor.
  @retval (UINTN)-1  The PCI descriptor is not found.
**/
UINTN
GetPciDescriptor (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN VTD_SOURCE_ID  SourceId
  )
{
  UINTN  Index;

  if (Segment != mVtdUnitInformation[VtdIndex].Segment) {
    return (UINTN)-1;
  }

  for (Index = 0; Index < mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptorNumber; Index++) {
    if ((mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptors[Index].Bits.Bus == SourceId.Bits.Bus) &&
        (mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptors[Index].Bits.Device == SourceId.Bits.Device) &&
        (mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptors[Index].Bits.Function == SourceId.Bits.Function) ) {
      return Index;
    }
  }

  return (UINTN)-1;
}

/**
  Register PCI device to VTd engine as PCI descriptor.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Segment               The segment of the source.
  @param[in]  SourceId              The SourceId of the source.
  @param[in]  IsRealPciDevice       TRUE: It is a real PCI device.
                                    FALSE: It is not a real PCI device.
  @param[in]  CheckExist            TRUE: ERROR will be returned if the PCI device is already registered.
                                    FALSE: SUCCESS will be returned if the PCI device is registered.

  @retval EFI_SUCCESS           The PCI device is registered.
  @retval EFI_OUT_OF_RESOURCES  No enough resource to register a new PCI device.
  @retval EFI_ALREADY_STARTED   The device is already registered.
**/
EFI_STATUS
RegisterPciDevice (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN VTD_SOURCE_ID  SourceId,
  IN BOOLEAN        IsRealPciDevice,
  IN BOOLEAN        CheckExist
  )
{
  PCI_DEVICE_INFORMATION  *PciDeviceInfo;
  VTD_SOURCE_ID           *PciDescriptor;
  UINTN                   PciDescriptorIndex;
  UINTN                   Index;
  BOOLEAN                 *NewIsRealPciDevice;
  VTD_SOURCE_ID           *NewPciDescriptors;

  PciDeviceInfo = &mVtdUnitInformation[VtdIndex].PciDeviceInfo;

  if (PciDeviceInfo->IncludeAllFlag) {
    //
    // Do not register device in other VTD Unit
    //
    for (Index = 0; Index < VtdIndex; Index++) {
      PciDescriptorIndex = GetPciDescriptor (Index, Segment, SourceId);
      if (PciDescriptorIndex != (UINTN)-1) {
        DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x already registered by Other Vtd(%d)\n", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, Index));
        return EFI_SUCCESS;
      }
    }
  }

  PciDescriptorIndex = GetPciDescriptor (VtdIndex, Segment, SourceId);
  if (PciDescriptorIndex == (UINTN)-1) {
    //
    // Register new
    //

    if (PciDeviceInfo->PciDescriptorNumber >= PciDeviceInfo->PciDescriptorMaxNumber) {
      //
      // Reallocate
      //
      NewIsRealPciDevice = AllocateZeroPool (sizeof(*NewIsRealPciDevice) * (PciDeviceInfo->PciDescriptorMaxNumber + MAX_PCI_DESCRIPTORS));
      if (NewIsRealPciDevice == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      NewPciDescriptors = AllocateZeroPool (sizeof(*NewPciDescriptors) * (PciDeviceInfo->PciDescriptorMaxNumber + MAX_PCI_DESCRIPTORS));
      if (NewPciDescriptors == NULL) {
        FreePool (NewIsRealPciDevice);
        return EFI_OUT_OF_RESOURCES;
      }
      PciDeviceInfo->PciDescriptorMaxNumber += MAX_PCI_DESCRIPTORS;
      if (PciDeviceInfo->IsRealPciDevice != NULL) {
        CopyMem (NewIsRealPciDevice, PciDeviceInfo->IsRealPciDevice, sizeof(*NewIsRealPciDevice) * PciDeviceInfo->PciDescriptorNumber);
        FreePool (PciDeviceInfo->IsRealPciDevice);
      }
      PciDeviceInfo->IsRealPciDevice = NewIsRealPciDevice;
      if (PciDeviceInfo->PciDescriptors != NULL) {
        CopyMem (NewPciDescriptors, PciDeviceInfo->PciDescriptors, sizeof(*NewPciDescriptors) * PciDeviceInfo->PciDescriptorNumber);
        FreePool (PciDeviceInfo->PciDescriptors);
      }
      PciDeviceInfo->PciDescriptors = NewPciDescriptors;
    }

    ASSERT (PciDeviceInfo->PciDescriptorNumber < PciDeviceInfo->PciDescriptorMaxNumber);

    PciDescriptor = &PciDeviceInfo->PciDescriptors[PciDeviceInfo->PciDescriptorNumber];
    PciDescriptor->Bits.Bus = SourceId.Bits.Bus;
    PciDescriptor->Bits.Device = SourceId.Bits.Device;
    PciDescriptor->Bits.Function = SourceId.Bits.Function;
    PciDeviceInfo->IsRealPciDevice[PciDeviceInfo->PciDescriptorNumber] = IsRealPciDevice;

    PciDeviceInfo->PciDescriptorNumber++;

    DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));
    if (!IsRealPciDevice) {
      DEBUG ((DEBUG_INFO, " (*)"));
    }
    DEBUG ((DEBUG_INFO, "\n"));
  } else {
    if (CheckExist) {
      DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x already registered\n", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Scan PCI bus and register PCI devices under the bus.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.

  @retval EFI_SUCCESS           The PCI devices under the bus are registered.
  @retval EFI_OUT_OF_RESOURCES  No enough resource to register a new PCI device.
**/
EFI_STATUS
ScanPciBus (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN UINT8          Bus
  )
{
  UINT8                   Device;
  UINT8                   Function;
  UINT8                   SecondaryBusNumber;
  UINT8                   HeaderType;
  UINT8                   BaseClass;
  UINT8                   SubClass;
  UINT32                  MaxFunction;
  UINT16                  VendorID;
  UINT16                  DeviceID;
  EFI_STATUS              Status;
  VTD_SOURCE_ID           SourceId;

  // Scan the PCI bus for devices
  for (Device = 0; Device < PCI_MAX_DEVICE + 1; Device++) {
    HeaderType = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, 0, PCI_HEADER_TYPE_OFFSET));
    MaxFunction = PCI_MAX_FUNC + 1;
    if ((HeaderType & HEADER_TYPE_MULTI_FUNCTION) == 0x00) {
      MaxFunction = 1;
    }
    for (Function = 0; Function < MaxFunction; Function++) {
      VendorID  = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_VENDOR_ID_OFFSET));
      DeviceID  = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_DEVICE_ID_OFFSET));
      if (VendorID == 0xFFFF && DeviceID == 0xFFFF) {
        continue;
      }

      SourceId.Bits.Bus = Bus;
      SourceId.Bits.Device = Device;
      SourceId.Bits.Function = Function;
      Status = RegisterPciDevice (VtdIndex, Segment, SourceId, TRUE, FALSE);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      BaseClass = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_CLASSCODE_OFFSET + 2));
      if (BaseClass == PCI_CLASS_BRIDGE) {
        SubClass = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_CLASSCODE_OFFSET + 1));
        if (SubClass == PCI_CLASS_BRIDGE_P2P) {
          SecondaryBusNumber = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
          DEBUG ((DEBUG_INFO,"  ScanPciBus: PCI bridge S%04x B%02x D%02x F%02x (SecondBus:%02x)\n", Segment, Bus, Device, Function, SecondaryBusNumber));
          if (SecondaryBusNumber != 0) {
            Status = ScanPciBus (VtdIndex, Segment, SecondaryBusNumber);
            if (EFI_ERROR (Status)) {
              return Status;
            }
          }
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Dump the PCI device information managed by this VTd engine.

  @param[in]  VtdIndex              The index of VTd engine.
**/
VOID
DumpPciDeviceInfo (
  IN UINTN  VtdIndex
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_INFO,"PCI Device Information (Number 0x%x, IncludeAll - %d):\n",
    mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptorNumber,
    mVtdUnitInformation[VtdIndex].PciDeviceInfo.IncludeAllFlag
    ));
  for (Index = 0; Index < mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptorNumber; Index++) {
    DEBUG ((DEBUG_INFO,"  S%04x B%02x D%02x F%02x\n",
      mVtdUnitInformation[VtdIndex].Segment,
      mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptors[Index].Bits.Bus,
      mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptors[Index].Bits.Device,
      mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDescriptors[Index].Bits.Function
      ));
  }
}

/**
  Find the VTd index by the Segment and SourceId.

  @param[in]  Segment               The segment of the source.
  @param[in]  SourceId              The SourceId of the source.
  @param[out] ExtContextEntry       The ExtContextEntry of the source.
  @param[out] ContextEntry          The ContextEntry of the source.

  @return The index of the PCI descriptor.
  @retval (UINTN)-1  The PCI descriptor is not found.
**/
UINTN
FindVtdIndexByPciDevice (
  IN  UINT16                  Segment,
  IN  VTD_SOURCE_ID           SourceId,
  OUT VTD_EXT_CONTEXT_ENTRY   **ExtContextEntry,
  OUT VTD_CONTEXT_ENTRY       **ContextEntry
  )
{
  UINTN                   VtdIndex;
  VTD_ROOT_ENTRY          *RootEntry;
  VTD_CONTEXT_ENTRY       *ContextEntryTable;
  VTD_CONTEXT_ENTRY       *ThisContextEntry;
  VTD_EXT_ROOT_ENTRY      *ExtRootEntry;
  VTD_EXT_CONTEXT_ENTRY   *ExtContextEntryTable;
  VTD_EXT_CONTEXT_ENTRY   *ThisExtContextEntry;
  UINTN                   PciDescriptorIndex;

  for (VtdIndex = 0; VtdIndex < mVtdUnitNumber; VtdIndex++) {
    if (Segment != mVtdUnitInformation[VtdIndex].Segment) {
      continue;
    }

    PciDescriptorIndex = GetPciDescriptor (VtdIndex, Segment, SourceId);
    if (PciDescriptorIndex == (UINTN)-1) {
      continue;
    }

//    DEBUG ((DEBUG_INFO,"FindVtdIndex(0x%x) for S%04x B%02x D%02x F%02x\n", VtdIndex, Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));

    if (mVtdUnitInformation[VtdIndex].ExtRootEntryTable != 0) {
      ExtRootEntry = &mVtdUnitInformation[VtdIndex].ExtRootEntryTable[SourceId.Index.RootIndex];
      ExtContextEntryTable = (VTD_EXT_CONTEXT_ENTRY *)(UINTN)LShiftU64 (ExtRootEntry->Bits.LowerContextTablePointer, 12) ;
      ThisExtContextEntry  = &ExtContextEntryTable[SourceId.Index.ContextIndex];
      if (ThisExtContextEntry->Bits.AddressWidth == 0) {
        continue;
      }
      *ExtContextEntry = ThisExtContextEntry;
      *ContextEntry    = NULL;
    } else {
      RootEntry = &mVtdUnitInformation[VtdIndex].RootEntryTable[SourceId.Index.RootIndex];
      ContextEntryTable = (VTD_CONTEXT_ENTRY *)(UINTN)LShiftU64 (RootEntry->Bits.ContextTablePointer, 12) ;
      ThisContextEntry  = &ContextEntryTable[SourceId.Index.ContextIndex];
      if (ThisContextEntry->Bits.AddressWidth == 0) {
        continue;
      }
      *ExtContextEntry = NULL;
      *ContextEntry    = ThisContextEntry;
    }

    return VtdIndex;
  }

  return (UINTN)-1;
}

