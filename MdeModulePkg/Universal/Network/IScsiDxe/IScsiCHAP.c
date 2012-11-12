/** @file
  This file is for Challenge-Handshake Authentication Protocol (CHAP) Configuration.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"
#include "Md5.h"

/**
  Initator caculates its own expected hash value.

  @param[in]   ChapIdentifier     iSCSI CHAP identifier sent by authenticator.
  @param[in]   ChapSecret         iSCSI CHAP secret of the authenticator.
  @param[in]   SecretLength       The length of iSCSI CHAP secret.
  @param[in]   ChapChallenge      The challenge message sent by authenticator.
  @param[in]   ChallengeLength    The length of iSCSI CHAP challenge message.
  @param[out]  ChapResponse       The calculation of the expected hash value.

  @retval EFI_SUCCESS             The expected hash value was caculatedly successfully.
  @retval EFI_PROTOCOL_ERROR      The length of the secret should be at least the
                                  length of the hash value for the hashing algorithm chosen.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
IScsiCHAPCalculateResponse (
  IN  UINT32  ChapIdentifier,
  IN  CHAR8   *ChapSecret,
  IN  UINT32  SecretLength,
  IN  UINT8   *ChapChallenge,
  IN  UINT32  ChallengeLength,
  OUT UINT8   *ChapResponse
  )
{
  MD5_CTX     Md5Ctx;
  CHAR8       IdByte[1];
  EFI_STATUS  Status;

  Status = MD5Init (&Md5Ctx);

  //
  // Hash Identifier - Only calculate 1 byte data (RFC1994)
  //
  IdByte[0] = (CHAR8) ChapIdentifier;
  MD5Update (&Md5Ctx, IdByte, 1);

  //
  // Hash Secret
  //
  if (SecretLength < ISCSI_CHAP_SECRET_MIN_LEN - 1) {
    return EFI_PROTOCOL_ERROR;
  }

  MD5Update (&Md5Ctx, ChapSecret, SecretLength);

  //
  // Hash Challenge received from Target
  //
  MD5Update (&Md5Ctx, ChapChallenge, ChallengeLength);

  Status = MD5Final (&Md5Ctx, ChapResponse);

  return Status;
}

/**
  The initator checks the CHAP response replied by target against its own
  calculation of the expected hash value.

  @param[in]   AuthData             iSCSI CHAP authentication data.
  @param[in]   TargetResponse       The response from target.

  @retval EFI_SUCCESS               The response from target passed authentication.
  @retval EFI_SECURITY_VIOLATION    The response from target was not expected value.
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
  UINT8       VerifyRsp[ISCSI_CHAP_RSP_LEN];

  Status      = EFI_SUCCESS;

  SecretSize  = (UINT32) AsciiStrLen (AuthData->AuthConfig.ReverseCHAPSecret);
  Status = IScsiCHAPCalculateResponse (
            AuthData->OutIdentifier,
            AuthData->AuthConfig.ReverseCHAPSecret,
            SecretSize,
            AuthData->OutChallenge,
            AuthData->OutChallengeLength,
            VerifyRsp
            );

  if (CompareMem (VerifyRsp, TargetResponse, ISCSI_CHAP_RSP_LEN) != 0) {
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
  @retval EFI_PROTOCOL_ERROR   Some kind of protocol error happend.
  @retval Others               Other errors as indicated.
**/
EFI_STATUS
IScsiCHAPOnRspReceived (
  IN ISCSI_CONNECTION  *Conn
  )
{
  EFI_STATUS                Status;
  ISCSI_SESSION             *Session;
  ISCSI_CHAP_AUTH_DATA      *AuthData;
  CHAR8                     *Value;
  UINT8                     *Data;
  UINT32                    Len;
  LIST_ENTRY                *KeyValueList;
  UINTN                     Algorithm;
  CHAR8                     *Identifier;
  CHAR8                     *Challenge;
  CHAR8                     *Name;
  CHAR8                     *Response;
  UINT8                     TargetRsp[ISCSI_CHAP_RSP_LEN];
  UINT32                    RspLen;

  ASSERT (Conn->CurrentStage == ISCSI_SECURITY_NEGOTIATION);
  ASSERT (Conn->RspQue.BufNum != 0);

  Session     = Conn->Session;
  AuthData    = &Session->AuthData;

  Len         = Conn->RspQue.BufSize;
  Data        = AllocatePool (Len);
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
    FreePool (Data);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_PROTOCOL_ERROR;

  switch (Conn->CHAPStep) {
  case ISCSI_CHAP_INITIAL:
    //
    // The first Login Response.
    //
    Value = IScsiGetValueByKeyFromList (KeyValueList, ISCSI_KEY_TARGET_PORTAL_GROUP_TAG);
    if (Value == NULL) {
      goto ON_EXIT;
    }

    Session->TargetPortalGroupTag = (UINT16) AsciiStrDecimalToUintn (Value);

    Value                         = IScsiGetValueByKeyFromList (KeyValueList, ISCSI_KEY_AUTH_METHOD);
    if (Value == NULL) {
      goto ON_EXIT;
    }
    //
    // Initiator mandates CHAP authentication but target replies without "CHAP" or
    // initiator suggets "None" but target replies with some kind of auth method.
    //
    if (AsciiStrCmp (Value, ISCSI_AUTH_METHOD_CHAP) == 0) {
      if (AuthData->AuthConfig.CHAPType == ISCSI_CHAP_NONE) {
        goto ON_EXIT;
      }
    } else {
      if (AuthData->AuthConfig.CHAPType != ISCSI_CHAP_NONE) {
        goto ON_EXIT;
      }
    }
    //
    // Transit to CHAP step one.
    //
    Conn->CHAPStep  = ISCSI_CHAP_STEP_ONE;
    Status          = EFI_SUCCESS;
    break;

  case ISCSI_CHAP_STEP_TWO:
    //
    // The Target replies with CHAP_A=<A> CHAP_I=<I> CHAP_C=<C>
    //
    Value = IScsiGetValueByKeyFromList (KeyValueList, ISCSI_KEY_CHAP_ALGORITHM);
    if (Value == NULL) {
      goto ON_EXIT;
    }

    Algorithm = AsciiStrDecimalToUintn (Value);
    if (Algorithm != ISCSI_CHAP_ALGORITHM_MD5) {
      //
      // Unsupported algorithm is chosen by target.
      //
      goto ON_EXIT;
    }

    Identifier = IScsiGetValueByKeyFromList (KeyValueList, ISCSI_KEY_CHAP_IDENTIFIER);
    if (Identifier == NULL) {
      goto ON_EXIT;
    }

    Challenge = IScsiGetValueByKeyFromList (KeyValueList, ISCSI_KEY_CHAP_CHALLENGE);
    if (Challenge == NULL) {
      goto ON_EXIT;
    }
    //
    // Process the CHAP identifier and CHAP Challenge from Target
    // Calculate Response value
    //
    AuthData->InIdentifier      = (UINT32) AsciiStrDecimalToUintn (Identifier);
    AuthData->InChallengeLength = ISCSI_CHAP_AUTH_MAX_LEN;
    IScsiHexToBin ((UINT8 *) AuthData->InChallenge, &AuthData->InChallengeLength, Challenge);
    Status = IScsiCHAPCalculateResponse (
              AuthData->InIdentifier,
              AuthData->AuthConfig.CHAPSecret,
              (UINT32) AsciiStrLen (AuthData->AuthConfig.CHAPSecret),
              AuthData->InChallenge,
              AuthData->InChallengeLength,
              AuthData->CHAPResponse
              );

    //
    // Transit to next step.
    //
    Conn->CHAPStep = ISCSI_CHAP_STEP_THREE;
    break;

  case ISCSI_CHAP_STEP_THREE:
    //
    // one way CHAP authentication and the target would like to
    // authenticate us.
    //
    Status = EFI_SUCCESS;
    break;

  case ISCSI_CHAP_STEP_FOUR:
    ASSERT (AuthData->AuthConfig.CHAPType == ISCSI_CHAP_MUTUAL);
    //
    // The forth step, CHAP_N=<N> CHAP_R=<R> is received from Target.
    //
    Name = IScsiGetValueByKeyFromList (KeyValueList, ISCSI_KEY_CHAP_NAME);
    if (Name == NULL) {
      goto ON_EXIT;
    }

    Response = IScsiGetValueByKeyFromList (KeyValueList, ISCSI_KEY_CHAP_RESPONSE);
    if (Response == NULL) {
      goto ON_EXIT;
    }

    RspLen = ISCSI_CHAP_RSP_LEN;
    IScsiHexToBin (TargetRsp, &RspLen, Response);

    //
    // Check the CHAP Response replied by Target.
    //
    Status = IScsiCHAPAuthTarget (AuthData, TargetRsp);
    break;

  default:
    break;
  }

ON_EXIT:

  IScsiFreeKeyValueList (KeyValueList);

  FreePool (Data);

  return Status;
}

