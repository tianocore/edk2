/** @file
  Header file for IDE controller driver.

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IDE_CONTROLLER_H_
#define _IDE_CONTROLLER_H_

#include <Uefi.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/PciIo.h>
#include <Protocol/IdeControllerInit.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <IndustryStandard/Pci.h>

//
// Global Variables definitions
//
extern EFI_DRIVER_BINDING_PROTOCOL  gIdeControllerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gIdeControllerComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gIdeControllerComponentName2;

///
/// Supports 2 channel max
///
#define ICH_IDE_MAX_CHANNEL 0x02

///
/// Supports 2 devices max
///
#define ICH_IDE_MAX_DEVICES 0x02
#define ICH_IDE_ENUMER_ALL  FALSE

//
// Driver binding functions declaration
//
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
  IN EFI_DRIVER_BINDING_PROTOCOL       *This,
  IN EFI_HANDLE                        Controller,
  IN EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath
  )
;

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
  IN EFI_DRIVER_BINDING_PROTOCOL        *This,
  IN EFI_HANDLE                         Controller,
  IN EFI_DEVICE_PATH_PROTOCOL           *RemainingDevicePath
  )
;

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
  IN  EFI_DRIVER_BINDING_PROTOCOL       *This,
  IN  EFI_HANDLE                        Controller,
  IN  UINTN                             NumberOfChildren,
  IN  EFI_HANDLE                        *ChildHandleBuffer
  )
;

//
// IDE controller init functions declaration
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
;

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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  EFI_IDE_CONTROLLER_ENUM_PHASE     Phase,
  IN  UINT8                             Channel
  )
;

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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  IN  EFI_IDENTIFY_DATA                 *IdentifyData
  )
;

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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  IN  EFI_ATA_COLLECTIVE_MODE           *BadModes
  )
;

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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  OUT EFI_ATA_COLLECTIVE_MODE           **SupportedModes
  )
;

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
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  IN  EFI_ATA_COLLECTIVE_MODE           *Modes
  )
;

//
// Forward reference declaration
//
/**
  Retrieves a Unicode string that is the user readable name of the EFI Driver.

  @param This           A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param Language       A pointer to a three character ISO 639-2 language identifier.
                        This is the language of the driver name that that the caller
                        is requesting, and it must match one of the languages specified
                        in SupportedLanguages.  The number of languages supported by a
                        driver is up to the driver writer.
  @param DriverName     A pointer to the Unicode string to return.  This Unicode string
                        is the name of the driver specified by This in the language
                        specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by This
                                and the language specified by Language was returned
                                in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.
**/
EFI_STATUS
EFIAPI
IdeControllerComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
;

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an EFI Driver.

  @param This                   A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param ControllerHandle       The handle of a controller that the driver specified by
                                This is managing.  This handle specifies the controller
                                whose name is to be returned.
  @param OPTIONAL   ChildHandle The handle of the child controller to retrieve the name
                                of.  This is an optional parameter that may be NULL.  It
                                will be NULL for device drivers.  It will also be NULL
                                for a bus drivers that wish to retrieve the name of the
                                bus controller.  It will not be NULL for a bus driver
                                that wishes to retrieve the name of a child controller.
  @param Language               A pointer to a three character ISO 639-2 language
                                identifier.  This is the language of the controller name
                                that that the caller is requesting, and it must match one
                                of the languages specified in SupportedLanguages.  The
                                number of languages supported by a driver is up to the
                                driver writer.
  @param ControllerName         A pointer to the Unicode string to return.  This Unicode
                                string is the name of the controller specified by
                                ControllerHandle and ChildHandle in the language
                                specified by Language from the point of view of the
                                driver specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in the
                                language specified by Language for the driver
                                specified by This was returned in DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.
**/
EFI_STATUS
EFIAPI
IdeControllerComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  )
;

#endif
