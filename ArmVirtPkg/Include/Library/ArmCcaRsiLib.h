/** @file
  Library that implements the Arm CCA Realm Service Interface calls.

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version A-bet0
     (https://developer.arm.com/documentation/den0137/)
**/

#ifndef ARM_CCA_RSI_LIB_
#define ARM_CCA_RSI_LIB_

#include <Base.h>

/**
  A macro defining the size of a Realm Granule.
  See Section A2.2, RMM Specification, version A-bet0
  DNBXXX A Granule is a unit of physical memory whose size is 4KB.
*/
#define REALM_GRANULE_SIZE  SIZE_4KB

/**
  A macro defining the mask for the RSI RIPAS type.
  See Section B4.4.5 RsiRipas type, RMM Specification, version A-bet0.
*/
#define RIPAS_TYPE_MASK  0xFF

/* Maximum attestation token size
  RBXKKY The size of an attestation token is no larger than 4KB.
*/
#define MAX_ATTESTATION_TOKEN_SIZE  SIZE_4KB

/* Maximum challenge data size in bits.
*/
#define MAX_CHALLENGE_DATA_SIZE_BITS  512

/* Minimum recommended challenge data size in bits.
*/
#define MIN_CHALLENGE_DATA_SIZE_BITS  256

/** An enum describing the RSI RIPAS.
   See Section A5.2.2 Realm IPA state, RMM Specification, version A-bet0
*/
typedef enum Ripas {
  RipasEmpty,      ///< Unused IPA location.
  RipasRam,        ///< Private code or data owned by the Realm.
  RipasMax         ///< A valid RIPAS type value is less than RipasMax.
} RIPAS;

/** A structure describing the Realm Configuration.
  See Section B4.4.4 RsiRealmConfig type, RMM Specification, version A-bet0
  The width of the RsiRealmConfig structure is 4096 (0x1000) bytes.
*/
typedef struct RealmConfig {
  // Width of IPA in bits.
  UINT64    IpaWidth;
  // Unused bits of the RsiRealmConfig structure should be zero.
  UINT8     Reserved[SIZE_4KB - sizeof (UINT64)];
} REALM_CONFIG;

/**
  Retrieve an attestation token from the RMM.

  @param [in]       ChallengeData         Pointer to the challenge data to be
                                          included in the attestation token.
  @param [in]       ChallengeDataSizeBits Size of the challenge data in bits.
  @param [out]      TokenBuffer           Pointer to a buffer to store the
                                          retrieved attestation token.
  @param [in, out]  TokenBufferSize       Size of the token buffer on input and
                                          number of bytes stored in token buffer
                                          on return.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_ABORTED            The operation was aborted as the state
                                    of the Realm or REC does not match the
                                    state expected by the command.
  @retval RETURN_NOT_READY          The operation requested by the command
                                    is not complete.
**/
RETURN_STATUS
EFIAPI
RsiGetAttestationToken (
  IN      CONST UINT8   *CONST  ChallengeData,
  IN            UINT64          ChallengeDataSizeBits,
  OUT           UINT8   *CONST  TokenBuffer,
  IN OUT        UINT64  *CONST  TokenBufferSize
  );

/**
  Returns the IPA state for the page pointed by the address.

  @param [in]   Address     Address to retrive IPA state.
  @param [out]  State       The RIPAS state for the address specified.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
RsiGetIpaState (
  IN   UINT64  *Address,
  OUT  RIPAS   *State
  );

/**
  Sets the IPA state for the pages pointed by the memory range.

  @param [in]   Address     Address to the start of the memory range.
  @param [in]   Size        Length of the memory range.
  @param [in]   State       The RIPAS state to be configured.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
RsiSetIpaState (
  IN  UINT64  *Address,
  IN  UINT64  Size,
  IN  RIPAS   State
  );

/**
  Read the Realm Configuration.

  @param [out]  Config     Pointer to the address of the buffer to retrieve
                           the Realm configuration.

  Note: The buffer to retrieve the Realm configuration must be aligned to the
        Realm granule size.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
RsiGetRealmConfig (
  IN  REALM_CONFIG  *Config
  );

/**
   Get the version of the RSI implementation.

  @param [out] Major  The major version of the RSI implementation.
  @param [out] Minor  The minor version of the RSI implementation.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
RsiGetVersion (
  OUT UINT16 *CONST  Major,
  OUT UINT16 *CONST  Minor
  );

#endif // ARM_CCA_RSI_LIB_