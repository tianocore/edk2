/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    DriverSupportedEfiVersion.c

Abstract:

    Driver Supported EFI Version Protocol

Revision History:

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (DriverSupportedEfiVersion)

EFI_GUID  gEfiDriverSupportedEfiVersionProtocolGuid = EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL_GUID;

EFI_GUID_STRING
  (&gEfiDriverSupportedEfiVersionProtocolGuid, "Driver Supported EFI Version Protocol", "UEFI 2.1 Driver Supported EFI Version Protocol");
