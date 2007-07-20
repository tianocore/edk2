/**@file

Copyright (c) 2006 - 2007, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_ISA_BUS_COMPONENT_NAME_H
#define _EFI_ISA_BUS_COMPONENT_NAME_H

//
// Include common header file for this module.
//
#include "InternalIsaBus.h"

extern EFI_COMPONENT_NAME_PROTOCOL  gIsaBusComponentName;

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
IsaBusComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This        - GC_TODO: add argument description
  Language    - GC_TODO: add argument description
  DriverName  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IsaBusComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This              - GC_TODO: add argument description
  ControllerHandle  - GC_TODO: add argument description
  ChildHandle       - GC_TODO: add argument description
  Language          - GC_TODO: add argument description
  ControllerName    - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
