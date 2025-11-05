/** @file
  RISC-V specific functionality for DxeLoad.

  Copyright (C) 2023, Rivos Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/FdtLib.h>
#include <Library/PcdLib.h>
#include "UefiPayloadEntry.h"

/**
  Entry point to the C language phase of UEFI payload.
  @param[in]   Param1, Hartid which is ignored
  @param[in]   Param2, Device Tree
  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  Param1,
  IN UINTN  Param2
  )
{
  return FitUplEntryPoint (Param2);
}
