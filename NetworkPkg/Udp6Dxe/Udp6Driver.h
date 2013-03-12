/** @file
  Driver Binding functions and Service Binding functions for the Network driver module.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UDP6_DRIVER_H_
#define _UDP6_DRIVER_H_

#include <Protocol/DriverBinding.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/DevicePath.h>

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
Udp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  );

/**
  Start this driver on ControllerHandle.

  This service is called by the  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to bind a driver to.
  @param[in]  RemainingDevicePath    Optional parameter use to pick a specific child
                                     device to start.

  @retval EFI_SUCCES             This driver is added to ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES   The required system resource can't be allocated.
  @retval other                  This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Udp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  );

/**
  Stop this driver on ControllerHandle.

  This service is called by the  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop(), it must also follow these calling restrictions.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to stop the driver on.
  @param[in]  NumberOfChildren       Number of Handles in ChildHandleBuffer. If number
                                     of children is zero, stop the entire bus driver.
  @param[in]  ChildHandleBuffer      List of Child Handles to Stop. It is optional.

  @retval EFI_SUCCESS            This driver removed ControllerHandle.
  @retval EFI_DEVICE_ERROR       Can't find the NicHandle from the ControllerHandle and specified GUID.
  @retval other                  This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
Udp6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in]       This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in, out]  ChildHandle Pointer to the handle of the child to create. If it is NULL,
                               then a new handle is created. If it is a pointer to an existing UEFI handle,
                               then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER This is NULL or ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
Udp6ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                *ChildHandle
  );

/**
  Destroys a child handle with a set of I/O services.
  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCES             The I/O services were removed from the child
                                 handle.
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                 that are being removed.
  @retval EFI_INVALID_PARAMETER  Child handle is NULL.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its I/O services are being used.
  @retval other                  The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
Udp6ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

#endif

