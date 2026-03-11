/** @file
  In some implementations of the TPM, the hardware can provide a secret
  value to the TPM. This secret value is statistically unique to the
  instance of the TPM. Typical uses of this value are to provide
  personalization to the random number generation and as a shared secret
  between the TPM and the manufacturer.

  To see the plat_XXX interfaces in TPM reference library, see:
    - https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/Platform/src

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/PlatformTpmLib.h>

/**
  _plat__GetUnique()

  This function is used to access the platform-specific unique value.
  This function places the unique value in the provided buffer
  and returns the number of bytes transferred. The function will not
  copy more data than 'Size'.

  NOTE: If a platform unique value has unequal distribution of uniqueness
  and 'Size' is smaller than the size of the unique value, the 'Size'
  portion with the most uniqueness should be returned.

  @param [in]  Which    0: reserved, 1: permanent vendor unique value.
  @param [in]  Size     Size of Buffer
  @param [out] Buffer   Output Buffer

  @return Size of unique value.

**/
UINT32
EFIAPI
PlatformTpmLibGetUnique (
  IN  UINT32  Which,
  IN  UINT32  Size,
  OUT UINT8   *Buffer
  )
{
  return 0;
}
