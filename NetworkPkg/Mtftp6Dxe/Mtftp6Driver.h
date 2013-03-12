/** @file
  Driver Binding functions and Service Binding functions
  declaration for Mtftp6 Driver.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_MTFTP6_DRIVER_H__
#define __EFI_MTFTP6_DRIVER_H__

#include <Protocol/ServiceBinding.h>

extern EFI_COMPONENT_NAME_PROTOCOL  gMtftp6ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gMtftp6ComponentName2;
extern EFI_UNICODE_STRING_TABLE     *gMtftp6ControllerNameTable;

/**
  Test to see if this driver supports Controller. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported(), it must also follow these calling restrictions.

  @param[in]  This                Protocol instance pointer.
  @param[in]  Controller          Handle of device to test.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific child
                                  device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval Others              This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Mtftp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Start this driver on Controller. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  Controller           Handle of device to bind driver to.
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific child
                                   device to start.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval EFI_ALREADY_STARTED  This driver is already running on Controller.
  @retval Others               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Mtftp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop this driver on Controller. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop(), it must also follow these calling restrictions.

  @param[in]  This              Protocol instance pointer.
  @param[in]  Controller        Handle of device to stop driver on
  @param[in]  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                                children is zero, stop the entire bus driver.
  @param[in]  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval EFI_DEVICE_ERROR  An unexpected error.
  @retval Others            This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
Mtftp6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  );

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in]      This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in, out] ChildHandle Pointer to the handle of the child to create. If it is NULL,
                              then a new handle is created. If it is a pointer to an existing
                              UEFI handle, then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval Others                The child handle was not created.

**/
EFI_STATUS
EFIAPI
Mtftp6ServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL *This,
  IN OUT EFI_HANDLE                   *ChildHandle
  );

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in]  ChildHandle Handle of the child to destroy.

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval Others                The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
Mtftp6ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                   ChildHandle
  );

#endif
