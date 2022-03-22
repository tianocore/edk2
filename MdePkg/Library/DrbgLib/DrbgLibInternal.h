/** @file
  Arm DRBG library internal definitions.
  Cf. [1] s9 DRBG Mechanism Functions

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
        Recommendation forKey Management:Part 1 - General.
        (https://csrc.nist.gov/publications/detail/sp/800-57-part-1/rev-5/final)
  - [5] Unified Extensible Firmware Interface (UEFI) Specification,
        Version 2.8 Errata B, May 2020
        (https://www.uefi.org/specifications)

  @par Glossary:
    - TRNG - True Random Number Generator
    - Sec  - Security
    - DRBG - Deterministic Random Bits Generator
    - CTR  - Counter
**/

#ifndef ARM_DRBG_LIB_INTERNAL_H_
#define ARM_DRBG_LIB_INTERNAL_H_

// Forward declarations of DRBG_INFO.
typedef struct DrbgInfo  DRBG_INFO;
typedef DRBG_INFO        *DRBG_HANDLE;

#include <Library/DrbgLib.h>
#include <Protocol/Rng.h>
#include "BitStream.h"

/** Security strengths for block cipher algorithms.

    Cf [2] 5.6.1.1, Table 2: Comparable security strengths of symmetric block
    cipher and asymmetric-key algorithms.

    [2]: 'Although 3TDEA is listed as providing 112 bits of security strength,
          its use has been deprecated (see SP 800-131A)'
*/
typedef enum {
  SecStrengthMin     = 128,               ///< Min Security strength of 128 bits.
  SecStrength128bits = SecStrengthMin,    ///< Security strength of 128 bits.
  SecStrength196bits = 196,               ///< Security strength of 196 bits.
  SecStrength256bits = 256,               ///< Security strength of 256 bits.
  SecStrengthMax     = SecStrength256bits ///< Maximum Security strength.
} SECURITY_STRENGTH;

/** Get a nonce.

  @param  [in, out] DrbgHandle    The Drbg handle.
  @param  [out]     NonceStream   Stream containing the Nonce.

  @retval EFI_SUCCESS             Success.
**/
typedef EFI_STATUS EFIAPI (*DRBG_GET_NONCE) (
  IN  OUT DRBG_HANDLE  DrbgHandle,
  OUT     BIT_STREAM   *NonceStream
  );

/** Check the internal state of the Drbg handle.

  @param  [in] DrbgHandle    The Drbg handle.

  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_SUCCESS             Success.
**/
typedef EFI_STATUS EFIAPI (*DRBG_CHECK_INTERNAL_STATE) (
  IN  DRBG_HANDLE  DrbgHandle
  );

/** GetEntropyInput implementation (no conditionning function).

  Cf. [3] 10.3.3 Get_entropy_input Constructions for Accessing Entropy Sources

  @param [in]   DrbgHandle        The Drbg hanble.
  @param [in]   MinEntropy        Minimum entropy.
  @param [out]  EntropyBitsStream Stream containing the generated entropy.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
typedef EFI_STATUS EFIAPI (*DRBG_GET_ENTROPY_INTPUT) (
  IN  DRBG_HANDLE  DrbgHandle,
  IN  UINTN        MinEntropy,
  OUT BIT_STREAM   **EntropyBitsStream
  );

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
typedef EFI_STATUS EFIAPI (*DRBG_UPDATE) (
  IN      BIT_STREAM   *ProvidedData,
  IN OUT  DRBG_HANDLE  DrbgHandle
  );

/** Reseed algorithm

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
typedef EFI_STATUS EFIAPI (*DRBG_RESEED) (
  IN      BIT_STREAM   *EntropyInput,
  IN      BIT_STREAM   *AddInput,
  IN OUT  DRBG_HANDLE  DrbgHandle
  );

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
typedef EFI_STATUS EFIAPI (*DRBG_INSTANTIATE) (
  IN  BIT_STREAM   *EntropyInput,
  IN  BIT_STREAM   *PersStrBitStream,
  OUT DRBG_HANDLE  DrbgHandle
  );

/** Generate algorithm.

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
typedef EFI_STATUS EFIAPI (*DRBG_GENERATE_ALGORITHM) (
  IN      BIT_STREAM   *AddInput,
  IN      UINTN        RequestedNbBits,
  OUT     BIT_STREAM   **RndBitStream,
  IN OUT  DRBG_HANDLE  DrbgHandle
  );

/** Uninstantiate a DRBG instance.

  @param  [in, out] DrbgHandle    The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
typedef EFI_STATUS EFIAPI (*DRBG_UNINSTANTIATE_FUNCTION) (
  IN  OUT DRBG_HANDLE  DrbgHandle
  );

/** Drbg algo specific value definitions.
*/
typedef VOID *DRBG_ALGO_VALUE_DEFINITIONS;

