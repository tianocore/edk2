/** @file
  Legacy BIOS Platform support

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LegacyPlatform.h"

EFI_SETUP_BBS_MAP mSetupBbsMap[] = {
  { 1, 2,     1, 1 },     // ATA HardDrive
  { 2, 3,     1, 1 },     // ATAPI CDROM
  { 3, 0x80,  2, 0 },     // PXE
  { 4, 1,     0, 6 },     // USB Floppy
  { 4, 2,     0, 6 },     // USB HDD
  { 4, 3,     0, 6 },     // USB CD
  { 4, 1,     0, 0 },     // USB ZIP Bugbug since Class/SubClass code is uninitialized
  { 4, 2,     0, 0 }      // USB ZIP Bugbug since Class/SubClass code is uninitialized
};

//
// Global variables for System ROMs
//
#define SYSTEM_ROM_FILE_GUID \
{ 0x1547B4F3, 0x3E8A, 0x4FEF, { 0x81, 0xC8, 0x32, 0x8E, 0xD6, 0x47, 0xAB, 0x1A } }

#define NULL_ROM_FILE_GUID \
{ 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }

SYSTEM_ROM_TABLE mSystemRomTable[] = {
  { SYSTEM_ROM_FILE_GUID,  1 },
  { NULL_ROM_FILE_GUID,    0 }
};

EFI_HANDLE  mVgaHandles[0x20];
EFI_HANDLE  mDiskHandles[0x20];
EFI_HANDLE  mIsaHandles[0x20];

EFI_LEGACY_IRQ_PRIORITY_TABLE_ENTRY IrqPriorityTable[MAX_IRQ_PRIORITY_ENTRIES] = {
  {0x0B,0},
  {0x09,0},
  {0x0A,0},
  {0x05,0},
  {0x07,0},
  {0x00,0},
  {0x00,0}
};

//
// PIRQ Table
// - Slot numbering will be used to update the bus number and determine bridge
//   to check to get bus number.  The Slot number - 1 is an index into a decode
//   table to get the bridge information.
//
EFI_LEGACY_PIRQ_TABLE PirqTableHead = {
  {
    EFI_LEGACY_PIRQ_TABLE_SIGNATURE, // UINT32  Signature
    0x00,             // UINT8   MinorVersion
    0x01,             // UINT8   MajorVersion
    0x0000,           // UINT16  TableSize
    0x00,             // UINT8   Bus
    0x08,             // UINT8   DevFun
    0x0000,           // UINT16  PciOnlyIrq
    0x8086,           // UINT16  CompatibleVid
    0x122e,           // UINT16  CompatibleDid
    0x00000000,       // UINT32  Miniport
    {                 // UINT8   Reserved[11]
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00
    },
    0x00,             // UINT8   Checksum
  },
  {
    //           -- Pin 1 --   -- Pin 2 --   -- Pin 3 --   -- Pin 4 --
    // Bus  Dev   Reg   Map     Reg   Map     Reg   Map     Reg   Map
    //
    {0x00,0x08,{{0x60,0xDEB8},{0x61,0xDEB8},{0x62,0xDEB8},{0x63,0xDEB8}},0x00,0x00},
    {0x00,0x10,{{0x61,0xDEB8},{0x62,0xDEB8},{0x63,0xDEB8},{0x60,0xDEB8}},0x01,0x00},
    {0x00,0x18,{{0x62,0xDEB8},{0x63,0xDEB8},{0x60,0xDEB8},{0x61,0xDEB8}},0x02,0x00},
    {0x00,0x20,{{0x63,0xDEB8},{0x60,0xDEB8},{0x61,0xDEB8},{0x62,0xDEB8}},0x03,0x00},
    {0x00,0x28,{{0x60,0xDEB8},{0x61,0xDEB8},{0x62,0xDEB8},{0x63,0xDEB8}},0x04,0x00},
    {0x00,0x30,{{0x61,0xDEB8},{0x62,0xDEB8},{0x63,0xDEB8},{0x60,0xDEB8}},0x05,0x00},
  }
};

LEGACY_BIOS_PLATFORM_INSTANCE       mPrivateData;
EFI_HANDLE                          mImageHandle = NULL;

/**
  Return the handles and assorted information for the specified PCI Class code

  @param[in]     PciClasses    Array of PCI_CLASS_RECORD to find terminated with ClassCode 0xff
  @param[in,out] DeviceTable   Table to place handles etc in.
  @param[in,out] DeviceIndex   Number of devices found
  @param[in]     DeviceFlags   FALSE if a valid legacy ROM is required, TRUE otherwise.

  @retval EFI_SUCCESS     One or more devices found
  @retval EFI_NOT_FOUND   No device found

**/
EFI_STATUS
FindAllDeviceTypes (
  IN       PCI_CLASS_RECORD      *PciClasses,
  IN OUT   DEVICE_STRUCTURE      *DeviceTable,
  IN OUT   UINT16                *DeviceIndex,
  IN       BOOLEAN               DeviceFlags
  )
{
  UINTN                       HandleCount;
  EFI_HANDLE                  *HandleBuffer;
  UINTN                       Index;
  UINTN                       StartIndex;
  PCI_TYPE00                  PciConfigHeader;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  EFI_LEGACY_BIOS_PROTOCOL    *LegacyBios;
  UINTN                       Flags;
  EFI_STATUS                  Status;
  UINTN                       Index2;

  //
  // Get legacy BIOS protocol as it is required to deal with Option ROMs.
  //
  StartIndex = *DeviceIndex;
  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  (VOID**)&LegacyBios
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Get all PCI handles and check them to generate a list of matching devices.
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciIoProtocolGuid,
         NULL,
         &HandleCount,
         &HandleBuffer
         );
  for (Index = 0; Index < HandleCount; Index++) {
    gBS->HandleProtocol (
           HandleBuffer[Index],
           &gEfiPciIoProtocolGuid,
           (VOID**)&PciIo
           );
    PciIo->Pci.Read (
                 PciIo,
                 EfiPciIoWidthUint32,
                 0,
                 sizeof (PciConfigHeader) / sizeof (UINT32),
                 &PciConfigHeader
                 );
    for (Index2 = 0; PciClasses[Index2].Class != 0xff; Index2++) {
        if ((PciConfigHeader.Hdr.ClassCode[2] == PciClasses[Index2].Class) &&
            (PciConfigHeader.Hdr.ClassCode[1] == PciClasses[Index2].SubClass)) {
        LegacyBios->CheckPciRom (
                      LegacyBios,
                      HandleBuffer[Index],
                      NULL,
                      NULL,
                      &Flags
                      );

        //
        // Verify that results of OPROM check match request.
        // The two valid requests are:
        //   DeviceFlags = 0 require a valid legacy ROM
        //   DeviceFlags = 1 require either no ROM or a valid legacy ROM
        //
        if (
            ((DeviceFlags != 0) && (Flags == NO_ROM)) ||
            ((Flags & (ROM_FOUND | VALID_LEGACY_ROM)) == (ROM_FOUND | VALID_LEGACY_ROM))
           ) {
          DeviceTable->Handle = HandleBuffer[Index];
          DeviceTable->Vid    = PciConfigHeader.Hdr.VendorId;
          DeviceTable->Did    = PciConfigHeader.Hdr.DeviceId;
          DeviceTable->SvId   = PciConfigHeader.Device.SubsystemVendorID;
          DeviceTable->SysId  = PciConfigHeader.Device.SubsystemID;
          ++ *DeviceIndex;
          DeviceTable++;
        }
      }
    }
  }

  //
  // Free any allocated buffers
  //
  gBS->FreePool (HandleBuffer);

  if (*DeviceIndex != StartIndex) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Load and initialize the Legacy BIOS SMM handler.

  @param  This                   The protocol instance pointer.
  @param  EfiToLegacy16BootTable A pointer to Legacy16 boot table.

  @retval EFI_SUCCESS           SMM code loaded.
  @retval EFI_DEVICE_ERROR      SMM code failed to load

