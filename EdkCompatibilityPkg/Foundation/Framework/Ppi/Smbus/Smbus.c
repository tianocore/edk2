/*++

Copyright (c) 1999 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

 Smbus.c

Abstract:

  Smbus PPI GUID as defined in EFI 2.0

--*/

#include "Tiano.h"
#include "Pei.h"
#include EFI_PPI_DEFINITION (Smbus)

EFI_GUID  gPeiSmbusPpiGuid = PEI_SMBUS_PPI_GUID;

EFI_GUID_STRING(&gPeiSmbusPpiGuid, "Smbus", "Smbus PPI");
