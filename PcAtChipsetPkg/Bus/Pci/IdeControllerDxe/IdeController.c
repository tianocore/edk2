/** @file
  This driver module produces IDE_CONTROLLER_INIT protocol and will be used by
  IDE Bus driver to support platform dependent timing information. This driver
  is responsible for early initialization of IDE controller.

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN  EFI_IDE_CONTROLLER_ENUM_PHASE      Phase,
  IN  UINT8                              Channel
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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_IDENTIFY_DATA                   *IdentifyData
  )
{
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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_ATA_COLLECTIVE_MODE             *BadModes
  )
{
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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL    *This,
  IN  UINT8                               Channel,
  IN  UINT8                               Device,
  IN  EFI_ATA_COLLECTIVE_MODE             *Modes
  )
{
  return EFI_SUCCESS;
}
