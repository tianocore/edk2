/** @file

  Copyright (c) 2014-2015, ARM Ltd. All rights reserved.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArmVExpressInternal.h"

//
// Description of the two AARCH64 model platforms :
// just the platform id for the time being.
// Platform ids are defined in ArmVExpressInternal.h for
// all "ArmVExpress-like" platforms (AARCH64 or ARM architecture,
// model or hardware platforms).
//
CONST ARM_VEXPRESS_PLATFORM ArmVExpressPlatforms[] = {
  { ARM_FVP_VEXPRESS_AEMv8x4 },
  { ARM_FVP_BASE_AEMv8x4_AEMv8x4 },
  { ARM_FVP_FOUNDATION },
  { ARM_FVP_VEXPRESS_UNKNOWN }
};

/**
  Get information about the VExpress platform the firmware is running on.

  @param[out]  Platform   Address where the pointer to the platform information
                          (type ARM_VEXPRESS_PLATFORM*) should be stored.
                          The returned pointer does not point to an allocated
                          memory area.

  @retval  EFI_SUCCESS    The platform information was returned.
  @retval  EFI_NOT_FOUND  The platform was not recognised.

**/
EFI_STATUS
ArmVExpressGetPlatform (
  OUT CONST ARM_VEXPRESS_PLATFORM** Platform
  )
{
  EFI_STATUS            Status;
  UINT32                SysId;

  ASSERT (Platform != NULL);

  Status = EFI_NOT_FOUND;

  SysId = MmioRead32 (ARM_VE_SYS_ID_REG);
  if (SysId != ARM_RTSM_SYS_ID) {
    // Take out the FVP GIC variant to reduce the permutations. The GIC driver
    // detects the version and does the right thing.
    SysId &= ~ARM_FVP_SYS_ID_VARIANT_MASK;
    if (SysId == (ARM_FVP_BASE_SYS_ID & ~ARM_FVP_SYS_ID_VARIANT_MASK)) {
      Status = ArmVExpressGetPlatformFromId (ARM_FVP_BASE_AEMv8x4_AEMv8x4, Platform);
    } else if (SysId == (ARM_FVP_FOUNDATION_SYS_ID & ~ARM_FVP_SYS_ID_VARIANT_MASK)) {
      Status = ArmVExpressGetPlatformFromId (ARM_FVP_FOUNDATION, Platform);
    }
  } else {
    Status = ArmVExpressGetPlatformFromId (ARM_FVP_VEXPRESS_AEMv8x4, Platform);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unsupported AArch64 RTSM (SysId:0x%X).\n", SysId));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
