/** @file
  Header file for Console Platfrom DXE Driver.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CON_PLATFORM_H_
#define _CON_PLATFORM_H_

#include <Uefi.h>

#include <Protocol/SimpleTextOut.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/PciIo.h>
#include <Protocol/GraphicsOutput.h>

#include <Guid/GlobalVariable.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/StandardErrorDevice.h>
#include <Guid/ConsoleOutDevice.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL  gConPlatformTextInDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gConPlatformComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gConPlatformComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL  gConPlatformTextOutDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gConPlatformComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gConPlatformComponentName2;


typedef enum {
  Check,
  Append,
  Delete
} CONPLATFORM_VAR_OPERATION;

/**
  Test to see if specific protocol could be supported on the ControllerHandle. 

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  ProtocolGuid        The specfic protocol.

  @retval EFI_SUCCESS         This driver supports this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
ConPlatformDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_GUID                     *ProtocolGuid
  );

/**
  Test to see if EFI_SIMPLE_TEXT_INPUT_PROTOCOL is supported on ControllerHandle. 

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConPlatformTextInDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  );

/**
  Test to see if EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL is supported on ControllerHandle. 

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConPlatformTextOutDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  );

/**
  Start this driver on the device for console input.

  Start this driver on ControllerHandle by opening Simple Text Input Protocol,
  reading Device Path, and installing Console In Devcice GUID on ControllerHandle.

  If this devcie is not one hot-plug devce, append its device path into the 
  console environment variables ConInDev.
  
  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConPlatformTextInDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  );

/**
  Start this driver on the device for console output and stardard error output.

  Start this driver on ControllerHandle by opening Simple Text Output Protocol,
  reading Device Path, and installing Console Out Devcic GUID, Standard Error
  Device GUID on ControllerHandle.

  If this devcie is not one hot-plug devce, append its device path into the 
  console environment variables ConOutDev, StdErrDev.
  
  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
ConPlatformTextOutDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle by removing Console In Devcice GUID 
  and closing the Simple Text Input protocol on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConPlatformTextInDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Stop this driver on ControllerHandle by removing Console Out Devcice GUID 
  and closing the Simple Text Output protocol on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConPlatformTextOutDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Uninstall the specified protocol.

  @param This            Protocol instance pointer.
  @param Handle          Handle of device to uninstall protocol on.
  @param ProtocolGuid    The specified protocol need to be uninstalled.

**/
VOID
ConPlatformUnInstallProtocol (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_GUID                     *ProtocolGuid
  );

/**
  Read the EFI variable (Name) and return a dynamically allocated
  buffer, and the size of the buffer. On failure return NULL.

  @param  Name             String part of EFI variable name

  @return Dynamically allocated memory that contains a copy of the EFI variable.
          Caller is repsoncible freeing the buffer. Return NULL means Variable 
          was not read.

**/
VOID *
ConPlatformGetVariable (
  IN  CHAR16              *Name
  );

/**
  Function compares a device path data structure to that of all the nodes of a
  second device path instance.


  @param Multi           A pointer to a multi-instance device path data structure.
  @param Single          A pointer to a single-instance device path data structure.
  @param NewDevicePath   If Delete is TRUE, this parameter must not be null, and it
                         points to the remaining device path data structure.
                         (remaining device path = Multi - Single.)
  @param Delete          If TRUE, means removing Single from Multi.
                         If FALSE, the routine just check whether Single matches
                         with any instance in Multi.

  @retval EFI_SUCCESS           If the Single is contained within Multi.
  @retval EFI_NOT_FOUND         If the Single is not contained within Multi.
  @retval EFI_INVALID_PARAMETER Multi is NULL.
  @retval EFI_INVALID_PARAMETER Single is NULL.
  @retval EFI_INVALID_PARAMETER NewDevicePath is NULL when Delete is TRUE.

**/
EFI_STATUS
ConPlatformMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath OPTIONAL,
  IN  BOOLEAN                   Delete
  );

/**
  Update console environment variables.

  @param  VariableName    Console environment variables, ConOutDev, ConInDev
                          StdErrDev, ConIn or ConOut.
  @param  DevicePath      Console devcie's device path.
  @param  Operation       Variable operations, including APPEND, CHECK and DELETE.

  @retval EFI_SUCCESS           Variable operates successfully.
  @retval EFI_OUT_OF_RESOURCES  If variable cannot be appended.
  @retval other                 Variable updating failed.

**/
EFI_STATUS
ConPlatformUpdateDeviceVariable (
  IN  CHAR16                    *VariableName,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONPLATFORM_VAR_OPERATION Operation
  );

/**
  Check if the device supports hot-plug through its device path.

  This function could be updated to check more types of Hot Plug devices.
  Currently, it checks USB and PCCard device.

  @param  DevicePath            Pointer to device's device path.

  @retval TRUE                  The devcie is a hot-plug device
  @retval FALSE                 The devcie is not a hot-plug device.

**/
BOOLEAN
IsHotPlugDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  );

//
// EFI Component Name Functions
//
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
ConPlatformComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
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
ConPlatformComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

/**
  Update ConOutDev and ErrOutDev variables to add the device path of
  GOP controller itself and the sibling controllers.

  @param  DevicePath            Pointer to device's device path.

  @retval TRUE                  The devcie is a GOP device.
  @retval FALSE                 The devcie is not a GOP device.

**/
BOOLEAN
ConPlatformUpdateGopCandidate (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  );

#endif
