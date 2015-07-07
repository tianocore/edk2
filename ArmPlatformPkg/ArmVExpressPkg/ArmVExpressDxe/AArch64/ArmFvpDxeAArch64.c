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
#include <Library/ArmGicLib.h>

//
// Description of the AARCH64 model platforms :
// Platform ids are defined in ArmVExpressInternal.h for
// all "ArmVExpress-like" platforms (AARCH64 or ARM architecture,
// model or hardware platforms).
//
CONST ARM_VEXPRESS_PLATFORM ArmVExpressPlatforms[] = {
  { ARM_FVP_VEXPRESS_AEMv8x4, FixedPcdGetPtr (PcdFdtFvpVExpressAEMv8x4), L"rtsm_ve-aemv8a.dtb" },
  { ARM_FVP_BASE, FixedPcdGetPtr (PcdFdtFvpBaseAEMv8x4), L"fvp-base.dtb" },
  { ARM_FVP_FOUNDATION, FixedPcdGetPtr (PcdFdtFvpFoundation), L"fvp-foundation.dtb" },
  { ARM_FVP_VEXPRESS_UNKNOWN, &gZeroGuid }
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
  EFI_STATUS  Status;
  UINT32      SysId;
  UINT32      FvpSysId;

  ASSERT (Platform != NULL);

  Status = EFI_NOT_FOUND;

  SysId = MmioRead32 (ARM_VE_SYS_ID_REG);
  if (SysId != ARM_RTSM_SYS_ID) {
    //
    // Keep only the HBI board number and the platform type fields of the
    // system id register to identify if we are running on the FVP base or
    // foundation model.
    //
    FvpSysId = SysId & (ARM_FVP_SYS_ID_HBI_MASK |
                        ARM_FVP_SYS_ID_PLAT_MASK );

    if (FvpSysId == ARM_FVP_BASE_BOARD_SYS_ID) {
      Status = ArmVExpressGetPlatformFromId (ARM_FVP_BASE, Platform);
    } else if (FvpSysId == ARM_FVP_FOUNDATION_BOARD_SYS_ID) {
      Status = ArmVExpressGetPlatformFromId (ARM_FVP_FOUNDATION, Platform);
    }
  } else {
    //
    // FVP Versatile Express AEMv8
    //
    Status = ArmVExpressGetPlatformFromId (ARM_FVP_VEXPRESS_AEMv8x4, Platform);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unsupported AArch64 RTSM (SysId:0x%X).\n", SysId));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
