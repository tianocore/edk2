/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  LegacyRegion.c
    
Abstract:

  EFI Legacy Region Protocol

Revision History

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (LegacyRegion)

EFI_GUID  gEfiLegacyRegionProtocolGuid = EFI_LEGACY_REGION_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiLegacyRegionProtocolGuid, "Legacy Region Protocol", "Legacy Region Protocol");
