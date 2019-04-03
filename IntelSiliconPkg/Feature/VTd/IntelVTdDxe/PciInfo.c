/** @file

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DmaProtection.h"

/**
  Return the index of PCI data.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
  @param[in]  Segment           The Segment used to identify a VTd engine.
  @param[in]  SourceId          The SourceId used to identify a VTd engine and table entry.

  @return The index of the PCI data.
  @retval (UINTN)-1  The PCI data is not found.
**/
UINTN
GetPciDataIndex (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN VTD_SOURCE_ID  SourceId
  )
{
  UINTN          Index;
  VTD_SOURCE_ID  *PciSourceId;

  if (Segment != mVtdUnitInformation[VtdIndex].Segment) {
    return (UINTN)-1;
  }

  for (Index = 0; Index < mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceDataNumber; Index++) {
    PciSourceId = &mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceData[Index].PciSourceId;
    if ((PciSourceId->Bits.Bus == SourceId.Bits.Bus) &&
        (PciSourceId->Bits.Device == SourceId.Bits.Device) &&
        (PciSourceId->Bits.Function == SourceId.Bits.Function) ) {
      return Index;
    }
  }

  return (UINTN)-1;
}

/**
  Register PCI device to VTd engine.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Segment               The segment of the source.
  @param[in]  SourceId              The SourceId of the source.
  @param[in]  DeviceType            The DMAR device scope type.
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
  IN UINT8          DeviceType,
  IN BOOLEAN        CheckExist
  )
{
  PCI_DEVICE_INFORMATION           *PciDeviceInfo;
  VTD_SOURCE_ID                    *PciSourceId;
  UINTN                            PciDataIndex;
  UINTN                            Index;
  PCI_DEVICE_DATA                  *NewPciDeviceData;
  EDKII_PLATFORM_VTD_PCI_DEVICE_ID *PciDeviceId;

  PciDeviceInfo = &mVtdUnitInformation[VtdIndex].PciDeviceInfo;

  if (PciDeviceInfo->IncludeAllFlag) {
    //
    // Do not register device in other VTD Unit
    //
    for (Index = 0; Index < VtdIndex; Index++) {
      PciDataIndex = GetPciDataIndex (Index, Segment, SourceId);
      if (PciDataIndex != (UINTN)-1) {
        DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x already registered by Other Vtd(%d)\n", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, Index));
        return EFI_SUCCESS;
      }
    }
  }

  PciDataIndex = GetPciDataIndex (VtdIndex, Segment, SourceId);
  if (PciDataIndex == (UINTN)-1) {
    //
    // Register new
    //

    if (PciDeviceInfo->PciDeviceDataNumber >= PciDeviceInfo->PciDeviceDataMaxNumber) {
      //
      // Reallocate
      //
      NewPciDeviceData = AllocateZeroPool (sizeof(*NewPciDeviceData) * (PciDeviceInfo->PciDeviceDataMaxNumber + MAX_VTD_PCI_DATA_NUMBER));
      if (NewPciDeviceData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      PciDeviceInfo->PciDeviceDataMaxNumber += MAX_VTD_PCI_DATA_NUMBER;
      if (PciDeviceInfo->PciDeviceData != NULL) {
        CopyMem (NewPciDeviceData, PciDeviceInfo->PciDeviceData, sizeof(*NewPciDeviceData) * PciDeviceInfo->PciDeviceDataNumber);
        FreePool (PciDeviceInfo->PciDeviceData);
      }
      PciDeviceInfo->PciDeviceData = NewPciDeviceData;
    }

    ASSERT (PciDeviceInfo->PciDeviceDataNumber < PciDeviceInfo->PciDeviceDataMaxNumber);

    PciSourceId = &PciDeviceInfo->PciDeviceData[PciDeviceInfo->PciDeviceDataNumber].PciSourceId;
    PciSourceId->Bits.Bus = SourceId.Bits.Bus;
    PciSourceId->Bits.Device = SourceId.Bits.Device;
    PciSourceId->Bits.Function = SourceId.Bits.Function;

    DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));

    PciDeviceId = &PciDeviceInfo->PciDeviceData[PciDeviceInfo->PciDeviceDataNumber].PciDeviceId;
    if ((DeviceType == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT) ||
        (DeviceType == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE)) {
      PciDeviceId->VendorId   = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_VENDOR_ID_OFFSET));
      PciDeviceId->DeviceId   = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_DEVICE_ID_OFFSET));
      PciDeviceId->RevisionId = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_REVISION_ID_OFFSET));

      DEBUG ((DEBUG_INFO, " (%04x:%04x:%02x", PciDeviceId->VendorId, PciDeviceId->DeviceId, PciDeviceId->RevisionId));

      if (DeviceType == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT) {
        PciDeviceId->SubsystemVendorId = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_SUBSYSTEM_VENDOR_ID_OFFSET));
        PciDeviceId->SubsystemDeviceId = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_SUBSYSTEM_ID_OFFSET));
        DEBUG ((DEBUG_INFO, ":%04x:%04x", PciDeviceId->SubsystemVendorId, PciDeviceId->SubsystemDeviceId));
      }
      DEBUG ((DEBUG_INFO, ")"));
    }

    PciDeviceInfo->PciDeviceData[PciDeviceInfo->PciDeviceDataNumber].DeviceType = DeviceType;

    if ((DeviceType != EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT) &&
        (DeviceType != EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE)) {
      DEBUG ((DEBUG_INFO, " (*)"));
    }
    DEBUG ((DEBUG_INFO, "\n"));

    PciDeviceInfo->PciDeviceDataNumber++;
  } else {
    if (CheckExist) {
      DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x already registered\n", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  The scan bus callback function to register PCI device.

  @param[in]  Context               The context of the callback.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Device                The device of the source.
  @param[in]  Function              The function of the source.

  @retval EFI_SUCCESS           The PCI device is registered.
**/
EFI_STATUS
EFIAPI
ScanBusCallbackRegisterPciDevice (
  IN VOID           *Context,
  IN UINT16         Segment,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Function
  )
{
  VTD_SOURCE_ID           SourceId;
  UINTN                   VtdIndex;
  UINT8                   BaseClass;
  UINT8                   SubClass;
  UINT8                   DeviceType;
  EFI_STATUS              Status;

  VtdIndex = (UINTN)Context;
  SourceId.Bits.Bus = Bus;
  SourceId.Bits.Device = Device;
  SourceId.Bits.Function = Function;

  DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT;
  BaseClass = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_CLASSCODE_OFFSET + 2));
  if (BaseClass == PCI_CLASS_BRIDGE) {
    SubClass = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_CLASSCODE_OFFSET + 1));
    if (SubClass == PCI_CLASS_BRIDGE_P2P) {
      DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE;
    }
  }

  Status = RegisterPciDevice (VtdIndex, Segment, SourceId, DeviceType, FALSE);
  return Status;
}

