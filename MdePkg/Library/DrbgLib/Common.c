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

#include <Library/AesLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TrngLib.h>

#include "Common.h"
#include "CtrDrbg.h"

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
  )
{
  EFI_STATUS  Status;
  UINTN       TrngCollectedEntropy;
  UINTN       TrngReqBits;
  UINTN       TrngMaxBits;
  UINTN       TrngMaxBytes;
  UINTN       MinLen;
  UINT8       *QueriedBitsBuff;

  if ((DrbgHandle == NULL)        ||
      (EntropyBitsStream == NULL) ||
      (*EntropyBitsStream != NULL))
  {
    ASSERT (DrbgHandle != NULL);
    ASSERT (EntropyBitsStream != NULL);
    ASSERT (*EntropyBitsStream == NULL);
    return EFI_INVALID_PARAMETER;
  }

  MinLen = 0;
  TrngMaxBits     = GetTrngMaxSupportedEntropyBits ();
  TrngMaxBytes    = BitsToUpperBytes (TrngMaxBits);
  QueriedBitsBuff = NULL;

  // 1. If requested_entropy > max_length, return an error indication
  // and a null value for the entropy_bitstring.
  //
  // Note: we also check for MinLen
  if ((ReqEntropy > DrbgHandle->DrbgVal.MaxLen) ||
      (ReqEntropy < DrbgHandle->DrbgVal.MinLen))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // 2. collected_entropy = 0.
  TrngCollectedEntropy = 0;

  // 3. entropy_bitstring = the Null string.
  Status = BitStreamAlloc (ReqEntropy, EntropyBitsStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  QueriedBitsBuff = (UINT8 *)AllocateZeroPool (TrngMaxBytes);
  if (QueriedBitsBuff == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  // 4. While collected_entropy < requested_entropy
  while (TrngCollectedEntropy < ReqEntropy) {
    TrngReqBits = MIN ((MinLen - TrngCollectedEntropy), TrngMaxBits);

    // 4.1 Query one or more entropy sources to obtain queried_bits and the
    // assessed_entropy for those bits.
    //
    // Cf. Arm True Random Number Generator Firmware, Interface 1.0,
    // s2.4.2 Usage, the number of bits requested to the TRNG equals the
    // number of bits returned. So assessed_entropy == #queried_bits
    Status = GetTrngEntropy (TrngReqBits, TrngMaxBytes, QueriedBitsBuff);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ErrorHandler;
    }

    // 4.2 entropy_bitstring = entropy_bitstring || queried_bits.
    //
    // We are concatenating the other way around. Since this is a TRNG and
    // the endianness of queried_bits is meaningless, this is the same.
    Status = BitStreamWrite (
               QueriedBitsBuff,
               TrngCollectedEntropy,
               TrngReqBits,
               *EntropyBitsStream
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ErrorHandler;
    }

    // 4.3 collected_entropy = collected_entropy + assessed_entropy.
    //
    // Cf above, for Arm TRNG, assessed_entropy == #queried_bits
    TrngCollectedEntropy += TrngReqBits;
  } // while

  // 6. If (n > max_length), then
  // entropy_bitstring = df(entropy_bitstring, max_length).
  //
  // Note: This cannot happen.
  if (DrbgHandle->DrbgVal.MaxLen != 0) {
    Status = EFI_ABORTED;
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  FreePool (QueriedBitsBuff);

  // 7. Return (SUCCESS, entropy_bitstring).
  return Status;

ErrorHandler:
  if (QueriedBitsBuff) {
    FreePool (QueriedBitsBuff);
  }

  BitStreamFree (EntropyBitsStream);
  return Status;
}

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
  )
{
  EFI_STATUS             Status;
  AES_CTX                AesCtx;
  CTR_INTERNAL_STATE     *CtrIntState;
  CTR_VALUE_DEFINITIONS  *CtrVal;

  if ((DrbgHandle == NULL)                  ||
      (DrbgHandle->IntState.DrbgAlgoIntState == NULL) ||
      (DrbgHandle->DrbgVal.DrbgAlgoVal == NULL) ||
      (OutBlockStream == NULL)              ||
      (*OutBlockStream != NULL))
  {
    ASSERT (DrbgHandle != NULL);
    ASSERT (DrbgHandle->IntState.DrbgAlgoIntState != NULL);
    ASSERT (DrbgHandle->DrbgVal.DrbgAlgoVal != NULL);
    ASSERT (OutBlockStream != NULL);
    ASSERT (*OutBlockStream == NULL);
    return EFI_INVALID_PARAMETER;
  }

  CtrVal      = (CTR_VALUE_DEFINITIONS *)DrbgHandle->DrbgVal.DrbgAlgoVal;
  CtrIntState = (CTR_INTERNAL_STATE *)DrbgHandle->IntState.DrbgAlgoIntState;
  if (IsBitStreamEmpty (CtrIntState->Key)  ||
      IsBitStreamEmpty (CtrIntState->Val)
      )
  {
    ASSERT (!IsBitStreamEmpty (CtrIntState->Key));
    ASSERT (!IsBitStreamEmpty (CtrIntState->Val));
    return EFI_INVALID_PARAMETER;
  }

  Status = AesInitCtx (
             BitStreamData (CtrIntState->Key),
             BitStreamBitLen (CtrIntState->Key),
             &AesCtx
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = BitStreamAlloc (CtrVal->BlockLen, OutBlockStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AesEncrypt (
             &AesCtx,
             BitStreamData (CtrIntState->Val),
             BitStreamData (*OutBlockStream)
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    BitStreamFree (OutBlockStream);
    // Fall through.
  }

  return Status;
}
