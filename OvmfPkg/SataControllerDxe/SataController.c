/** @file
  This driver module produces IDE_CONTROLLER_INIT protocol for Sata Controllers.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  UINTN                             BailLogMask;
  EFI_STATUS                        Status;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINT64                            OriginalPciAttributes;
  PCI_TYPE00                        PciData;
  EFI_SATA_CONTROLLER_PRIVATE_DATA  *SataPrivateData;
  UINT32                            Data32;
  UINTN                             ChannelDeviceCount;

  DEBUG ((EFI_D_INFO, "SataControllerStart START\n"));

  BailLogMask = DEBUG_ERROR;
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
    if (Status == EFI_ALREADY_STARTED) {
      //
      // This is an expected condition for OpenProtocol() / BY_DRIVER, in a
      // DriverBindingStart() member function; degrade the log mask to
      // DEBUG_INFO.
      //
      BailLogMask = DEBUG_INFO;
    }
    goto Bail;
  }

  //
  // Save original PCI attributes, and enable IO space access, memory space
  // access, and Bus Master (DMA).
  //
  Status = PciIo->Attributes (PciIo, EfiPciIoAttributeOperationGet, 0,
                    &OriginalPciAttributes);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }
  Status = PciIo->Attributes (PciIo, EfiPciIoAttributeOperationEnable,
                    EFI_PCI_DEVICE_ENABLE, NULL);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  //
  // Allocate Sata Private Data structure
  //
  SataPrivateData = AllocateZeroPool (sizeof (EFI_SATA_CONTROLLER_PRIVATE_DATA));
  if (SataPrivateData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto RestorePciAttributes;
  }

  //
  // Initialize Sata Private Data
  //
  SataPrivateData->Signature = SATA_CONTROLLER_SIGNATURE;
  SataPrivateData->PciIo = PciIo;
  SataPrivateData->OriginalPciAttributes = OriginalPciAttributes;
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
  if (EFI_ERROR (Status)) {
    goto FreeSataPrivateData;
  }

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
    SataPrivateData->IdeInit.ChannelCount = (UINT8) ((Data32 & B_AHCI_CAP_NPS) + 1);
    SataPrivateData->DeviceCount = AHCI_MAX_DEVICES;
    if ((Data32 & B_AHCI_CAP_SPM) == B_AHCI_CAP_SPM) {
      SataPrivateData->DeviceCount = AHCI_MULTI_MAX_DEVICES;
    }
  }

  ChannelDeviceCount = (UINTN) (SataPrivateData->IdeInit.ChannelCount) * (UINTN) (SataPrivateData->DeviceCount);
  SataPrivateData->DisqualifiedModes = AllocateZeroPool ((sizeof (EFI_ATA_COLLECTIVE_MODE)) * ChannelDeviceCount);
  if (SataPrivateData->DisqualifiedModes == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeSataPrivateData;
  }

  SataPrivateData->IdentifyData = AllocateZeroPool ((sizeof (EFI_IDENTIFY_DATA)) * ChannelDeviceCount);
  if (SataPrivateData->IdentifyData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeDisqualifiedModes;
  }

  SataPrivateData->IdentifyValid = AllocateZeroPool ((sizeof (BOOLEAN)) * ChannelDeviceCount);
  if (SataPrivateData->IdentifyValid == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeIdentifyData;
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

  if (EFI_ERROR (Status)) {
    goto FreeIdentifyValid;
  }

  DEBUG ((EFI_D_INFO, "SataControllerStart END status = %r\n", Status));
  return Status;

FreeIdentifyValid:
  FreePool (SataPrivateData->IdentifyValid);

FreeIdentifyData:
  FreePool (SataPrivateData->IdentifyData);

FreeDisqualifiedModes:
  FreePool (SataPrivateData->DisqualifiedModes);

FreeSataPrivateData:
  FreePool (SataPrivateData);

RestorePciAttributes:
  PciIo->Attributes (PciIo, EfiPciIoAttributeOperationSet,
           OriginalPciAttributes, NULL);

ClosePciIo:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

Bail:
  DEBUG ((BailLogMask, "SataControllerStart error return status = %r\n",
    Status));
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
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINT64                            OriginalPciAttributes;

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

  PciIo                 = SataPrivateData->PciIo;
  OriginalPciAttributes = SataPrivateData->OriginalPciAttributes;

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

  if (SataPrivateData->DisqualifiedModes != NULL) {
    FreePool (SataPrivateData->DisqualifiedModes);
  }
  if (SataPrivateData->IdentifyData != NULL) {
    FreePool (SataPrivateData->IdentifyData);
  }
  if (SataPrivateData->IdentifyValid != NULL) {
    FreePool (SataPrivateData->IdentifyValid);
  }
  FreePool (SataPrivateData);

  //
  // Restore original PCI attributes
  //
  PciIo->Attributes (
           PciIo,
           EfiPciIoAttributeOperationSet,
           OriginalPciAttributes,
           NULL
           );

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

/**
  Calculate the flat array subscript of a (Channel, Device) pair.

  @param[in] SataPrivateData  The private data structure corresponding to the
                              SATA controller that attaches the device for
                              which the flat array subscript is being
                              calculated.

  @param[in] Channel          The channel (ie. port) number on the SATA
                              controller that the device is attached to.

  @param[in] Device           The device number on the channel.

  @return  The flat array subscript suitable for indexing DisqualifiedModes,
           IdentifyData, and IdentifyValid.
**/
STATIC
UINTN
FlatDeviceIndex (
  IN CONST EFI_SATA_CONTROLLER_PRIVATE_DATA  *SataPrivateData,
  IN UINTN                                   Channel,
  IN UINTN                                   Device
  )
{
  ASSERT (SataPrivateData != NULL);
  ASSERT (Channel < SataPrivateData->IdeInit.ChannelCount);
  ASSERT (Device < SataPrivateData->DeviceCount);

  return Channel * SataPrivateData->DeviceCount + Device;
}

