/** @file

  Copyright (c) 2014-2015, ARM Ltd. All rights reserved.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM_VEXPRESS_INTERNAL_H__
#define __ARM_VEXPRESS_INTERNAL_H__

#include <Uefi.h>

#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <VExpressMotherBoard.h>

// This 'enum' is needed as variations based on existing platform exist
typedef enum {
  ARM_FVP_VEXPRESS_UNKNOWN = 0,
  ARM_FVP_VEXPRESS_A9x4,
  ARM_FVP_VEXPRESS_A15x1,
  ARM_FVP_VEXPRESS_A15x2,
  ARM_FVP_VEXPRESS_A15x4,
  ARM_FVP_VEXPRESS_A15x1_A7x1,
  ARM_FVP_VEXPRESS_A15x4_A7x4,
  ARM_FVP_VEXPRESS_AEMv8x4,
  ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV2,
  ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV2_LEGACY,
  ARM_FVP_BASE_AEMv8x4_AEMv8x4_GICV3,
  ARM_FVP_FOUNDATION_GICV2,
  ARM_FVP_FOUNDATION_GICV2_LEGACY,
  ARM_FVP_FOUNDATION_GICV3,
  ARM_HW_A9x4,
  ARM_HW_A15x2_A7x3,
  ARM_HW_A15,
  ARM_HW_A5
} ARM_VEXPRESS_PLATFORM_ID;

typedef struct {
  ARM_VEXPRESS_PLATFORM_ID  Id;

  // Flattened Device Tree (FDT) File
  CONST EFI_GUID            *FdtGuid; /// Name of the FDT when present into the FV
  CONST CHAR16              *FdtName; /// Name of the FDT when present into a File System
} ARM_VEXPRESS_PLATFORM;

// Array that contains the list of the VExpress based platform supported by this DXE driver
extern CONST ARM_VEXPRESS_PLATFORM ArmVExpressPlatforms[];

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
  );

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
  );

#endif // __ARM_VEXPRESS_INTERNAL_H__
