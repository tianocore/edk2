/** @file
  Values defined and used by the Opal UEFI Driver.

Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OPAL_DRIVER_H_
#define _OPAL_DRIVER_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci.h>

#include <Protocol/PciIo.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/BlockIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/StorageSecurityCommand.h>

#include <Guid/EventGroup.h>
#include <Guid/S3StorageDeviceInitList.h>
#include <Guid/MdeModuleHii.h>

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/PciLib.h>
#include <Library/LockBoxLib.h>
#include <Library/TcgStorageOpalLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>

#include "OpalPasswordCommon.h"
#include "OpalHiiFormValues.h"

#define EFI_DRIVER_NAME_UNICODE  L"1.0 UEFI Opal Driver"

// UEFI 2.1
#define LANGUAGE_RFC_3066_ENGLISH  ((CHAR8*)"en")

// UEFI/EFI < 2.1
#define LANGUAGE_ISO_639_2_ENGLISH  ((CHAR8*)"eng")

#define CONCAT_(x, y)  x ## y
#define CONCAT(x, y)   CONCAT_(x, y)

#define UNICODE_STR(x)  CONCAT( L, x )

extern EFI_DRIVER_BINDING_PROTOCOL   gOpalDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gOpalComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gOpalComponentName2;

#define OPAL_MSID_LENGTH  128

#define MAX_PASSWORD_TRY_COUNT  5

// PSID Length
#define PSID_CHARACTER_LENGTH  0x20
#define MAX_PSID_TRY_COUNT     5

//
// The max timeout value assume the user can wait for the revert action. The unit of this macro is second.
// If the revert time value bigger than this one, driver needs to popup a dialog to let user confirm the
// revert action.
//
#define MAX_ACCEPTABLE_REVERTING_TIME  10

#pragma pack(1)

//
// Structure that is used to represent the available actions for an OpalDisk.
// The data can then be utilized to expose/hide certain actions available to an end user
// by the consumer of this library.
//
typedef struct {
  //
  // Indicates if the disk can support PSID Revert action.  should verify disk supports PSID authority
  //
  UINT16    PsidRevert           : 1;

  //
  // Indicates if the disk can support Revert action
  //
  UINT16    Revert               : 1;

  //
  // Indicates if the user must keep data for revert action.  It is true if no media encryption is supported.
  //
  UINT16    RevertKeepDataForced : 1;

  //
  // Indicates if the disk can support set Admin password
  //
  UINT16    AdminPass            : 1;

  //
  // Indicates if the disk can support set User password.  This action requires that a user
  // password is first enabled.
  //
  UINT16    UserPass             : 1;

  //
  // Indicates if unlock action is available.  Requires disk to be currently locked.
  //
  UINT16    Unlock               : 1;

  //
  // Indicates if Secure Erase action is available.  Action requires admin credentials and media encryption support.
  //
  UINT16    SecureErase          : 1;

  //
  // Indicates if Disable User action is available.  Action requires admin credentials.
  //
  UINT16    DisableUser          : 1;
} OPAL_DISK_ACTIONS;

//
// Structure that is used to represent an OPAL_DISK.
//
typedef struct {
  UINT32                                   MsidLength;                    // Byte length of MSID Pin for device
  UINT8                                    Msid[OPAL_MSID_LENGTH];        // MSID Pin for device
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *Sscp;
  UINT32                                   MediaId;                       // MediaId is used by Ssc Protocol.
  EFI_DEVICE_PATH_PROTOCOL                 *OpalDevicePath;
  UINT16                                   OpalBaseComId;                 // Opal SSC 1 base com id.
  OPAL_OWNER_SHIP                          Owner;
  OPAL_DISK_SUPPORT_ATTRIBUTE              SupportedAttributes;
  TCG_LOCKING_FEATURE_DESCRIPTOR           LockingFeature;                // Locking Feature Descriptor retrieved from performing a Level 0 Discovery
  UINT8                                    PasswordLength;
  UINT8                                    Password[OPAL_MAX_PASSWORD_SIZE];

  UINT32                                   EstimateTimeCost;
  BOOLEAN                                  SentBlockSID;                  // Check whether BlockSid command has been sent.
} OPAL_DISK;

//
// Device with block IO protocol
//
typedef struct _OPAL_DRIVER_DEVICE OPAL_DRIVER_DEVICE;

struct _OPAL_DRIVER_DEVICE {
  OPAL_DRIVER_DEVICE                       *Next;                     ///< Linked list pointer
  EFI_HANDLE                               Handle;                    ///< Device handle
  OPAL_DISK                                OpalDisk;                  ///< User context
  CHAR16                                   *Name16;                   ///< Allocated/freed by UEFI Filter Driver at device creation/removal
  CHAR8                                    *NameZ;                    ///< Allocated/freed by UEFI Filter Driver at device creation/removal
  UINT32                                   MediaId;                   ///< Required parameter for EFI_STORAGE_SECURITY_COMMAND_PROTOCOL, from BLOCK_IO_MEDIA

  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *Sscp;                     /// Device protocols consumed
  EFI_DEVICE_PATH_PROTOCOL                 *OpalDevicePath;
};

//
// Opal Driver UEFI Driver Model
//
typedef struct {
  EFI_HANDLE            Handle;             ///< Driver image handle
  OPAL_DRIVER_DEVICE    *DeviceList;        ///< Linked list of controllers owned by this Driver
} OPAL_DRIVER;

#pragma pack()

//
// Retrieves a OPAL_DRIVER_DEVICE based on the pointer to its StorageSecurity protocol.
//
#define DRIVER_DEVICE_FROM_OPALDISK(OpalDiskPointer)  (OPAL_DRIVER_DEVICE*)(BASE_CR(OpalDiskPointer, OPAL_DRIVER_DEVICE, OpalDisk))

/**
  Get devcie list info.

  @retval     return the device list pointer.
**/
OPAL_DRIVER_DEVICE *
OpalDriverGetDeviceList (
  VOID
  );

