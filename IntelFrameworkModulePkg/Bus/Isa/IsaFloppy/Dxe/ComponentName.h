/*++

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  ComponentName.h

Abstract:

Revision History:

--*/

#ifndef _ISA_FLOPPY_COMPONENT_NAME_H
#define _ISA_FLOPPY_COMPONENT_NAME_H

#define FLOPPY_DRIVE_NAME           L"ISA Floppy Drive # "
#define FLOPPY_DRIVE_NAME_ASCII_LEN (sizeof ("ISA Floppy Drive # ") - 1)
#define ADD_FLOPPY_NAME(x)                 AddName ((x))

extern EFI_COMPONENT_NAME_PROTOCOL  gIsaFloppyComponentName;

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
IsaFloppyComponentNameGetDriverName (
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
IsaFloppyComponentNameGetControllerName (
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

VOID
AddName (
  IN  FDC_BLK_IO_DEV               *FdcDev
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcDev  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