//
// Interface functions of IDE_CONTROLLER_INIT protocol
//
/**
  Returns the information about the specified IDE channel.
  
  This function can be used to obtain information about a particular IDE channel.
  The driver entity uses this information during the enumeration process. 
  
  If Enabled is set to FALSE, the driver entity will not scan the channel. Note 
  that it will not prevent an operating system driver from scanning the channel.
  
  For most of today's controllers, MaxDevices will either be 1 or 2. For SATA 
  controllers, this value will always be 1. SATA configurations can contain SATA 
  port multipliers. SATA port multipliers behave like SATA bridges and can support
  up to 16 devices on the other side. If a SATA port out of the IDE controller 
  is connected to a port multiplier, MaxDevices will be set to the number of SATA 
  devices that the port multiplier supports. Because today's port multipliers 
  support up to fifteen SATA devices, this number can be as large as fifteen. The IDE  
  bus driver is required to scan for the presence of port multipliers behind an SATA 
  controller and enumerate up to MaxDevices number of devices behind the port 
  multiplier.    
  
  In this context, the devices behind a port multiplier constitute a channel.  
  
  @param[in]  This         The pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in]  Channel      Zero-based channel number.
  @param[out] Enabled      TRUE if this channel is enabled.  Disabled channels 
                           are not scanned to see if any devices are present.
  @param[out] MaxDevices   The maximum number of IDE devices that the bus driver
                           can expect on this channel.  For the ATA/ATAPI 
                           specification, version 6, this number will either be 
                           one or two. For Serial ATA (SATA) configurations with a 
                           port multiplier, this number can be as large as fifteen.

  @retval EFI_SUCCESS             Information was returned without any errors.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).

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
  The notifications from the driver entity that it is about to enter a certain
  phase of the IDE channel enumeration process.
  
  This function can be used to notify the IDE controller driver to perform 
  specific actions, including any chipset-specific initialization, so that the 
  chipset is ready to enter the next phase. Seven notification points are defined 
  at this time. 
  
  More synchronization points may be added as required in the future.  

  @param[in] This      The pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in] Phase     The phase during enumeration.
  @param[in] Channel   Zero-based channel number.

  @retval EFI_SUCCESS             The notification was accepted without any errors.
  @retval EFI_UNSUPPORTED         Phase is not supported.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_NOT_READY           This phase cannot be entered at this time; for 
                                  example, an attempt was made to enter a Phase 
                                  without having entered one or more previous 
                                  Phase.

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
  Submits the device information to the IDE controller driver.

  This function is used by the driver entity to pass detailed information about 
  a particular device to the IDE controller driver. The driver entity obtains 
  this information by issuing an ATA or ATAPI IDENTIFY_DEVICE command. IdentifyData
  is the pointer to the response data buffer. The IdentifyData buffer is owned 
  by the driver entity, and the IDE controller driver must make a local copy 
  of the entire buffer or parts of the buffer as needed. The original IdentifyData 
  buffer pointer may not be valid when
  
    - EFI_IDE_CONTROLLER_INIT_PROTOCOL.CalculateMode() or
    - EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyMode() is called at a later point.
    
  The IDE controller driver may consult various fields of EFI_IDENTIFY_DATA to 
  compute the optimum mode for the device. These fields are not limited to the 
  timing information. For example, an implementation of the IDE controller driver 
  may examine the vendor and type/mode field to match known bad drives.  
  
  The driver entity may submit drive information in any order, as long as it 
  submits information for all the devices belonging to the enumeration group 
  before EFI_IDE_CONTROLLER_INIT_PROTOCOL.CalculateMode() is called for any device
  in that enumeration group. If a device is absent, EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData()
  should be called with IdentifyData set to NULL.  The IDE controller driver may 
  not have any other mechanism to know whether a device is present or not. Therefore, 
  setting IdentifyData to NULL does not constitute an error condition. 
  EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData() can be called only once for a 
  given (Channel, Device) pair.  
    
  @param[in] This           A pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in] Channel        Zero-based channel number.
  @param[in] Device         Zero-based device number on the Channel.
  @param[in] IdentifyData   The device's response to the ATA IDENTIFY_DEVICE command.

  @retval EFI_SUCCESS             The information was accepted without any errors.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_INVALID_PARAMETER   Device is invalid.

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
  UINTN                             DeviceIndex;

  SataPrivateData = SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (SataPrivateData != NULL);

  if ((Channel >= This->ChannelCount) || (Device >= SataPrivateData->DeviceCount)) {
    return EFI_INVALID_PARAMETER;
  }

  DeviceIndex = FlatDeviceIndex (SataPrivateData, Channel, Device);

  //
  // Make a local copy of device's IdentifyData and mark the valid flag
  //
  if (IdentifyData != NULL) {
    CopyMem (
      &(SataPrivateData->IdentifyData[DeviceIndex]),
      IdentifyData,
      sizeof (EFI_IDENTIFY_DATA)
      );

    SataPrivateData->IdentifyValid[DeviceIndex] = TRUE;
  } else {
    SataPrivateData->IdentifyValid[DeviceIndex] = FALSE;
  }

  return EFI_SUCCESS;
}

/**
  Disqualifies specific modes for an IDE device.

  This function allows the driver entity or other drivers (such as platform 
  drivers) to reject certain timing modes and request the IDE controller driver
  to recalculate modes. This function allows the driver entity and the IDE 
  controller driver to negotiate the timings on a per-device basis. This function 
  is useful in the case of drives that lie about their capabilities. An example 
  is when the IDE device fails to accept the timing modes that are calculated 
  by the IDE controller driver based on the response to the Identify Drive command.

  If the driver entity does not want to limit the ATA timing modes and leave that 
  decision to the IDE controller driver, it can either not call this function for 
  the given device or call this function and set the Valid flag to FALSE for all 
  modes that are listed in EFI_ATA_COLLECTIVE_MODE.
  
  The driver entity may disqualify modes for a device in any order and any number 
  of times.
  
  This function can be called multiple times to invalidate multiple modes of the 
  same type (e.g., Programmed Input/Output [PIO] modes 3 and 4). See the ATA/ATAPI 
  specification for more information on PIO modes.  
  
  For Serial ATA (SATA) controllers, this member function can be used to disqualify
  a higher transfer rate mode on a given channel. For example, a platform driver
  may inform the IDE controller driver to not use second-generation (Gen2) speeds 
  for a certain SATA drive.
  
  @param[in] This       The pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in] Channel    The zero-based channel number.
  @param[in] Device     The zero-based device number on the Channel.
  @param[in] BadModes   The modes that the device does not support and that
                        should be disqualified.

  @retval EFI_SUCCESS             The modes were accepted without any errors.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_INVALID_PARAMETER   Device is invalid.
  @retval EFI_INVALID_PARAMETER   IdentifyData is NULL.
                                
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
  UINTN                             DeviceIndex;

  SataPrivateData = SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (SataPrivateData != NULL);

  if ((Channel >= This->ChannelCount) || (BadModes == NULL) || (Device >= SataPrivateData->DeviceCount)) {
    return EFI_INVALID_PARAMETER;
  }

  DeviceIndex = FlatDeviceIndex (SataPrivateData, Channel, Device);

  //
  // Record the disqualified modes per channel per device. From ATA/ATAPI spec,
  // if a mode is not supported, the modes higher than it is also not supported.
  //
  CopyMem (
    &(SataPrivateData->DisqualifiedModes[DeviceIndex]),
    BadModes,
    sizeof (EFI_ATA_COLLECTIVE_MODE)
    );

  return EFI_SUCCESS;
}

/**
  Returns the information about the optimum modes for the specified IDE device.

  This function is used by the driver entity to obtain the optimum ATA modes for
  a specific device.  The IDE controller driver takes into account the following 
  while calculating the mode:
    - The IdentifyData inputs to EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData()
    - The BadModes inputs to EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyMode()

  The driver entity is required to call EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData() 
  for all the devices that belong to an enumeration group before calling 
  EFI_IDE_CONTROLLER_INIT_PROTOCOL.CalculateMode() for any device in the same group.  
  
  The IDE controller driver will use controller- and possibly platform-specific 
  algorithms to arrive at SupportedModes.  The IDE controller may base its 
  decision on user preferences and other considerations as well. This function 
  may be called multiple times because the driver entity may renegotiate the mode 
  with the IDE controller driver using EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyMode().
    
  The driver entity may collect timing information for various devices in any 
  order. The driver entity is responsible for making sure that all the dependencies
  are satisfied. For example, the SupportedModes information for device A that 
  was previously returned may become stale after a call to 
  EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyMode() for device B.
  
  The buffer SupportedModes is allocated by the callee because the caller does 
  not necessarily know the size of the buffer. The type EFI_ATA_COLLECTIVE_MODE 
  is defined in a way that allows for future extensibility and can be of variable 
  length. This memory pool should be deallocated by the caller when it is no 
  longer necessary.  
  
  The IDE controller driver for a Serial ATA (SATA) controller can use this 
  member function to force a lower speed (first-generation [Gen1] speeds on a 
  second-generation [Gen2]-capable hardware).  The IDE controller driver can 
  also allow the driver entity to stay with the speed that has been negotiated 
  by the physical layer.
  
  @param[in]  This             The pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in]  Channel          A zero-based channel number.
  @param[in]  Device           A zero-based device number on the Channel.
  @param[out] SupportedModes   The optimum modes for the device.

  @retval EFI_SUCCESS             SupportedModes was returned.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_INVALID_PARAMETER   Device is invalid. 
  @retval EFI_INVALID_PARAMETER   SupportedModes is NULL.
  @retval EFI_NOT_READY           Modes cannot be calculated due to a lack of 
                                  data.  This error may happen if 
                                  EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData() 
                                  and EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyData() 
                                  were not called for at least one drive in the 
                                  same enumeration group.

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
  EFI_ATA_COLLECTIVE_MODE           *DisqualifiedModes;
  UINT16                            SelectedMode;
  EFI_STATUS                        Status;
  UINTN                             DeviceIndex;

  SataPrivateData = SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (SataPrivateData != NULL);

  if ((Channel >= This->ChannelCount) || (SupportedModes == NULL) || (Device >= SataPrivateData->DeviceCount)) {
    return EFI_INVALID_PARAMETER;
  }

  *SupportedModes = AllocateZeroPool (sizeof (EFI_ATA_COLLECTIVE_MODE));
  if (*SupportedModes == NULL) {
    ASSERT (*SupportedModes != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  DeviceIndex = FlatDeviceIndex (SataPrivateData, Channel, Device);

  IdentifyData = &(SataPrivateData->IdentifyData[DeviceIndex]);
  IdentifyValid = SataPrivateData->IdentifyValid[DeviceIndex];
  DisqualifiedModes = &(SataPrivateData->DisqualifiedModes[DeviceIndex]);

  //
  // Make sure we've got the valid identify data of the device from SubmitData()
  //
  if (!IdentifyValid) {
    FreePool (*SupportedModes);
    return EFI_NOT_READY;
  }

  Status = CalculateBestPioMode (
            IdentifyData,
            (DisqualifiedModes->PioMode.Valid ? ((UINT16 *) &(DisqualifiedModes->PioMode.Mode)) : NULL),
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
            (DisqualifiedModes->UdmaMode.Valid ? ((UINT16 *) &(DisqualifiedModes->UdmaMode.Mode)) : NULL),
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
  Commands the IDE controller driver to program the IDE controller hardware
  so that the specified device can operate at the specified mode.

  This function is used by the driver entity to instruct the IDE controller 
  driver to program the IDE controller hardware to the specified modes. This 
  function can be called only once for a particular device. For a Serial ATA 
  (SATA) Advanced Host Controller Interface (AHCI) controller, no controller-
  specific programming may be required.

  @param[in] This      Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in] Channel   Zero-based channel number.
  @param[in] Device    Zero-based device number on the Channel.
  @param[in] Modes     The modes to set.

  @retval EFI_SUCCESS             The command was accepted without any errors.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_INVALID_PARAMETER   Device is invalid.
  @retval EFI_NOT_READY           Modes cannot be set at this time due to lack of data.
  @retval EFI_DEVICE_ERROR        Modes cannot be set due to hardware failure.
                                  The driver entity should not use this device.

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
