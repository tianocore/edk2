/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    DriverConfiguration.c

Abstract:

    EFI Driver Configuration Protocol

Revision History

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (DriverConfiguration2)

EFI_GUID  gEfiDriverConfiguration2ProtocolGuid = EFI_DRIVER_CONFIGURATION2_PROTOCOL_GUID;

EFI_GUID_STRING
  (&gEfiDriverConfiguration2ProtocolGuid, "Driver Configuration2 Protocol", "UEFI 2.0 Driver Configuration2 Protocol");
