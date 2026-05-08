/** @file
  Arm CCA Attestation Token parser.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
**/

#include <stdio.h>

#include "ArmCcaTokenDefs.h"
#include "Include/ArmCcaTokenParser.h"
#include "Include/BootSyncDebug.h"

/** A helper macro to populate the claims.
**/
#define ADD_CLAIM(Optional, Type, Label, TitleStr) \
          { {Label, TitleStr, Optional, Type}, }

/** A helper macro to populate the COSE_Sign1 wrapper.
**/
#define ADD_COSE_SIGN1_WRAPPER()                                       \
  {                                                                    \
    ADD_CLAIM (FALSE, ClaimDataByteString, 0, "COSE_Sign1: Header"),   \
    ADD_CLAIM (FALSE, ClaimDataByteString, 0, "COSE_Sign1: Payload"),  \
    ADD_CLAIM (FALSE, ClaimDataByteString, 0, "COSE_Sign1: Signature") \
  }

/** A helper macro to populate the Software Component Claims.
**/
#define ADD_SW_COMPONENT_CLAIM()                                       \
  {                                                                    \
    {                                                                  \
      ADD_CLAIM (TRUE, ClaimDataText,                                  \
        CCA_PLAT_SW_COMPONENT_TYPE,                                    \
        "cca-platform-sw-component: component type"),                  \
      ADD_CLAIM (TRUE, ClaimDataText,                                  \
        CCA_PLAT_SW_COMPONENT_HASH_ALGORITHM_ID,                       \
        "cca-platform-sw-component: hash algorithm identifier"),       \
      ADD_CLAIM (FALSE,  ClaimDataByteString,                          \
        CCA_PLAT_SW_COMPONENT_MEASUREMENT_VALUE,                       \
        "cca-platform-sw-component: measurement value"),               \
      ADD_CLAIM (TRUE, ClaimDataText,                                  \
        CCA_PLAT_SW_COMPONENT_VERSION,                                 \
        "cca-platform-sw-component: version"),                         \
      ADD_CLAIM (FALSE,  ClaimDataByteString,                          \
        CCA_PLAT_SW_COMPONENT_SIGNER_ID,                               \
        "cca-platform-sw-component: signer id")                        \
    }                                                                  \
  }

/** Macros defining the indices for the Platform token claims.
 */
#define PLAT_TOKEN_CLAIM_IDX_CHALLENGE             0
#define PLAT_TOKEN_CLAIM_IDX_VERIFICATION_SERVICE  1
#define PLAT_TOKEN_CLAIM_IDX_PROFILE               2
#define PLAT_TOKEN_CLAIM_IDX_INSTANCE_ID           3
#define PLAT_TOKEN_CLAIM_IDX_IMPLEMENTATION_ID     4
#define PLAT_TOKEN_CLAIM_IDX_LIFECYCLE             5
#define PLAT_TOKEN_CLAIM_IDX_CONFIG                6
#define PLAT_TOKEN_CLAIM_IDX_HASH_ALGO_ID          7

/** Macros defining the indices for the Realm token claims.
 */
#define REALM_TOKEN_CLAIM_IDX_CHALLENGE            0
#define REALM_TOKEN_CLAIM_IDX_RPV                  1
#define REALM_TOKEN_CLAIM_IDX_PUBKEY               2
#define REALM_TOKEN_CLAIM_IDX_HASH_ALGO_ID         3
#define REALM_TOKEN_CLAIM_IDX_PUBKEY_HASH_ALGO_ID  4
#define REALM_TOKEN_CLAIM_IDX_PROFILE              5
#define REALM_TOKEN_CLAIM_IDX_RIM                  6

