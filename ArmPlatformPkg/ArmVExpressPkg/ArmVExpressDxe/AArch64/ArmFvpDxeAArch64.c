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
  { ARM_FVP_VEXPRESS_AEMv8x4,                  FixedPcdGetPtr (PcdFdtFvpVExpressAEMv8x4),        L"rtsm_ve-aemv8a.dtb"                  },
  { ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV2,        FixedPcdGetPtr (PcdFdtFvpBaseAEMv8x4GicV2),       L"fvp-base-gicv2-psci.dtb"             },
  { ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV2_LEGACY, FixedPcdGetPtr (PcdFdtFvpBaseAEMv8x4GicV2Legacy), L"fvp-base-gicv2legacy-psci.dtb"       },
  { ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV3,        FixedPcdGetPtr (PcdFdtFvpBaseAEMv8x4GicV3),       L"fvp-base-gicv3-psci.dtb"             },
  { ARM_FVP_FOUNDATION_GICV2,                  FixedPcdGetPtr (PcdFdtFvpFoundationGicV2),        L"fvp-foundation-gicv2-psci.dtb"       },
  { ARM_FVP_FOUNDATION_GICV2_LEGACY,           FixedPcdGetPtr (PcdFdtFvpFoundationGicV2Legacy),  L"fvp-foundation-gicv2legacy-psci.dtb" },
  { ARM_FVP_FOUNDATION_GICV3,                  FixedPcdGetPtr (PcdFdtFvpFoundationGicV3),        L"fvp-foundation-gicv3-psci.dtb"       },
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
  UINT32                FvpSysId;
  UINT32                VariantSysId;
  ARM_GIC_ARCH_REVISION GicRevision;

  ASSERT (Platform != NULL);

  Status = EFI_NOT_FOUND;

  SysId = MmioRead32 (ARM_VE_SYS_ID_REG);
  if (SysId != ARM_RTSM_SYS_ID) {
    // Remove the GIC variant to identify if we are running on the FVP Base or
    // Foundation models
    FvpSysId     = SysId & (ARM_FVP_SYS_ID_HBI_MASK |
                            ARM_FVP_SYS_ID_PLAT_MASK );
    // Extract the variant from the SysId
    VariantSysId = SysId & ARM_FVP_SYS_ID_VARIANT_MASK;

    if (FvpSysId == ARM_FVP_BASE_BOARD_SYS_ID) {
      if (VariantSysId == ARM_FVP_GIC_VE_MMAP) {
        // FVP Base Model with legacy GIC memory map
        Status = ArmVExpressGetPlatformFromId (ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV2_LEGACY, Platform);
      } else {
        GicRevision = ArmGicGetSupportedArchRevision ();

        if (GicRevision == ARM_GIC_ARCH_REVISION_2) {
          // FVP Base Model with GICv2 support
          Status = ArmVExpressGetPlatformFromId (ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV2, Platform);
        } else {
          // FVP Base Model with GICv3 support
          Status = ArmVExpressGetPlatformFromId (ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV3, Platform);
        }
      }
    } else if (FvpSysId == ARM_FVP_FOUNDATION_BOARD_SYS_ID) {
      if (VariantSysId == ARM_FVP_GIC_VE_MMAP) {
        // FVP Foundation Model with legacy GIC memory map
        Status = ArmVExpressGetPlatformFromId (ARM_FVP_FOUNDATION_GICV2_LEGACY, Platform);
      } else {
        GicRevision = ArmGicGetSupportedArchRevision ();

        if (GicRevision == ARM_GIC_ARCH_REVISION_2) {
          // FVP Foundation Model with GICv2
          Status = ArmVExpressGetPlatformFromId (ARM_FVP_FOUNDATION_GICV2, Platform);
        } else {
          // FVP Foundation Model with GICv3
          Status = ArmVExpressGetPlatformFromId (ARM_FVP_FOUNDATION_GICV3, Platform);
        }
      }
    }
  } else {
    // FVP Versatile Express AEMv8
    Status = ArmVExpressGetPlatformFromId (ARM_FVP_VEXPRESS_AEMv8x4, Platform);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unsupported AArch64 RTSM (SysId:0x%X).\n", SysId));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
