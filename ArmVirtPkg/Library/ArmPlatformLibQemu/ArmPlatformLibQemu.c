/** @file

  Copyright (c) 2011-2012, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>

/**
  Return the current Boot Mode.

  This function returns the boot reason on the platform

  @return   Return the current Boot Mode of the platform

**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  Initialize controllers that must setup in the normal world.

  This function is called by the ArmPlatformPkg/PrePi or
  ArmPlatformPkg/PlatformPei in the PEI phase.

  @param[in]     MpId               ID of the calling CPU

  @return        RETURN_SUCCESS unless the operation failed
**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN  MpId
  )
{
  return RETURN_SUCCESS;
}

/**
  Return the Platform specific PPIs.

  This function exposes the Platform Specific PPIs. They can be used by any
  PrePi modules or passed to the PeiCore by PrePeiCore.

  @param[out]   PpiListSize         Size in Bytes of the Platform PPI List
  @param[out]   PpiList             Platform PPI List

**/
VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  *PpiListSize = 0;
  *PpiList     = NULL;
}
