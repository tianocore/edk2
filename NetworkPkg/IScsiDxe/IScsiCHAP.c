/** @file
  This file is for Challenge-Handshake Authentication Protocol (CHAP)
  Configuration.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IScsiImpl.h"

//
// Supported CHAP hash algorithms, mapped to sets of BaseCryptLib APIs and
// macros. CHAP_HASH structures at lower subscripts in the array are preferred
// by the initiator.
//
STATIC CONST CHAP_HASH mChapHash[] = {
  {
    ISCSI_CHAP_ALGORITHM_SHA256,
    SHA256_DIGEST_SIZE,
    Sha256GetContextSize,
    Sha256Init,
    Sha256Update,
    Sha256Final
  },
#ifdef ENABLE_MD5_DEPRECATED_INTERFACES
  //
  // Keep the deprecated MD5 entry at the end of the array (making MD5 the
  // least preferred choice of the initiator).
  //
  {
    ISCSI_CHAP_ALGORITHM_MD5,
    MD5_DIGEST_SIZE,
    Md5GetContextSize,
    Md5Init,
    Md5Update,
    Md5Final
  },
#endif // ENABLE_MD5_DEPRECATED_INTERFACES
};

//
// Ordered list of mChapHash[*].Algorithm values. It is formatted for the
// CHAP_A=<A1,A2...> value string, by the IScsiCHAPInitHashList() function. It
// is sent by the initiator in ISCSI_CHAP_STEP_ONE.
//
STATIC CHAR8 mChapHashListString[
               3 +                                      // UINT8 identifier in
                                                        //   decimal
               (1 + 3) * (ARRAY_SIZE (mChapHash) - 1) + // comma prepended for
                                                        //   entries after the
                                                        //   first
               1 +                                      // extra character for
                                                        //   AsciiSPrint()
                                                        //   truncation check
               1                                        // terminating NUL
               ];

/**
  Initiator calculates its own expected hash value.

  @param[in]   ChapIdentifier     iSCSI CHAP identifier sent by authenticator.
  @param[in]   ChapSecret         iSCSI CHAP secret of the authenticator.
  @param[in]   SecretLength       The length of iSCSI CHAP secret.
  @param[in]   ChapChallenge      The challenge message sent by authenticator.
  @param[in]   ChallengeLength    The length of iSCSI CHAP challenge message.
  @param[in]   Hash               Pointer to the CHAP_HASH structure that
                                  determines the hashing algorithm to use. The
                                  caller is responsible for making Hash point
                                  to an "mChapHash" element.
  @param[out]  ChapResponse       The calculation of the expected hash value.

  @retval EFI_SUCCESS             The expected hash value was calculatedly
                                  successfully.
  @retval EFI_PROTOCOL_ERROR      The length of the secret should be at least
                                  the length of the hash value for the hashing
                                  algorithm chosen.
  @retval EFI_PROTOCOL_ERROR      Hash operation fails.
  @retval EFI_OUT_OF_RESOURCES    Failure to allocate resource to complete
                                  hashing.

**/
EFI_STATUS
IScsiCHAPCalculateResponse (
  IN  UINT32          ChapIdentifier,
  IN  CHAR8           *ChapSecret,
  IN  UINT32          SecretLength,
  IN  UINT8           *ChapChallenge,
  IN  UINT32          ChallengeLength,
  IN  CONST CHAP_HASH *Hash,
  OUT UINT8           *ChapResponse
  )
{
  UINTN       ContextSize;
  VOID        *Ctx;
  CHAR8       IdByte[1];
  EFI_STATUS  Status;

  if (SecretLength < ISCSI_CHAP_SECRET_MIN_LEN) {
    return EFI_PROTOCOL_ERROR;
  }

  ASSERT (Hash != NULL);

  ContextSize = Hash->GetContextSize ();
  Ctx = AllocatePool (ContextSize);
  if (Ctx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_PROTOCOL_ERROR;

  if (!Hash->Init (Ctx)) {
    goto Exit;
  }

  //
  // Hash Identifier - Only calculate 1 byte data (RFC1994)
  //
  IdByte[0] = (CHAR8) ChapIdentifier;
  if (!Hash->Update (Ctx, IdByte, 1)) {
    goto Exit;
  }

  //
  // Hash Secret
  //
  if (!Hash->Update (Ctx, ChapSecret, SecretLength)) {
    goto Exit;
  }

  //
  // Hash Challenge received from Target
  //
  if (!Hash->Update (Ctx, ChapChallenge, ChallengeLength)) {
    goto Exit;
  }

  if (Hash->Final (Ctx, ChapResponse)) {
    Status = EFI_SUCCESS;
  }

Exit:
  FreePool (Ctx);
  return Status;
}

/**
  The initiator checks the CHAP response replied by target against its own
  calculation of the expected hash value.

  @param[in]   AuthData             iSCSI CHAP authentication data.
  @param[in]   TargetResponse       The response from target.

  @retval EFI_SUCCESS               The response from target passed
                                    authentication.
  @retval EFI_SECURITY_VIOLATION    The response from target was not expected
                                    value.
  @retval Others                    Other errors as indicated.

**/
EFI_STATUS
IScsiCHAPAuthTarget (
  IN  ISCSI_CHAP_AUTH_DATA  *AuthData,
  IN  UINT8                 *TargetResponse
  )
{
  EFI_STATUS  Status;
  UINT32      SecretSize;
  UINT8       VerifyRsp[ISCSI_CHAP_MAX_DIGEST_SIZE];
  INTN        Mismatch;

  Status      = EFI_SUCCESS;

  SecretSize  = (UINT32) AsciiStrLen (AuthData->AuthConfig->ReverseCHAPSecret);

  ASSERT (AuthData->Hash != NULL);

  Status = IScsiCHAPCalculateResponse (
             AuthData->OutIdentifier,
             AuthData->AuthConfig->ReverseCHAPSecret,
             SecretSize,
             AuthData->OutChallenge,
             AuthData->Hash->DigestSize,              // ChallengeLength
             AuthData->Hash,
             VerifyRsp
             );

  Mismatch = CompareMem (
               VerifyRsp,
               TargetResponse,
               AuthData->Hash->DigestSize
               );
  if (Mismatch != 0) {
    Status = EFI_SECURITY_VIOLATION;
  }

  return Status;
}


/**
  This function checks the received iSCSI Login Response during the security
  negotiation stage.

  @param[in] Conn             The iSCSI connection.

  @retval EFI_SUCCESS          The Login Response passed the CHAP validation.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR   Some kind of protocol error occurred.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
IScsiCHAPOnRspReceived (
  IN ISCSI_CONNECTION  *Conn
  )
{
  EFI_STATUS                  Status;
  ISCSI_SESSION               *Session;
  ISCSI_CHAP_AUTH_DATA        *AuthData;
  CHAR8                       *Value;
  UINT8                       *Data;
  UINT32                      Len;
  LIST_ENTRY                  *KeyValueList;
  UINTN                       Algorithm;
  CHAR8                       *Identifier;
  CHAR8                       *Challenge;
  CHAR8                       *Name;
  CHAR8                       *Response;
  UINT8                       TargetRsp[ISCSI_CHAP_MAX_DIGEST_SIZE];
  UINT32                      RspLen;
  UINTN                       Result;
  UINTN                       HashIndex;

  ASSERT (Conn->CurrentStage == ISCSI_SECURITY_NEGOTIATION);
  ASSERT (Conn->RspQue.BufNum != 0);

  Session     = Conn->Session;
  AuthData    = &Session->AuthData.CHAP;
  Len         = Conn->RspQue.BufSize;
  Data        = AllocateZeroPool (Len);
  if (Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Copy the data in case the data spans over multiple PDUs.
  //
  NetbufQueCopy (&Conn->RspQue, 0, Len, Data);

  //
  // Build the key-value list from the data segment of the Login Response.
  //
  KeyValueList = IScsiBuildKeyValueList ((CHAR8 *) Data, Len);
  if (KeyValueList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = EFI_PROTOCOL_ERROR;

  switch (Conn->AuthStep) {
  case ISCSI_AUTH_INITIAL:
    //
    // The first Login Response.
    //
    Value = IScsiGetValueByKeyFromList (
              KeyValueList,
              ISCSI_KEY_TARGET_PORTAL_GROUP_TAG
              );
    if (Value == NULL) {
      goto ON_EXIT;
    }

    Result = IScsiNetNtoi (Value);
    if (Result > 0xFFFF) {
      goto ON_EXIT;
    }

    Session->TargetPortalGroupTag = (UINT16) Result;

    Value                         = IScsiGetValueByKeyFromList (
                                      KeyValueList,
                                      ISCSI_KEY_AUTH_METHOD
                                      );
    if (Value == NULL) {
      goto ON_EXIT;
    }
    //
    // Initiator mandates CHAP authentication but target replies without
    // "CHAP", or initiator suggets "None" but target replies with some kind of
    // auth method.
    //
    if (Session->AuthType == ISCSI_AUTH_TYPE_NONE) {
      if (AsciiStrCmp (Value, ISCSI_KEY_VALUE_NONE) != 0) {
        goto ON_EXIT;
      }
    } else if (Session->AuthType == ISCSI_AUTH_TYPE_CHAP) {
      if (AsciiStrCmp (Value, ISCSI_AUTH_METHOD_CHAP) != 0) {
        goto ON_EXIT;
      }
    } else {
      goto ON_EXIT;
    }

    //
    // Transit to CHAP step one.
    //
    Conn->AuthStep  = ISCSI_CHAP_STEP_ONE;
    Status          = EFI_SUCCESS;
    break;

  case ISCSI_CHAP_STEP_TWO:
    //
    // The Target replies with CHAP_A=<A> CHAP_I=<I> CHAP_C=<C>
    //
    Value = IScsiGetValueByKeyFromList (
              KeyValueList,
              ISCSI_KEY_CHAP_ALGORITHM
              );
    if (Value == NULL) {
      goto ON_EXIT;
    }

    Algorithm = IScsiNetNtoi (Value);
    for (HashIndex = 0; HashIndex < ARRAY_SIZE (mChapHash); HashIndex++) {
      if (Algorithm == mChapHash[HashIndex].Algorithm) {
        break;
      }
    }
    if (HashIndex == ARRAY_SIZE (mChapHash)) {
      //
      // Unsupported algorithm is chosen by target.
      //
      goto ON_EXIT;
    }
    //
    // Remember the target's chosen hash algorithm.
    //
    ASSERT (AuthData->Hash == NULL);
    AuthData->Hash = &mChapHash[HashIndex];

    Identifier = IScsiGetValueByKeyFromList (
                   KeyValueList,
                   ISCSI_KEY_CHAP_IDENTIFIER
                   );
    if (Identifier == NULL) {
      goto ON_EXIT;
    }

    Challenge = IScsiGetValueByKeyFromList (
                  KeyValueList,
                  ISCSI_KEY_CHAP_CHALLENGE
                  );
    if (Challenge == NULL) {
      goto ON_EXIT;
    }
    //
    // Process the CHAP identifier and CHAP Challenge from Target.
    // Calculate Response value.
    //
    Result = IScsiNetNtoi (Identifier);
    if (Result > 0xFF) {
      goto ON_EXIT;
    }

    AuthData->InIdentifier      = (UINT32) Result;
    AuthData->InChallengeLength = (UINT32) sizeof (AuthData->InChallenge);
    Status = IScsiHexToBin (
               (UINT8 *) AuthData->InChallenge,
               &AuthData->InChallengeLength,
               Challenge
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_PROTOCOL_ERROR;
      goto ON_EXIT;
    }
    Status = IScsiCHAPCalculateResponse (
               AuthData->InIdentifier,
               AuthData->AuthConfig->CHAPSecret,
               (UINT32) AsciiStrLen (AuthData->AuthConfig->CHAPSecret),
               AuthData->InChallenge,
               AuthData->InChallengeLength,
               AuthData->Hash,
               AuthData->CHAPResponse
               );

    //
    // Transit to next step.
    //
    Conn->AuthStep = ISCSI_CHAP_STEP_THREE;
    break;

  case ISCSI_CHAP_STEP_THREE:
    //
    // One way CHAP authentication and the target would like to
    // authenticate us.
    //
    Status = EFI_SUCCESS;
    break;

  case ISCSI_CHAP_STEP_FOUR:
    ASSERT (AuthData->AuthConfig->CHAPType == ISCSI_CHAP_MUTUAL);
    //
    // The forth step, CHAP_N=<N> CHAP_R=<R> is received from Target.
    //
    Name = IScsiGetValueByKeyFromList (KeyValueList, ISCSI_KEY_CHAP_NAME);
    if (Name == NULL) {
      goto ON_EXIT;
    }

    Response = IScsiGetValueByKeyFromList (
                 KeyValueList,
                 ISCSI_KEY_CHAP_RESPONSE
                 );
    if (Response == NULL) {
      goto ON_EXIT;
    }

    ASSERT (AuthData->Hash != NULL);
    RspLen = AuthData->Hash->DigestSize;
    Status = IScsiHexToBin (TargetRsp, &RspLen, Response);
    if (EFI_ERROR (Status) || RspLen != AuthData->Hash->DigestSize) {
      Status = EFI_PROTOCOL_ERROR;
      goto ON_EXIT;
    }

    //
    // Check the CHAP Name and Response replied by Target.
    //
    Status = IScsiCHAPAuthTarget (AuthData, TargetRsp);
    break;

  default:
    break;
  }

ON_EXIT:

  if (KeyValueList != NULL) {
    IScsiFreeKeyValueList (KeyValueList);
  }

  FreePool (Data);

  return Status;
}


/**
  This function fills the CHAP authentication information into the login PDU
  during the security negotiation stage in the iSCSI connection login.

  @param[in]       Conn        The iSCSI connection.
  @param[in, out]  Pdu         The PDU to send out.

  @retval EFI_SUCCESS           All check passed and the phase-related CHAP
                                authentication info is filled into the iSCSI
                                PDU.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR    Some kind of protocol error occurred.

**/
EFI_STATUS
IScsiCHAPToSendReq (
  IN      ISCSI_CONNECTION  *Conn,
  IN OUT  NET_BUF           *Pdu
  )
{
  EFI_STATUS                  Status;
  ISCSI_SESSION               *Session;
  ISCSI_LOGIN_REQUEST         *LoginReq;
  ISCSI_CHAP_AUTH_DATA        *AuthData;
  CHAR8                       *Value;
  CHAR8                       ValueStr[256];
  CHAR8                       *Response;
  UINT32                      RspLen;
  CHAR8                       *Challenge;
  UINT32                      ChallengeLen;
  EFI_STATUS                  BinToHexStatus;

  ASSERT (Conn->CurrentStage == ISCSI_SECURITY_NEGOTIATION);

  Session     = Conn->Session;
  AuthData    = &Session->AuthData.CHAP;
  LoginReq    = (ISCSI_LOGIN_REQUEST *) NetbufGetByte (Pdu, 0, 0);
  if (LoginReq == NULL) {
    return EFI_PROTOCOL_ERROR;
  }
  Status      = EFI_SUCCESS;

  RspLen      = 2 * ISCSI_CHAP_MAX_DIGEST_SIZE + 3;
  Response    = AllocateZeroPool (RspLen);
  if (Response == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ChallengeLen  = 2 * ISCSI_CHAP_MAX_DIGEST_SIZE + 3;
  Challenge     = AllocateZeroPool (ChallengeLen);
  if (Challenge == NULL) {
    FreePool (Response);
    return EFI_OUT_OF_RESOURCES;
  }

  switch (Conn->AuthStep) {
  case ISCSI_AUTH_INITIAL:
    //
    // It's the initial Login Request. Fill in the key=value pairs mandatory
    // for the initial Login Request.
    //
    IScsiAddKeyValuePair (
      Pdu,
      ISCSI_KEY_INITIATOR_NAME,
      mPrivate->InitiatorName
      );
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_SESSION_TYPE, "Normal");
    IScsiAddKeyValuePair (
      Pdu,
      ISCSI_KEY_TARGET_NAME,
      Session->ConfigData->SessionConfigData.TargetName
      );

    if (Session->AuthType == ISCSI_AUTH_TYPE_NONE) {
      Value = ISCSI_KEY_VALUE_NONE;
      ISCSI_SET_FLAG (LoginReq, ISCSI_LOGIN_REQ_PDU_FLAG_TRANSIT);
    } else {
      Value = ISCSI_AUTH_METHOD_CHAP;
    }

    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_AUTH_METHOD, Value);

    break;

  case ISCSI_CHAP_STEP_ONE:
    //
    // First step, send the Login Request with CHAP_A=<A1,A2...> key-value
    // pair.
    //
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_ALGORITHM, mChapHashListString);

    Conn->AuthStep = ISCSI_CHAP_STEP_TWO;
    break;

  case ISCSI_CHAP_STEP_THREE:
    //
    // Third step, send the Login Request with CHAP_N=<N> CHAP_R=<R> or
    // CHAP_N=<N> CHAP_R=<R> CHAP_I=<I> CHAP_C=<C> if target authentication is
    // required too.
    //
    // CHAP_N=<N>
    //
    IScsiAddKeyValuePair (
      Pdu,
      ISCSI_KEY_CHAP_NAME,
      (CHAR8 *) &AuthData->AuthConfig->CHAPName
      );
    //
    // CHAP_R=<R>
    //
    ASSERT (AuthData->Hash != NULL);
    BinToHexStatus = IScsiBinToHex (
                       (UINT8 *) AuthData->CHAPResponse,
                       AuthData->Hash->DigestSize,
                       Response,
                       &RspLen
                       );
    ASSERT_EFI_ERROR (BinToHexStatus);
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_RESPONSE, Response);

    if (AuthData->AuthConfig->CHAPType == ISCSI_CHAP_MUTUAL) {
      //
      // CHAP_I=<I>
      //
      IScsiGenRandom ((UINT8 *) &AuthData->OutIdentifier, 1);
      AsciiSPrint (ValueStr, sizeof (ValueStr), "%d", AuthData->OutIdentifier);
      IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_IDENTIFIER, ValueStr);
      //
      // CHAP_C=<C>
      //
      IScsiGenRandom (
        (UINT8 *) AuthData->OutChallenge,
        AuthData->Hash->DigestSize
        );
      BinToHexStatus = IScsiBinToHex (
                         (UINT8 *) AuthData->OutChallenge,
                         AuthData->Hash->DigestSize,
                         Challenge,
                         &ChallengeLen
                         );
      ASSERT_EFI_ERROR (BinToHexStatus);
      IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_CHALLENGE, Challenge);

      Conn->AuthStep = ISCSI_CHAP_STEP_FOUR;
    }
    //
    // Set the stage transition flag.
    //
    ISCSI_SET_FLAG (LoginReq, ISCSI_LOGIN_REQ_PDU_FLAG_TRANSIT);
    break;

  default:
    Status = EFI_PROTOCOL_ERROR;
    break;
  }

  FreePool (Response);
  FreePool (Challenge);

  return Status;
}