/** A template definition for the Arm CCA sttestation claims.
**/
STATIC ATTESTATION_CLAIMS  AttClaimsTemplate = {
  // plat_cose_sign1_wrapper
  ADD_COSE_SIGN1_WRAPPER (),

  /* plat_token_claims
    cca-platform-claims = (cca-platform-claim-map)
    cca-platform-claim-map = {
      cca-platform-profile
      cca-platform-challenge
      cca-platform-implementation-id
      cca-platform-instance-id
      cca-platform-config
      cca-platform-lifecycle
      cca-platform-sw-components
      ? cca-platform-verification-service
      cca-platform-hash-algo-id
    }
  */
  {
    ADD_CLAIM (FALSE,       ClaimDataByteString,  CCA_PLAT_CHALLENGE_LABEL,              "cca-platform-challenge"),
    ADD_CLAIM (TRUE,        ClaimDataText,        CCA_PLAT_VERIFICATION_SERVICE_LABEL,   "cca-platform-verification-service"),
    ADD_CLAIM (FALSE,       ClaimDataText,        CCA_PLAT_PROFILE_LABEL,                "cca-platform-profile"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  CCA_PLAT_INSTANCE_ID_LABEL,            "cca-platform-instance-id"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  CCA_PLAT_IMPLEMENTATION_ID_LABEL,      "cca-platform-implementation-id"),
    ADD_CLAIM (FALSE,       ClaimDataInt64,       CCA_PLAT_LIFECYCLE_LABEL,              "cca-platform-lifecycle"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  CCA_PLAT_CONFIG_LABEL,                 "cca-platform-config"),
    ADD_CLAIM (FALSE,       ClaimDataText,        CCA_PLAT_HASH_ALGO_ID_LABEL,           "cca-platform-hash-algo-id")
  },

  // sw_component_claims
  {
    // Component 0 - 15
    ADD_SW_COMPONENT_CLAIM (), // Claim 0
    ADD_SW_COMPONENT_CLAIM (), // Claim 1
    ADD_SW_COMPONENT_CLAIM (), // Claim 2
    ADD_SW_COMPONENT_CLAIM (), // Claim 3
    ADD_SW_COMPONENT_CLAIM (), // Claim 4
    ADD_SW_COMPONENT_CLAIM (), // Claim 5
    ADD_SW_COMPONENT_CLAIM (), // Claim 6
    ADD_SW_COMPONENT_CLAIM (), // Claim 7
    ADD_SW_COMPONENT_CLAIM (), // Claim 8
    ADD_SW_COMPONENT_CLAIM (), // Claim 9
    ADD_SW_COMPONENT_CLAIM (), // Claim 10
    ADD_SW_COMPONENT_CLAIM (), // Claim 11
    ADD_SW_COMPONENT_CLAIM (), // Claim 12
    ADD_SW_COMPONENT_CLAIM (), // Claim 13
    ADD_SW_COMPONENT_CLAIM (), // Claim 14
    ADD_SW_COMPONENT_CLAIM ()  // Claim 15
  },

  // Realm COSE Sign1 wrapper
  ADD_COSE_SIGN1_WRAPPER (),

  /* realm_token_claims
    cca-realm-claims = (cca-realm-claim-map)
      cca-realm-claim-map = {
      cca-realm-challenge
      ? cca-realm-profile
      cca-realm-personalization-value
      cca-realm-initial-measurement
      cca-realm-extensible-measurements
      cca-realm-hash-algo-id
      cca-realm-public-key
      cca-realm-public-key-hash-algo-id
    }
  */
  {
    ADD_CLAIM (FALSE,       ClaimDataByteString,  CCA_REALM_CHALLENGE_LABEL,             "cca-realm-challenge"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  CCA_REALM_PERSONALIZATION_VALUE_LABEL, "cca-realm-personalization-value"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  CCA_REALM_PUBKEY_LABEL,                "cca-realm-public-key"),
    ADD_CLAIM (FALSE,       ClaimDataText,        CCA_REALM_HASH_ALGO_ID_LABEL,          "cca-realm-hash-algo-id"),
    ADD_CLAIM (FALSE,       ClaimDataText,        CCA_REALM_PUBKEY_HASH_ALGO_ID_LABEL,   "cca-realm-public-key-hash-algo-id"),
    ADD_CLAIM (TRUE,        ClaimDataText,        CCA_REALM_PROFILE_LABEL,               "cca-realm-profile"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  CCA_REALM_INITIAL_MEASUREMENT_LABEL,   "cca-realm-initial-measurement")
  },

  /* realm_measurement_claims
    cca-realm-measurement-type = bytes .size 32 / bytes .size 48 / bytes .size 64
    cca-realm-extensible-measurements-label = 44239
    cca-realm-extensible-measurements = (
      cca-realm-extensible-measurements-label => [ 4*4 cca-realm-measurement-type ]
      )
  */
  {
    ADD_CLAIM (FALSE,       ClaimDataByteString,  0,                                     "cca-realm-extensible-measurements-0"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  1,                                     "cca-realm-extensible-measurements-1"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  2,                                     "cca-realm-extensible-measurements-2"),
    ADD_CLAIM (FALSE,       ClaimDataByteString,  3,                                     "cca-realm-extensible-measurements-3")
  }
};

