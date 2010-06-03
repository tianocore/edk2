/** @file
  This driver module produces IDE_CONTROLLER_INIT protocol and will be used by
  IDE Bus driver to support platform dependent timing information. This driver
  is responsible for early initialization of IDE controller.

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IdeController.h"

///
///  EFI_DRIVER_BINDING_PROTOCOL instance
///
EFI_DRIVER_BINDING_PROTOCOL gIdeControllerDriverBinding = {
  IdeControllerSupported,
  IdeControllerStart,
  IdeControllerStop,
  0xa,
  NULL,
  NULL
};

///
///  EFI_IDE_CONTROLLER_PROVATE_DATA Template
///
EFI_IDE_CONTROLLER_INIT_PROTOCOL  gEfiIdeControllerInit = {
  IdeInitGetChannelInfo,
  IdeInitNotifyPhase,
  IdeInitSubmitData,
  IdeInitDisqualifyMode,
  IdeInitCalculateMode,
  IdeInitSetTiming,
  ICH_IDE_ENUMER_ALL,
  ICH_IDE_MAX_CHANNEL
};

///
///  EFI_ATA_COLLECTIVE_MODE Template
///
EFI_ATA_COLLECTIVE_MODE  gEfiAtaCollectiveModeTemplate = {
  {
    TRUE,                   ///< PioMode.Valid
    0                       ///< PioMode.Mode
  },
  {
    TRUE,                   ///< SingleWordDmaMode.Valid
    0
  },
  {
    FALSE,                  ///< MultiWordDmaMode.Valid
    0
  },
  {
    TRUE,                   ///< UdmaMode.Valid
    0                       ///< UdmaMode.Mode
  }
};

/**
  Chipset Ide Driver EntryPoint function. It follows the standard EFI driver model.
  It's called by StartImage() of DXE Core.

  @param ImageHandle    While the driver image loaded be the ImageLoader(),
                        an image handle is assigned to this driver binary,
                        all activities of the driver is tied to this ImageHandle
  @param SystemTable    A pointer to the system table, for all BS(Boo Services) and
                        RT(Runtime Services)

  @return EFI_STATUS    Status of  EfiLibInstallDriverBindingComponentName2().
**/
EFI_STATUS
EFIAPI
InitializeIdeControllerDriver (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIdeControllerDriverBinding,
             ImageHandle,
             &gIdeControllerComponentName,
             &gIdeControllerComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Register Driver Binding protocol for this driver.

  @param This                   A pointer points to the Binding Protocol instance
  @param Controller             The handle of controller to be tested.
  @param RemainingDevicePath    A pointer to the device path. Ignored by device
                                driver but used by bus driver

  @retval EFI_SUCCESS           Driver loaded.
  @retval !EFI_SUCESS           Driver not loaded.
**/
EFI_STATUS
EFIAPI
IdeControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT8                     PciClass;
  UINT8                     PciSubClass;

  //
  // Attempt to Open PCI I/O Protocol
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
    return Status;
  }

  //
  // Now further check the PCI header: Base class (offset 0x0B) and
  // Sub Class (offset 0x0A). This controller should be an Ide controller
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET + 2,
                        1,
                        &PciClass
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET + 1,
                        1,
                        &PciSubClass
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Examine Ide PCI Configuration table fields
  //
  if ((PciClass != PCI_CLASS_MASS_STORAGE) || (PciSubClass != PCI_CLASS_MASS_STORAGE_IDE)) {
    Status = EFI_UNSUPPORTED;
  }

Done:
  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

/**
  This routine is called right after the .Supported() called and return
  EFI_SUCCESS. Notes: The supported protocols are checked but the Protocols
  are closed.

  @param This                   A pointer points to the Binding Protocol instance
  @param Controller             The handle of controller to be tested. Parameter
                                passed by the caller
  @param RemainingDevicePath    A pointer to the device path. Should be ignored by
                                device driver

  @return EFI_STATUS            Status of InstallMultipleProtocolInterfaces()
**/
EFI_STATUS
EFIAPI
IdeControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;

  //
  // Now test and open the EfiPciIoProtocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  //
  // Status == EFI_SUCCESS - A normal execution flow, SUCCESS and the program proceeds.
  // Status == ALREADY_STARTED - A non-zero Status code returned. It indicates
  //           that the protocol has been opened and should be treated as a
  //           normal condition and the program proceeds. The Protocol will not
  //           opened 'again' by this call.
  // Status != ALREADY_STARTED - Error status, terminate program execution
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install IDE_CONTROLLER_INIT protocol
  //
  return gBS->InstallMultipleProtocolInterfaces (
                &Controller,
                &gEfiIdeControllerInitProtocolGuid, &gEfiIdeControllerInit,
                NULL
                );
}

