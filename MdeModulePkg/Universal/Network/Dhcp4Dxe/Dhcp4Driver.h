/** @file

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Dhcp4Driver.h

Abstract:

  Header for the DHCP4 driver


**/

#ifndef __EFI_DHCP4_DRIVER_H__
#define __EFI_DHCP4_DRIVER_H__

extern EFI_COMPONENT_NAME_PROTOCOL  gDhcp4ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gDhcp4ComponentName2;

/**
  Test to see if this driver supports ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to test.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCES             This driver supports this device
  @retval EFI_ALREADY_STARTED    This driver is already running on this device.
  @retval other                  This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Dhcp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to bind driver to
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCES             This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED    This driver is already running on ControllerHandle
  @retval other                  This driver does not support this device

**/
EFI_STATUS
EFIAPI
Dhcp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to stop driver on
  @param  NumberOfChildren       Number of Handles in ChildHandleBuffer. If number
                                 of  children is zero stop the entire bus driver.
  @param  ChildHandleBuffer      List of Child Handles to Stop.

  @retval EFI_SUCCES             This driver is removed ControllerHandle
  @retval other                  This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
Dhcp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Creates a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Pointer to the handle of the child to create. If
                                 it is NULL, then a new handle is created. If it is
                                 not NULL, then the I/O services are  added to the
                                 existing child handle.

  @retval EFI_SUCCES             The child handle was created with the I/O
                                 services.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources availabe to create
                                 the child.
  @retval other                  The child handle was not created.

**/
EFI_STATUS
EFIAPI
Dhcp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  );

/**
  Destroys a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Handle of the child to destroy.

  @retval EFI_SUCCES             The I/O services were removed from the child
                                 handle.
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                  that are being removed.
  @retval EFI_INVALID_PARAMETER  Child handle is not a valid EFI Handle.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its  I/O services are being used.
  @retval other                  The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
Dhcp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

#endif
