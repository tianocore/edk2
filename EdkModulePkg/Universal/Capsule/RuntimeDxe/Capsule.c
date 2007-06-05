/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Capsule.c

Abstract:

  Capsule Runtime Service Initialization

--*/

#include "CapsuleService.h"


EFI_STATUS
EFIAPI
CapsuleServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  This code is capsule runtime service initialization.

Arguments:

  ImageHandle          The image handle
  SystemTable          The system table.

Returns:

  EFI STATUS

--*/
{
  EFI_STATUS  Status;
  EFI_HANDLE  NewHandle;

  SystemTable->RuntimeServices->UpdateCapsule                    = UpdateCapsule;
  SystemTable->RuntimeServices->QueryCapsuleCapabilities         = QueryCapsuleCapabilities;

  //
  // Now install the Capsule Architectural Protocol on a new handle
  //
  NewHandle = NULL;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiCapsuleArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
