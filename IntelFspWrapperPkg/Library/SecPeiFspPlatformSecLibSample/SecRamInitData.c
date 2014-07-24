/** @file
  Sample to provide TempRamInitParams data.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/PcdLib.h>

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT32 TempRamInitParams[4] = {
  ((UINT32)FixedPcdGet64 (PcdCpuMicrocodePatchAddress) + FixedPcdGet32 (PcdFlashMicroCodeOffset)),
  ((UINT32)FixedPcdGet64 (PcdCpuMicrocodePatchRegionSize) - FixedPcdGet32 (PcdFlashMicroCodeOffset)),
  FixedPcdGet32 (PcdFlashCodeCacheAddress),
  FixedPcdGet32 (PcdFlashCodeCacheSize)
};
