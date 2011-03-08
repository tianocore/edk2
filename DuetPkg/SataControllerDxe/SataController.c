/** @file
  This driver module produces IDE_CONTROLLER_INIT protocol for Sata Controllers.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SataController.h"

///
/// EFI_DRIVER_BINDING_PROTOCOL instance
///
EFI_DRIVER_BINDING_PROTOCOL gSataControllerDriverBinding = {
  SataControllerSupported,
  SataControllerStart,
  SataControllerStop,
  0xa,
  NULL,
  NULL
};

/**
  Read AHCI Operation register.

  @param PciIo      The PCI IO protocol instance.
  @param Offset     The operation register offset.

  @return The register content read.

**/
UINT32
EFIAPI
AhciReadReg (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT32                 Offset
  )
{
  UINT32    Data;

  ASSERT (PciIo != NULL);
  
  Data = 0;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,
               AHCI_BAR_INDEX,
               (UINT64) Offset,
               1,
               &Data
               );

  return Data;
}

/**
  Write AHCI Operation register.

  @param PciIo      The PCI IO protocol instance.
  @param Offset     The operation register offset.
  @param Data       The data used to write down.

**/
VOID
EFIAPI
AhciWriteReg (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT32                 Offset,
  IN UINT32                 Data
  )
{
  ASSERT (PciIo != NULL);

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint32,
               AHCI_BAR_INDEX,
               (UINT64) Offset,
               1,
               &Data
               );

  return;
}

/**
  This function is used to calculate the best PIO mode supported by specific IDE device

  @param IdentifyData   The identify data of specific IDE device.
  @param DisPioMode     Disqualified PIO modes collection.
  @param SelectedMode   Available PIO modes collection.

  @retval EFI_SUCCESS       Best PIO modes are returned.
  @retval EFI_UNSUPPORTED   The device doesn't support PIO mode,
                            or all supported modes have been disqualified.
**/
EFI_STATUS
CalculateBestPioMode (
  IN EFI_IDENTIFY_DATA  *IdentifyData,
  IN UINT16             *DisPioMode OPTIONAL,
  OUT UINT16            *SelectedMode
  )
{
  UINT16    PioMode;
  UINT16    AdvancedPioMode;
  UINT16    Temp;
  UINT16    Index;
  UINT16    MinimumPioCycleTime;

  Temp = 0xff;

  PioMode = (UINT8) (((ATA5_IDENTIFY_DATA *) (&(IdentifyData->AtaData)))->pio_cycle_timing >> 8);

  //
  // See whether Identify Data word 64 - 70 are valid
  //
  if ((IdentifyData->AtaData.field_validity & 0x02) == 0x02) {

    AdvancedPioMode = IdentifyData->AtaData.advanced_pio_modes;
    DEBUG ((EFI_D_INFO, "CalculateBestPioMode: AdvancedPioMode = %x\n", AdvancedPioMode));

    for (Index = 0; Index < 8; Index++) {
      if ((AdvancedPioMode & 0x01) != 0) {
        Temp = Index;
      }

      AdvancedPioMode >>= 1;
    }

    //
    // If Temp is modified, mean the advanced_pio_modes is not zero;
    // if Temp is not modified, mean there is no advanced PIO mode supported,
    // the best PIO Mode is the value in pio_cycle_timing.
    //
    if (Temp != 0xff) {
      AdvancedPioMode = (UINT16) (Temp + 3);
    } else {
      AdvancedPioMode = PioMode;
    }

    //
    // Limit the PIO mode to at most PIO4.
    //
    PioMode = (UINT16) MIN (AdvancedPioMode, 4);

    MinimumPioCycleTime = IdentifyData->AtaData.min_pio_cycle_time_with_flow_control;

    if (MinimumPioCycleTime <= 120) {
      PioMode = (UINT16) MIN (4, PioMode);
    } else if (MinimumPioCycleTime <= 180) {
      PioMode = (UINT16) MIN (3, PioMode);
    } else if (MinimumPioCycleTime <= 240) {
      PioMode = (UINT16) MIN (2, PioMode);
    } else {
      PioMode = 0;
    }

    //
    // Degrade the PIO mode if the mode has been disqualified
    //
    if (DisPioMode != NULL) {
      if (*DisPioMode < 2) {
        return EFI_UNSUPPORTED; // no mode below ATA_PIO_MODE_BELOW_2
      }

      if (PioMode >= *DisPioMode) {
        PioMode = (UINT16) (*DisPioMode - 1);
      }
    }

    if (PioMode < 2) {
      *SelectedMode = 1;        // ATA_PIO_MODE_BELOW_2;
    } else {
      *SelectedMode = PioMode;  // ATA_PIO_MODE_2 to ATA_PIO_MODE_4;
    }

  } else {
    //
    // Identify Data word 64 - 70 are not valid
    // Degrade the PIO mode if the mode has been disqualified
    //
    if (DisPioMode != NULL) {
      if (*DisPioMode < 2) {
        return EFI_UNSUPPORTED; // no mode below ATA_PIO_MODE_BELOW_2
      }

      if (PioMode == *DisPioMode) {
        PioMode--;
      }
    }

    if (PioMode < 2) {
      *SelectedMode = 1;        // ATA_PIO_MODE_BELOW_2;
    } else {
      *SelectedMode = 2;        // ATA_PIO_MODE_2;
    }

  }

  return EFI_SUCCESS;
}

