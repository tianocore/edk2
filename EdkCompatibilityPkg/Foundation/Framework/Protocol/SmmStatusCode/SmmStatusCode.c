/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SmmStatusCode.c

Abstract:

  SMM Status code Protocol as defined in EFI 2.0 (for Status Code Architectural Protocol)

  This code abstracts Status Code reporting.

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (SmmStatusCode)

EFI_GUID  gEfiSmmStatusCodeProtocolGuid = EFI_SMM_STATUS_CODE_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiSmmStatusCodeProtocolGuid, "SMM Status Code", "SMM Status Code Protocol");
