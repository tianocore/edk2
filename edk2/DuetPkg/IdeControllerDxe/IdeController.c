/*++

Copyright (c) 2006 - 2007 Intel Corporation. All rights reserved
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

  IdeController.c

Abstract:
    
  This driver module produces IDE_CONTROLLER_INIT protocol and will be used by 
  IDE Bus driver to support platform dependent timing information. This driver
  is responsible for early initialization of IDE controller.

Revision History
--*/

#include "IdeController.h"
#include "IdeData.h"

//
// IDE Controller Init Guid
//
EFI_GUID
  gIdeControllerDriverGuid = { 0x91e365e9, 0xe0c0, 0x4647, 0xb0, 0xeb, 0xf6, 0x78, 0xf6, 0x21, 0xf8, 0x8d };

//
//  EFI_DRIVER_BINDING_PROTOCOL instance
//
EFI_DRIVER_BINDING_PROTOCOL gIdeControllerDriverBinding = {
  IdeControllerSupported,
  IdeControllerStart,
  IdeControllerStop,
  0xa,
  NULL,
  NULL
};

//
// Internal function definitions
//
EFI_STATUS
EnableNativeIdeDecode (
  IN  EFI_PCI_IO_PROTOCOL        *PciIo
  );

EFI_STATUS
EnableLegacyIdeDecode (
  EFI_EVENT  Event,
  VOID       *Context
  );

EFI_STATUS
IdeDetectCableType (
  IN  UINT8                      Channel,
  IN  UINT8                      Device,
  IN  EFI_PCI_IO_PROTOCOL        *PciIo,
  IN  EFI_IDENTIFY_DATA          *IdentifyData
  );

EFI_STATUS
AdjustUdmaModeByCableType (
  IN     UINT8                   Channel,
  IN     UINT8                   Device,
  IN     EFI_PCI_IO_PROTOCOL     *PciIo,
  IN OUT EFI_ATA_COLLECTIVE_MODE *Modes
  );

EFI_STATUS
CalculateBestPioMode (
  IN  EFI_IDENTIFY_DATA          * IdentifyData,
  IN  UINT16                     *DisPioMode OPTIONAL,
  OUT UINT16                     *SelectedMode
  );

EFI_STATUS
CalculateBestUdmaMode (
  IN  EFI_IDENTIFY_DATA          * IdentifyData,
  IN  UINT16                     *DisUDmaMode OPTIONAL,
  OUT UINT16                     *SelectedMode
  );

EFI_STATUS
IdeInitSetUdmaTiming (
  IN  UINT8                       Channel,
  IN  UINT8                       Device,
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  IN  EFI_ATA_COLLECTIVE_MODE     *Modes
  );

EFI_STATUS
IdeInitSetPioTiming (
  IN  UINT8                       Channel,
  IN  UINT8                       Device,
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  IN  EFI_IDENTIFY_DATA           *IdentifyData,
  IN  EFI_ATA_COLLECTIVE_MODE     *Modes
  );