**/
EFI_STATUS
EFIAPI
SmmInit (
  IN  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL           *This,
  IN  VOID                                        *EfiToLegacy16BootTable
  )
{
  return EFI_SUCCESS;
}

/**
  Finds the device path that should be used as the primary display adapter.

  @param  VgaHandle - The handle of the video device

**/
VOID
GetSelectedVgaDeviceInfo (
  OUT EFI_HANDLE                *VgaHandle
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;
  UINT8                     MinBus;
  UINT8                     MaxBus;
  UINTN                     Segment;
  UINTN                     Bus;
  UINTN                     Device;
  UINTN                     Function;
  UINTN                     SelectedAddress;
  UINTN                     CurrentAddress;

  //
  // Initialize return to 'not found' state
  //
  *VgaHandle = NULL;

  //
  // Initialize variable states.  Ths is important for selecting the VGA device
  // if multiple devices exist behind a single bridge.
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  SelectedAddress = PCI_LIB_ADDRESS(0xff, 0x1f, 0x7, 0);

  //
  // The bus range to search for a VGA device in.
  //
  MinBus = MaxBus = 0;

  //
  // Start to check all the pci io to find all possible VGA device
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID**)&PciIo);
    if (!EFI_ERROR (Status)) {
      //
      // Detemine if this is in the correct bus range.
      //
      Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
      if (EFI_ERROR(Status) || (Bus < MinBus || Bus > MaxBus)) {
        continue;
      }

      //
      // Read device information.
      //
      Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Make sure the device is a VGA device.
      //
      if (!IS_PCI_VGA (&Pci)) {
        continue;
      }
      DEBUG ((EFI_D_INFO,
        "PCI VGA: 0x%04x:0x%04x\n",
        Pci.Hdr.VendorId,
        Pci.Hdr.DeviceId
        ));

      //
      // Currently we use the lowest numbered bus/device/function if multiple
      // devices are found in the target bus range.
      //
      CurrentAddress = PCI_LIB_ADDRESS(Bus, Device, Function, 0);
      if (CurrentAddress < SelectedAddress) {
        SelectedAddress = CurrentAddress;
        *VgaHandle = HandleBuffer[Index];
      }
    }
  }

  FreePool (HandleBuffer);
}


