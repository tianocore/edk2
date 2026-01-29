/** @file
  Handle TPM 2.0 physical presence requests from OS.

  This library will handle TPM 2.0 physical presence request from OS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction() and Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction()
  will receive untrusted input and do validation.

Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Library/HobLib.h>

#include "MmTcg2PhysicalPresenceLibCommon.h"

/**
  The constructor function locates SmmVariable protocol.

  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor successfully added string package.
  @retval Other value   The constructor can't add string package.
**/
EFI_STATUS
EFIAPI
Tcg2PhysicalPresenceLibStandaloneMmConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return Tcg2PhysicalPresenceLibCommonConstructor ();
}

/**
  Check if Tcg2 PP version is lower than PP_INF_VERSION_1_3.

  @retval TRUE    Tcg2 PP version is lower than PP_INF_VERSION_1_3.
  @retval Other   Tcg2 PP version is not lower than PP_INF_VERSION_1_3.
**/
BOOLEAN
IsTcg2PPVerLowerThan_1_3 (
  VOID
  )
{
  VOID  *GuidHob;

  GuidHob = GetFirstGuidHob (&gEdkiiTcgPhysicalPresenceInterfaceVerHobGuid);
  ASSERT (GuidHob != NULL);

  if (AsciiStrnCmp (PP_INF_VERSION_1_2, (CHAR8 *)GET_GUID_HOB_DATA (GuidHob), sizeof (PP_INF_VERSION_1_2) - 1) >= 0) {
    return TRUE;
  }

  return FALSE;
}
