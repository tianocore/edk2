/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UefiIfrCommon.c

Abstract:

  Common Library Routines to assist handle HII elements.


**/

#include "UefiIfrLibraryInternal.h"

EFI_HII_DATABASE_PROTOCOL *gIfrLibHiiDatabase;
EFI_HII_STRING_PROTOCOL   *gIfrLibHiiString;


EFI_STATUS
EFIAPI
IfrSupportLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **) &gIfrLibHiiDatabase);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **) &gIfrLibHiiString);
  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
}


