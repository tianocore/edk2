/*++

Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  HiiPackageList.c

Abstract:

  EFI_HII_PACKAGE_LIST_PROTOCOL as defined in UEFI 2.1 spec.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (HiiPackageList)

EFI_GUID  gEfiHiiPackageListProtocolGuid = EFI_HII_PACKAGE_LIST_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiHiiPackageListProtocolGuid, "HII Package List Protocol", "HII Package List 2.1 protocol");