/**
  Get devcie name through the component name protocol.

  @param[in]       Dev                The device which need to get name.

  @retval     TRUE        Find the name for this device.
  @retval     FALSE       Not found the name for this device.
**/
BOOLEAN
OpalDriverGetDriverDeviceName (
  OPAL_DRIVER_DEVICE  *Dev
  );

/**
  Get current device count.

  @retval  return the current created device count.

**/
UINT8
GetDeviceCount (
  VOID
  );

/**
  Update password for the Opal disk.

  @param[in, out] OpalDisk          The disk to update password.
  @param[in]      Password          The input password.
  @param[in]      PasswordLength    The input password length.

**/
VOID
OpalSupportUpdatePassword (
  IN OUT OPAL_DISK  *OpalDisk,
  IN VOID           *Password,
  IN UINT32         PasswordLength
  );

/**

  The function performs determines the available actions for the OPAL_DISK provided.

  @param[in]   SupportedAttributes   The support attribute for the device.
  @param[in]   LockingFeature        The locking status for the device.
  @param[in]   OwnerShip             The ownership for the device.
  @param[out]  AvalDiskActions       Pointer to fill-out with appropriate disk actions.

**/
TCG_RESULT
EFIAPI
OpalSupportGetAvailableActions (
  IN  OPAL_DISK_SUPPORT_ATTRIBUTE     *SupportedAttributes,
  IN  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockingFeature,
  IN  UINT16                          OwnerShip,
  OUT OPAL_DISK_ACTIONS               *AvalDiskActions
  );

/**
  Enable Opal Feature for the input device.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Msid               Msid
  @param[in]      MsidLength         Msid Length
  @param[in]      Password           Admin password
  @param[in]      PassLength         Length of password in bytes

**/
TCG_RESULT
EFIAPI
OpalSupportEnableOpalFeature (
  IN OPAL_SESSION  *Session,
  IN VOID          *Msid,
  IN UINT32        MsidLength,
  IN VOID          *Password,
  IN UINT32        PassLength
  );

/**
  Unloads UEFI Driver.  Very useful for debugging and testing.

  @param ImageHandle            Image handle this driver.

  @retval EFI_SUCCESS           This function always complete successfully.
  @retval EFI_INVALID_PARAMETER The input ImageHandle is not valid.
**/
EFI_STATUS
EFIAPI
EfiDriverUnload (
  EFI_HANDLE  ImageHandle
  );

/**
  Test to see if this driver supports Controller.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_ALREADY_STARTED This driver is already running on this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverBindingSupported (
  EFI_DRIVER_BINDING_PROTOCOL  *This,
  EFI_HANDLE                   Controller,
  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Enables Opal Management on a supported device if available.

  The start function is designed to be called after the Opal UEFI Driver has confirmed the
  "controller", which is a child handle, contains the EF_STORAGE_SECURITY_COMMAND protocols.
  This function will complete the other necessary checks, such as verifying the device supports
  the correct version of Opal.  Upon verification, it will add the device to the
  Opal HII list in order to expose Opal management options.

  @param[in]  This                  A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle      The handle of the controller to start. This handle
                                    must support a protocol interface that supplies
                                    an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath   A pointer to the remaining portion of a device path.  This
                                    parameter is ignored by device drivers, and is optional for bus
                                    drivers. For a bus driver, if this parameter is NULL, then handles
                                    for all the children of Controller are created by this driver.
                                    If this parameter is not NULL and the first Device Path Node is
                                    not the End of Device Path Node, then only the handle for the
                                    child device specified by the first Device Path Node of
                                    RemainingDevicePath is created by this driver.
                                    If the first Device Path Node of RemainingDevicePath is
                                    the End of Device Path Node, no child handle is created by this
                                    driver.

  @retval EFI_SUCCESS               Opal management was enabled.
  @retval EFI_DEVICE_ERROR          The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES      The request could not be completed due to a lack of resources.
  @retval Others                    The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverBindingStart (
  EFI_DRIVER_BINDING_PROTOCOL  *This,
  EFI_HANDLE                   Controller,
  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop this driver on Controller.

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval other             This driver could not be removed from this device.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverBindingStop (
  EFI_DRIVER_BINDING_PROTOCOL  *This,
  EFI_HANDLE                   Controller,
  UINTN                        NumberOfChildren,
  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverComponentNameGetDriverName (
  EFI_COMPONENT_NAME_PROTOCOL  *This,
  CHAR8                        *Language,
  CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverComponentNameGetControllerName (
  EFI_COMPONENT_NAME_PROTOCOL  *This,
  EFI_HANDLE                   ControllerHandle,
  EFI_HANDLE                   ChildHandle,
  CHAR8                        *Language,
  CHAR16                       **ControllerName
  );

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverComponentName2GetDriverName (
  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  CHAR8                         *Language,
  CHAR16                        **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverComponentName2GetControllerName (
  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  EFI_HANDLE                    ControllerHandle,
  EFI_HANDLE                    ChildHandle,
  CHAR8                         *Language,
  CHAR16                        **ControllerName
  );

#endif //_OPAL_DRIVER_H_
