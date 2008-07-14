/** @file
Utility functions which helps in opcode creation, HII configuration string manipulations, 
pop up window creations, setup browser persistence data set and get.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiIfrLibraryInternal.h"

EFI_HII_DATABASE_PROTOCOL *gIfrLibHiiDatabase;
EFI_HII_STRING_PROTOCOL   *gIfrLibHiiString;


/**
  IfrSupportLib's constructor. It locates the required protocol:
  gEfiHiiDatabaseProtocolGuid and gEfiHiiStringProtocolGuid.

  @param ImageHandle     The firmware allocated handle for the EFI image.
  
  @param SystemTable     A pointer to the EFI System Table.

  @retval EFI_SUCCESS    This function always completes successfully.

**/
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