/** An helper funciton to convert the Qcbor error codes to EFI Status codes.

  @param[in]  ErrVal        The Qcbor error code.

  @retval EFI_NOT_FOUND     The Qcbor label was not found.
  @retval EFI_END_OF_FILE   No more items found.
  @retval EFI_ABORTED       The parsing failed.
  @retval EFI_SUCCESS       Success.
**/
STATIC
EFI_STATUS
QcborErrorToEfiStatus (
  QCBORError  ErrVal
  )
{
  switch (ErrVal) {
    case QCBOR_SUCCESS:
      return EFI_SUCCESS;
    case QCBOR_ERR_LABEL_NOT_FOUND:
      return EFI_NOT_FOUND;
    case QCBOR_ERR_NO_MORE_ITEMS:
      return EFI_END_OF_FILE;
    default:
      return EFI_ABORTED;
  }

  return EFI_ABORTED;
}

/** A helper function to return the status of the decoder.

  @param[in]  Context       Pointer to the Qcbor Decode Context.
  @param[in]  ResetError    Option to specify if error code must be reset
                            in the Qcbor Decode Context.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The Qcbor label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
GetDecodeStatus (
  IN QCBORDecodeContext  *Context,
  IN BOOLEAN             ResetError
  )
{
  QCBORError  ErrVal;

  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ResetError) {
    ErrVal = QCBORDecode_GetAndResetError (Context);
  } else {
    ErrVal = QCBORDecode_GetError (Context);
  }

  return QcborErrorToEfiStatus (ErrVal);
}

/** A helper function to parse claim with the an Int64 datatype.

  @param[in]      Context       Pointer to the Qcbor Decode Context.
  @param[in, out] Claim         Pointer to the claim. The claim config is
                                used as input and the claim data is populated
                                on output.
  @param[in]      Map           An option to specify if the parsed claim is
                                in a map.

**/
STATIC
VOID
ParseClaimDataInt64 (
  IN  QCBORDecodeContext  *Context,
  IN OUT CLAIM            *Claim,
  IN BOOLEAN              Map
  )
{
  if ((Context == NULL) || (Claim == NULL)) {
    return;
  }

  if (Map) {
    QCBORDecode_GetInt64InMapN (
      Context,
      Claim->Config.Label,
      (int64_t *)&(Claim->Data.IntData)
      );
  } else {
    QCBORDecode_GetInt64 (Context, (int64_t *)&(Claim->Data.IntData));
  }
}

/** A helper function to parse claim with the a boolean datatype.

  @param[in]      Context       Pointer to the Qcbor Decode Context.
  @param[in, out] Claim         Pointer to the claim. The claim config is
                                used as input and the claim data is populated
                                on output.
  @param[in]      Map           An option to specify if the parsed claim is
                                in a map.

**/
STATIC
VOID
ParseClaimDataBool (
  IN  QCBORDecodeContext  *Context,
  IN OUT CLAIM            *Claim,
  IN BOOLEAN              Map
  )
{
  if ((Context == NULL) || (Claim == NULL)) {
    return;
  }

  if (Map) {
    QCBORDecode_GetBoolInMapN (
      Context,
      Claim->Config.Label,
      (bool *)&(Claim->Data.BoolData)
      );
  } else {
    QCBORDecode_GetBool (Context, (bool *)&(Claim->Data.BoolData));
  }
}

