/** @file
  Handle TPM 2.0 physical presence requests from OS.

  This library will handle TPM 2.0 physical presence request from OS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction() and Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction()
  will receive untrusted input and do validation.

Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>

#include "MmTcg2PhysicalPresenceMinimumLibCommon.h"

/**
  This constructor function calls the common, shared implementation

  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor successfully added string package.
**/
EFI_STATUS
EFIAPI
TraditionalMmTcg2PhysicalPresenceMinimumLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return Tcg2PhysicalPresenceMinimumLibCommonConstructor ();
}
