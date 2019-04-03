/** @file
  Sample to provide TempRamInitParams data.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PcdLib.h>

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT32 TempRamInitParams[4] = {
  ((UINT32)FixedPcdGet64 (PcdCpuMicrocodePatchAddress) + FixedPcdGet32 (PcdFlashMicroCodeOffset)),
  ((UINT32)FixedPcdGet64 (PcdCpuMicrocodePatchRegionSize) - FixedPcdGet32 (PcdFlashMicroCodeOffset)),
  FixedPcdGet32 (PcdFlashCodeCacheAddress),
  FixedPcdGet32 (PcdFlashCodeCacheSize)
};