/**
  Returns a buffer of handles for the requested subfunction.

  @param  This                  The protocol instance pointer.
  @param  Mode                  Specifies what handle to return. See EFI_GET_PLATFORM_HANDLE_MODE enum.
  @param  Type                  Mode specific. See EFI_GET_PLATFORM_HANDLE_MODE enum.
  @param  HandleBuffer          Mode specific. See EFI_GET_PLATFORM_HANDLE_MODE enum.
  @param  HandleCount           Mode specific. See EFI_GET_PLATFORM_HANDLE_MODE enum.
  @param  AdditionalData        Mode specific. See EFI_GET_PLATFORM_HANDLE_MODE enum.

  @retval EFI_SUCCESS           Handle is valid.
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.
  @retval EFI_NOT_FOUND         Handle is not known.

**/
EFI_STATUS
EFIAPI
GetPlatformHandle (
  IN  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL           *This,
  IN  EFI_GET_PLATFORM_HANDLE_MODE                Mode,
  IN  UINT16                                      Type,
  OUT EFI_HANDLE                                  **HandleBuffer,
  OUT UINTN                                       *HandleCount,
  OUT VOID                                        **AdditionalData OPTIONAL
  )
{
  DEVICE_STRUCTURE    LocalDevice[0x40];
  UINT32              LocalIndex;
  UINT32              Index;
  DEVICE_STRUCTURE    TempDevice;
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINTN               Segment;
  UINTN               Bus;
  UINTN               Device;
  UINTN               Function;
  HDD_INFO            *HddInfo;
  PCI_TYPE00          PciConfigHeader;
  UINT32              HddIndex;
  EFI_HANDLE          IdeHandle;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  PCI_CLASS_RECORD    ClassLists[10];
  UINTN               PriorityIndex;

  static BOOLEAN      bConnected = FALSE;

  LocalIndex  = 0x00;
  HddInfo     = NULL;
  HddIndex    = 0;

  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  (VOID**)&LegacyBios
                  );

  //
  // Process mode specific operations
  //
  switch (Mode) {
    case EfiGetPlatformVgaHandle:
      //
      // Get the handle for the currently selected VGA device.
      //
      GetSelectedVgaDeviceInfo (&mVgaHandles[0]);
      *HandleBuffer = &mVgaHandles[0];
      *HandleCount  = (mVgaHandles[0] != NULL) ? 1 : 0;
      return EFI_SUCCESS;
    case EfiGetPlatformIdeHandle:
      IdeHandle  = NULL;
      if (AdditionalData != NULL) {
        HddInfo = (HDD_INFO *) *AdditionalData;
      }

      //
      // Locate all found block io devices
      //
      ClassLists[0].Class    = PCI_CLASS_MASS_STORAGE;
      ClassLists[0].SubClass = PCI_CLASS_MASS_STORAGE_SCSI;
      ClassLists[1].Class    = PCI_CLASS_MASS_STORAGE;
      ClassLists[1].SubClass = PCI_CLASS_MASS_STORAGE_IDE;
      ClassLists[2].Class    = PCI_CLASS_MASS_STORAGE;
      ClassLists[2].SubClass = PCI_CLASS_MASS_STORAGE_RAID;
      ClassLists[3].Class    = PCI_CLASS_MASS_STORAGE;
      ClassLists[3].SubClass = PCI_CLASS_MASS_STORAGE_SATADPA;
      ClassLists[4].Class    = 0xff;
      FindAllDeviceTypes (ClassLists, LocalDevice, (UINT16 *) &LocalIndex, TRUE);
      if (LocalIndex == 0) {
        return EFI_NOT_FOUND;
      }

      //
      // Make sure all IDE controllers are connected. This is necessary
      // in NO_CONFIG_CHANGE boot path to ensure IDE controller is correctly
      // initialized and all IDE drives are enumerated
      //
      if (!bConnected) {
        for (Index = 0; Index < LocalIndex; Index++) {
          gBS->ConnectController (LocalDevice[Index].Handle, NULL, NULL, TRUE);
        }
      }

      //
      // Locate onboard controllers.
      //
      for (Index = 0; Index < LocalIndex; Index++) {
        if (LocalDevice[Index].Vid == V_INTEL_VENDOR_ID) {
          if (LocalDevice[Index].Did == V_PIIX4_IDE_DEVICE_ID) {
            IdeHandle = LocalDevice[Index].Handle;
          }
        }
      }

      //
      // Set the IDE contorller as primary devices.
      //
      PriorityIndex = 0;
      for (Index = 0; Index < LocalIndex; Index++) {
        if (LocalDevice[Index].Handle == IdeHandle && PriorityIndex == 0) {
          TempDevice = LocalDevice[PriorityIndex];
          LocalDevice[PriorityIndex] = LocalDevice[Index];
          LocalDevice[Index] = TempDevice;
          PriorityIndex++;
          break;
        }
      }

      //
      // Copy over handles and update return values.
      //
      for (Index = 0; Index < LocalIndex; Index++) {
        mDiskHandles[Index] = LocalDevice[Index].Handle;
      }
      *HandleBuffer = &mDiskHandles[0];
      *HandleCount  = LocalIndex;

      //
      // We have connected all IDE controllers once. No more needed
      //
      bConnected = TRUE;

      //
      // Log all onboard controllers.
      //
      for (Index = 0; (Index < LocalIndex) && (AdditionalData != NULL); Index++) {
        if ((LocalDevice[Index].Handle != NULL) &&
            (LocalDevice[Index].Handle == IdeHandle)) {
          Status = gBS->HandleProtocol (
                          LocalDevice[Index].Handle,
                          &gEfiPciIoProtocolGuid,
                          (VOID **) &PciIo
                          );
          PciIo->Pci.Read (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,
                       sizeof (PciConfigHeader) / sizeof (UINT32),
                       &PciConfigHeader
                       );
          if (!EFI_ERROR (Status)) {
            PciIo->GetLocation (
                     PciIo,
                     &Segment,
                     &Bus,
                     &Device,
                     &Function
                     );

            //
            // Be sure to only fill out correct information based on platform
            // configureation.
            //
            HddInfo[HddIndex].Status        |= HDD_PRIMARY;
            HddInfo[HddIndex].Bus           = (UINT32)Bus;
            HddInfo[HddIndex].Device        = (UINT32)Device;
            HddInfo[HddIndex].Function      = (UINT32)Function;
            HddInfo[HddIndex + 1].Status    |= HDD_SECONDARY;
            HddInfo[HddIndex + 1].Bus       = (UINT32)Bus;
            HddInfo[HddIndex + 1].Device    = (UINT32)Device;
            HddInfo[HddIndex + 1].Function  = (UINT32)Function;

            //
            // Primary controller data
            //
            if ((PciConfigHeader.Hdr.ClassCode[0] & 0x01) != 0) {
              HddInfo[HddIndex].CommandBaseAddress =
                (UINT16)(PciConfigHeader.Device.Bar[0] & 0xfffc);
              HddInfo[HddIndex].ControlBaseAddress =
                (UINT16)((PciConfigHeader.Device.Bar[1] & 0xfffc)+2);
              HddInfo[HddIndex].BusMasterAddress =
                (UINT16)(PciConfigHeader.Device.Bar[4] & 0xfffc);
              HddInfo[HddIndex].HddIrq = PciConfigHeader.Device.InterruptLine;
            } else {
              HddInfo[HddIndex].HddIrq = 14;
              HddInfo[HddIndex].CommandBaseAddress = 0x1f0;
              HddInfo[HddIndex].ControlBaseAddress = 0x3f6;
              HddInfo[HddIndex].BusMasterAddress = 0;
            }
            HddIndex++;

            //
            // Secondary controller data
            //
            if ((PciConfigHeader.Hdr.ClassCode[0] & 0x04) != 0) {
              HddInfo[HddIndex].CommandBaseAddress =
                (UINT16)(PciConfigHeader.Device.Bar[2] & 0xfffc);
              HddInfo[HddIndex].ControlBaseAddress =
                (UINT16)((PciConfigHeader.Device.Bar[3] & 0xfffc)+2);
              HddInfo[HddIndex].BusMasterAddress =
                (UINT16)(HddInfo[HddIndex].BusMasterAddress + 8);
              HddInfo[HddIndex].HddIrq = PciConfigHeader.Device.InterruptLine;
            } else {
              HddInfo[HddIndex].HddIrq = 15;
              HddInfo[HddIndex].CommandBaseAddress = 0x170;
              HddInfo[HddIndex].ControlBaseAddress = 0x376;
              HddInfo[HddIndex].BusMasterAddress = 0;
            }
            HddIndex++;
          }
        }
      }
      return EFI_SUCCESS;
    case EfiGetPlatformIsaBusHandle:
      ClassLists[0].Class    = (UINT8) PCI_CLASS_BRIDGE;
      ClassLists[0].SubClass = (UINT8) PCI_CLASS_BRIDGE_ISA_PDECODE;
      ClassLists[1].Class    = (UINT8) PCI_CLASS_BRIDGE;
      ClassLists[1].SubClass = (UINT8) PCI_CLASS_BRIDGE_ISA;
      ClassLists[2].Class    = 0xff;

      //
      // Locate all found block io devices
      //
      FindAllDeviceTypes (ClassLists, LocalDevice, (UINT16 *) (&LocalIndex), TRUE);
      if (LocalIndex == 0) {
        return EFI_NOT_FOUND;
      }

      //
      // Find our ISA bridge.
      //
      for (Index = 0; Index < LocalIndex; Index++) {
        if (LocalDevice[Index].Vid == V_INTEL_VENDOR_ID) {
          TempDevice          = LocalDevice[0];
          LocalDevice[0]      = LocalDevice[Index];
          LocalDevice[Index]  = TempDevice;
        }
      }

      //
      // Perform copy and update return values.
      //
      for (Index = 0; Index < LocalIndex; Index++) {
        mIsaHandles[Index] = LocalDevice[Index].Handle;
      }
      *HandleBuffer = &mIsaHandles[0];
      *HandleCount  = LocalIndex;
      return EFI_SUCCESS;
    case EfiGetPlatformUsbHandle:
    default:
      return EFI_UNSUPPORTED;
  };
}

