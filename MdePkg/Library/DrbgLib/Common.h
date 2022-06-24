/** @file
  Implementation of arch specific functions for the Drbg library.

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
  - [4] NIST Special Publication 800-57 Part 1 Revision 5, May 2020,
        Recommendation for Key Management:Part 1 - General.
        (https://csrc.nist.gov/publications/detail/sp/800-57-part-1/rev-5/final)

  @par Glossary:
    - TRNG - True Random Number Generator
    - Sec  - Security
    - DRBG - Deterministic Random Bits Generator
    - CTR  - Counter
**/

#ifndef ARCH_IMPLEM_H_
#define ARCH_IMPLEM_H_

#include "DrbgLibInternal.h"

/** GetEntropy implementation using Arm Trng.

  Cf. [3] 10.3.1.2 Condensing After Entropy Collection

  The min and max entropy length are in the DrbgHandle.

  @param [in]   DrbgHandle        The Drbg hanble.
  @param [in]   ReqEntropy        Requested entropy.
  @param [out]  EntropyBitsStream Stream containing the generated entropy.

  @retval EFI_SUCCESS             Success.
  @retval EFI_ABORTED             An error occured.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
GetEntropy (
  IN  DRBG_HANDLE  DrbgHandle,
  IN  UINTN        ReqEntropy,
  OUT BIT_STREAM   **EntropyBitsStream
  );

/** Block encryption (for AES).

  Cf. [1] s10.3.3 'BCC and Block_Encrypt'.

  @param [in]   DrbgHandle      The Drbg hanble.
  @param [out]  OutBlockStream  Stream containing the encrypted block.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BlockEncrypt (
  IN  DRBG_HANDLE  DrbgHandle,
  OUT BIT_STREAM   **OutBlockStream
  );

#endif // ARCH_IMPLEM_H_
