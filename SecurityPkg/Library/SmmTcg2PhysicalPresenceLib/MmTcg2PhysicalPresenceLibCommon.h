/** @file
  Handle TPM 2.0 physical presence requests from OS.

  This library will handle TPM 2.0 physical presence request from OS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction() and Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction()
  will receive untrusted input and do validation.

Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MM_TCG2_PHYSICAL_PRESENCE_LIB_COMMON_H_
#define _MM_TCG2_PHYSICAL_PRESENCE_LIB_COMMON_H_

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

#endif
