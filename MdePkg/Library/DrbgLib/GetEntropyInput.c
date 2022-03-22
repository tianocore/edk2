/** @file
  GetEntropyInput function implementation.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - [1] NIST Special Publication 800-90A Revision 1, June 2015, Recommendation
        for Random Number Generation Using Deterministic Random Bit Generators.
        (https://csrc.nist.gov/publications/detail/sp/800-90a/rev-1/final)
  - [2] NIST Special Publication 800-90B, Recommendation for the Entropy
        Sources Used for Random Bit Generation.
        (https://csrc.nist.gov/publications/detail/sp/800-90b/final)
  - [3] (Second Draft) NIST Special Publication 800-90C, Recommendation for
        Random Bit Generator (RBG) Constructions.
        (https://csrc.nist.gov/publications/detail/sp/800-90c/draft)

  @par Glossary:
    - TRNG - True Random Number Generator
    - Sec  - Security
    - DRBG - Deterministic Random Bits Generator
    - CTR  - Counter
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "Common.h"

/** GetEntropyInput implementation (no conditionning function).

  Cf. [3] 10.3.3.1 Construction When a Conditioning Function is not Used

  @param [in]   DrbgHandle        The Drbg hanble.
  @param [in]   MinEntropy        Minimum entropy.
  @param [out]  EntropyBitsStream Stream containing the generated entropy.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
GetEntropyInputNoCondFn (
  IN  DRBG_HANDLE  DrbgHandle,
  IN  UINTN        MinEntropy,
  OUT BIT_STREAM   **EntropyBitsStream
  )
{
  EFI_STATUS  Status;

  if ((DrbgHandle == NULL)          ||
      (EntropyBitsStream == NULL)   ||
      (*EntropyBitsStream != NULL))
  {
    ASSERT (DrbgHandle != NULL);
    ASSERT (EntropyBitsStream != NULL);
    ASSERT (*EntropyBitsStream == NULL);
    return EFI_INVALID_PARAMETER;
  }

  // 1. (status, entropy_bitstring) = Get_Entropy(min_entropy, max_length).
  // 2. If (status != SUCCESS), then return (status, Null).
  // 3. Return SUCCESS, entropy_bitstring.
  Status = GetEntropy (DrbgHandle, MinEntropy, EntropyBitsStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    // Fall through.
  }

  return Status;
}
