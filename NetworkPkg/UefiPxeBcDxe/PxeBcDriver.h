/** @file
  Driver Binding functions declaration for UefiPxeBc Driver.

  Copyright (c) 2007 - 2013, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_PXEBC_DRIVER_H__
#define __EFI_PXEBC_DRIVER_H__

extern EFI_COMPONENT_NAME_PROTOCOL   gPxeBcComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gPxeBcComponentName2;

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param[in]  This                The pointer to the driver binding protocol.
  @param[in]  ControllerHandle    The handle of device to be tested.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific child
                                  device to be started.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_UNSUPPORTED     This driver does not support this device.

**/
EFI_STATUS
EFIAPI
PxeBcIp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param[in]  This                 The pointer to the driver binding protocol.
  @param[in]  ControllerHandle     The handle of device to be started.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to be started.

  @retval EFI_SUCCESS          This driver is installed to ControllerHandle.
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
PxeBcIp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop() it must also follow these calling restrictions.

  @param[in]  This              Protocol instance pointer.
  @param[in]  ControllerHandle  Handle of device to stop driver on
  @param[in]  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                                children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS           This driver is removed ControllerHandle
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval Others                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
PxeBcIp4DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param[in]  This                The pointer to the driver binding protocol.
  @param[in]  ControllerHandle    The handle of device to be tested.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific child
                                  device to be started.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_UNSUPPORTED     This driver does not support this device.

**/
EFI_STATUS
EFIAPI
PxeBcIp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param[in]  This                 The pointer to the driver binding protocol.
  @param[in]  ControllerHandle     The handle of device to be started.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to be started.

  @retval EFI_SUCCESS          This driver is installed to ControllerHandle.
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
PxeBcIp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop() it must also follow these calling restrictions.

  @param[in]  This              Protocol instance pointer.
  @param[in]  ControllerHandle  Handle of device to stop driver on
  @param[in]  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                                children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS           This driver is removed ControllerHandle
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval Others                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
PxeBcIp6DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

#endif