/**
  This function fills the CHAP authentication information into the login PDU
  during the security negotiation stage in the iSCSI connection login.

  @param[in]       Conn        The iSCSI connection.
  @param[in, out]  Pdu         The PDU to send out.

  @retval EFI_SUCCESS          All check passed and the phase-related CHAP
                               authentication info is filled into the iSCSI PDU.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR   Some kind of protocol error happend.
**/
EFI_STATUS
IScsiCHAPToSendReq (
  IN      ISCSI_CONNECTION  *Conn,
  IN OUT  NET_BUF           *Pdu
  )
{
  EFI_STATUS                Status;
  ISCSI_SESSION             *Session;
  ISCSI_LOGIN_REQUEST       *LoginReq;
  ISCSI_CHAP_AUTH_DATA      *AuthData;
  CHAR8                     *Value;
  CHAR8                     ValueStr[256];
  CHAR8                     *Response;
  UINT32                    RspLen;
  CHAR8                     *Challenge;
  UINT32                    ChallengeLen;

  ASSERT (Conn->CurrentStage == ISCSI_SECURITY_NEGOTIATION);

  Session     = Conn->Session;
  AuthData    = &Session->AuthData;
  LoginReq    = (ISCSI_LOGIN_REQUEST *) NetbufGetByte (Pdu, 0, 0);
  if (LoginReq == NULL) {
    return EFI_PROTOCOL_ERROR;
  }
  Status      = EFI_SUCCESS;

  RspLen      = 2 * ISCSI_CHAP_RSP_LEN + 3;
  Response    = AllocatePool (RspLen);
  if (Response == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ChallengeLen  = 2 * ISCSI_CHAP_RSP_LEN + 3;
  Challenge     = AllocatePool (ChallengeLen);
  if (Challenge == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  switch (Conn->CHAPStep) {
  case ISCSI_CHAP_INITIAL:
    //
    // It's the initial Login Request. Fill in the key=value pairs mandatory
    // for the initial Login Request.
    //
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_INITIATOR_NAME, Session->InitiatorName);
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_SESSION_TYPE, "Normal");
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_TARGET_NAME, Session->ConfigData.NvData.TargetName);

    if (AuthData->AuthConfig.CHAPType == ISCSI_CHAP_NONE) {
      Value = ISCSI_KEY_VALUE_NONE;
      ISCSI_SET_FLAG (LoginReq, ISCSI_LOGIN_REQ_PDU_FLAG_TRANSIT);
    } else {
      Value = ISCSI_AUTH_METHOD_CHAP;
    }

    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_AUTH_METHOD, Value);

    break;

  case ISCSI_CHAP_STEP_ONE:
    //
    // First step, send the Login Request with CHAP_A=<A1,A2...> key-value pair.
    //
    AsciiSPrint (ValueStr, sizeof (ValueStr), "%d", ISCSI_CHAP_ALGORITHM_MD5);
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_ALGORITHM, ValueStr);

    Conn->CHAPStep = ISCSI_CHAP_STEP_TWO;
    break;

  case ISCSI_CHAP_STEP_THREE:
    //
    // Third step, send the Login Request with CHAP_N=<N> CHAP_R=<R> or
    // CHAP_N=<N> CHAP_R=<R> CHAP_I=<I> CHAP_C=<C> if target ahtentication is
    // required too.
    //
    // CHAP_N=<N>
    //
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_NAME, (CHAR8 *) &AuthData->AuthConfig.CHAPName);
    //
    // CHAP_R=<R>
    //
    IScsiBinToHex ((UINT8 *) AuthData->CHAPResponse, ISCSI_CHAP_RSP_LEN, Response, &RspLen);
    IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_RESPONSE, Response);

    if (AuthData->AuthConfig.CHAPType == ISCSI_CHAP_MUTUAL) {
      //
      // CHAP_I=<I>
      //
      IScsiGenRandom ((UINT8 *) &AuthData->OutIdentifier, 1);
      AsciiSPrint (ValueStr, sizeof (ValueStr), "%d", AuthData->OutIdentifier);
      IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_IDENTIFIER, ValueStr);
      //
      // CHAP_C=<C>
      //
      IScsiGenRandom ((UINT8 *) AuthData->OutChallenge, ISCSI_CHAP_RSP_LEN);
      AuthData->OutChallengeLength = ISCSI_CHAP_RSP_LEN;
      IScsiBinToHex ((UINT8 *) AuthData->OutChallenge, ISCSI_CHAP_RSP_LEN, Challenge, &ChallengeLen);
      IScsiAddKeyValuePair (Pdu, ISCSI_KEY_CHAP_CHALLENGE, Challenge);

      Conn->CHAPStep = ISCSI_CHAP_STEP_FOUR;
    }
    //
    // set the stage transition flag.
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
