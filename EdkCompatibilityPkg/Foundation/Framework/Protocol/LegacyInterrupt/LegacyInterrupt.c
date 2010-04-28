/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  LegacyInterrupt.c
    
Abstract:

  EFI Legacy InterruptProtocol

Revision History

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (LegacyInterrupt)

EFI_GUID  gEfiLegacyInterruptProtocolGuid = EFI_LEGACY_INTERRUPT_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiLegacyInterruptProtocolGuid, "Legacy Interrupt Protocol", "Legacy Interrupt Protocol");