/**
  Allows platform to perform any required action after a LegacyBios operation.
  Invokes the specific sub function specified by Mode.

  @param  This                  The protocol instance pointer.
  @param  Mode                  Specifies what handle to return. See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  Type                  Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  DeviceHandle          Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  ShadowAddress         Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  Compatibility16Table  Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.
  @param  AdditionalData        Mode specific.  See EFI_GET_PLATFORM_HOOK_MODE enum.

  @retval EFI_SUCCESS           The operation performed successfully. Mode specific.
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.

**/
EFI_STATUS
EFIAPI
PlatformHooks (
  IN       EFI_LEGACY_BIOS_PLATFORM_PROTOCOL     *This,
  IN       EFI_GET_PLATFORM_HOOK_MODE            Mode,
  IN       UINT16                                Type,
     OUT   EFI_HANDLE                            DeviceHandle, OPTIONAL
  IN OUT   UINTN                                 *Shadowaddress, OPTIONAL
  IN       EFI_COMPATIBILITY16_TABLE             *Compatibility16Table, OPTIONAL
     OUT   VOID                                  **AdditionalData OPTIONAL
  )
{
  EFI_IA32_REGISTER_SET     Regs;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  EFI_STATUS                Status;

  switch (Mode) {
    case EfiPlatformHookPrepareToScanRom:
      Status = gBS->LocateProtocol (
                      &gEfiLegacyBiosProtocolGuid,
                      NULL,
                      (VOID**)&LegacyBios
                      );

      //
      // Set the 80x25 Text VGA Mode
      //
      Regs.H.AH = 0x00;
      Regs.H.AL = 0x03;
      Status = LegacyBios->Int86 (LegacyBios, 0x10, &Regs);
      return Status;
    case EfiPlatformHookShadowServiceRoms:
      return EFI_SUCCESS;
    case EfiPlatformHookAfterRomInit:
    default:
      return EFI_UNSUPPORTED;
  };
}

