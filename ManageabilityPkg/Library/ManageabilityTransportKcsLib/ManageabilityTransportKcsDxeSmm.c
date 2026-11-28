/** @file

  KCS instance of Manageability Transport Library

  Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/IpmiKcs.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportHelperLib.h>

#include "ManageabilityTransportKcs.h"

MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO  mKcsHardwareInfo;
MANAGEABILITY_TRANSPORT_KCS                *mSingleSessionToken = NULL;

/**
  Get the KCS hardware information of the current boot phase.

  @return Pointer to the KCS hardware information.
**/
MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *
EFIAPI
KcsGetHardwareInfo (
  VOID
  )
{
  return &mKcsHardwareInfo;
}

/**
  Get the KCS session token of the current boot phase.

  @return Pointer to the session token, or NULL if no session is acquired.
**/
MANAGEABILITY_TRANSPORT_KCS *
EFIAPI
KcsGetSessionToken (
  VOID
  )
{
  return mSingleSessionToken;
}

/**
  Record the KCS session token of the current boot phase.

  @param[in]  SessionToken  Session token to record, or NULL to clear it.

  @retval EFI_SUCCESS       The session token was recorded.
**/
EFI_STATUS
EFIAPI
KcsSetSessionToken (
  IN  MANAGEABILITY_TRANSPORT_KCS  *SessionToken
  )
{
  mSingleSessionToken = SessionToken;
  return EFI_SUCCESS;
}
