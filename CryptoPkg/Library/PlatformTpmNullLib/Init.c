/** @file
  Platform specific Initialisation for PlatformTpmLib.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
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
  IN VOID
  )
{
  return EFI_SUCCESS;
}
