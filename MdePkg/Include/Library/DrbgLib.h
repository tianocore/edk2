/** @file
  DRBG library.

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

  @par Glossary:
    - TRNG - True Random Number Generator
    - Sec  - Security
    - DRBG - Deterministic Random Bits Generator
    - CTR  - Counter
**/

#ifndef DRBG_LIB_H_
#define DRBG_LIB_H_

/** Drbg Mechanisms.
*/
typedef enum {
  DrbgMechansimHash = 0,        ///< Hash (not supported yet)
  DrbgMechansimHmac,            ///< HMAC (not supported yet)
  DrbgMechansimCtr,             ///< CTR
  DrbgMechansimMax              ///< Maximum value.
} DRBG_MECHANISM;

/** Drbg Entropy sources.
*/
typedef enum {
  /// Cf. [3] s10.3.3.1
  /// Construction When a Conditioning Function is not Used
  DrbgEntropyNoCondFn = 0,
  /// Cf. [3] s10.3.3.2 (no supported yet)
  /// Construction When a Vetted Conditioning Function is Used
  /// and Full Entropy is Not Required)
  DrbgEntropyNoFullEntropy,
  /// Cf. [3] s10.3.3.3 (no supported yet)
  /// Construction When a Vetted Conditioning Function is Used
  /// to Obtain Full Entropy Bitstrings
  DrbgEntropyFullEntropy,
  /// Maximum value.
  DrbgEntropyMax
} DRBG_ENTROPY_SRC;

/** Reseed a DRBG instance.

  Implementation of Reseed_function.
  Cf. [1] s9.2 'Reseeding a DRBG Instantiation'

  @param  [in] PredResRequest   Indicates whether prediction resistance
                                is to be provided during the request.
                                Might not be supported by all Drbgs.
  @param  [in] AddInput         An optional additional input.
                                Might not be supported by all Drbgs.
  @param  [in] AddInputLen      Additional input length (in bits).
                                Might not be supported by all Drbgs.
  @param  [in, out] Handle  The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
DrbgReseedFn (
  IN        BOOLEAN  PredResRequest,
  IN  CONST CHAR8    *AddInput,
  IN        UINTN    AddInputLen,
  IN  OUT   VOID     *Handle
  );

/** Create a Drbg instance.

  Implementation of Instantiate_function.
  Cf. [1] s9.1 Instantiating a DRBG

  @param  [in] DrbgMechanism    DRBG mechanism chosen.
  @param  [in] DrbgEntropySrc   Entropy source chosen.
  @param  [in] ReqSecStrength   Requested security strength (in bits).
                                The security strenght granted can be different.
  @param  [in] PredRes          Prediction resistance flag.
                                If relevant, instantiate a DRBG that supports
                                prediction resistance.
                                Might not be supported by all Drbgs.
  @param  [in] PersStr          Personnalization string.
                                Might not be supported by all Drbgs.
  @param  [in] PersStrLen       Personnalization string length (in bits).
                                Might not be supported by all Drbgs.
  @param  [out] HandlePtr   Pointer containting the created Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
DrbgInstantiateFn (
  IN        DRBG_MECHANISM    DrbgMechanism,
  IN        DRBG_ENTROPY_SRC  DrbgEntropySrc,
  IN        UINTN             ReqSecStrength,
  IN        BOOLEAN           PredRes,
  IN  CONST CHAR8             *PersStr,
  IN        UINTN             PersStrLen,
  OUT       VOID              **HandlePtr
  );

/** Generate a random number.

  Implementation of Generate_function.
  Cf. [1] s9.3.1 The Generate Function

  @param  [in] ReqSecStrength   Requested security strength (in bits).
                                If the DrbgHandle cannot satisfy the request,
                                an error is returned.
  @param  [in] PredResReq       Request prediction resistance.
                                If the DrbgHandle cannot satisfy the request,
                                an error is returned.
  @param  [in] AddInput         Additional input.
                                Might not be supported by all Drbgs.
  @param  [in] AddInputLen      Additional input length (in bits).
                                Might not be supported by all Drbgs.
  @param  [in] ReqNbBits        Number of random bits requested.
  @param  [in, out] OutBuffer   If success, contains the random bits.
                                The buffer must be at least ReqNbBits bits
                                long.
  @param  [in, out] Handle  The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
DrbgGenerateFn (
  IN        UINTN    ReqSecStrength,
  IN        BOOLEAN  PredResReq,
  IN  CONST CHAR8    *AddInput,
  IN        UINTN    AddInputLen,
  IN        UINTN    ReqNbBits,
  IN  OUT   UINT8    *OutBuffer,
  IN  OUT   VOID     *Handle
  );

/** Remove a DRBG instance.

  Implementation of Uninstantiate_function.
  Cf. [1] s9.4 Removing a DRBG Instantiation

  @param  [in, out] Handle    The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
DrbgUninstantiateFn (
  IN  OUT VOID  *Handle
  );

#endif // DRBG_LIB_H_