/**
  Scan PCI bus and invoke callback function for each PCI devices under the bus.

  @param[in]  Context               The context of the callback function.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Callback              The callback function in PCI scan.

  @retval EFI_SUCCESS           The PCI devices under the bus are scaned.
**/
EFI_STATUS
ScanPciBus (
  IN VOID                         *Context,
  IN UINT16                       Segment,
  IN UINT8                        Bus,
  IN SCAN_BUS_FUNC_CALLBACK_FUNC  Callback
  )
{
  UINT8                   Device;
  UINT8                   Function;
  UINT8                   SecondaryBusNumber;
  UINT8                   HeaderType;
  UINT8                   BaseClass;
  UINT8                   SubClass;
  UINT16                  VendorID;
  UINT16                  DeviceID;
  EFI_STATUS              Status;

  // Scan the PCI bus for devices
  for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
    for (Function = 0; Function <= PCI_MAX_FUNC; Function++) {
      VendorID  = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_VENDOR_ID_OFFSET));
      DeviceID  = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_DEVICE_ID_OFFSET));
      if (VendorID == 0xFFFF && DeviceID == 0xFFFF) {
        if (Function == 0) {
          //
          // If function 0 is not implemented, do not scan other functions.
          //
          break;
        }
        continue;
      }

      Status = Callback (Context, Segment, Bus, Device, Function);
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
            Status = ScanPciBus (Context, Segment, SecondaryBusNumber, Callback);
            if (EFI_ERROR (Status)) {
              return Status;
            }
          }
        }
      }

      if (Function == 0) {
        HeaderType = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, 0, PCI_HEADER_TYPE_OFFSET));
        if ((HeaderType & HEADER_TYPE_MULTI_FUNCTION) == 0x00) {
          //
          // It is not a multi-function device, do not scan other functions.
          //
          break;
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
    mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceDataNumber,
    mVtdUnitInformation[VtdIndex].PciDeviceInfo.IncludeAllFlag
    ));
  for (Index = 0; Index < mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceDataNumber; Index++) {
    DEBUG ((DEBUG_INFO,"  S%04x B%02x D%02x F%02x\n",
      mVtdUnitInformation[VtdIndex].Segment,
      mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceData[Index].PciSourceId.Bits.Bus,
      mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceData[Index].PciSourceId.Bits.Device,
      mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceData[Index].PciSourceId.Bits.Function
      ));
  }
}

/**
  Find the VTd index by the Segment and SourceId.

  @param[in]  Segment               The segment of the source.
  @param[in]  SourceId              The SourceId of the source.
  @param[out] ExtContextEntry       The ExtContextEntry of the source.
  @param[out] ContextEntry          The ContextEntry of the source.

  @return The index of the VTd engine.
  @retval (UINTN)-1  The VTd engine is not found.
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
  UINTN                   PciDataIndex;

  for (VtdIndex = 0; VtdIndex < mVtdUnitNumber; VtdIndex++) {
    if (Segment != mVtdUnitInformation[VtdIndex].Segment) {
      continue;
    }

    PciDataIndex = GetPciDataIndex (VtdIndex, Segment, SourceId);
    if (PciDataIndex == (UINTN)-1) {
      continue;
    }

//    DEBUG ((DEBUG_INFO,"FindVtdIndex(0x%x) for S%04x B%02x D%02x F%02x\n", VtdIndex, Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));

    if (mVtdUnitInformation[VtdIndex].ExtRootEntryTable != 0) {
      ExtRootEntry = &mVtdUnitInformation[VtdIndex].ExtRootEntryTable[SourceId.Index.RootIndex];
      ExtContextEntryTable = (VTD_EXT_CONTEXT_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(ExtRootEntry->Bits.LowerContextTablePointerLo, ExtRootEntry->Bits.LowerContextTablePointerHi) ;
      ThisExtContextEntry  = &ExtContextEntryTable[SourceId.Index.ContextIndex];
      if (ThisExtContextEntry->Bits.AddressWidth == 0) {
        continue;
      }
      *ExtContextEntry = ThisExtContextEntry;
      *ContextEntry    = NULL;
    } else {
      RootEntry = &mVtdUnitInformation[VtdIndex].RootEntryTable[SourceId.Index.RootIndex];
      ContextEntryTable = (VTD_CONTEXT_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(RootEntry->Bits.ContextTablePointerLo, RootEntry->Bits.ContextTablePointerHi) ;
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

