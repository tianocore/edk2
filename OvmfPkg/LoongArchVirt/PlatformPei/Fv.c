/** @file
  Build FV related hobs for platform.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/HobLib.h>

/**
  Publish PEI & DXE (Decompressed) Memory based FVs to let PEI
  and DXE know about them.

  @retval EFI_SUCCESS   Platform PEI FVs were initialized successfully.
**/
EFI_STATUS
PeiFvInitialization (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "Platform PEI Firmware Volume Initialization\n"));

  //
  // Create a memory allocation HOB for the PEI FV.
  //
  BuildMemoryAllocationHob (
    FixedPcdGet64 (PcdOvmfSecPeiTempRamBase),
    FixedPcdGet32 (PcdOvmfSecPeiTempRamSize),
    EfiBootServicesData
    );

  return EFI_SUCCESS;
}
