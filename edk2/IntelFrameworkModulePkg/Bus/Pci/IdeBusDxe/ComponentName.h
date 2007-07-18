/** @file
  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IDE_BUS_COMPONENT_NAME_H
#define _IDE_BUS_COMPONENT_NAME_H

#define ADD_NAME(x) AddName ((x));

extern EFI_COMPONENT_NAME_PROTOCOL  gIDEBusComponentName;


//
// EFI Component Name Functions
//
/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  Language TODO: add argument description
  @param  DriverName TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBusComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  ControllerHandle TODO: add argument description
  @param  ChildHandle TODO: add argument description
  @param  Language TODO: add argument description
  @param  ControllerName TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBusComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  )
;

/**
  TODO: Add function description

  @param  IdeBlkIoDevicePtr TODO: add argument description

  TODO: add return values

**/
VOID
AddName (
  IN  IDE_BLK_IO_DEV               *IdeBlkIoDevicePtr
  )
;

#endif
