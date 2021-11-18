/** @file
  Platform CSM Support Library

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CsmSupportLib.h"

/**
  The constructor function for the platform CSM support library

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
CsmSupportLibConstructor (
  VOID
  )
{
  LegacyRegionInit ();

  LegacyInterruptInstall ();

  LegacyBiosPlatformInstall ();

  return EFI_SUCCESS;
}
