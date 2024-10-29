/** @file
  Arm CCA Token parser interface definitions.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
**/

#pragma once

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Uefi/UefiBaseType.h>

#include <Library/ArmCcaRsiLib.h>

#include <qcbor/qcbor_decode.h>
#include <qcbor/qcbor_spiffy_decode.h>

#define CLAIM_COUNT_COSE_SIGN1_WRAPPER  3
#define CLAIM_COUNT_PLATFORM_TOKEN      8
#define CLAIM_COUNT_REALM_TOKEN         7
#define CLAIM_COUNT_REMS                ARM_CCA_MAX_REM_INDEX
#define CLAIM_COUNT_SW_COMPONENT        5

#define MAX_SW_COMPONENT_COUNT  16

#define INDEX_COSE_SIGN1_WRAPPER_HEADER     0
#define INDEX_COSE_SIGN1_WRAPPER_PAYLOAD    1
#define INDEX_COSE_SIGN1_WRAPPER_SIGNATURE  2

/** An enum representing the data type of the Claims
**/
typedef enum ClaimDataType {
  ClaimDataInt64,       ///< INT64 data
  ClaimDataBool,        ///< BOOLEAN data
  ClaimDataByteString,  ///< Byte Stream data
  ClaimDataText,        ///< Test data
  ClaimDataMax
} CLAIM_DATA_TYPE;

/** A structure representing the configuraiton for a Claim.
**/
typedef struct ClaimConfig {
  /// The claim label.
  INT64              Label;
  /// A test description for the claim.
  CONST CHAR8        *Title;
  /// A property indicating if the claim is optional.
  BOOLEAN            Optional;
  /// The datatype of the claim.
  CLAIM_DATA_TYPE    Type;
} CLAIM_CONFIG;

/** A structure representing the claim data.
**/
typedef struct ClaimData {
  /// an unnamed union representing the claim data.
  union {
    /// A 64-bit interger value.
    INT64                    IntData;
    /// A boolean value.
    BOOLEAN                  BoolData;
    /// A packed buffer structure with pointer to the data and length.
    struct q_useful_buf_c    BufferData;
  };

  /// A property indicating if the claim data is populated.
  BOOLEAN    Present;
} CLAIM_DATA;

/** A structure representing an Arm CCA Claim.
**/
typedef struct Claim {
  /// The claim configuration.
  CLAIM_CONFIG    Config;
  /// The parsed claim data.
  CLAIM_DATA      Data;
} CLAIM;

/** A strucutre representing a swoftware component claim.
**/
typedef struct SwComponentClaims {
  /// An array of software component claims.
  CLAIM    Claims[CLAIM_COUNT_SW_COMPONENT];
} SW_COMPONENT_CLAIMS;

/** A structure containing claims that form an Arm CCA attestation token. The
  Arm CCA attestation token is packed as specified by the RMM specification.
**/
typedef struct AttestationClaims {
  // Platform attestation token.
  /// The COSE1 Sign wrapper part of the platform attestation token.
  CLAIM                  PlatCoseSign1Wrapper[CLAIM_COUNT_COSE_SIGN1_WRAPPER];
  /// The platform token claims.
  CLAIM                  PlatTokenClaims[CLAIM_COUNT_PLATFORM_TOKEN];
  /// The software component claims.
  SW_COMPONENT_CLAIMS    SwComponentClaims[MAX_SW_COMPONENT_COUNT];

  // The Realm delegated token.
  /// The COSE1 Sign wrapper part of the Realm attestation token.
  CLAIM                  RealmCoseSign1Wrapper[CLAIM_COUNT_COSE_SIGN1_WRAPPER];
  /// The Realm token claims.
  CLAIM                  RealmTokenClaims[CLAIM_COUNT_REALM_TOKEN];
  /// The Realm extensible measurement claims.
  CLAIM                  RealmMeasurementClaims[CLAIM_COUNT_REMS];
} ATTESTATION_CLAIMS;

/** A function to parse the Arm CCA attestation token claims.

  @param[in]  Token     Pointer to the Arm CCA attestation token buffer.
  @param[in]  TokenLen  Length of the Arm CCA attestation token buffer.
  @param[out] Claims    Pointer to the Arm CCA attestation claims data
                        structure.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
ParseAttestationToken (
  IN CONST UINT8          *Token,
  IN UINTN                TokenLen,
  OUT ATTESTATION_CLAIMS  *AttestClaims
  );

/** Get the Realm Initial Measurement (RIM) from the
    Arm CCA attestation token claims.

  @param[in]  AttestClaims  Pointer to the Arm CCA attestation token claims.
  @param[out] RimData       Pointer to the RIM data.
  @param[out] RimDataLen    Length of the RIM data.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
AttestationTokenClaimGetRim (
  IN  ATTESTATION_CLAIMS  *AttestClaims,
  OUT UINT8               **RimData,
  OUT UINTN               *RimDataLen
  );

/** Get the Realm Extensible Measurement (REM) from the
    Arm CCA attestation token claims.

  @param[in]  AttestClaims  Pointer to the Arm CCA attestation token claims.
  @param[in]  RemIndex      A zero based index of the REM.
  @param[out] RimData       Pointer to the REM data.
  @param[out] RimDataLen    Length of the REM data.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
AttestationTokenClaimGetRem (
  IN  ATTESTATION_CLAIMS  *AttestClaims,
  IN  UINT8               RemIndex,
  OUT UINT8               **RemData,
  OUT UINTN               *RemDataLen
  );

/** Get the Realm Personalisation Value(RPV) from the
    Arm CCA attestation token claims.

  @param[in]  AttestClaims  Pointer to the Arm CCA attestation token claims.
  @param[out] RpvData       Pointer to the RPV data.
  @param[out] RpvDataLen    Length of the RPV data.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
AttestationTokenClaimGetRpv (
  IN  ATTESTATION_CLAIMS  *AttestClaims,
  OUT UINT8               **RpvData,
  OUT UINTN               *RpvDataLen
  );

/** Get the Realm Challenge from the Arm CCA attestation token claims.

  @param[in]  AttestClaims  Pointer to the Arm CCA attestation token claims.
  @param[out] RpvData       Pointer to the Realm Challenge data.
  @param[out] RpvDataLen    Length of the Realm Challenge data.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
AttestationTokenClaimGetChallenge (
  IN  ATTESTATION_CLAIMS  *AttestClaims,
  OUT UINT8               **Data,
  OUT UINTN               *DataLen
  );