/**
  Returns information associated with PCI IRQ routing.
  This function returns the following information associated with PCI IRQ routing:
    * An IRQ routing table and number of entries in the table.
    * The $PIR table and its size.
    * A list of PCI IRQs and the priority order to assign them.

  @param  This                    The protocol instance pointer.
  @param  RoutingTable            The pointer to PCI IRQ Routing table.
                                  This location is the $PIR table minus the header.
  @param  RoutingTableEntries     The number of entries in table.
  @param  LocalPirqTable          $PIR table.
  @param  PirqTableSize           $PIR table size.
  @param  LocalIrqPriorityTable   A list of interrupts in priority order to assign.
  @param  IrqPriorityTableEntries The number of entries in the priority table.

  @retval EFI_SUCCESS           Data was successfully returned.

**/
EFI_STATUS
EFIAPI
GetRoutingTable (
  IN  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL           *This,
  OUT VOID                                        **RoutingTable,
  OUT UINTN                                       *RoutingTableEntries,
  OUT VOID                                        **LocalPirqTable, OPTIONAL
  OUT UINTN                                       *PirqTableSize, OPTIONAL
  OUT VOID                                        **LocalIrqPriorityTable, OPTIONAL
  OUT UINTN                                       *IrqPriorityTableEntries OPTIONAL
  )
{
  UINT16                        PTableSize;
  UINT32                        Index;
  UINT8                         Bus;
  UINT8                         Device;
  UINT8                         Function;
  UINT8                         Checksum;
  UINT8                         *Ptr;
  EFI_STATUS                    Status;
  EFI_LEGACY_INTERRUPT_PROTOCOL *LegacyInterrupt;

  Checksum = 0;

  if (LocalPirqTable != NULL) {
    PTableSize = sizeof (EFI_LEGACY_PIRQ_TABLE_HEADER) +
                 sizeof (EFI_LEGACY_IRQ_ROUTING_ENTRY) * MAX_IRQ_ROUTING_ENTRIES;

    Status = gBS->LocateProtocol (
                    &gEfiLegacyInterruptProtocolGuid,
                    NULL,
                    (VOID**)&LegacyInterrupt
                    );
    ASSERT_EFI_ERROR (Status);
    LegacyInterrupt->GetLocation (
                       LegacyInterrupt,
                       &Bus,
                       &Device,
                       &Function
                       );

    //
    // Update fields in $PIR table header
    //
    PirqTableHead.PirqTable.TableSize = PTableSize;
    PirqTableHead.PirqTable.Bus       = Bus;
    PirqTableHead.PirqTable.DevFun    = (UINT8) ((Device << 3) + Function);
    Ptr = (UINT8 *) (&PirqTableHead);

    //
    // Calculate checksum.
    //
    for (Index = 0; Index < PTableSize; Index++) {
      Checksum = (UINT8) (Checksum + (UINT8) *Ptr);
      Ptr += 1;
    }
    Checksum                          = (UINT8) (0x00 - Checksum);
    PirqTableHead.PirqTable.Checksum  = Checksum;

    //
    // Update return values.
    //
    *LocalPirqTable                   = (VOID *) (&PirqTableHead);
    *PirqTableSize                    = PTableSize;
  }

  //
  // More items to return.
  //
  *RoutingTable         = PirqTableHead.IrqRoutingEntry;
  *RoutingTableEntries  = MAX_IRQ_ROUTING_ENTRIES;
  if (LocalIrqPriorityTable != NULL) {
    *LocalIrqPriorityTable    = IrqPriorityTable;
    *IrqPriorityTableEntries  = MAX_IRQ_PRIORITY_ENTRIES;
  }

  return EFI_SUCCESS;
}

