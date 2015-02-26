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
// just the platform id for the time being.
// Platform ids are defined in ArmVExpressInternal.h for
// all "ArmVExpress-like" platforms (AARCH64 or ARM architecture,
// model or hardware platforms).
//
CONST ARM_VEXPRESS_PLATFORM ArmVExpressPlatforms[] = {
  { ARM_FVP_VEXPRESS_AEMv8x4, { 0x240d627e, 0xeb4d, 0x447d, { 0x86, 0x30, 0xf2, 0x12, 0x3b, 0xc3, 0x89, 0x3f } }, L"rtsm_ve-aemv8a.dtb" },
  { ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV2, { 0xfbed0b94, 0x3c81, 0x49e4, { 0xbd, 0xec, 0xd4, 0xf4, 0xa8, 0x47, 0xeb, 0x51 } }, L"fvp-base-gicv2-psci.dtb" },
  { ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV2_LEGACY, { 0x6d8dfd91, 0x431a, 0x4e6c, { 0x84, 0x5a, 0x15, 0xf1, 0xfc, 0x8d, 0x9b, 0xe2 } }, L"fvp-base-gicv2legacy-psci.dtb" },
  { ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV3, { 0xa8ac2953, 0xc4bb, 0x4914, { 0x87, 0xf5, 0xc5, 0x97, 0x5d, 0x7b, 0x32, 0xd1 } }, L"fvp-base-gicv3-psci.dtb" },
  { ARM_FVP_FOUNDATION_GICV2, { 0x4328cb83, 0x6e50, 0x4cb8, { 0xaa, 0x8c, 0xfb, 0x7a, 0xba, 0xba, 0x37, 0xd8 } }, L"fvp-foundation-gicv2-psci.dtb" },
  { ARM_FVP_FOUNDATION_GICV2_LEGACY, { 0x6af00d18, 0x6dfe, 0x45ed, { 0xa6, 0xc4, 0xb4, 0x4d, 0x57, 0xfd, 0xd8, 0xf5 } }, L"fvp-foundation-gicv2legacy-psci.dtb" },
  { ARM_FVP_FOUNDATION_GICV3, { 0x4a17b7fa, 0xf052, 0x4dc1, { 0x92, 0x79, 0x43, 0x80, 0x92, 0x41, 0xf9, 0x98 } }, L"fvp-foundation-gicv3-psci.dtb" },
  { ARM_FVP_VEXPRESS_UNKNOWN, { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } } }
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
