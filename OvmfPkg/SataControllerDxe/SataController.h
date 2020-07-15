/** @file
  Header file for Sata Controller driver.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  // Original PCI attributes
  //
  UINT64                            OriginalPciAttributes;

  //
  // The number of devices that are supported by this channel
  //
  UINT8                             DeviceCount;

  //
  // The highest disqulified mode for each attached device,
  // From ATA/ATAPI spec, if a mode is not supported,
  // the modes higher than it is also not supported
  //
  EFI_ATA_COLLECTIVE_MODE           *DisqualifiedModes;

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
;

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
;

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
;

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
;

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
;

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
