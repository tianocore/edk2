/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ComponentName.h
  
Abstract:


Revision History

--*/

#ifndef _EFI_PCI_BUS_COMPONENT_NAME_H
#define _EFI_PCI_BUS_COMPONENT_NAME_H

extern EFI_COMPONENT_NAME_PROTOCOL  gPciBusComponentName;

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
PciBusComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  Language    - TODO: add argument description
  DriverName  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciBusComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  ControllerHandle  - TODO: add argument description
  ChildHandle       - TODO: add argument description
  Language          - TODO: add argument description
  ControllerName    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
