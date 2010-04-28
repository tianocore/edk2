/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    DriverDiagnostics2.c

Abstract:

    EFI Driver Diagnostics2 Protocol

Revision History

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (DriverDiagnostics2)

EFI_GUID  gEfiDriverDiagnostics2ProtocolGuid = EFI_DRIVER_DIAGNOSTICS2_PROTOCOL_GUID;

EFI_GUID_STRING
  (&gEfiDriverDiagnostics2ProtocolGuid, "Driver Diagnostics Protocol", "UEFI 2.0 Driver Diagnostics2 Protocol");
