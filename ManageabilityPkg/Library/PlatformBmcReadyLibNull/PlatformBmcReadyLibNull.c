/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/PlatformBmcReadyLib.h>

/**
  This function checks whether BMC is ready for transaction or not.

  @return TRUE  The BMC is ready.
  @return FALSE  The BMC is not ready.

**/
BOOLEAN
EFIAPI
PlatformBmcReady (
  VOID
  )
{
  // BMC is always ready as default
  return TRUE;
}
