/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UnicodeCollation2.c

Abstract:

  Unicode Collation2 protocol that follows the UEFI 2.0 specification.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (UnicodeCollation2)

EFI_GUID  gEfiUnicodeCollation2ProtocolGuid = EFI_UNICODE_COLLATION2_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiUnicodeCollation2ProtocolGuid, "Unicode Collation Protocol", "UEFI 2.0 Unicode Collation2 Protocol");