//
// *************************************
//  IdeController Driver Entry Point
// *************************************
//
EFI_STATUS
EFIAPI
InitializeIdeControllerDriver (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
/*++
  Routine Description:
  
    Chipset Ide Driver EntryPoint function. It follows the standard EFI driver 
    model. It's called by StartImage() of DXE Core
    
  Argments:
  
    ImageHnadle  -- While the driver image loaded be the ImageLoader(), 
                    an image handle is assigned to this driver binary, 
                    all activities of the driver is tied to this ImageHandle
    *SystemTable -- A pointer to the system table, for all BS(Boo Services) and
                    RT(Runtime Services)

  Retruns:
  
    Always call EfiLibInstallDriverBindingProtocol( ) and retrun the result

--*/
{

  return  EfiLibInstallDriverBindingComponentName2 (
            ImageHandle,
            SystemTable,
            &gIdeControllerDriverBinding,
            ImageHandle,
            &gIdeControllerName,
            &gIdeControllerName2
            );
}

EFI_STATUS
EFIAPI
IdeControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
/*++

  Routine Description:
  
  Register Driver Binding protocol for this driver.
  
  Arguments:
  
    This                 -- a pointer points to the Binding Protocol instance
    Controller           -- The handle of controller to be tested. 
    *RemainingDevicePath -- A pointer to the device path. Ignored by device
                            driver but used by bus driver

  Returns:

    EFI_SUCCESS          -- Driver loaded.
    other                -- Driver not loaded.
--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                PciData;

  //
  // Ide Controller is a device driver, and should ingore the
  // "RemainingDevicePath" according to EFI spec
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID *) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    //
    // EFI_ALREADY_STARTED is also an error
    //
    return Status;
  }
  //
  // Close the protocol because we don't use it here
  //
  gBS->CloseProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Now test the EfiPciIoProtocol
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
                        0,
                        sizeof (PciData),
                        &PciData
                        );

  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiPciIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return EFI_UNSUPPORTED;
  }
  //
  // Examine Ide PCI Configuration table fields
  //
  if ((PciData.Hdr.ClassCode[2] != PCI_CLASS_MASS_STORAGE) ||
      (PciData.Hdr.ClassCode[1] != PCI_SUB_CLASS_IDE)
      ) {

    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

EFI_STATUS
EFIAPI
IdeControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
  
    This routine is called right after the .Supported() called and return 
    EFI_SUCCESS. Notes: The supported protocols are checked but the Protocols
    are closed.

  Arguments:
      
    This                 -- a pointer points to the Binding Protocol instance
    Controller           -- The handle of controller to be tested. Parameter
                            passed by the caller
    *RemainingDevicePath -- A pointer to the device path. Should be ignored by
                            device driver
--*/
{
  EFI_STATUS                      Status;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  EFI_IDE_CONTROLLER_PRIVATE_DATA *IdePrivateData;

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
  // Status == 0 - A normal execution flow, SUCCESS and the program proceeds.
  // Status == ALREADY_STARTED - A non-zero Status code returned. It indicates
  //           that the protocol has been opened and should be treated as a
  //           normal condition and the program proceeds. The Protocol will not
  //           opened 'again' by this call.
  // Status != ALREADY_STARTED - Error status, terminate program execution
  //
  if (EFI_ERROR (Status)) {
    //
    // EFI_ALREADY_STARTED is also an error
    //
    return Status;
  }
  //
  // Allocate Ide private data structure
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_IDE_CONTROLLER_PRIVATE_DATA),
                  (VOID **) &IdePrivateData
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Initialize Ide controller private data
  //
  ZeroMem (IdePrivateData, sizeof (EFI_IDE_CONTROLLER_PRIVATE_DATA));
  IdePrivateData->Signature               = IDE_CONTROLLER_SIGNATURE;
  IdePrivateData->PciIo                   = PciIo;
  IdePrivateData->IdeInit.GetChannelInfo  = IdeInitGetChannelInfo;
  IdePrivateData->IdeInit.NotifyPhase     = IdeInitNotifyPhase;
  IdePrivateData->IdeInit.SubmitData      = IdeInitSubmitData;
  IdePrivateData->IdeInit.DisqualifyMode  = IdeInitDisqualifyMode;
  IdePrivateData->IdeInit.CalculateMode   = IdeInitCalculateMode;
  IdePrivateData->IdeInit.SetTiming       = IdeInitSetTiming;
  IdePrivateData->IdeInit.EnumAll         = ICH_IDE_ENUMER_ALL;
  IdePrivateData->IdeInit.ChannelCount    = ICH_IDE_MAX_CHANNEL;

  //
  // Install IDE_CONTROLLER_INIT protocol & private data to this instance
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gIdeControllerDriverGuid,
                  IdePrivateData,
                  &gEfiIdeControllerInitProtocolGuid,
                  &(IdePrivateData->IdeInit),
                  NULL
                  );

  return Status;
}

EFI_STATUS
EFIAPI
IdeControllerStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
/*++
  
  Routine Description:
    Stop this driver on Controller Handle. 

  Arguments:
    This              - Protocol instance pointer.
    Controller        - Handle of device to stop driver on 
    NumberOfChildren  - Not used
    ChildHandleBuffer - Not used

  Returns:
    EFI_SUCCESS       - This driver is removed DeviceHandle
    other             - This driver was not removed from this device
  
--*/
{
  EFI_STATUS                      Status;
  EFI_IDE_CONTROLLER_PRIVATE_DATA *IdePrivateData;

  //
  // Get private data
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gIdeControllerDriverGuid,
                  (VOID **) &IdePrivateData,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Close protocols opened by Ide controller driver
  //
  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );

  gBS->UninstallMultipleProtocolInterfaces (
        Controller,
        &gIdeControllerDriverGuid,
        IdePrivateData,
        &gEfiIdeControllerInitProtocolGuid,
        &(IdePrivateData->IdeInit),
        NULL
        );

  gBS->FreePool (IdePrivateData);

  return EFI_SUCCESS;
}

