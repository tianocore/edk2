/** @file
  Get entropy part of PlatformTpmLib to use TpmLib.

  To see the plat_XXX interfaces in TPM reference library, see:
    - https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/Platform/src

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformTpmLib.h>

/**
  _plat__GetEntropy()

  This function is used to get available hardware entropy. In a hardware
  implementation of this function, there would be no call to the system
  to get entropy.

  @param [out] Entropy
  @param [in]  Amount    amount reuqested.

  @return < 0   Failed to generate entropy
  @return >= 0  The returned amount of entropy (bytes)

**/
INT32
EFIAPI
PlatformTpmLibGetEntropy (
  OUT UINT8   *Entropy, // output buffer
  IN  UINT32  Amount    // amount requested
  )
{
  return -1;
}
