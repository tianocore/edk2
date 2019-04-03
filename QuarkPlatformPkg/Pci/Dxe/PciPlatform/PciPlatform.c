/** @file
Registers onboard PCI ROMs with PCI.IO

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "CommonHeader.h"

#include "PciPlatform.h"


PCI_OPTION_ROM_TABLE mPciOptionRomTable[] = {
  { NULL_ROM_FILE_GUID,                    0, 0, 0, 0, 0xffff, 0xffff }
};
EFI_PCI_PLATFORM_PROTOCOL mPciPlatform = {
  PhaseNotify,
  PlatformPrepController,
  GetPlatformPolicy,
  GetPciRom
};

EFI_HANDLE mPciPlatformHandle = NULL;
EFI_HANDLE mImageHandle       = NULL;


EFI_STATUS
PhaseNotify (
  IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN EFI_HANDLE                                     HostBridge,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  UINT8                  UsbHostBusNumber = IOH_BUS;
  if (Phase == EfiPciHostBridgeEndResourceAllocation) {
    // Required for QuarkSouthCluster.
    // Enable USB controller memory, io and bus master before Ehci driver.
    EnableUsbMemIoBusMaster (UsbHostBusNumber);
    return EFI_SUCCESS;
  }
  return EFI_UNSUPPORTED;
}


EFI_STATUS
PlatformPrepController (
  IN  EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_HANDLE                                     RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS    PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
GetPlatformPolicy (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL                     *This,
  OUT       EFI_PCI_PLATFORM_POLICY                       *PciPolicy
  )
{
  if (PciPolicy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
GetPciRom (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL                   *This,
  IN        EFI_HANDLE                                  PciHandle,
  OUT       VOID                                        **RomImage,
  OUT       UINTN                                       *RomSize
  )
/*++

  Routine Description:
    Return a PCI ROM image for the onboard device represented by PciHandle

  Arguments:
    This      - Protocol instance pointer.
    PciHandle - PCI device to return the ROM image for.
    RomImage  - PCI Rom Image for onboard device
    RomSize   - Size of RomImage in bytes

  Returns:
    EFI_SUCCESS   - RomImage is valid
    EFI_NOT_FOUND - No RomImage

--*/
{
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINTN                         Segment;
  UINTN                         Bus;
  UINTN                         Device;
  UINTN                         Function;
  UINT16                        VendorId;
  UINT16                        DeviceId;
  UINT16                        DeviceClass;
  UINTN                         TableIndex;

  Status = gBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0x0A, 1, &DeviceClass);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0, 1, &VendorId);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 2, 1, &DeviceId);

  //
  // Loop through table of video option rom descriptions
  //
  for (TableIndex = 0; mPciOptionRomTable[TableIndex].VendorId != 0xffff; TableIndex++) {

    //
    // See if the PCI device specified by PciHandle matches at device in mPciOptionRomTable
    //
    if (VendorId != mPciOptionRomTable[TableIndex].VendorId ||
        DeviceId != mPciOptionRomTable[TableIndex].DeviceId ||
        Segment != mPciOptionRomTable[TableIndex].Segment ||
        Bus != mPciOptionRomTable[TableIndex].Bus ||
        Device != mPciOptionRomTable[TableIndex].Device ||
        Function != mPciOptionRomTable[TableIndex].Function) {
      continue;
    }

    Status = GetSectionFromFv (
               &mPciOptionRomTable[TableIndex].FileName,
               EFI_SECTION_RAW,
               0,
               RomImage,
               RomSize
               );

    if (EFI_ERROR (Status)) {
      continue;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
PciPlatformDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_STATUS

--*/
{
  EFI_STATUS  Status;

  mImageHandle = ImageHandle;

  //
  // Install on a new handle
  //
  Status = gBS->InstallProtocolInterface (
                  &mPciPlatformHandle,
                  &gEfiPciPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPciPlatform
                  );

  return Status;
}
