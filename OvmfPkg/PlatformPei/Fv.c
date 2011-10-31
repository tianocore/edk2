/** @file
  Build FV related hobs for platform.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiPei.h"
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>


/**
  Perform a call-back into the SEC simulator to get address of the Firmware Hub

  @param  FfsHeader     Ffs Header availible to every PEIM
  @param  PeiServices   General purpose services available to every PEIM.

  @retval EFI_SUCCESS   Platform PEI FVs were initialized successfully.

**/
EFI_STATUS
PeiFvInitialization (
  VOID
  )
{
  DEBUG ((EFI_D_ERROR, "Platform PEI Firmware Volume Initialization\n"));

  DEBUG (
    (EFI_D_ERROR, "Firmware Volume HOB: 0x%x 0x%x\n",
      PcdGet32 (PcdOvmfMemFvBase),
      PcdGet32 (PcdOvmfMemFvSize)
      )
    );

  BuildFvHob (PcdGet32 (PcdOvmfMemFvBase), PcdGet32 (PcdOvmfMemFvSize));

  //
  // Create a memory allocation HOB.
  //
  BuildMemoryAllocationHob (
    PcdGet32 (PcdOvmfMemFvBase),
    PcdGet32 (PcdOvmfMemFvSize),
    EfiBootServicesData
    );

  return EFI_SUCCESS;
}