/**
  This function is used to calculate the best UDMA mode supported by specific IDE device

  @param IdentifyData   The identify data of specific IDE device.
  @param DisUDmaMode     Disqualified UDMA modes collection.
  @param SelectedMode   Available UDMA modes collection.

  @retval EFI_SUCCESS       Best UDMA modes are returned.
  @retval EFI_UNSUPPORTED   The device doesn't support UDMA mode,
                            or all supported modes have been disqualified.
**/
EFI_STATUS
CalculateBestUdmaMode (
  IN EFI_IDENTIFY_DATA  *IdentifyData,
  IN UINT16             *DisUDmaMode OPTIONAL,
  OUT UINT16            *SelectedMode
  )
{
  UINT16    TempMode;
  UINT16    DeviceUDmaMode;

  DeviceUDmaMode = 0;

  //
  // Check whether the WORD 88 (supported UltraDMA by drive) is valid
  //
  if ((IdentifyData->AtaData.field_validity & 0x04) == 0x00) {
    return EFI_UNSUPPORTED;
  }

  DeviceUDmaMode = IdentifyData->AtaData.ultra_dma_mode;
  DEBUG ((EFI_D_INFO, "CalculateBestUdmaMode: DeviceUDmaMode = %x\n", DeviceUDmaMode));
  DeviceUDmaMode &= 0x3f;
  TempMode = 0;                 // initialize it to UDMA-0

  while ((DeviceUDmaMode >>= 1) != 0) {
    TempMode++;
  }

  //
  // Degrade the UDMA mode if the mode has been disqualified
  //
  if (DisUDmaMode != NULL) {
    if (*DisUDmaMode == 0) {
      *SelectedMode = 0;
      return EFI_UNSUPPORTED;   // no mode below ATA_UDMA_MODE_0
    }

    if (TempMode >= *DisUDmaMode) {
      TempMode = (UINT16) (*DisUDmaMode - 1);
    }
  }

  //
  // Possible returned mode is between ATA_UDMA_MODE_0 and ATA_UDMA_MODE_5
  //
  *SelectedMode = TempMode;

  return EFI_SUCCESS;
}

