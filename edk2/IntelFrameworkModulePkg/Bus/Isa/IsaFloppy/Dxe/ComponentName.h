/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

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
