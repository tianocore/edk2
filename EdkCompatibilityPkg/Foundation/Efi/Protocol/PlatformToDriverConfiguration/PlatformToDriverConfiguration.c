/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    PlatformToDriverConfiguration.c

Abstract:

    UEFI Platform to Driver Configuration Protocol

Revision History:

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (PlatformToDriverConfiguration)

EFI_GUID  gEfiPlatformToDriverConfigurationProtocolGuid = EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL_GUID;
EFI_GUID  gEfiPlatformToDriverConfigurationClpGuid = EFI_PLATFORM_TO_DRIVER_CONFIGURATION_CLP_GUID;

EFI_GUID_STRING
  (&gEfiPlatformToDriverConfigurationProtocolGuid, "Platform to Driver Configuration Protocol", "UEFI 2.1 Platform to Driver Configuration Protocol");
