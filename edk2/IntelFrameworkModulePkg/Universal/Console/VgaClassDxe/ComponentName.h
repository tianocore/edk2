/**@file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _VGA_CLASS_COMPONENT_NAME_H
#define _VGA_CLASS_COMPONENT_NAME_H

#include <PiDxe.h>
#include <Protocol/ComponentName.h>

extern EFI_COMPONENT_NAME_PROTOCOL  gVgaClassComponentName;

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
VgaClassComponentNameGetDriverName (
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
VgaClassComponentNameGetControllerName (
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
