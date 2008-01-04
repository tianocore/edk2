/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PxeBcDriver.h

Abstract:


**/

#ifndef __EFI_PXEBC_DRIVER_H__
#define __EFI_PXEBC_DRIVER_H__

EFI_STATUS
PxeBcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

  Routine Description:
    Test to see if this driver supports ControllerHandle.

  Arguments:
    This                - Protocol instance pointer.
    ControllerHandle    - Handle of device to test
    RemainingDevicePath - Optional parameter use to pick a specific child
                          device to start.

  Returns:
    EFI_SUCCES
    EFI_ALREADY_STARTED
    Others

--*/
// GC_NOTO:    Controller - add argument and description to function comment
;


/**
  Start this driver on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @return EFI_SUCCES
  @return EFI_ALREADY_STARTED
  @return EFI_OUT_OF_RESOURCES
  @return Others

**/
// GC_NOTO:    Controller - add argument and description to function comment
EFI_STATUS
PxeBcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
;


/**
  Stop this driver on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to stop driver on
  @param  NumberOfChildren     Number of Handles in ChildHandleBuffer. If number of
                                children is zero stop the entire bus driver.
  @param  ChildHandleBuffer    List of Child Handles to Stop.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return Others

**/
// GC_NOTO:    Controller - add argument and description to function comment
EFI_STATUS
PxeBcDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  )
;

extern EFI_COMPONENT_NAME2_PROTOCOL gPxeBcComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL  gPxeBcComponentName;
extern EFI_DRIVER_BINDING_PROTOCOL  gPxeBcDriverBinding;
#endif