//
// Interface functions of IDE_CONTROLLER_INIT protocol
//
EFI_STATUS
EFIAPI
IdeInitGetChannelInfo (
  IN   EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN   UINT8                            Channel,
  OUT  BOOLEAN                          *Enabled,
  OUT  UINT8                            *MaxDevices
  )
/*++
Routine Description:

  This function can be used to obtain information about a specified channel. 
  It's usually used by IDE Bus driver during enumeration process.

Arguments:

  This       -- the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  Channel    -- Channel number (0 based, either 0 or 1)
  Enabled    -- TRUE if the channel is enabled. If the channel is disabled, 
                then it will no be enumerated.
  MaxDevices -- The Max number of IDE devices that the bus driver can expect
                on this channel. For ATA/ATAPI, this number is either 1 or 2.

Returns:
  EFI_STATUS 

--*/
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


EFI_STATUS
EFIAPI
IdeInitNotifyPhase (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN  EFI_IDE_CONTROLLER_ENUM_PHASE      Phase,
  IN  UINT8                              Channel
  )
/*++

Routine Description:

  This function is called by IdeBus driver before executing certain actions. 
  This allows IDE Controller Init to prepare for each action.

Arguments:

  This     -- the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  Phase    -- phase indicator defined by IDE_CONTROLLER_INIT protocol
  Channel  -- Channel number (0 based, either 0 or 1)

Returns:
    
--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IdeInitSubmitData (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_IDENTIFY_DATA                   *IdentifyData
  )
/*++

Routine Description:

  This function is called by IdeBus driver to submit EFI_IDENTIFY_DATA data structure
  obtained from IDE deivce. This structure is used to set IDE timing

Arguments:

  This         -- the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  Channel      -- IDE channel number (0 based, either 0 or 1)
  Device       -- IDE device number
  IdentifyData -- A pointer to EFI_IDENTIFY_DATA data structure

Returns:
    
--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IdeInitDisqualifyMode (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_ATA_COLLECTIVE_MODE             *BadModes
  )
/*++

Routine Description:

  This function is called by IdeBus driver to disqualify unsupported operation
  mode on specfic IDE device

Arguments:

  This     -- the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  Channel  -- IDE channel number (0 based, either 0 or 1)
  Device   -- IDE device number
  BadModes -- Operation mode indicator

Returns:
    
--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IdeInitCalculateMode (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL       *This,
  IN  UINT8                                  Channel,
  IN  UINT8                                  Device,
  OUT EFI_ATA_COLLECTIVE_MODE                **SupportedModes
  )
/*++

Routine Description:

  This function is called by IdeBus driver to calculate the best operation mode
  supported by specific IDE device

Arguments:

  This           -- the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  Channel        -- IDE channel number (0 based, either 0 or 1)
  Device         -- IDE device number
  SupportedModes -- Modes collection supported by IDE device

Returns:
    
--*/
{
  if (Channel >= ICH_IDE_MAX_CHANNEL || Device >= ICH_IDE_MAX_DEVICES) {
    return EFI_INVALID_PARAMETER;
  }

  *SupportedModes = AllocateZeroPool (sizeof (EFI_ATA_COLLECTIVE_MODE));
  if (*SupportedModes == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // In EoE enviroment, when nothing is known about the platform hardware,
  // just set the mode to lowest PIO mode for compatibility.
  //
  (*SupportedModes)->PioMode.Valid  = TRUE;
  (*SupportedModes)->PioMode.Mode   = AtaPioModeBelow2;
  (*SupportedModes)->UdmaMode.Valid = FALSE;
  (*SupportedModes)->SingleWordDmaMode.Valid = FALSE;
  (*SupportedModes)->MultiWordDmaMode.Valid = FALSE;

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
IdeInitSetTiming (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_ATA_COLLECTIVE_MODE             *Modes
  )
/*++

Routine Description:

  This function is called by IdeBus driver to set appropriate timing on IDE
  controller according supported operation mode

Arguments:

  This           -- the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  Channel        -- IDE channel number (0 based, either 0 or 1)
  Device         -- IDE device number

Returns:
    
--*/
{
  return EFI_SUCCESS;
}


