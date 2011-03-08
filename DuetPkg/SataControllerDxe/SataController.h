/** @file
  Header file for Sata Controller driver.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SATA_CONTROLLER_H_
#define _SATA_CONTROLLER_H_

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
extern EFI_DRIVER_BINDING_PROTOCOL  gSataControllerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gSataControllerComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gSataControllerComponentName2;

#define AHCI_BAR_INDEX 0x05
#define R_AHCI_CAP 0x0
#define   B_AHCI_CAP_NPS (BIT4 | BIT3 | BIT2 | BIT1 | BIT0) // Number of Ports
#define   B_AHCI_CAP_SPM BIT17 // Supports Port Multiplier

///
/// AHCI each channel can have up to 1 device
///
#define AHCI_MAX_DEVICES 0x01

///
/// AHCI each channel can have 15 devices in the presence of a multiplier
///
#define AHCI_MULTI_MAX_DEVICES 0x0F

///
/// IDE supports 2 channel max
///
#define IDE_MAX_CHANNEL 0x02

///
/// IDE supports 2 devices max
///
#define IDE_MAX_DEVICES 0x02

#define SATA_ENUMER_ALL FALSE

//
// Sata Controller driver private data structure
//

#define SATA_CONTROLLER_SIGNATURE SIGNATURE_32('S','A','T','A')

typedef struct _EFI_SATA_CONTROLLER_PRIVATE_DATA {
  //
  // Standard signature used to identify Sata Controller private data
  //
  UINT32                            Signature;

  //
  // Protocol instance of IDE_CONTROLLER_INIT produced by this driver
  //
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  IdeInit;

  //
  // Copy of protocol pointers used by this driver
  //
  EFI_PCI_IO_PROTOCOL               *PciIo;

  //
  // The number of devices that are supported by this channel
  //
  UINT8                             DeviceCount;

  //
  // The highest disqulified mode for each attached device,
  // From ATA/ATAPI spec, if a mode is not supported,
  // the modes higher than it is also not supported
  //
  EFI_ATA_COLLECTIVE_MODE           *DisqulifiedModes;

  //
  // A copy of EFI_IDENTIFY_DATA data for each attached SATA device and its flag
  //
  EFI_IDENTIFY_DATA                 *IdentifyData;
  BOOLEAN                           *IdentifyValid;
} EFI_SATA_CONTROLLER_PRIVATE_DATA;

#define SATA_CONTROLLER_PRIVATE_DATA_FROM_THIS(a) CR(a, EFI_SATA_CONTROLLER_PRIVATE_DATA, IdeInit, SATA_CONTROLLER_SIGNATURE)

//
// Driver binding functions declaration
//
/**
  Supported function of Driver Binding protocol for this driver.
  Test to see if this driver supports ControllerHandle.

  @param This                   Protocol instance pointer.
  @param Controller             Handle of device to test.
  @param RemainingDevicePath    A pointer to the device path. Should be ignored by
                                device driver.

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
;

/**
  This routine is called right after the .Supported() called and 
  Start this driver on ControllerHandle.

  @param This                   Protocol instance pointer.
  @param Controller             Handle of device to bind driver to.
  @param RemainingDevicePath    A pointer to the device path. Should be ignored by
                                device driver.

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
;

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
;

//
// IDE controller init functions declaration
//
/**
  This function can be used to obtain information about a specified channel.
  It's usually used by IDE Bus driver during enumeration process.

  @param This           the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel        Channel number. Parallel ATA (PATA) controllers can support up to two channels.
                        Advanced Host Controller Interface (AHCI) Serial ATA (SATA) controllers
                        can support up to 32 channels, each of which can have up to one device.
                        In the presence of a multiplier, each channel can have 15 devices.
  @param Enabled        TRUE if the channel is enabled. If the channel is disabled,
                        then it will no be enumerated.
  @param MaxDevices     For Parallel ATA (PATA) controllers, this number will either be 1 or 2.
                        For Serial ATA (SATA) controllers with a port multiplier, this number can be as large as 15.

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
;

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
;

/**
  This function is called by IdeBus driver to submit EFI_IDENTIFY_DATA data structure
  obtained from IDE deivce. This structure is used to set IDE timing.

  @param This           The EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel        Channel number.
  @param Device         Device number.
  @param IdentifyData   A pointer to EFI_IDENTIFY_DATA data structure.

  @retval EFI_SUCCESS           The information was accepted without any errors.
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
;

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
;

/**
  This function is called by IdeBus driver to calculate the best operation mode
  supported by specific IDE device.

  @param This               The EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param Channel            Channel number.
  @param Device             Device number.
  @param SupportedModes     The optimum modes for the device.

  @retval EFI_SUCCESS           SupportedModes was returned.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate pool.
  @retval EFI_INVALID_PARAMETER Invalid channel id or device id.
**/
EFI_STATUS
EFIAPI
IdeInitCalculateMode (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL   *This,
  IN UINT8                              Channel,
  IN UINT8                              Device,
  OUT EFI_ATA_COLLECTIVE_MODE           **SupportedModes
  )
;

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
;

//
// Forward reference declaration
//
/**
  Retrieves a Unicode string that is the user readable name of the UEFI Driver.

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
SataControllerComponentNameGetDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL    *This,
  IN CHAR8                          *Language,
  OUT CHAR16                        **DriverName
  )
;

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an UEFI Driver.

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
SataControllerComponentNameGetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_HANDLE                     ChildHandle OPTIONAL,
  IN CHAR8                          *Language,
  OUT CHAR16                        **ControllerName
  )
;

#endif
