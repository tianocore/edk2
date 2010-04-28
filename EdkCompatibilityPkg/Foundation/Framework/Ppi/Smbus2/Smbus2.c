/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

 Smbus2.c

Abstract:

  Smbus2 PPI GUID as defined in PI1.0

--*/

#include "Tiano.h"
#include "Pei.h"
#include EFI_PPI_DEFINITION (Smbus2)

EFI_GUID  gPeiSmbus2PpiGuid = PEI_SMBUS2_PPI_GUID;

EFI_GUID_STRING(&gPeiSmbus2PpiGuid, "Smbus2", "Smbus2 PPI");