/**
  Finds the binary data or other platform information.

  @param  This                  The protocol instance pointer.
  @param  Mode                  Specifies what data to return. See See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  Table                 Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  TableSize             Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  Location              Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  Alignment             Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  LegacySegment         Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.
  @param  LegacyOffset          Mode specific.  See EFI_GET_PLATFORM_INFO_MODE enum.

  @retval EFI_SUCCESS           Data returned successfully.
  @retval EFI_UNSUPPORTED       Mode is not supported on the platform.
  @retval EFI_NOT_FOUND         Binary image or table not found.

**/
EFI_STATUS
EFIAPI
GetPlatformInfo (
  IN  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL           *This,
  IN  EFI_GET_PLATFORM_INFO_MODE                  Mode,
  OUT VOID                                        **Table,
  OUT UINTN                                       *TableSize,
  OUT UINTN                                       *Location,
  OUT UINTN                                       *Alignment,
  IN  UINT16                                      LegacySegment,
  IN  UINT16                                      LegacyOffset
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;

  switch (Mode) {
    case EfiGetPlatformBinarySystemRom:
      //
      // Loop through table of System rom descriptions
      //
      for (Index = 0; mSystemRomTable[Index].Valid != 0; Index++) {
        Status = GetSectionFromFv (
                   &mSystemRomTable[Index].FileName,
                   EFI_SECTION_RAW,
                   0,
                   Table,
                   (UINTN *) TableSize
                   );
        if (EFI_ERROR (Status)) {
          continue;
        }
        return EFI_SUCCESS;
      }

      return EFI_NOT_FOUND;
    case EfiGetPlatformBinaryOem16Data:
    case EfiGetPlatformBinaryMpTable:
    case EfiGetPlatformBinaryOemIntData:
    case EfiGetPlatformBinaryOem32Data:
    case EfiGetPlatformBinaryTpmBinary:
    case EfiGetPlatformPciExpressBase:
    default:
      return EFI_UNSUPPORTED;
  };
}