/** A helper function to parse claim with the a byte string datatype.

  @param[in]      Context       Pointer to the Qcbor Decode Context.
  @param[in, out] Claim         Pointer to the claim. The claim config is
                                used as input and the claim data is populated
                                on output.
  @param[in]      Map           An option to specify if the parsed claim is
                                in a map.

**/
STATIC
VOID
ParseClaimDataByteString (
  IN  QCBORDecodeContext  *Context,
  IN OUT CLAIM            *Claim,
  IN BOOLEAN              Map
  )
{
  if ((Context == NULL) || (Claim == NULL)) {
    return;
  }

  if (Map) {
    QCBORDecode_GetByteStringInMapN (
      Context,
      Claim->Config.Label,
      &(Claim->Data.BufferData)
      );
  } else {
    QCBORDecode_GetByteString (Context, &(Claim->Data.BufferData));
  }
}

/** A helper function to parse claim with the a text string datatype.

  @param[in]      Context       Pointer to the Qcbor Decode Context.
  @param[in, out] Claim         Pointer to the claim. The claim config is
                                used as input and the claim data is populated
                                on output.
  @param[in]      Map           An option to specify if the parsed claim is
                                in a map.

**/
STATIC
VOID
ParseClaimDataTextString (
  IN  QCBORDecodeContext  *Context,
  IN OUT CLAIM            *Claim,
  IN BOOLEAN              Map
  )
{
  if ((Context == NULL) || (Claim == NULL)) {
    return;
  }

  if (Map) {
    QCBORDecode_GetTextStringInMapN (
      Context,
      Claim->Config.Label,
      &(Claim->Data.BufferData)
      );
  } else {
    QCBORDecode_GetTextString (Context, &(Claim->Data.BufferData));
  }
}