/**
  The Entry Point of module. It follows the standard UEFI driver model.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The entry point is executed successfully.
  @retval other         Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeSataControllerDriver (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gSataControllerDriverBinding,
             ImageHandle,
             &gSataControllerComponentName,
             &gSataControllerComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Supported function of Driver Binding protocol for this driver.
  Test to see if this driver supports ControllerHandle.

  @param This                   Protocol instance pointer.
  @param Controller             Handle of device to test.
  @param RemainingDevicePath    A pointer to the device path.
                                it should be ignored by device driver.

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_ALREADY_STARTED   This driver is already running on this device.
  @retval other                 This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SataControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  PCI_TYPE00            PciData;

  //
  // Attempt to open PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Now further check the PCI header: Base Class (offset 0x0B) and
  // Sub Class (offset 0x0A). This controller should be an SATA controller
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (PciData.Hdr.ClassCode),
                        PciData.Hdr.ClassCode
                        );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (IS_PCI_IDE (&PciData) || IS_PCI_SATADPA (&PciData)) {
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  This routine is called right after the .Supported() called and 
  Start this driver on ControllerHandle.

  @param This                   Protocol instance pointer.
  @param Controller             Handle of device to bind driver to.
  @param RemainingDevicePath    A pointer to the device path.
                                it should be ignored by device driver.

  @retval EFI_SUCCESS           This driver is added to this device.
  @retval EFI_ALREADY_STARTED   This driver is already running on this device.
  @retval other                 Some error occurs when binding this driver to this device.

**/
EFI_STATUS
EFIAPI
SataControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  PCI_TYPE00                        PciData;
  EFI_SATA_CONTROLLER_PRIVATE_DATA  *SataPrivateData;
  UINT32                            Data32;
  UINTN                             ChannelDeviceCount;

  DEBUG ((EFI_D_INFO, "SataControllerStart START\n"));

  SataPrivateData = NULL;

  //
  // Now test and open PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SataControllerStart error return status = %r\n", Status));
    return Status;
  }

  //
  // Allocate Sata Private Data structure
  //
  SataPrivateData = AllocateZeroPool (sizeof (EFI_SATA_CONTROLLER_PRIVATE_DATA));
  if (SataPrivateData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Initialize Sata Private Data
  //
  SataPrivateData->Signature = SATA_CONTROLLER_SIGNATURE;
  SataPrivateData->PciIo = PciIo;
  SataPrivateData->IdeInit.GetChannelInfo = IdeInitGetChannelInfo;
  SataPrivateData->IdeInit.NotifyPhase = IdeInitNotifyPhase;
  SataPrivateData->IdeInit.SubmitData = IdeInitSubmitData;
  SataPrivateData->IdeInit.DisqualifyMode = IdeInitDisqualifyMode;
  SataPrivateData->IdeInit.CalculateMode = IdeInitCalculateMode;
  SataPrivateData->IdeInit.SetTiming = IdeInitSetTiming;
  SataPrivateData->IdeInit.EnumAll = SATA_ENUMER_ALL;

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (PciData.Hdr.ClassCode),
                        PciData.Hdr.ClassCode
                        );
  ASSERT_EFI_ERROR (Status);

  if (IS_PCI_IDE (&PciData)) {
    SataPrivateData->IdeInit.ChannelCount = IDE_MAX_CHANNEL;
    SataPrivateData->DeviceCount = IDE_MAX_DEVICES;
  } else if (IS_PCI_SATADPA (&PciData)) {
    //
    // Read Host Capability Register(CAP) to get Number of Ports(NPS) and Supports Port Multiplier(SPM)
    //   NPS is 0's based value indicating the maximum number of ports supported by the HBA silicon.
    //   A maximum of 32 ports can be supported. A value of '0h', indicating one port, is the minimum requirement.
    //
    Data32 = AhciReadReg (PciIo, R_AHCI_CAP);
    SataPrivateData->IdeInit.ChannelCount = (UINT8) (Data32 & B_AHCI_CAP_NPS) + 1;
    SataPrivateData->DeviceCount = AHCI_MAX_DEVICES;
    if ((Data32 & B_AHCI_CAP_SPM) == B_AHCI_CAP_SPM) {
      SataPrivateData->DeviceCount = AHCI_MULTI_MAX_DEVICES;
    }
  }

  ChannelDeviceCount = (UINTN) (SataPrivateData->IdeInit.ChannelCount) * (UINTN) (SataPrivateData->DeviceCount);
  SataPrivateData->DisqulifiedModes = AllocateZeroPool ((sizeof (EFI_ATA_COLLECTIVE_MODE)) * ChannelDeviceCount);
  if (SataPrivateData->DisqulifiedModes == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  SataPrivateData->IdentifyData = AllocateZeroPool ((sizeof (EFI_IDENTIFY_DATA)) * ChannelDeviceCount);
  if (SataPrivateData->IdentifyData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  SataPrivateData->IdentifyValid = AllocateZeroPool ((sizeof (BOOLEAN)) * ChannelDeviceCount);
  if (SataPrivateData->IdentifyValid == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Install IDE Controller Init Protocol to this instance
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiIdeControllerInitProtocolGuid,
                  &(SataPrivateData->IdeInit),
                  NULL
                  );

Done:
  if (EFI_ERROR (Status)) {

    gBS->CloseProtocol (
          Controller,
          &gEfiPciIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    if (SataPrivateData != NULL) {
      if (SataPrivateData->DisqulifiedModes != NULL) {
        FreePool (SataPrivateData->DisqulifiedModes);
      }
      if (SataPrivateData->IdentifyData != NULL) {
        FreePool (SataPrivateData->IdentifyData);
      }
      if (SataPrivateData->IdentifyValid != NULL) {
        FreePool (SataPrivateData->IdentifyValid);
      }
      FreePool (SataPrivateData);
    }
  }

  DEBUG ((EFI_D_INFO, "SataControllerStart END status = %r\n", Status));

  return Status;
}

/**
  Stop this driver on ControllerHandle.

  @param This               Protocol instance pointer.
  @param Controller         Handle of device to stop driver on.
  @param NumberOfChildren   Not used.
  @param ChildHandleBuffer  Not used.

  @retval EFI_SUCCESS   This driver is removed from this device.
  @retval other         Some error occurs when removing this driver from this device.

**/
EFI_STATUS
EFIAPI
SataControllerStop (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN UINTN                          NumberOfChildren,
  IN EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                        Status;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeInit;
  EFI_SATA_CONTROLLER_PRIVATE_DATA  *SataPrivateData;

  //
  // Open the produced protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIdeControllerInitProtocolGuid,
                  (VOID **) &IdeInit,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  SataPrivateData = SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS (IdeInit);
  ASSERT (SataPrivateData != NULL);

  //
  // Uninstall the IDE Controller Init Protocol from this instance
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiIdeControllerInitProtocolGuid,
                  &(SataPrivateData->IdeInit),
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SataPrivateData != NULL) {
    if (SataPrivateData->DisqulifiedModes != NULL) {
      FreePool (SataPrivateData->DisqulifiedModes);
    }
    if (SataPrivateData->IdentifyData != NULL) {
      FreePool (SataPrivateData->IdentifyData);
    }
    if (SataPrivateData->IdentifyValid != NULL) {
      FreePool (SataPrivateData->IdentifyValid);
    }
    FreePool (SataPrivateData);
  }

  //
  // Close protocols opened by Sata Controller driver
  //
  return gBS->CloseProtocol (
                Controller,
                &gEfiPciIoProtocolGuid,
                This->DriverBindingHandle,
                Controller
                );
}

//
// Interface functions of IDE_CONTROLLER_INIT protocol
//
/**
  This function can be used to obtain information about a specified channel.
  It's usually used by IDE Bus driver during enumeration process.

  @param This           the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel        Channel number. Parallel ATA (PATA) controllers can support up to two channels.
                        Advanced Host Controller Interface (AHCI) Serial ATA (SATA) controllers
                        can support up to 32 channels, each of which can have up to 1 device.
                        In the presence of a multiplier, each channel can have 15 devices.
  @param Enabled        TRUE if the channel is enabled. If the channel is disabled,
                        then it will no be enumerated.
  @param MaxDevices     For Parallel ATA (PATA) controllers, this number will either be 1 or 2.
                        For Serial ATA (SATA) controllers, it always be 1,
                        but with a port multiplier, this number can be as large as 15.

  @retval EFI_SUCCESS           Success to get channel information.
  @retval EFI_INVALID_PARAMETER Invalid channel id.
**/
EFI_STATUS
EFIAPI
IdeInitGetChannelInfo (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN UINT8                              Channel,
  OUT BOOLEAN                           *Enabled,
  OUT UINT8                             *MaxDevices
  )
{
  EFI_SATA_CONTROLLER_PRIVATE_DATA  *SataPrivateData;
  SataPrivateData = SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (SataPrivateData != NULL);

  if (Channel < This->ChannelCount) {
    *Enabled = TRUE;
    *MaxDevices = SataPrivateData->DeviceCount;
    return EFI_SUCCESS;
  }

  *Enabled = FALSE;
  return EFI_INVALID_PARAMETER;
}

/**
  This function is called by IdeBus driver before executing certain actions.
  This allows IDE Controller Init to prepare for each action.

  @param This       The EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Phase      Phase indicator defined by IDE_CONTROLLER_INIT protocol.
  @param Channel    Channel number.

  @retval EFI_SUCCESS   Success operation.
**/
EFI_STATUS
EFIAPI
IdeInitNotifyPhase (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN EFI_IDE_CONTROLLER_ENUM_PHASE      Phase,
  IN UINT8                              Channel
  )
{
  return EFI_SUCCESS;
}

/**
  This function is called by IdeBus driver to submit EFI_IDENTIFY_DATA data structure
  obtained from IDE deivce. This structure is used to set IDE timing.

  @param This           The EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel        Channel number.
  @param Device         Device number.
  @param IdentifyData   A pointer to EFI_IDENTIFY_DATA data structure.

  @retval EFI_SUCCESS           The information is accepted without any errors.
  @retval EFI_INVALID_PARAMETER Invalid channel id or device id.
**/
EFI_STATUS
EFIAPI
IdeInitSubmitData (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN UINT8                              Channel,
  IN UINT8                              Device,
  IN EFI_IDENTIFY_DATA                  *IdentifyData
  )
{
  EFI_SATA_CONTROLLER_PRIVATE_DATA  *SataPrivateData;
  SataPrivateData = SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (SataPrivateData != NULL);

  if ((Channel >= This->ChannelCount) || (Device >= SataPrivateData->DeviceCount)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Make a local copy of device's IdentifyData and mark the valid flag
  //
  if (IdentifyData != NULL) {
    CopyMem (
      &(SataPrivateData->IdentifyData[Channel * Device]),
      IdentifyData,
      sizeof (EFI_IDENTIFY_DATA)
      );

    SataPrivateData->IdentifyValid[Channel * Device] = TRUE;
  } else {
    SataPrivateData->IdentifyValid[Channel * Device] = FALSE;
  }

  return EFI_SUCCESS;
}

/**
  This function is called by IdeBus driver to disqualify unsupported operation
  mode on specfic IDE device.

  @param This       The EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel    Channel number.
  @param Device     Device number.
  @param BadModes   The modes that the device does not support and that
                    should be disqualified.

  @retval EFI_SUCCESS           The modes were accepted without any errors.
  @retval EFI_INVALID_PARAMETER Invalid channel id or device id.
**/
EFI_STATUS
EFIAPI
IdeInitDisqualifyMode (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN UINT8                              Channel,
  IN UINT8                              Device,
  IN EFI_ATA_COLLECTIVE_MODE            *BadModes
  )
{
  EFI_SATA_CONTROLLER_PRIVATE_DATA  *SataPrivateData;
  SataPrivateData = SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (SataPrivateData != NULL);

  if ((Channel >= This->ChannelCount) || (BadModes == NULL) || (Device >= SataPrivateData->DeviceCount)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Record the disqualified modes per channel per device. From ATA/ATAPI spec,
  // if a mode is not supported, the modes higher than it is also not supported.
  //
  CopyMem (
    &(SataPrivateData->DisqulifiedModes[Channel * Device]),
    BadModes,
    sizeof (EFI_ATA_COLLECTIVE_MODE)
    );

  return EFI_SUCCESS;
}

/**
  This function is called by IdeBus driver to calculate the best operation mode
  supported by specific IDE device.

  @param This               The EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel            Channel number.
  @param Device             Device number.
  @param SupportedModes     The optimum modes for the device.

  @retval EFI_SUCCESS           Supported modes are returned.
  @retval EFI_INVALID_PARAMETER Invalid channel id or device id.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate pool.
  @retval EFI_NOT_READY         Identify data of the device is not ready.
**/
EFI_STATUS
EFIAPI
IdeInitCalculateMode (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN UINT8                              Channel,
  IN UINT8                              Device,
  OUT EFI_ATA_COLLECTIVE_MODE           **SupportedModes
  )
{
  EFI_SATA_CONTROLLER_PRIVATE_DATA  *SataPrivateData;
  EFI_IDENTIFY_DATA                 *IdentifyData;
  BOOLEAN                           IdentifyValid;
  EFI_ATA_COLLECTIVE_MODE           *DisqulifiedModes;
  UINT16                            SelectedMode;
  EFI_STATUS                        Status;

  SataPrivateData = SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (SataPrivateData != NULL);

  if ((Channel >= This->ChannelCount) || (SupportedModes == NULL) || (Device >= SataPrivateData->DeviceCount)) {
    return EFI_INVALID_PARAMETER;
  }

  *SupportedModes = AllocateZeroPool (sizeof (EFI_ATA_COLLECTIVE_MODE));
  if (*SupportedModes == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IdentifyData = &(SataPrivateData->IdentifyData[Channel * Device]);
  IdentifyValid = SataPrivateData->IdentifyValid[Channel * Device];
  DisqulifiedModes = &(SataPrivateData->DisqulifiedModes[Channel * Device]);

  //
  // Make sure we've got the valid identify data of the device from SubmitData()
  //
  if (!IdentifyValid) {
    return EFI_NOT_READY;
  }

  Status = CalculateBestPioMode (
            IdentifyData,
            (DisqulifiedModes->PioMode.Valid ? ((UINT16 *) &(DisqulifiedModes->PioMode.Mode)) : NULL),
            &SelectedMode
            );
  if (!EFI_ERROR (Status)) {
    (*SupportedModes)->PioMode.Valid = TRUE;
    (*SupportedModes)->PioMode.Mode = SelectedMode;

  } else {
    (*SupportedModes)->PioMode.Valid = FALSE;
  }
  DEBUG ((EFI_D_INFO, "IdeInitCalculateMode: PioMode = %x\n", (*SupportedModes)->PioMode.Mode));

  Status = CalculateBestUdmaMode (
            IdentifyData,
            (DisqulifiedModes->UdmaMode.Valid ? ((UINT16 *) &(DisqulifiedModes->UdmaMode.Mode)) : NULL),
            &SelectedMode
            );

  if (!EFI_ERROR (Status)) {
    (*SupportedModes)->UdmaMode.Valid = TRUE;
    (*SupportedModes)->UdmaMode.Mode  = SelectedMode;

  } else {
    (*SupportedModes)->UdmaMode.Valid = FALSE;
  }
  DEBUG ((EFI_D_INFO, "IdeInitCalculateMode: UdmaMode = %x\n", (*SupportedModes)->UdmaMode.Mode));

  //
  // The modes other than PIO and UDMA are not supported
  //
  return EFI_SUCCESS;
}

/**
  This function is called by IdeBus driver to set appropriate timing on IDE
  controller according supported operation mode.

  @param This       The EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel    Channel number.
  @param Device     Device number.
  @param Modes      The modes to set.

  @retval EFI_SUCCESS   Sucess operation.
**/
EFI_STATUS
EFIAPI
IdeInitSetTiming (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN UINT8                              Channel,
  IN UINT8                              Device,
  IN EFI_ATA_COLLECTIVE_MODE            *Modes
  )
{
  return EFI_SUCCESS;
}
