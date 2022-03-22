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

#ifndef CTR_DRBG_LIB_H_
#define CTR_DRBG_LIB_H_

/* Blocklen when using CTR DRBG AES-128, AES-192 and
  AES-256 algorithm.

  Cf. [6] Figure 4. 'Key-Block-Round Combinations'
  Cf. [1] Table 3 'Definitions for the CTR_DRBG'

  Output block length is also called 'outlen'.
*/
#define CTR_DRBG_AES_BLOCKLEN  128

/* Key length when using CTR DRBG AES-256 algorithm.

  Cf. [1] Table 3 'Definitions for the CTR_DRBG'
  Cf. [5] s37.5 'Random Number Generator Protocol':
  'Security level must be at least 256 bits'
*/
#define CTR_DRBG_AES_256_KEYLEN  256

/** Seed length when using CTR DRBG AES-256 algorithm.

  Cf. [1] Table 3 'Definitions for the CTR_DRBG'
  Seed length (seedlen = outlen + keylen)
*/
#define CTR_DRBG_AES_256_SEEDLEN  (CTR_DRBG_AES_BLOCKLEN +   \
                                    CTR_DRBG_AES_256_KEYLEN)

/** Ctr specific internal state.

  Some fields defined at [1] s10.2.1.1 'CTR_DRBG Internal State' might be
  in the DRBG_INTERNAL_STATE structure.
*/
typedef struct {
  // Working state.
  /// Value
  BIT_STREAM    *Val;
  /// Key
  BIT_STREAM    *Key;
  /// Counter: Number of requests for pseudorandom bits
  ///          since instantiation or reseeding
  UINTN         ReseedCounter;
} CTR_INTERNAL_STATE;

/** Ctr specific values.

  Cf. [1] Table 3: Definitions for the CTR_DRBG
  Some fields might be in the DRBG_VALUE_DEFINITIONS structure.
*/
typedef struct {
  /// Input and output block length
  UINTN     BlockLen;
  /// Counter field length.
  UINTN     CtrLen;
  // Key length.
  UINTN     KeyLen;
  /// Required minimum entropy for instantiate and reseed.
  UINTN     MinRequiredEntropy;
  /// Seed length.
  UINTN     SeedLen;
  /// Maximum number of requests between two reseeds.
  UINT64    ReseedInterval;
} CTR_VALUE_DEFINITIONS;

#endif // CTR_DRBG_LIB_H_
