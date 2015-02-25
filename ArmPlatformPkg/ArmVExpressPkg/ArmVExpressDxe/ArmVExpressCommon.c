/** @file

  Copyright (c) 2014, ARM Ltd. All rights reserved.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArmVExpressInternal.h"

/**
  Get information about the VExpress platform the firmware is running on given its Id.

  @param[in]   PlatformId  Id of the VExpress platform.
  @param[out]  Platform    Address where the pointer to the platform information
                           (type ARM_VEXPRESS_PLATFORM*) should be stored.
                           The returned pointer does not point to an allocated
                           memory area.

  @retval  EFI_SUCCESS    The platform information was returned.
  @retval  EFI_NOT_FOUND  The platform was not recognised.

**/
EFI_STATUS
ArmVExpressGetPlatformFromId (
  IN  CONST ARM_VEXPRESS_PLATFORM_ID PlatformId,
  OUT CONST ARM_VEXPRESS_PLATFORM**  Platform
  )
{
  UINTN Index;

  ASSERT (Platform != NULL);

  for (Index = 0; ArmVExpressPlatforms[Index].Id != ARM_FVP_VEXPRESS_UNKNOWN; Index++) {
    if (ArmVExpressPlatforms[Index].Id == PlatformId) {
      *Platform = &ArmVExpressPlatforms[Index];
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
