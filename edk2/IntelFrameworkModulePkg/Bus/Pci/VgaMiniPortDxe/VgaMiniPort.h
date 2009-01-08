/** @file

Copyright (c) 2006 - 2007 Intel Corporation. All rights reserved
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VGA_MINIPORT_H_
#define _VGA_MINIPORT_H_

//
// The package level header files this module uses
//
#include <PiDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/PciIo.h>
#include <Protocol/VgaMiniPort.h>
#include <Protocol/ComponentName2.h>


//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <IndustryStandard/Pci22.h>

//
// PCI VGA MiniPort Device Structure
//
#define PCI_VGA_MINI_PORT_DEV_SIGNATURE   SIGNATURE_32('P','V','M','P')

typedef struct {
  UINTN                         Signature;
  EFI_HANDLE                    Handle;
  EFI_VGA_MINI_PORT_PROTOCOL    VgaMiniPort;
  EFI_PCI_IO_PROTOCOL           *PciIo;
} PCI_VGA_MINI_PORT_DEV;

#define PCI_VGA_MINI_PORT_DEV_FROM_THIS(a) CR(a, PCI_VGA_MINI_PORT_DEV, VgaMiniPort, PCI_VGA_MINI_PORT_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gPciVgaMiniPortDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gPciVgaMiniPortComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gPciVgaMiniPortComponentName2;

//
// Driver Binding Protocol functions
//
/**
  Supported.

  (Standard DriverBinding Protocol Supported() function)

  @param  This                   The driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval EFI_UNSUPPORTED        This device isn't supported.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Install VGA Mini Port Protocol onto VGA device handles

  (Standard DriverBinding Protocol Start() function)

  @param  This                   The driver binding instance.
  @param  Controller             The controller to check.
  @param  RemainingDevicePath    The remaining device patch.

  @retval EFI_SUCCESS            The controller is controlled by the driver.
  @retval EFI_ALREADY_STARTED    The controller is already controlled by the driver.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop.

  (Standard DriverBinding Protocol Stop() function)

  @param  This                   The driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The child number that opened controller
                                 BY_CHILD.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The controller or children are stopped.
  @retval EFI_DEVICE_ERROR       Failed to stop the driver.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

//
// VGA Mini Port Protocol functions
//
/**
  Thunk function of EFI_VGA_MINI_PORT_SET_MODE.

  @param  This             Point to instance of EFI_VGA_MINI_PORT_PROTOCOL.
  @param  ModeNumber       Mode number.

  @retval EFI_UNSUPPORTED  Invalid mode number.
  @retval EFI_SUCCESS      Success.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortSetMode (
  IN  EFI_VGA_MINI_PORT_PROTOCOL  *This,
  IN  UINTN                       ModeNumber
  );

#endif
