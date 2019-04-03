/** @file
  Null Recovery Library instance does nothing and returns unsupported status.

  This library instance is no longer used and module using this library
  class should update to directly locate EFI_PEI_RECOVERY_MODULE_PPI defined
  in PI 1.2 specification.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Library/RecoveryLib.h>

/**
  Calling this function causes the system do recovery boot path.

  @retval EFI_UNSUPPORTED       Recovery is not supported.
**/
EFI_STATUS
EFIAPI
PeiRecoverFirmware (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