/**
  Translates the given PIRQ accounting for bridge.
  This function translates the given PIRQ back through all buses, if required,
  and returns the true PIRQ and associated IRQ.

  @param  This                  The protocol instance pointer.
  @param  PciBus                The PCI bus number for this device.
  @param  PciDevice             The PCI device number for this device.
  @param  PciFunction           The PCI function number for this device.
  @param  Pirq                  Input is PIRQ reported by device, and output is true PIRQ.
  @param  PciIrq                The IRQ already assigned to the PIRQ, or the IRQ to be
                                assigned to the PIRQ.

  @retval EFI_SUCCESS           The PIRQ was translated.

**/
EFI_STATUS
EFIAPI
TranslatePirq (
  IN        EFI_LEGACY_BIOS_PLATFORM_PROTOCOL           *This,
  IN        UINTN                                       PciBus,
  IN        UINTN                                       PciDevice,
  IN        UINTN                                       PciFunction,
  IN  OUT   UINT8                                       *Pirq,
      OUT   UINT8                                       *PciIrq
  )
{
  EFI_LEGACY_INTERRUPT_PROTOCOL      *LegacyInterrupt;
  EFI_STATUS                         Status;
  UINTN                              Index;
  UINTN                              Index1;
  UINT8                              LocalPirq;
  UINT8                              PirqData;
  UINT8                              MatchData;

  Status = gBS->LocateProtocol (
                  &gEfiLegacyInterruptProtocolGuid,
                  NULL,
                  (VOID**)&LegacyInterrupt
                  );
  ASSERT_EFI_ERROR (Status);
  LocalPirq = (UINT8) (*Pirq);

  for (Index = 0; Index < MAX_IRQ_ROUTING_ENTRIES; Index++) {
    if ((PirqTableHead.IrqRoutingEntry[Index].Bus == PciBus) &&
        (PirqTableHead.IrqRoutingEntry[Index].Device == PciDevice)) {
      LocalPirq = (UINT8) (PirqTableHead.IrqRoutingEntry[Index].PirqEntry[LocalPirq].Pirq & 0x0f);
      if (LocalPirq > 4) {
        LocalPirq -= 4;
      }

      LegacyInterrupt->ReadPirq (LegacyInterrupt, LocalPirq, &PirqData);
      MatchData = PCI_UNUSED;
      while (PirqData == 0) {
        for (Index1 = 0; Index1 < MAX_IRQ_PRIORITY_ENTRIES; Index1++) {
          if ((IrqPriorityTable[Index1].Used == MatchData) &&
              (IrqPriorityTable[Index1].Irq != 0)) {
            PirqData = IrqPriorityTable[Index1].Irq;
            IrqPriorityTable[Index1].Used = 0xff;
            LegacyInterrupt->WritePirq (
                               LegacyInterrupt,
                               LocalPirq,
                               PirqData
                               );
            break;
          }
        }

        if (PirqData == 0) {

          //
          // No unused interrpts, so start reusing them.
          //
          MatchData = (UINT8) (~MatchData);
        }
      }

      *PciIrq = PirqData;
      *Pirq   = LocalPirq;
    }
  }

  return EFI_SUCCESS;
}


