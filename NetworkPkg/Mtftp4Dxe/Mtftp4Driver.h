/** @file
  Mtftp drivers function header.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_MTFTP4_DRIVER_H__
#define __EFI_MTFTP4_DRIVER_H__

#include <Uefi.h>

#include <Protocol/ServiceBinding.h>

#include <Library/NetLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>

extern EFI_COMPONENT_NAME_PROTOCOL   gMtftp4ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gMtftp4ComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL   gMtftp4DriverBinding;
extern EFI_UNICODE_STRING_TABLE      *gMtftp4ControllerNameTable;

/**
  Test whether MTFTP driver support this controller.

  @param  This                   The MTFTP driver binding instance
  @param  Controller             The controller to test
  @param  RemainingDevicePath    The remaining device path

  @retval EFI_SUCCESS            The controller has UDP service binding protocol
                                 installed, MTFTP can support it.
  @retval Others                 MTFTP can't support the controller.

**/
EFI_STATUS
EFIAPI
Mtftp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Start the MTFTP driver on this controller.

  MTFTP driver will install a MTFTP SERVICE BINDING protocol on the supported
  controller, which can be used to create/destroy MTFTP children.

  @param  This                   The MTFTP driver binding protocol.
  @param  Controller             The controller to manage.
  @param  RemainingDevicePath    Remaining device path.

  @retval EFI_ALREADY_STARTED    The MTFTP service binding protocol has been
                                 started  on the controller.
  @retval EFI_SUCCESS            The MTFTP service binding is installed on the
                                 controller.

**/
EFI_STATUS
EFIAPI
Mtftp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop the MTFTP driver on controller. The controller is a UDP
  child handle.

  @param  This                   The MTFTP driver binding protocol
  @param  Controller             The controller to stop
  @param  NumberOfChildren       The number of children
  @param  ChildHandleBuffer      The array of the child handle.

  @retval EFI_SUCCESS            The driver is stopped on the controller.
  @retval EFI_DEVICE_ERROR       Failed to stop the driver on the controller.

**/
EFI_STATUS
EFIAPI
Mtftp4DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  );

/**
  Create a MTFTP child for the service binding instance, then
  install the MTFTP protocol to the ChildHandle.

  @param  This                   The MTFTP service binding instance.
  @param  ChildHandle            The Child handle to install the MTFTP protocol.

  @retval EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource for the new child.
  @retval EFI_SUCCESS            The child is successfully create.

**/
EFI_STATUS
EFIAPI
Mtftp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                *ChildHandle
  );

/**
  Destroy one of the service binding's child.

  @param  This                   The service binding instance
  @param  ChildHandle            The child handle to destroy

  @retval EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval EFI_UNSUPPORTED        The child may have already been destroyed.
  @retval EFI_SUCCESS            The child is destroyed and removed from the
                                 parent's child list.

**/
EFI_STATUS
EFIAPI
Mtftp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                   ChildHandle
  );



#endif
