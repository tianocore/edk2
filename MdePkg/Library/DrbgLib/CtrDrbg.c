/** @file
  Ctr Drbg implementation.
  (Counter Deterministic Random Bit Generator)
  Cf. [1] s10.2.1 CTR_DRBG

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
  - [5] Unified Extensible Firmware Interface (UEFI) Specification,
        Version 2.8 Errata B, May 2020
        (https://www.uefi.org/specifications)
  - [6] FIPS 197 November 26, 2001:
        Specification for the ADVANCED ENCRYPTION STANDARD (AES)

  @par Glossary:
    - TRNG - True Random Number Generator
    - Sec  - Security
    - DRBG - Deterministic Random Bits Generator
    - CTR  - Counter
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include "Common.h"
#include "CtrDrbg.h"

/** Get a nonce.

  [1] s10.2.1.3.1 Instantiation When a Derivation Function is Not Used

  When instantiation is performed using this method, full-entropy input
  is required, and a nonce is not used.

  @param  [in, out] DrbgHandle    The Drbg handle.
  @param  [out]     NonceStream   Stream containing the Nonce.

  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
CtrDrbgGetNonce (
  IN  OUT DRBG_HANDLE  DrbgHandle,
  OUT     BIT_STREAM   *NonceStream
  )
{
  // Nothing to do.
  return EFI_SUCCESS;
}

/** Check the internal state.

  @param  [in] DrbgHandle    The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
CtrDrbgCheckInternalState (
  IN DRBG_HANDLE  DrbgHandle
  )
{
  CTR_INTERNAL_STATE  *CtrIntState;

  CtrIntState = (CTR_INTERNAL_STATE *)DrbgHandle->IntState.DrbgAlgoIntState;

  // Just check that key and value BitStreams are still ok.
  if ((IsBitStreamEmpty (CtrIntState->Val)   ||
       (IsBitStreamEmpty (CtrIntState->Key))))
  {
    ASSERT (!IsBitStreamEmpty (CtrIntState->Val));
    ASSERT (!IsBitStreamEmpty (CtrIntState->Key));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Update algorithm.

  CTR_DRBG_Update implementation.

  Cf. [1] s10.2.1.2 The Update Function (CTR_DRBG_Update)

  @param [in]       ProvidedData  The data to be used. This must be exactly
                                  seedlen bits in length.
  @param [in, out]  DrbgHandle    The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
CtrDrbgUpdate (
  IN      BIT_STREAM   *ProvidedData,
  IN OUT  DRBG_HANDLE  DrbgHandle
  )
{
  EFI_STATUS             Status;
  BIT_STREAM             *IncStream;
  BIT_STREAM             *OutBlkStream;
  BIT_STREAM             *TempStream;
  CTR_INTERNAL_STATE     *CtrIntState;
  CTR_VALUE_DEFINITIONS  *CtrVal;

  if (IsBitStreamEmpty (ProvidedData)  ||
      (DrbgHandle == NULL)            ||
      (DrbgHandle->IntState.DrbgAlgoIntState == NULL) ||
      (DrbgHandle->DrbgVal.DrbgAlgoVal == NULL))
  {
    ASSERT (!IsBitStreamEmpty (ProvidedData));
    ASSERT (DrbgHandle != NULL);
    ASSERT (DrbgHandle->IntState.DrbgAlgoIntState != NULL);
    ASSERT (DrbgHandle->DrbgVal.DrbgAlgoVal != NULL);
    return EFI_INVALID_PARAMETER;
  }

  IncStream    = NULL;
  OutBlkStream = NULL;
  TempStream   = NULL;
  CtrIntState  = (CTR_INTERNAL_STATE *)DrbgHandle->IntState.DrbgAlgoIntState;
  CtrVal       = (CTR_VALUE_DEFINITIONS *)DrbgHandle->DrbgVal.DrbgAlgoVal;

  if (BitStreamBitLen (ProvidedData) != CtrVal->SeedLen) {
    ASSERT (BitStreamBitLen (ProvidedData) == CtrVal->SeedLen);
    return EFI_INVALID_PARAMETER;
  }

  // 1. temp = Null.
  Status = BitStreamAlloc (0, &TempStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //  2. While (len (temp) < seedlen) do
  while (BitStreamBitLen (TempStream) < CtrVal->SeedLen) {
    // 2.1 If ctr_len < blocklen
    if (CtrVal->CtrLen < CtrVal->SeedLen) {
      // 2.1.1 inc = (rightmost (V, ctr_len) + 1) mod 2 ^ ctr_len.
      Status = BitStreamRightmost (
                 CtrIntState->Val,
                 CtrVal->CtrLen,
                 &IncStream
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      Status = BitStreamAddModulo (1, CtrVal->CtrLen, IncStream);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      // 2.1.2 V = leftmost (V, blocklen - ctr_len) || inc.
      Status = BitStreamLeftmost (
                 CtrIntState->Val,
                 CtrVal->BlockLen - CtrVal->CtrLen,
                 &CtrIntState->Val
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      Status = BitStreamConcat (
                 CtrIntState->Val,
                 IncStream,
                 &CtrIntState->Val
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      Status = BitStreamFree (&IncStream);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }
    } else {
      // (2.1) Else V = (V+1) mod 2 ^ blocklen.
      Status = BitStreamAddModulo (1, CtrVal->BlockLen, CtrIntState->Val);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }
    }

    // 2.2 output_block = Block_Encrypt (Key, V).
    Status = BlockEncrypt (DrbgHandle, &OutBlkStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }

    // 2.3 temp = temp || output_block.
    Status = BitStreamConcat (TempStream, OutBlkStream, &TempStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }

    Status = BitStreamFree (&OutBlkStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }
  } // while

  // 3. temp = leftmost (temp, seedlen).
  Status = BitStreamLeftmost (TempStream, CtrVal->SeedLen, &TempStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 4. temp = temp XoR provided_data.
  Status = BitStreamXor (TempStream, ProvidedData, &TempStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 5. Key = leftmost (temp, keylen).
  Status = BitStreamFree (&CtrIntState->Key);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  Status = BitStreamLeftmost (TempStream, CtrVal->KeyLen, &CtrIntState->Key);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 6. V = rightmost (temp, blocklen).
  Status = BitStreamFree (&CtrIntState->Val);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  Status = BitStreamRightmost (
             TempStream,
             CtrVal->BlockLen,
             &CtrIntState->Val
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    // Fall through.
  }

ExitHandler:
  if (IncStream != NULL) {
    BitStreamFree (&IncStream);
  }

  if (OutBlkStream != NULL) {
    BitStreamFree (&OutBlkStream);
  }

  if (TempStream != NULL) {
    BitStreamFree (&TempStream);
  }

  // 7. Return (Key, V).
  return Status;
}

/** Instantiate algorithm.

  CTR_DRBG_Instantiate_algorithm implementation.

  Cf. [1] s10.2.1.3.1 Instantiation When a Derivation Function is Not Used

  @param  [in]  EntropyInput      The string of bits obtained from the
                                  randomness source.
  @param  [in]  PersStrBitStream  The personalization string received from the
                                  consuming application. Note that the length
                                  of the personalization_string may be zero.
                                  Note: PersStrBitStream must be initialized,
                                  even with a NULL BitStream.
  @param  [out] DrbgHandle        The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
CtrDrbgInstantiateAlgo (
  IN  BIT_STREAM   *EntropyInput,
  IN  BIT_STREAM   *PersStrBitStream,
  OUT DRBG_HANDLE  DrbgHandle
  )
{
  EFI_STATUS             Status;
  BIT_STREAM             *LocalStream;
  BIT_STREAM             *SeedMaterial;
  CTR_INTERNAL_STATE     *CtrIntState;
  CTR_VALUE_DEFINITIONS  *CtrVal;
  UINTN                  Temp;

  if (IsBitStreamEmpty (EntropyInput)  ||
      (PersStrBitStream == NULL)      ||
      (DrbgHandle == NULL) ||
      (DrbgHandle->IntState.DrbgAlgoIntState == NULL) ||
      (DrbgHandle->DrbgVal.DrbgAlgoVal == NULL))
  {
    ASSERT (!IsBitStreamEmpty (EntropyInput));
    ASSERT (PersStrBitStream != NULL);
    ASSERT (DrbgHandle != NULL);
    ASSERT (DrbgHandle->IntState.DrbgAlgoIntState != NULL);
    ASSERT (DrbgHandle->DrbgVal.DrbgAlgoVal != NULL);
    return EFI_INVALID_PARAMETER;
  }

  LocalStream  = NULL;
  SeedMaterial = NULL;
  CtrIntState  = (CTR_INTERNAL_STATE *)DrbgHandle->IntState.DrbgAlgoIntState;
  CtrVal       = (CTR_VALUE_DEFINITIONS *)DrbgHandle->DrbgVal.DrbgAlgoVal;

  // 1. temp = len (personalization_string).
  Temp = BitStreamBitLen (PersStrBitStream);

  // 2. If (temp < seedlen), then
  // personalization_string = personalization_string || 0 ^ seedlen - temp.
  if (Temp < CtrVal->SeedLen) {
    Status = BitStreamAlloc (CtrVal->SeedLen - Temp, &LocalStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Status = BitStreamConcat (
               PersStrBitStream,
               LocalStream,
               &PersStrBitStream
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }
  }

  // 3. seed_material = entropy_input XoR personalization_string.
  Status = BitStreamXor (EntropyInput, PersStrBitStream, &SeedMaterial);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 4. Key = 0 ^ keylen.
  Status = BitStreamAlloc (CtrVal->KeyLen, &CtrIntState->Key);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 5. V = 0 ^ blocklen.
  Status = BitStreamAlloc (CtrVal->BlockLen, &CtrIntState->Val);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 6. (Key, V) = CTR_DRBG_Update (seed_material, Key, V).
  Status = CtrDrbgUpdate (SeedMaterial, DrbgHandle);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 7. reseed_counter = 1.
  CtrIntState->ReseedCounter = 1;

ExitHandler:
  if (SeedMaterial != NULL) {
    BitStreamFree (&SeedMaterial);
  }

  if (LocalStream != NULL) {
    BitStreamFree (&LocalStream);
  }

  // 8. Return (V, Key, reseed_counter).
  return Status;
}

/** Reseed algorithm

  CTR_DRBG_Reseed_algorithm implementation.

  Cf. [1] s10.2.1.4.1 Reseeding When a Derivation Function is Not Used

  @param [in]       EntropyInput  The string of bits obtained from the
                                  randomness source.
  @param [in]       AddInput      The additional input string received
                                  from the consuming application. Note
                                  that the length of the additional_input
                                  string may be zero.
  @param [in, out]  DrbgHandle    The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
CtrDrbgReseedAlgo (
  IN      BIT_STREAM   *EntropyInput,
  IN      BIT_STREAM   *AddInput,
  IN OUT  DRBG_HANDLE  DrbgHandle
  )
{
  EFI_STATUS             Status;
  UINTN                  Temp;
  BIT_STREAM             *LocalStream;
  BIT_STREAM             *SeedMaterial;
  CTR_INTERNAL_STATE     *CtrIntState;
  CTR_VALUE_DEFINITIONS  *CtrVal;

  if (IsBitStreamEmpty (EntropyInput)  ||
      (AddInput == NULL)              ||
      (DrbgHandle == NULL) ||
      (DrbgHandle->IntState.DrbgAlgoIntState == NULL) ||
      (DrbgHandle->DrbgVal.DrbgAlgoVal == NULL))
  {
    ASSERT (!IsBitStreamEmpty (EntropyInput));
    ASSERT (AddInput != NULL);
    ASSERT (DrbgHandle != NULL);
    ASSERT (DrbgHandle->IntState.DrbgAlgoIntState != NULL);
    ASSERT (DrbgHandle->DrbgVal.DrbgAlgoVal != NULL);
    return EFI_INVALID_PARAMETER;
  }

  LocalStream  = NULL;
  SeedMaterial = NULL;
  CtrIntState  = (CTR_INTERNAL_STATE *)DrbgHandle->IntState.DrbgAlgoIntState;
  CtrVal       = (CTR_VALUE_DEFINITIONS *)DrbgHandle->DrbgVal.DrbgAlgoVal;

  // 1. temp = len (additional_input).
  Temp = BitStreamBitLen (AddInput);

  // 2. If (temp < seedlen),
  // then additional_input = additional_input || 0 ^ (seedlen - temp).
  if (Temp < CtrVal->SeedLen) {
    Status = BitStreamAlloc (CtrVal->SeedLen - Temp, &LocalStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Status = BitStreamConcat (
               AddInput,
               LocalStream,
               &AddInput
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }
  }

  // 3. seed_material = entropy_input XoR additional_input.
  Status = BitStreamXor (EntropyInput, AddInput, &SeedMaterial);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 4. (Key, V) = CTR_DRBG_Update (seed_material, Key, V).
  Status = CtrDrbgUpdate (SeedMaterial, DrbgHandle);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 5. reseed_counter = 1.
  CtrIntState->ReseedCounter = 1;

ExitHandler:
  if (LocalStream != NULL) {
    BitStreamFree (&LocalStream);
  }

  if (SeedMaterial != NULL) {
    BitStreamFree (&SeedMaterial);
  }

  // 6. Return (V, Key, reseed_counter).
  return Status;
}

/** Generate algorithm.

  CTR_DRBG_Generate_algorithm implementation.

  Cf. s10.2.1.5.1 Generating Pseudorandom Bits When a Derivation Function
  is Not Used

  To reflect that 'a reseed is required before the requested pseudorandom bits
  can be generated', the EFI_NOT_READY return code is used.

  @param [in]       AddInput          The additional input string received from
                                      the consuming application. Note that the
                                      length of the additional_input string
                                      may be zero.
  @param [in]       RequestedNbBits   The number of pseudorandom bits to be
                                      returned to the generate function.
  @param [out]      RndBitStream      BitStream containing the random bits.
  @param [in, out]  DrbgHandle        The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
  @retval EFI_NOT_READY           A reseed is required before the requested
                                  pseudorandom bits can be generated.
**/
STATIC
EFI_STATUS
EFIAPI
CtrDrbgGenerateAlgo (
  IN      BIT_STREAM   *AddInput,
  IN      UINTN        RequestedNbBits,
  OUT     BIT_STREAM   **RndBitStream,
  IN OUT  DRBG_HANDLE  DrbgHandle
  )
{
  EFI_STATUS             Status;
  UINTN                  Temp;
  BIT_STREAM             *IncStream;
  BIT_STREAM             *LocalStream;
  BIT_STREAM             *OutBlkStream;
  BIT_STREAM             *TempStream;
  CTR_INTERNAL_STATE     *CtrIntState;
  CTR_VALUE_DEFINITIONS  *CtrVal;

  if ((AddInput == NULL)      ||
      (RequestedNbBits == 0)  ||
      (RndBitStream == NULL)  ||
      (*RndBitStream != NULL) ||
      (DrbgHandle == NULL) ||
      (DrbgHandle->IntState.DrbgAlgoIntState == NULL) ||
      (DrbgHandle->DrbgVal.DrbgAlgoVal == NULL))
  {
    ASSERT (AddInput != NULL);
    ASSERT (RequestedNbBits != 0);
    ASSERT (RndBitStream != NULL);
    ASSERT (*RndBitStream == NULL);
    ASSERT (DrbgHandle != NULL);
    ASSERT (DrbgHandle->IntState.DrbgAlgoIntState != NULL);
    ASSERT (DrbgHandle->DrbgVal.DrbgAlgoVal != NULL);
    return EFI_INVALID_PARAMETER;
  }

  IncStream    = NULL;
  LocalStream  = NULL;
  OutBlkStream = NULL;
  TempStream   = NULL;
  CtrIntState  = (CTR_INTERNAL_STATE *)DrbgHandle->IntState.DrbgAlgoIntState;
  CtrVal       = (CTR_VALUE_DEFINITIONS *)DrbgHandle->DrbgVal.DrbgAlgoVal;

  // 1. If reseed_counter > reseed_interval,
  // then return an indication that a reseed is required.
  if (CtrIntState->ReseedCounter > CtrVal->ReseedInterval) {
    return EFI_NOT_READY;
  }

  // 2. If (additional_input != Null), then
  if (!IsBitStreamEmpty (AddInput)) {
    // 2.1 temp = len (additional_input).
    Temp = BitStreamBitLen (AddInput);

    // 2.2 If (temp < seedlen), then
    // additional_input = additional_input || 0 seedlen - temp .
    if (Temp < CtrVal->SeedLen) {
      Status = BitStreamAlloc (CtrVal->SeedLen - Temp, &LocalStream);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      Status = BitStreamConcat (AddInput, LocalStream, &AddInput);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }
    }

    // 2.3 (Key, V) = CTR_DRBG_Update (additional_input, Key, V).
    Status = CtrDrbgUpdate (AddInput, DrbgHandle);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }
  } else {
    // (2.) Else additional_input = 0 ^ seedlen.
    Status = BitStreamFree (&AddInput); // Freeing &AddInput, this is wrong, AddInput is now a local parameter
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }

    Status = BitStreamAlloc (CtrVal->SeedLen, &AddInput);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }
  }

  // 3. temp = Null.
  Status = BitStreamAlloc (0, &TempStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 4. While (len (temp) < requested_number_of_bits) do:
  while (BitStreamBitLen (TempStream) < RequestedNbBits) {
    // 4.1 If ctr_len < blocklen
    if (CtrVal->CtrLen < CtrVal->BlockLen) {
      // 4.1.1 inc = (rightmost (V, ctr_len) + 1) mod 2 ^ ctr_len.
      Status = BitStreamRightmost (
                 CtrIntState->Val,
                 CtrVal->CtrLen,
                 &IncStream
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      Status = BitStreamAddModulo (1, CtrVal->CtrLen, IncStream);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      // 4.1.2 V = leftmost (V, blocklen-ctr_len) || inc.
      Status = BitStreamLeftmost (
                 CtrIntState->Val,
                 CtrVal->BlockLen - CtrVal->CtrLen,
                 &CtrIntState->Val
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      Status = BitStreamConcat (
                 CtrIntState->Val,
                 IncStream,
                 &CtrIntState->Val
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      Status = BitStreamFree (&IncStream);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }
    } else {
      // (4.1) Else V = (V+1) mod 2 ^ blocklen.
      Status = BitStreamAddModulo (1, CtrVal->BlockLen, CtrIntState->Val);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }
    }

    // 4.2 output_block = Block_Encrypt (Key, V).
    Status = BlockEncrypt (DrbgHandle, &OutBlkStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }

    // 4.3 temp = temp || output_block.
    Status = BitStreamConcat (TempStream, OutBlkStream, &TempStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }

    Status = BitStreamFree (&OutBlkStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }
  } // while

  // 5. returned_bits = leftmost (temp, requested_number_of_bits).
  Status = BitStreamLeftmost (TempStream, RequestedNbBits, RndBitStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 6. (Key, V) = CTR_DRBG_Update (additional_input, Key, V).
  Status = CtrDrbgUpdate (AddInput, DrbgHandle);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 7. reseed_counter = reseed_counter + 1.
  CtrIntState->ReseedCounter += 1;

ExitHandler:
  if (IncStream != NULL) {
    BitStreamFree (&IncStream);
  }

  if (LocalStream != NULL) {
    BitStreamFree (&LocalStream);
  }

  if (OutBlkStream != NULL) {
    BitStreamFree (&OutBlkStream);
  }

  if (TempStream != NULL) {
    BitStreamFree (&TempStream);
  }

  // 8. Return (SUCCESS, returned_bits, Key, V, reseed_counter).
  return Status;
}

/** Uninstantiate a DRBG instance.

  [1] s9.4 Removing a DRBG Instantiation

  @param  [in, out] DrbgHandle    The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
CtrDrbgUninstantiateFn (
  IN  OUT DRBG_HANDLE  DrbgHandle
  )
{
  EFI_STATUS          Status;
  EFI_STATUS          ReturnStatus;
  CTR_INTERNAL_STATE  *CtrIntState;

  if ((DrbgHandle == NULL) ||
      (DrbgHandle->IntState.DrbgAlgoIntState == NULL) ||
      (DrbgHandle->DrbgVal.DrbgAlgoVal == NULL))
  {
    ASSERT (DrbgHandle != NULL);
    ASSERT (DrbgHandle->IntState.DrbgAlgoIntState != NULL);
    ASSERT (DrbgHandle->DrbgVal.DrbgAlgoVal != NULL);
    return EFI_INVALID_PARAMETER;
  }

  ReturnStatus = EFI_SUCCESS;
  CtrIntState  = (CTR_INTERNAL_STATE *)DrbgHandle->IntState.DrbgAlgoIntState;

  // 1. If state_handle indicates an invalid state, then return (ERROR_FLAG).
  // 2. Erase the contents of the internal state indicated by state_handle.

  Status = BitStreamFree (&CtrIntState->Key);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    ReturnStatus = Status;
  }

  Status = BitStreamFree (&CtrIntState->Val);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    ReturnStatus = Status;
  }

  FreePool (DrbgHandle);

  // 3. Return (SUCCESS).
  return ReturnStatus;
}

/** Drbg mechanism specific instantiation steps.

  @param  [in, out] DrbgHandle     The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
CtrInitHandle (
  IN  OUT DRBG_HANDLE  DrbgHandle
  )
{
  DRBG_VALUE_DEFINITIONS  *DrbgVal;
  CTR_INTERNAL_STATE      *CtrIntState;
  CTR_VALUE_DEFINITIONS   *CtrVal;

  if (DrbgHandle == NULL) {
    ASSERT (DrbgHandle != NULL);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (
    &DrbgHandle->Algo,
    &gEfiRngAlgorithmSp80090Ctr256Guid,
    sizeof (EFI_RNG_ALGORITHM)
    );

  // Allocate CtrIntState. Fields are init to 0.
  CtrIntState = AllocateZeroPool (sizeof (CTR_INTERNAL_STATE));
  if (CtrIntState == NULL) {
    ASSERT (CtrIntState != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  DrbgHandle->IntState.DrbgAlgoIntState = CtrIntState;

  // Allocate CtrVal. Fields are init to 0.
  CtrVal = AllocateZeroPool (sizeof (CTR_VALUE_DEFINITIONS));
  if (CtrVal == NULL) {
    ASSERT (CtrVal != NULL);
    FreePool (CtrIntState);
    return EFI_OUT_OF_RESOURCES;
  }

  DrbgHandle->DrbgVal.DrbgAlgoVal = CtrVal;

  // CTR DRBG definitions, cf [1] Table 3 'Definitions for the CTR_DRBG'.
  // No derivation function is used.
  // Only 256 bits AES is allowed (Cf. [5] 'Section 37.5 Random Number
  // Generator Protocol')

  // Drbg generic values.
  DrbgVal                         = &DrbgHandle->DrbgVal;
  DrbgVal->HighestSuppSecStrength = 256;
  DrbgVal->MinLen                 = CTR_DRBG_AES_256_SEEDLEN;
  DrbgVal->MaxLen                 = CTR_DRBG_AES_256_SEEDLEN;
  DrbgVal->MaxPersStrLen          = CTR_DRBG_AES_256_SEEDLEN;
  DrbgVal->MaxAddInputLen         = CTR_DRBG_AES_256_SEEDLEN;
  DrbgVal->MaxNbBitsPerRequest    = 1 << 19;

  // Ctr specific values.
  CtrVal->BlockLen = CTR_DRBG_AES_BLOCKLEN;
  // 4 <= ctr_len <= blocklen. Choose blocklen to be faster.
  CtrVal->CtrLen             = CTR_DRBG_AES_BLOCKLEN;
  CtrVal->KeyLen             = CTR_DRBG_AES_256_KEYLEN;
  CtrVal->MinRequiredEntropy = SecStrength256bits;
  CtrVal->SeedLen            = CTR_DRBG_AES_256_SEEDLEN;
  CtrVal->ReseedInterval     = (UINT64)1 << 48;

  // CtrDrbg supports both mechanisms.
  DrbgHandle->PredResSupported = TRUE;
  DrbgHandle->ReseedSupported  = TRUE;

  // CtrDrbg specific implementations.
  DrbgHandle->DrbgGetNonce           = CtrDrbgGetNonce;
  DrbgHandle->DrbgCheckInternalState = CtrDrbgCheckInternalState;
  DrbgHandle->DrbgUpdate             = CtrDrbgUpdate;
  DrbgHandle->DrbgReseedAlgo         = CtrDrbgReseedAlgo;
  DrbgHandle->DrbgGenerateAlgo       = CtrDrbgGenerateAlgo;
  DrbgHandle->DrbgInstantiateAlgo    = CtrDrbgInstantiateAlgo;
  DrbgHandle->DrbgUninstantiateFn    = CtrDrbgUninstantiateFn;

  return EFI_SUCCESS;
}
