/** @file
  Handle TPM 2.0 physical presence requests from OS.

  This library will handle TPM 2.0 physical presence request from OS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction() and Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction()
  will receive untrusted input and do validation.

Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MM_TCG2_PHYSICAL_PRESENCE_LIB_COMMON_H_
#define _MM_TCG2_PHYSICAL_PRESENCE_LIB_COMMON_H_

#include <Guid/Tcg2PhysicalPresenceData.h>

#include <Protocol/SmmVariable.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tcg2PpVendorLib.h>
#include <Library/MmServicesTableLib.h>

#define   PP_INF_VERSION_1_2  "1.2"

/**
  The constructor function locates MmVariable protocol.

  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @retval EFI_SUCCESS   The constructor successfully added string package.
  @retval Other value   The constructor can't add string package.
**/
EFI_STATUS
Tcg2PhysicalPresenceLibCommonConstructor (
  VOID
  );

/**
  Check if Tcg2 PP version is lower than PP_INF_VERSION_1_3.

  @retval TRUE    Tcg2 PP version is lower than PP_INF_VERSION_1_3.
  @retval Other   Tcg2 PP version is not lower than PP_INF_VERSION_1_3.
**/
BOOLEAN
IsTcg2PPVerLowerThan_1_3 (
  VOID
  );

#endif