/** Drbg algo internal state values.
*/
typedef VOID *DRBG_ALGO_INTERNAL_STATE;

/** Drbg values definitions.

  Values are specific to a Drbg mechanism.
  Cf. [1]:
  Table 2: Definitions for Hash-Based DRBG Mechanisms
  Table 3: Definitions for the CTR_DRBG
*/
typedef struct {
  /// Supported security strengths.
  // Not applicable.
  /// Highest supported security strength.
  SECURITY_STRENGTH              HighestSuppSecStrength;
  /// Minimum entropy input length.
  UINTN                          MinLen;
  /// Maximum entropy input length.
  UINTN                          MaxLen;
  /// Maximum personalization string length.
  UINTN                          MaxPersStrLen;
  /// Maximum additional_input length.
  UINTN                          MaxAddInputLen;
  /// Maximum number of bits per request.
  UINTN                          MaxNbBitsPerRequest;

  /// Drbg algo specific value definitions.
  DRBG_ALGO_VALUE_DEFINITIONS    DrbgAlgoVal;
} DRBG_VALUE_DEFINITIONS;

/** Internal state.

  For a Drbg mechanism, values are specific to a Drbg instance.
*/
typedef struct {
  // Administrative information.
  /// Security strength
  SECURITY_STRENGTH           SecStrength;
  /// Prediction resistance flag
  BOOLEAN                     PredResFlag;

  /// Drbg algo specific internal state values.
  DRBG_ALGO_INTERNAL_STATE    DrbgAlgoIntState;
} DRBG_INTERNAL_STATE;

/** Drbg info structure.

  This structure is a Drbg instance. It contains information specific to the
  Drbg mechanism chosen (e.g. the block size for Ctr mechanism) and instance
  specific information (i.e. the internal state of the instance).
*/
struct DrbgInfo {
  /// Drbg mechanism used.
  DRBG_MECHANISM                 DrbgMechanism;
  /// GUID of the algorithm used.
  EFI_RNG_ALGORITHM              Algo;

  /// Values specifics to the Drbg mechanism used.
  DRBG_VALUE_DEFINITIONS         DrbgVal;

  /// Internal state of the Drbg instance.
  DRBG_INTERNAL_STATE            IntState;

  /// Prediction resistance is supported for this Drbg instance.
  BOOLEAN                        PredResSupported;
  /// Reseeding is supported for this Drbg instance.
  BOOLEAN                        ReseedSupported;

  /// Callback to get a nonce.
  DRBG_GET_NONCE                 DrbgGetNonce;
  /// Callback to check the internal state.
  DRBG_CHECK_INTERNAL_STATE      DrbgCheckInternalState;
  /// Callback to get some entropy.
  DRBG_GET_ENTROPY_INTPUT        DrbgGetEntropyInput;

  /// Callback to update the instance
  /// (according to the mechanism used).
  DRBG_UPDATE                    DrbgUpdate;
  /// Callback to reseed the instance
  /// (according to the mechanism used).
  DRBG_RESEED                    DrbgReseedAlgo;
  /// Callback to instantiate the Drbg instance
  /// (according to the mechanism used).
  DRBG_INSTANTIATE               DrbgInstantiateAlgo;
  /// Callback to generate random bits the instance
  /// (according to the mechanism used).
  DRBG_GENERATE_ALGORITHM        DrbgGenerateAlgo;
  /// Callback to uninstantiate the Drbg instance
  /// (according to the mechanism used).
  DRBG_UNINSTANTIATE_FUNCTION    DrbgUninstantiateFn;
};

/** Drbg mechanism specific instantiation steps.

  @param  [in, out] DrbgHandle     The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
CtrInitHandle (
  IN  OUT DRBG_HANDLE  DrbgHandle
  );

#endif // ARM_DRBG_LIB_INTERNAL_H_