/** A helper function to parse a claim.

  @param[in]      Context       Pointer to the Qcbor Decode Context.
  @param[in, out] Claim         Pointer to the claim. The claim config is
                                used as input and the claim data is populated
                                on output.
  @param[in]      Map           An option to specify if the parsed claim is
                                in a map.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
ParseClaim (
  IN  QCBORDecodeContext  *Context,
  IN OUT CLAIM            *Claim,
  IN BOOLEAN              Map
  )
{
  EFI_STATUS  Status;

  if ((Context == NULL) || (Claim == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Claim->Config.Type) {
    case ClaimDataInt64:
      ParseClaimDataInt64 (Context, Claim, Map);
      break;
    case ClaimDataBool:
      ParseClaimDataBool (Context, Claim, Map);
      break;
    case ClaimDataByteString:
      ParseClaimDataByteString (Context, Claim, Map);
      break;
    case ClaimDataText:
      ParseClaimDataTextString (Context, Claim, Map);
      break;
    default:
      printf ("Parsing error: Invalid Data Type!\n");
      return EFI_INVALID_PARAMETER;
  } // switch

  Status = GetDecodeStatus (Context, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      (Claim->Config.Optional ? DEBUG_INFO : DEBUG_ERROR),
      "Parsing failed for '%a' claim: '%a', error: %r\n",
      (Claim->Config.Optional ? "Optional" : "Mandatory"),
      Claim->Config.Title,
      Status
      ));
  } else {
    printf ("Info: Claim found: %s\n", Claim->Config.Title);
    Claim->Data.Present = TRUE;
  }

  return Status;
}

/** A helper function to parse a list of claims.

  Note: If a claim was not found and it is not mandatory, then it is skipped
        and the next claim is parsed.

  @param[in]      Context       Pointer to the Qcbor Decode Context.
  @param[in, out] Claims        Pointer to the claims. The claims config is
                                used as input and the claims data is populated
                                on output.
  @param[in]      Map           An option to specify if the parsed claims is
                                in a map.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
ParseClaims (
  IN     QCBORDecodeContext  *Context,
  IN OUT CLAIM               *Claims,
  IN     UINTN               ClaimCount,
  IN     BOOLEAN             Map
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  if ((Context == NULL) || (Claims == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < ClaimCount; Index++) {
    Status = ParseClaim (Context, &Claims[Index], Map);
    if (EFI_ERROR (Status)) {
      if ((Status == EFI_NOT_FOUND) && (Claims[Index].Config.Optional)) {
        // If Claim was optional, continue parsing.
      } else {
        return Status;
      }
    }
  } // for

  return EFI_SUCCESS;
}

/** A helper function to parse the cca-platform-claims.

  @param[in]      PlatTokenPayload  Pointer to the Platform token payload.
  @param[in, out] Claims            Pointer to an array of claims. The claim
                                    config is used as input and the claim data
                                    is populated on output.


  The CCA platform claims is organised as:
  plat_token_claims
    cca-platform-claims = (cca-platform-claim-map)
    cca-platform-claim-map = {
      cca-platform-profile
      cca-platform-challenge
      cca-platform-implementation-id
      cca-platform-instance-id
      cca-platform-config
      cca-platform-lifecycle
      cca-platform-sw-components
      ? cca-platform-verification-service
      cca-platform-hash-algo-id
    }


  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
ParsePlatformTokenClaims (
  IN     struct q_useful_buf_c  PlatTokenPayload,
  IN OUT ATTESTATION_CLAIMS     *Claims
  )
{
  EFI_STATUS          Status;
  QCBORDecodeContext  Context;
  UINTN               Index;

  if (Claims == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  QCBORDecode_Init (&Context, PlatTokenPayload, QCBOR_DECODE_MODE_NORMAL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QCBORDecode_EnterMap (&Context, NULL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ParseClaims (
             &Context,
             Claims->PlatTokenClaims,
             CLAIM_COUNT_PLATFORM_TOKEN,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QCBORDecode_EnterArrayFromMapN (&Context, CCA_PLAT_SW_COMPONENTS_LABEL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Index = 0;
  while (Index < MAX_SW_COMPONENT_COUNT) {
    QCBORDecode_EnterMap (&Context, NULL);
    Status = GetDecodeStatus (&Context, FALSE);
    if (Status == EFI_END_OF_FILE) {
      // Decoder reached end of the map.
      break;
    } else if (EFI_ERROR (Status)) {
      // Any other error return.
      return Status;
    }

    Status = ParseClaims (
               &Context,
               Claims->SwComponentClaims[Index].Claims,
               CLAIM_COUNT_SW_COMPONENT,
               TRUE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    QCBORDecode_ExitMap (&Context);
    Status = GetDecodeStatus (&Context, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Index++;
  } // while

  // Decoder has reached the end of the map.
  QCBORDecode_GetAndResetError (&Context);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QCBORDecode_ExitArray (&Context);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QCBORDecode_ExitMap (&Context);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QCBORDecode_Finish (&Context);
  return EFI_SUCCESS;
}

/** A helper function to parse the cca-realm-claims.

  @param[in]      RealmTokenPayload   Pointer to the Realm token payload.
  @param[in, out] Claims              Pointer to an array of claims. The claim
                                      config is used as input and the claim data
                                      is populated on output.


  The CCA platform claims is organised as:
  realm_token_claims
    cca-realm-claims = (cca-realm-claim-map)
      cca-realm-claim-map = {
      cca-realm-challenge
      ? cca-realm-profile
      cca-realm-personalization-value
      cca-realm-initial-measurement
      cca-realm-extensible-measurements
      cca-realm-hash-algo-id
      cca-realm-public-key
      cca-realm-public-key-hash-algo-id
    }

  @retval EFI_BAD_BUFFER_SIZE     Invalid digest buffer size.
  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
ParseRealmTokenClaims (
  IN     struct q_useful_buf_c  RealmTokenPayload,
  IN OUT ATTESTATION_CLAIMS     *Claims
  )
{
  EFI_STATUS          Status;
  QCBORDecodeContext  Context;
  CLAIM               *RemClaims;
  UINTN               Index;

  if (Claims == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  QCBORDecode_Init (&Context, RealmTokenPayload, QCBOR_DECODE_MODE_NORMAL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QCBORDecode_EnterMap (&Context, NULL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ParseClaims (
             &Context,
             Claims->RealmTokenClaims,
             CLAIM_COUNT_REALM_TOKEN,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the Realm Extensible Measurements (REMs)
  QCBORDecode_EnterArrayFromMapN (
    &Context,
    CCA_REALM_EXTENSIBLE_MEASUREMENTS_LABEL
    );
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ParseClaims (
             &Context,
             Claims->RealmMeasurementClaims,
             CLAIM_COUNT_REMS,
             FALSE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RemClaims = Claims->RealmMeasurementClaims;
  for (Index = 0; Index < CLAIM_COUNT_REMS; Index++) {
    if ((RemClaims[Index].Data.BufferData.len != SHA256_DIGEST_SIZE) &&
        (RemClaims[Index].Data.BufferData.len != SHA512_DIGEST_SIZE))
    {
      return EFI_BAD_BUFFER_SIZE;
    }
  } // for

  QCBORDecode_ExitArray (&Context);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QCBORDecode_ExitMap (&Context);
  QCBORDecode_Finish (&Context);

  return EFI_SUCCESS;
}

/** A helper function to get the token payload from the COSE Sign 1 wrapper.

  @param[in]  CoseSign1Wrapper  Pointer to the COSE Sign 1 claim.
  @param[out] TokenPayload      Pointer to the wrapped token payload.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The token payload was not found.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
GetCoseSign1TokenPayload (
  IN  CLAIM                  *CoseSign1Wrapper,
  OUT struct q_useful_buf_c  *TokenPayload
  )
{
  if ((CoseSign1Wrapper == NULL) || (TokenPayload == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!CoseSign1Wrapper[INDEX_COSE_SIGN1_WRAPPER_PAYLOAD].Data.Present) {
    return EFI_NOT_FOUND;
  }

  *TokenPayload =
    CoseSign1Wrapper[INDEX_COSE_SIGN1_WRAPPER_PAYLOAD].Data.BufferData;
  return EFI_SUCCESS;
}

/** A helper function to parse the COSE Sign 1 wrapper.

  @param[in]      Token             Pointer to the token buffer.
  @param[in, out] CoseSign1Wrapper  Pointer to the COSE Sign 1 claim. The
                                    claim config is used as input and the
                                    claim data is populated on output.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
ParseCose1Wrapper (
  IN      struct q_useful_buf_c  Token,
  IN OUT  CLAIM                  *CoseSign1Wrapper
  )
{
  EFI_STATUS          Status;
  QCBORDecodeContext  Context;
  QCBORItem           Item;

  if (CoseSign1Wrapper == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  QCBORDecode_Init (&Context, Token, QCBOR_DECODE_MODE_NORMAL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check if the COSE Sign1 tag is present.
  QCBORDecode_PeekNext (&Context, &Item);
  if (!QCBORDecode_IsTagged (&Context, &Item, COSE_SIGN1_TAG)) {
    return EFI_NOT_FOUND;
  }

  QCBORDecode_EnterArray (&Context, NULL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the Protected header.
  Status = ParseClaim (
             &Context,
             &CoseSign1Wrapper[INDEX_COSE_SIGN1_WRAPPER_HEADER],
             FALSE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the Unprotected header.
  QCBORDecode_EnterMap (&Context, NULL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Ignore the contents.
  QCBORDecode_ExitMap (&Context);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the Payload.
  Status = ParseClaim (
             &Context,
             &CoseSign1Wrapper[INDEX_COSE_SIGN1_WRAPPER_PAYLOAD],
             FALSE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the Signature.
  Status = ParseClaim (
             &Context,
             &CoseSign1Wrapper[INDEX_COSE_SIGN1_WRAPPER_SIGNATURE],
             FALSE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QCBORDecode_ExitArray (&Context);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/** A helper function to parse the token buffer and return the Platform and
    the Realm Token.

  @param[in]    Token             Pointer to the token buffer.
  @param[out]   PlatToken         Pointer to the PlatToken buffer.
  @param[out]   RealmToken        Pointer to the RealmToken buffer.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
ParseToken (
  IN  struct q_useful_buf_c  Token,
  OUT struct q_useful_buf_c  *PlatToken,
  OUT struct q_useful_buf_c  *RealmToken
  )
{
  EFI_STATUS          Status;
  QCBORDecodeContext  Context;
  QCBORItem           Item;
  QCBORError          RetVal;
  UINT64              TokenTag;
  INT64               CoapTag;

  if ((PlatToken == NULL) || (RealmToken == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Initialise the decoder to parse the CCA Token.
  QCBORDecode_Init (&Context, Token, QCBOR_DECODE_MODE_NORMAL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Verify that the CCA_TOKEN tag is present.
  QCBORDecode_PeekNext (&Context, &Item);
  if (QCBORDecode_IsTagged (&Context, &Item, CCA_TOKEN_COLLECTION_TAG_REV1)) {
    TokenTag = CCA_TOKEN_COLLECTION_TAG_REV1;
  } else if (QCBORDecode_IsTagged (&Context, &Item, CCA_TOKEN_COLLECTION_TAG_REV0)) {
    TokenTag = CCA_TOKEN_COLLECTION_TAG_REV0;
  } else {
    return EFI_NOT_FOUND;
  }

  // Enter the Map.
  QCBORDecode_EnterMap (&Context, NULL);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (TokenTag == CCA_TOKEN_COLLECTION_TAG_REV0) {
    // The CCA platfrom token is the first item in the map.
    QCBORDecode_GetByteStringInMapN (&Context, CCA_PLAT_TOKEN, PlatToken);
    Status = GetDecodeStatus (&Context, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // The next item in the map is the delegated realm token.
    QCBORDecode_GetByteStringInMapN (
      &Context,
      CCA_REALM_DELEGATED_TOKEN,
      RealmToken
      );

    Status = GetDecodeStatus (&Context, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    // Get the CCA platform token.
    QCBORDecode_EnterArrayFromMapN (&Context, CCA_PLAT_TOKEN);
    Status = GetDecodeStatus (&Context, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    QCBORDecode_GetInt64 (&Context, (int64_t *)&CoapTag);
    Status = GetDecodeStatus (&Context, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (CoapTag != CCA_TOKEN_COAP_TYPE) {
      return EFI_INVALID_PARAMETER;
    }

    QCBORDecode_GetByteString (&Context, PlatToken);
    QCBORDecode_ExitArray (&Context);

    // Get the the realm token.
    QCBORDecode_EnterArrayFromMapN (&Context, CCA_REALM_DELEGATED_TOKEN);
    Status = GetDecodeStatus (&Context, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    QCBORDecode_GetInt64 (&Context, (int64_t *)&CoapTag);
    Status = GetDecodeStatus (&Context, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (CoapTag != CCA_TOKEN_COAP_TYPE) {
      return EFI_INVALID_PARAMETER;
    }

    QCBORDecode_GetByteString (&Context, RealmToken);
    QCBORDecode_ExitArray (&Context);
  }

  // Exit the Map.
  QCBORDecode_ExitMap (&Context);
  Status = GetDecodeStatus (&Context, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Uninitialise the decoder.
  RetVal = QCBORDecode_Finish (&Context);
  Status = QcborErrorToEfiStatus (RetVal);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Parsing failed. Error = 0x%llx\n", Status));
  }

  return Status;
}

/** A helper function to parse the PlatToken claims.

  @param[in]      PlatToken         Pointer to the PlatToken buffer.
  @param[in, out] Claims            Pointer to an array of claims. The claim
                                    config is used as input and the claim data
                                    is populated on output.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
ParsePlatformToken (
  IN struct q_useful_buf_c   *PlatToken,
  IN OUT ATTESTATION_CLAIMS  *Claims
  )
{
  EFI_STATUS                  Status;
  struct      q_useful_buf_c  PlatTokenPayload;

  if ((PlatToken == NULL) || (Claims == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Parse the Platform Token for the COSE_Sign1 wrapper.
  Status = ParseCose1Wrapper (
             *PlatToken,
             Claims->PlatCoseSign1Wrapper
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetCoseSign1TokenPayload (
             Claims->PlatCoseSign1Wrapper,
             &PlatTokenPayload
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Paset the platform token and retrive the claims.
  Status = ParsePlatformTokenClaims (PlatTokenPayload, Claims);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** A helper function to parse the RealmToken claims.

  @param[in]      RealmToken        Pointer to the RealmToken buffer.
  @param[in, out] Claims            Pointer to an array of claims. The claim
                                    config is used as input and the claim data
                                    is populated on output.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_END_OF_FILE         No more items found.
  @retval EFI_ABORTED             The parsing failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
ParseRealmToken (
  IN struct q_useful_buf_c  *RealmToken,
  OUT ATTESTATION_CLAIMS    *Claims
  )
{
  EFI_STATUS                  Status;
  struct      q_useful_buf_c  RealmTokenPayload;

  if ((RealmToken == NULL) || (Claims == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Parse the Realm Token for the COSE_Sign1 wrapper.
  Status = ParseCose1Wrapper (
             *RealmToken,
             Claims->RealmCoseSign1Wrapper
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetCoseSign1TokenPayload (
             Claims->RealmCoseSign1Wrapper,
             &RealmTokenPayload
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the Realm Token and retrieve the claims.
  Status = ParseRealmTokenClaims (RealmTokenPayload, Claims);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  )
{
  EFI_STATUS  Status;

  struct q_useful_buf_c  TokenBuffer;
  struct q_useful_buf_c  RealmToken;
  struct q_useful_buf_c  PlatToken;

  if ((Token == NULL) || (TokenLen == 0) || (AttestClaims == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (AttestClaims, sizeof (AttestClaims));

  // Copy the claims template.
  CopyMem (AttestClaims, &AttClaimsTemplate, sizeof (ATTESTATION_CLAIMS));

  TokenBuffer.len = TokenLen;
  TokenBuffer.ptr = Token;

  // Parse the token to get the Realm and Platform token buffers.
  Status = ParseToken (TokenBuffer, &PlatToken, &RealmToken);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the platform token.
  Status = ParsePlatformToken (&PlatToken, AttestClaims);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the Realm token.
  Status = ParseRealmToken (&RealmToken, AttestClaims);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/** A helper function to return the claim data.

  @param[in]  Claim      Pointer to the claim.
  @param[OUT] Data       Pointer to the claim data.
  @param[out] DataLen    Length of the claim data.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           The claim tag/label was not found.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
GetClaimData (
  IN  CLAIM  *Claim,
  OUT UINT8  **Data,
  OUT UINTN  *DataLen
  )
{
  if ((Claim == NULL) || (Data == NULL) || (DataLen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Claim->Data.Present) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "Info: Get Claim: %a\n", Claim->Config.Title));

  *Data    = (UINT8 *)Claim->Data.BufferData.ptr;
  *DataLen = (UINTN)Claim->Data.BufferData.len;

  return EFI_SUCCESS;
}

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
  )
{
  CLAIM  *Claim;

  if ((AttestClaims == NULL) || (RimData == NULL) || (RimDataLen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Claim = &AttestClaims->RealmTokenClaims[REALM_TOKEN_CLAIM_IDX_RIM];

  return GetClaimData (Claim, RimData, RimDataLen);
}

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
  )
{
  CLAIM  *Claim;

  if ((AttestClaims == NULL) ||
      (RemIndex >= CLAIM_COUNT_REMS) ||
      (RemData == NULL) ||
      (RemDataLen == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Claim = &AttestClaims->RealmMeasurementClaims[RemIndex];

  return GetClaimData (Claim, RemData, RemDataLen);
}

/** Get the Realm Personalisation Value(RPV) from the
    Arm CCA attestation token claims.

  @param[in]  AttestClaims  Pointer to the Arm CCA attestation token claims.
  @param[OUT] RpvData       Pointer to the RPV data.
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
  )
{
  CLAIM  *Claim;

  if ((AttestClaims == NULL) || (RpvData == NULL) || (RpvDataLen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Claim = &AttestClaims->RealmTokenClaims[REALM_TOKEN_CLAIM_IDX_RPV];
  return GetClaimData (Claim, RpvData, RpvDataLen);
}

/** Get the Realm Challenge from the Arm CCA attestation token claims.

  @param[in]  AttestClaims  Pointer to the Arm CCA attestation token claims.
  @param[OUT] RpvData       Pointer to the Realm Challenge data.
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
  )
{
  CLAIM  *Claim;

  if ((AttestClaims == NULL) || (Data == NULL) || (DataLen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Claim = &AttestClaims->RealmTokenClaims[REALM_TOKEN_CLAIM_IDX_CHALLENGE];
  return GetClaimData (Claim, Data, DataLen);
}
