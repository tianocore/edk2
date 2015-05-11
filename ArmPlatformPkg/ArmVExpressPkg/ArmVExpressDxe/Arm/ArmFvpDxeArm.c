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
#include <Library/ArmPlatformLib.h>  // To get Core Count

//
// Description of the four ARM model platforms :
// Platform ids are defined in ArmVExpressInternal.h for
// all "ArmVExpress-like" platforms (AARCH64 or ARM architecture,
// model or hardware platforms).
//
CONST ARM_VEXPRESS_PLATFORM ArmVExpressPlatforms[] = {
  { ARM_FVP_VEXPRESS_A9x4,  FixedPcdGetPtr (PcdFdtVExpressFvpA9x4),  L"rtsm_ve-cortex_a9x4.dtb"  },
  { ARM_FVP_VEXPRESS_A15x1, FixedPcdGetPtr (PcdFdtVExpressFvpA15x1), L"rtsm_ve-cortex_a15x1.dtb" },
  { ARM_FVP_VEXPRESS_A15x2, FixedPcdGetPtr (PcdFdtVExpressFvpA15x2), L"rtsm_ve-cortex_a15x2.dtb" },
  { ARM_FVP_VEXPRESS_A15x4, FixedPcdGetPtr (PcdFdtVExpressFvpA15x4), L"rtsm_ve-cortex_a15x4.dtb" },
  { ARM_FVP_VEXPRESS_UNKNOWN, }
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
  UINT32                SysId;
  UINTN                 CpuType;
  EFI_STATUS            Status;
  UINTN                 CoreCount;

  ASSERT (Platform != NULL);

  CpuType   = 0;
  Status    = EFI_NOT_FOUND;
  *Platform = NULL;

  SysId = MmioRead32 (ARM_VE_SYS_ID_REG);
  if (SysId == ARM_RTSM_SYS_ID) {
    // Get the Cortex-A version
    CpuType = (ArmReadMidr () >> 4) & ARM_CPU_TYPE_MASK;
    if (CpuType == ARM_CPU_TYPE_A9) {
      Status = ArmVExpressGetPlatformFromId (ARM_FVP_VEXPRESS_A9x4, Platform);
    } else if (CpuType == ARM_CPU_TYPE_A15) {
      CoreCount = ArmGetCpuCountPerCluster ();
      if (CoreCount == 1) {
        Status = ArmVExpressGetPlatformFromId (ARM_FVP_VEXPRESS_A15x1, Platform);
      } else if (CoreCount == 2) {
        Status = ArmVExpressGetPlatformFromId (ARM_FVP_VEXPRESS_A15x2, Platform);
      } else if (CoreCount == 4) {
        Status = ArmVExpressGetPlatformFromId (ARM_FVP_VEXPRESS_A15x4, Platform);
      }
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unsupported platform (SysId:0x%X, CpuType:0x%X)\n", SysId, CpuType));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
