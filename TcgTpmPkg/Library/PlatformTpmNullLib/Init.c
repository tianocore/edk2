/** @file
  Platform specific Initialisation for PlatformTpmLib.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/PlatformTpmLib.h>

/**
  This function is called in TpmLibConstructor() to
  initialise PlatformTpmLib once.

  @return EFI_SUCCESS   Success
  @return Others        Errors

**/
EFI_STATUS
EFIAPI
PlatformTpmLibInit (
  VOID
  )
{
  return EFI_SUCCESS;
}
