/** @file
  Endoresment Seed generation part of PlatformTpmLib to use TpmLib.

  To see the plat_XXX interface in TPM reference library, see:
    - https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/Platform/src

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/PlatformTpmLib.h>

/**
  _plat__GetEPS()

  This function used to generate Endorsement Seed when
  first initailization of TPM.

  EPS can be generated, for example, as follows:

    EPS = SHA-512(TRNG_output || nonce || optional_mixing || DeviceUnique)

  Alternatively, EPS can be generated using DRBG_Generate(),
  as done in the TCG TPM 2.0 reference implementation.

  This function is not expected to fail; however,
  if it does have the potential to fail,
  the platform should handle the failure appropriately,
  for example by disabling TPM functionality or
  retrying the manufacturing process.

  @param [in]   Size              Size of endorsement seed
  @param [out]  EndorsementSeed   Endorsement Seed.

**/
VOID
EFIAPI
PlatformTpmLibGetEPS (
  IN  UINT16  Size,
  OUT UINT8   *EndorsementSeed
  )
{
  return;
}