/**
  Attempt to legacy boot the BootOption. If the EFI contexted has been
  compromised this function will not return.

  @param  This                   The protocol instance pointer.
  @param  BbsDevicePath          The EFI Device Path from BootXXXX variable.
  @param  BbsTable               The Internal BBS table.
  @param  LoadOptionSize         The size of LoadOption in size.
  @param  LoadOption             The LoadOption from BootXXXX variable
  @param  EfiToLegacy16BootTable A pointer to BootTable structure

  @retval EFI_SUCCESS           Ready to boot.

**/
EFI_STATUS
EFIAPI
PrepareToBoot (
  IN  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL           *This,
  IN  BBS_BBS_DEVICE_PATH                         *BbsDevicePath,
  IN  VOID                                        *BbsTable,
  IN  UINT32                                      LoadOptionsSize,
  IN  VOID                                        *LoadOptions,
  IN  VOID                                        *EfiToLegacy16BootTable
  )
{
  BBS_TABLE                           *LocalBbsTable;
  EFI_TO_COMPATIBILITY16_BOOT_TABLE   *Legacy16BootTable;
  DEVICE_PRODUCER_DATA_HEADER         *SioPtr;
  UINT16                              DevicePathType;
  UINT16                              Index;
  UINT16                              Priority;

  //
  // Initialize values
  //
  Priority = 0;
  Legacy16BootTable = (EFI_TO_COMPATIBILITY16_BOOT_TABLE*) EfiToLegacy16BootTable;

  //
  // Set how Gate A20 is gated by hardware
  //
  SioPtr                  = &Legacy16BootTable->SioData;
  SioPtr->Flags.A20Kybd   = 1;
  SioPtr->Flags.A20Port90 = 1;
  SioPtr->MousePresent    = 1;

  LocalBbsTable           = BbsTable;

  //
  // There are 2 cases that must be covered.
  // Case 1: Booting to a legacy OS - BbsDevicePath is non-NULL.
  // Case 2: Booting to an EFI aware OS - BbsDevicePath is NULL.
  //         We need to perform the PrepareToBoot function to assign
  //         drive numbers to HDD devices to allow the shell or EFI
  //         to access them.
  //
  if (BbsDevicePath != NULL) {
    DevicePathType = BbsDevicePath->DeviceType;
  } else {
    DevicePathType = BBS_HARDDISK;
  }

  //
  // Skip the boot devices where priority is set by BDS and set the next one
  //
  for (Index = 0; Index < Legacy16BootTable->NumberBbsEntries; Index++) {
    if ((LocalBbsTable[Index].BootPriority != BBS_UNPRIORITIZED_ENTRY) &&
        (LocalBbsTable[Index].BootPriority != BBS_IGNORE_ENTRY) &&
        (LocalBbsTable[Index].BootPriority != BBS_LOWEST_PRIORITY) &&
        (Priority <= LocalBbsTable[Index].BootPriority)) {
      Priority = (UINT16) (LocalBbsTable[Index].BootPriority + 1);
    }
  }

  switch (DevicePathType) {
    case BBS_FLOPPY:
    case BBS_HARDDISK:
    case BBS_CDROM:
    case BBS_EMBED_NETWORK:
      for (Index = 0; Index < Legacy16BootTable->NumberBbsEntries; Index++) {
        if ((LocalBbsTable[Index].BootPriority == BBS_UNPRIORITIZED_ENTRY) &&
            (LocalBbsTable[Index].DeviceType == DevicePathType)) {
          LocalBbsTable[Index].BootPriority = Priority;
          ++Priority;
        }
      }
      break;
    case BBS_BEV_DEVICE:
      for (Index = 0; Index < Legacy16BootTable->NumberBbsEntries; Index++) {
        if ((LocalBbsTable[Index].BootPriority == BBS_UNPRIORITIZED_ENTRY) &&
            (LocalBbsTable[Index].Class == 01) &&
            (LocalBbsTable[Index].SubClass == 01)) {
          LocalBbsTable[Index].BootPriority = Priority;
          ++Priority;
        }
      }
      break;
    case BBS_USB:
    case BBS_PCMCIA:
    case BBS_UNKNOWN:
    default:
      break;
  };

  //
  // Set priority for rest of devices
  //
  for (Index = 0; Index < Legacy16BootTable->NumberBbsEntries; Index++) {
    if (LocalBbsTable[Index].BootPriority == BBS_UNPRIORITIZED_ENTRY) {
      LocalBbsTable[Index].BootPriority = Priority;
      ++Priority;
    }
  }

  return EFI_SUCCESS;
}


/**
  Initialize Legacy Platform support

  @retval EFI_SUCCESS   Successfully initialized

**/
EFI_STATUS
LegacyBiosPlatformInstall (
  VOID
  )
{
  EFI_STATUS                           Status;
  LEGACY_BIOS_PLATFORM_INSTANCE        *Private;

  mImageHandle = gImageHandle;
  Private = &mPrivateData;

  //
  // Grab a copy of all the protocols we depend on.
  //
  Private->Signature = LEGACY_BIOS_PLATFORM_INSTANCE_SIGNATURE;
  Private->LegacyBiosPlatform.GetPlatformInfo   = GetPlatformInfo;
  Private->LegacyBiosPlatform.GetPlatformHandle = GetPlatformHandle;
  Private->LegacyBiosPlatform.SmmInit           = SmmInit;
  Private->LegacyBiosPlatform.PlatformHooks     = PlatformHooks;
  Private->LegacyBiosPlatform.GetRoutingTable   = GetRoutingTable;
  Private->LegacyBiosPlatform.TranslatePirq     = TranslatePirq;
  Private->LegacyBiosPlatform.PrepareToBoot     = PrepareToBoot;
  Private->ImageHandle = gImageHandle;

  //
  // Make a new handle and install the protocol
  //
  Private->Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Private->Handle,
                  &gEfiLegacyBiosPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->LegacyBiosPlatform
                  );
  return Status;
}

