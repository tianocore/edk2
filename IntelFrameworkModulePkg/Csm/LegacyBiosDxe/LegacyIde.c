/** @file
  Collect IDE information from Native EFI Driver

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LegacyBiosInterface.h"

BOOLEAN mIdeDataBuiltFlag = FALSE;

/**
  Collect IDE Inquiry data from the IDE disks

  @param  Private        Legacy BIOS Instance data
  @param  HddInfo        Hdd Information
  @param  Flag           Reconnect IdeController or not

  @retval EFI_SUCCESS    It should always work.

**/
EFI_STATUS
LegacyBiosBuildIdeData (
  IN  LEGACY_BIOS_INSTANCE      *Private,
  IN  HDD_INFO                  **HddInfo,
  IN  UINT16                    Flag
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                IdeController;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_DISK_INFO_PROTOCOL    *DiskInfo;
  UINT32                    IdeChannel;
  UINT32                    IdeDevice;
  UINT32                    Size;
  UINT8                     *InquiryData;
  UINT32                    InquiryDataSize;
  HDD_INFO                  *LocalHddInfo;
  UINT32                    PciIndex;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePathNode;
  PCI_DEVICE_PATH           *PciDevicePath;

  //
  // Only build data once
  // We have a problem with GetBbsInfo in that it can be invoked two
  // places. Once in BDS, when all EFI drivers are connected and once in
  // LegacyBoot after all EFI drivers are disconnected causing this routine
  // to hang. In LegacyBoot this function is also called before EFI drivers
  // are disconnected.
  // Cases covered
  //    GetBbsInfo invoked in BDS. Both invocations in LegacyBoot ignored.
  //    GetBbsInfo not invoked in BDS. First invocation of this function
  //       proceeds normally and second via GetBbsInfo ignored.
  //
  PciDevicePath = NULL;
  LocalHddInfo  = *HddInfo;
  Status = Private->LegacyBiosPlatform->GetPlatformHandle (
                                          Private->LegacyBiosPlatform,
                                          EfiGetPlatformIdeHandle,
                                          0,
                                          &HandleBuffer,
                                          &HandleCount,
                                          (VOID *) &LocalHddInfo
                                          );
  if (!EFI_ERROR (Status)) {
    IdeController = HandleBuffer[0];    
    //
    // Force IDE drive spin up!
    //
    if (Flag != 0) {
      gBS->DisconnectController (
            IdeController,
            NULL,
            NULL
            );
    }

    gBS->ConnectController (IdeController, NULL, NULL, FALSE);

    //
    // Do GetIdeHandle twice since disconnect/reconnect will switch to native mode
    // And GetIdeHandle will switch to Legacy mode, if required.
    //
    Private->LegacyBiosPlatform->GetPlatformHandle (
                                  Private->LegacyBiosPlatform,
                                  EfiGetPlatformIdeHandle,
                                  0,
                                  &HandleBuffer,
                                  &HandleCount,
                                  (VOID *) &LocalHddInfo
                                  );
  }

  mIdeDataBuiltFlag = TRUE;

  //
  // Get Identity command from all drives
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiDiskInfoProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer
        );

  Private->IdeDriveCount = (UINT8) HandleCount;
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDiskInfoProtocolGuid,
                    (VOID **) &DiskInfo
                    );
    ASSERT_EFI_ERROR (Status);

    if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid)) {
      //
      //  Locate which PCI device
      //
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID *) &DevicePath
                      );
      ASSERT_EFI_ERROR (Status);

      DevicePathNode = DevicePath;
      while (!IsDevicePathEnd (DevicePathNode)) {
        TempDevicePathNode = NextDevicePathNode (DevicePathNode);
        if ((DevicePathType (DevicePathNode) == HARDWARE_DEVICE_PATH) &&
              ( DevicePathSubType (DevicePathNode) == HW_PCI_DP) &&
              ( DevicePathType(TempDevicePathNode) == MESSAGING_DEVICE_PATH) &&
              ( DevicePathSubType(TempDevicePathNode) == MSG_ATAPI_DP) ) {
          PciDevicePath = (PCI_DEVICE_PATH *) DevicePathNode;
          break;
        }
        DevicePathNode = NextDevicePathNode (DevicePathNode);
      }

      if (PciDevicePath == NULL) {
        continue;
      }

      //
      // Find start of PCI device in HddInfo. The assumption of the data
      // structure is 2 controllers(channels) per PCI device and each
      // controller can have 2 drives(devices).
      // HddInfo[PciIndex+0].[0] = Channel[0].Device[0] Primary Master
      // HddInfo[PciIndex+0].[1] = Channel[0].Device[1] Primary Slave
      // HddInfo[PciIndex+1].[0] = Channel[1].Device[0] Secondary Master
      // HddInfo[PciIndex+1].[1] = Channel[1].Device[1] Secondary Slave
      // @bug eventually need to pass in max number of entries
      // for end of for loop
      //
      for (PciIndex = 0; PciIndex < 8; PciIndex++) {
        if ((PciDevicePath->Device == LocalHddInfo[PciIndex].Device) &&
            (PciDevicePath->Function == LocalHddInfo[PciIndex].Function)
            ) {
          break;
        }
      }

      if (PciIndex == 8) {
        continue;
      }

      Status = DiskInfo->WhichIde (DiskInfo, &IdeChannel, &IdeDevice);
      if (!EFI_ERROR (Status)) {
        Size = sizeof (ATAPI_IDENTIFY);
        DiskInfo->Identify (
                    DiskInfo,
                    &LocalHddInfo[PciIndex + IdeChannel].IdentifyDrive[IdeDevice],
                    &Size
                    );
        if (IdeChannel == 0) {
          LocalHddInfo[PciIndex + IdeChannel].Status |= HDD_PRIMARY;
        } else if (IdeChannel == 1) {
          LocalHddInfo[PciIndex + IdeChannel].Status |= HDD_SECONDARY;
        }

        InquiryData     = NULL;
        InquiryDataSize = 0;
        Status = DiskInfo->Inquiry (
                             DiskInfo,
                             NULL,
                             &InquiryDataSize
                             );
        if (Status == EFI_BUFFER_TOO_SMALL) {
          InquiryData = (UINT8 *) AllocatePool (
                                  InquiryDataSize
                                  );
          if (InquiryData != NULL) {
            Status = DiskInfo->Inquiry (
                                 DiskInfo,
                                 InquiryData,
                                 &InquiryDataSize
                                 );
          }
        } else {
          Status = EFI_DEVICE_ERROR;
        }

        //
        // If ATAPI device then Inquiry will pass and ATA fail.
        //
        if (!EFI_ERROR (Status)) {
          ASSERT (InquiryData != NULL);
          //
          // If IdeDevice = 0 then set master bit, else slave bit
          //
          if (IdeDevice == 0) {
            if ((InquiryData[0] & 0x1f) == 0x05) {
              LocalHddInfo[PciIndex + IdeChannel].Status |= HDD_MASTER_ATAPI_CDROM;
            } else if ((InquiryData[0] & 0x1f) == 0x00) {
              LocalHddInfo[PciIndex + IdeChannel].Status |= HDD_MASTER_ATAPI_ZIPDISK;
            }
          } else {
            if ((InquiryData[0] & 0x1f) == 0x05) {
              LocalHddInfo[PciIndex + IdeChannel].Status |= HDD_SLAVE_ATAPI_CDROM;
            } else if ((InquiryData[0] & 0x1f) == 0x00) {
              LocalHddInfo[PciIndex + IdeChannel].Status |= HDD_SLAVE_ATAPI_ZIPDISK;
            }
          }
          FreePool (InquiryData);
        } else {
          if (IdeDevice == 0) {
            LocalHddInfo[PciIndex + IdeChannel].Status |= HDD_MASTER_IDE;
          } else {
            LocalHddInfo[PciIndex + IdeChannel].Status |= HDD_SLAVE_IDE;
          }
        }
      }
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;
}