/**
  Initialize the CHAP_A=<A1,A2...> *value* string for the entire driver, to be
  sent by the initiator in ISCSI_CHAP_STEP_ONE.

  This function sanity-checks the internal table of supported CHAP hashing
  algorithms, as well.
**/
VOID
IScsiCHAPInitHashList (
  VOID
  )
{
  CHAR8           *Position;
  UINTN           Left;
  UINTN           HashIndex;
  CONST CHAP_HASH *Hash;
  UINTN           Printed;

  Position = mChapHashListString;
  Left = sizeof (mChapHashListString);
  for (HashIndex = 0; HashIndex < ARRAY_SIZE (mChapHash); HashIndex++) {
    Hash = &mChapHash[HashIndex];

    //
    // Format the next hash identifier.
    //
    // Assert that we can format at least one non-NUL character, i.e. that we
    // can progress. Truncation is checked after printing.
    //
    ASSERT (Left >= 2);
    Printed = AsciiSPrint (
                Position,
                Left,
                "%a%d",
                (HashIndex == 0) ? "" : ",",
                Hash->Algorithm
                );
    //
    // There's no way to differentiate between the "buffer filled to the brim,
    // but not truncated" result and the "truncated" result of AsciiSPrint().
    // This is why "mChapHashListString" has an extra byte allocated, and the
    // reason why we use the less-than (rather than the less-than-or-equal-to)
    // relational operator in the assertion below -- we enforce "no truncation"
    // by excluding the "completely used up" case too.
    //
    ASSERT (Printed + 1 < Left);

    Position += Printed;
    Left -= Printed;

    //
    // Sanity-check the digest size for Hash.
    //
    ASSERT (Hash->DigestSize <= ISCSI_CHAP_MAX_DIGEST_SIZE);
  }
}
