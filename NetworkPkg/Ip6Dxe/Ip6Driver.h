/** @file
  The driver binding and service binding protocol for IP6 driver.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP6_DRIVER_H__
#define __EFI_IP6_DRIVER_H__

extern EFI_DRIVER_BINDING_PROTOCOL  gIp6DriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gIp6ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gIp6ComponentName2;
extern EFI_UNICODE_STRING_TABLE     *gIp6ControllerNameTable;

typedef struct {
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;
}IP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT;

/**
  Clean up an IP6 service binding instance. It releases all
  the resource allocated by the instance. The instance may be
  partly initialized, or partly destroyed. If a resource is
  destroyed, it is marked as that in case the destroy failed and
  being called again later.

  @param[in]  IpSb               The IP6 service binding instance to clean up.

  @retval EFI_SUCCESS            The resource used by the instance are cleaned up.
  @retval Others                 Failed to clean up some of the resources.

**/
EFI_STATUS
Ip6CleanService (
  IN IP6_SERVICE            *IpSb
  );

//
// Function prototype for the driver's entry point
//

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  The entry point for IP6 driver which installs the driver
  binding and component name protocol on its image.

  @param[in]  ImageHandle           The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
Ip6DriverEntryPoint (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  );

//
// Function prototypes for the Drivr Binding Protocol
//

/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to test.
  @param[in]  RemainingDevicePath    Optional parameter use to pick a specific child
                                     device to start.

  @retval EFI_SUCCESS                This driver supports this device.
  @retval EFI_ALREADY_STARTED        This driver is already running on this device.
  @retval other                      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Ip6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to bind driver to.
  @param[in]  RemainingDevicePath Optional parameter used to pick a specific child
                                  device to start.

  @retval EFI_SUCCES              This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED     This driver is already running on ControllerHandle.
  @retval other                   This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Ip6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ControllerHandle   Handle of device to stop driver on.
  @param[in]  NumberOfChildren   Number of Handles in ChildHandleBuffer. If number
                                 of children is zero, stop the entire  bus driver.
  @param[in]  ChildHandleBuffer  An array of child handles to be freed. May be NULL
                                 if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
Ip6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

//
// Function ptototypes for the ServiceBinding Prococol
//

/**
  Creates a child handle with a set of I/O services.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ChildHandle        Pointer to the handle of the child to create.   If
                                 it is NULL, then a new handle is created.   If it
                                 is not NULL, then the I/O services are added to
                                 the existing child handle.

  @retval EFI_SUCCES             The child handle was created with the I/O services.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources availabe to create
                                 the child.
  @retval other                  The child handle was not created.

**/
EFI_STATUS
EFIAPI
Ip6ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  );

/**
  Destroys a child handle with a set of I/O services.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCES             The I/O services were removed from the child
                                 handle.
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                  that are being removed.
  @retval EFI_INVALID_PARAMETER  Child handle is NULL.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its  I/O services are being used.
  @retval other                  The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
Ip6ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

#endif