/**
  If the IDE channel is in compatibility (legacy) mode, remove all
  PCI I/O BAR addresses from the controller.

  @param  IdeController  The handle of target IDE controller


**/
VOID
InitLegacyIdeController (
  IN EFI_HANDLE                        IdeController
  )
{
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINT32                            IOBarClear;
  EFI_STATUS                        Status;
  PCI_TYPE00                        PciData;

  //
  // If the IDE channel is in compatibility (legacy) mode, remove all
  // PCI I/O BAR addresses from the controller.  Some software gets
  // confused if an IDE controller is in compatibility (legacy) mode
  // and has PCI I/O resources allocated
  //
  Status = gBS->HandleProtocol (
                  IdeController, 
                  &gEfiPciIoProtocolGuid, 
                  (VOID **)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0, sizeof (PciData), &PciData);
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Check whether this is IDE
  //
  if ((PciData.Hdr.ClassCode[2] != PCI_CLASS_MASS_STORAGE) ||
      (PciData.Hdr.ClassCode[1] != PCI_CLASS_MASS_STORAGE_IDE)) {
    return ;
  }

  //
  // Clear bar for legacy IDE
  //
  IOBarClear = 0x00;
  if ((PciData.Hdr.ClassCode[0] & IDE_PI_REGISTER_PNE) == 0) {
    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x10, 1, &IOBarClear);
    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x14, 1, &IOBarClear);
  }
  if ((PciData.Hdr.ClassCode[0] & IDE_PI_REGISTER_SNE) == 0) {
    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x18, 1, &IOBarClear);
    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x1C, 1, &IOBarClear);
  }

  return ;
}
