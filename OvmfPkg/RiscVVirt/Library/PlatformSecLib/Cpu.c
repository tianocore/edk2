/** @file
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformSecLib.h"

/**
  Perform CPU initialization.

  @param  FdtPointer      The pointer to the device tree.

  @return EFI_SUCCESS     The platform initialized successfully.
  @retval  Others        - As the error code indicates

**/
EFI_STATUS
EFIAPI
CpuInitialization (
  VOID  *FdtPointer
  )
{
  //
  // for MMU type >= sv39
  //
  BuildCpuHob (56, 32);

  return EFI_SUCCESS;
}