/**
  Stop this driver on Controller Handle.

  @param This               Protocol instance pointer.
  @param Controller         Handle of device to stop driver on
  @param NumberOfChildren   Not used
  @param ChildHandleBuffer  Not used

  @retval EFI_SUCESS        This driver is removed DeviceHandle
  @retval !EFI_SUCCESS      This driver was not removed from this device
**/
EFI_STATUS
EFIAPI
IdeControllerStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                        Status;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeControllerInit;

  //
  // Open the produced protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIdeControllerInitProtocolGuid,
                  (VOID **) &IdeControllerInit,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
     return EFI_UNSUPPORTED;
  }

  //
  // Make sure the protocol was produced by this driver
  //
  if (IdeControllerInit != &gEfiIdeControllerInit) {
    return EFI_UNSUPPORTED;
  }

  //
  // Uninstall the IDE Controller Init Protocol
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiIdeControllerInitProtocolGuid, &gEfiIdeControllerInit,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close protocols opened by Ide controller driver
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
  @param Channel        Channel number (0 based, either 0 or 1)
  @param Enabled        TRUE if the channel is enabled. If the channel is disabled,
                        then it will no be enumerated.
  @param MaxDevices     The Max number of IDE devices that the bus driver can expect
                        on this channel. For ATA/ATAPI, this number is either 1 or 2.

  @retval EFI_SUCCESS           Success to get channel information
  @retval EFI_INVALID_PARAMETER Invalid channel id.
**/
EFI_STATUS
EFIAPI
IdeInitGetChannelInfo (
  IN   EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN   UINT8                            Channel,
  OUT  BOOLEAN                          *Enabled,
  OUT  UINT8                            *MaxDevices
  )
{
  //
  // Channel number (0 based, either 0 or 1)
  //
  if (Channel < ICH_IDE_MAX_CHANNEL) {
    *Enabled    = TRUE;
    *MaxDevices = ICH_IDE_MAX_DEVICES;
    return EFI_SUCCESS;
  }

  *Enabled = FALSE;
  return EFI_INVALID_PARAMETER;
}

/**
  This function is called by IdeBus driver before executing certain actions.
  This allows IDE Controller Init to prepare for each action.

  @param This       the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Phase      phase indicator defined by IDE_CONTROLLER_INIT protocol
  @param Channel    Channel number (0 based, either 0 or 1)

  @return EFI_SUCCESS Success operation.
**/
EFI_STATUS
EFIAPI
IdeInitNotifyPhase (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN  EFI_IDE_CONTROLLER_ENUM_PHASE      Phase,
  IN  UINT8                              Channel
  )
{
  return EFI_SUCCESS;
}

/**
  This function is called by IdeBus driver to submit EFI_IDENTIFY_DATA data structure
  obtained from IDE deivce. This structure is used to set IDE timing

  @param This           The EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel        IDE channel number (0 based, either 0 or 1)
  @param Device         IDE device number
  @param IdentifyData   A pointer to EFI_IDENTIFY_DATA data structure

  @return EFI_SUCCESS   Success operation.
**/
EFI_STATUS
EFIAPI
IdeInitSubmitData (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_IDENTIFY_DATA                   *IdentifyData
  )
{
  return EFI_SUCCESS;
}

/**
  This function is called by IdeBus driver to disqualify unsupported operation
  mode on specfic IDE device

  @param This       the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel    IDE channel number (0 based, either 0 or 1)
  @param Device     IDE device number
  @param BadModes   Operation mode indicator

  @return EFI_SUCCESS Success operation.
**/
EFI_STATUS
EFIAPI
IdeInitDisqualifyMode (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_ATA_COLLECTIVE_MODE             *BadModes
  )
{
  return EFI_SUCCESS;
}

/**
  This function is called by IdeBus driver to calculate the best operation mode
  supported by specific IDE device

  @param This               the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel            IDE channel number (0 based, either 0 or 1)
  @param Device             IDE device number
  @param SupportedModes     Modes collection supported by IDE device

  @retval EFI_OUT_OF_RESOURCES  Fail to allocate pool.
  @retval EFI_INVALID_PARAMETER Invalid channel id and device id.
**/
EFI_STATUS
EFIAPI
IdeInitCalculateMode (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL       *This,
  IN  UINT8                                  Channel,
  IN  UINT8                                  Device,
  OUT EFI_ATA_COLLECTIVE_MODE                **SupportedModes
  )
{
  if (Channel >= ICH_IDE_MAX_CHANNEL || Device >= ICH_IDE_MAX_DEVICES) {
    return EFI_INVALID_PARAMETER;
  }

  *SupportedModes = AllocateCopyPool (sizeof (EFI_ATA_COLLECTIVE_MODE), &gEfiAtaCollectiveModeTemplate);
  if (*SupportedModes == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  This function is called by IdeBus driver to set appropriate timing on IDE
  controller according supported operation mode.

  @param This       the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel    IDE channel number (0 based, either 0 or 1)
  @param Device     IDE device number
  @param Modes      IDE device modes

  @retval EFI_SUCCESS Sucess operation.
**/
EFI_STATUS
EFIAPI
IdeInitSetTiming (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_ATA_COLLECTIVE_MODE             *Modes
  )
{
  return EFI_SUCCESS;
}
