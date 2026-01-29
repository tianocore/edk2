/** @file
  Provide functions to provide tcg storage core spec related functions.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/TcgStorageCoreLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

typedef struct {
  UINT16                                  FeatureCode;
  TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER    *Feature;
  UINTN                                   FeatureSize;
} TCG_FIND_FEATURE_CTX;

/**
  Returns a human-readable string representing a method status return code.

  @param[in]  MethodStatus   Method status to translate to a string


  @retval   return the string info.
**/
CHAR8 *
EFIAPI
TcgMethodStatusString (
  UINT8  MethodStatus
  )
{
  switch (MethodStatus) {
    #define C(status)  case TCG_METHOD_STATUS_CODE_ ## status: return #status
    C (SUCCESS);
    C (NOT_AUTHORIZED);
    C (OBSOLETE);
    C (SP_BUSY);
    C (SP_FAILED);
    C (SP_DISABLED);
    C (SP_FROZEN);
    C (NO_SESSIONS_AVAILABLE);
    C (UNIQUENESS_CONFLICT);
    C (INSUFFICIENT_SPACE);
    C (INSUFFICIENT_ROWS);
    C (INVALID_PARAMETER);
    C (OBSOLETE2);
    C (OBSOLETE3);
    C (TPER_MALFUNCTION);
    C (TRANSACTION_FAILURE);
    C (RESPONSE_OVERFLOW);
    C (AUTHORITY_LOCKED_OUT);
    C (FAIL);
    #undef C
  }

  return "unknown";
}

/**
  adds call token and method Header (invoking id, and method id).

  @param    CreateStruct             The input create structure.
  @param    InvokingId               Invoking id.
  @param    MethodId                 Method id.

**/
TCG_RESULT
EFIAPI
TcgStartMethodCall (
  TCG_CREATE_STRUCT  *CreateStruct,
  TCG_UID            InvokingId,
  TCG_UID            MethodId
  )
{
  NULL_CHECK (CreateStruct);

  if ((CreateStruct->ComPacket == NULL) ||
      (CreateStruct->CurPacket == NULL) ||
      (CreateStruct->CurSubPacket == NULL)
      )
  {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  ERROR_CHECK (TcgAddCall (CreateStruct));
  ERROR_CHECK (TcgAddTcgUid (CreateStruct, InvokingId));
  ERROR_CHECK (TcgAddTcgUid (CreateStruct, MethodId));

  return TcgResultSuccess;
}

/**
  Adds START LIST token.

  @param    CreateStruct        The input create structure.

**/
TCG_RESULT
EFIAPI
TcgStartParameters (
  TCG_CREATE_STRUCT  *CreateStruct
  )
{
  NULL_CHECK (CreateStruct);

  if ((CreateStruct->ComPacket == NULL) ||
      (CreateStruct->CurPacket == NULL) ||
      (CreateStruct->CurSubPacket == NULL)
      )
  {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  return TcgAddStartList (CreateStruct);
}

/**
  Adds END LIST token.

  @param    CreateStruct        The input create structure.

**/
TCG_RESULT
EFIAPI
TcgEndParameters (
  TCG_CREATE_STRUCT  *CreateStruct
  )
{
  NULL_CHECK (CreateStruct);

  if ((CreateStruct->ComPacket == NULL) ||
      (CreateStruct->CurPacket == NULL) ||
      (CreateStruct->CurSubPacket == NULL)
      )
  {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  return TcgAddEndList (CreateStruct);
}

/**
  Adds END Data token and method list.

  @param    CreateStruct        The input create structure.

**/
TCG_RESULT
EFIAPI
TcgEndMethodCall (
  TCG_CREATE_STRUCT  *CreateStruct
  )
{
  NULL_CHECK (CreateStruct);

  if ((CreateStruct->ComPacket == NULL) ||
      (CreateStruct->CurPacket == NULL) ||
      (CreateStruct->CurSubPacket == NULL)
      )
  {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  ERROR_CHECK (TcgAddEndOfData (CreateStruct));

  ERROR_CHECK (TcgAddStartList (CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (CreateStruct, 0x00));   // expected to complete properly
  ERROR_CHECK (TcgAddUINT8 (CreateStruct, 0x00));   // reserved
  ERROR_CHECK (TcgAddUINT8 (CreateStruct, 0x00));   // reserved
  ERROR_CHECK (TcgAddEndList (CreateStruct));

  return TcgResultSuccess;
}

/**
  Retrieves the comID and Extended comID of the ComPacket in the Tcg response.
  It is intended to be used to confirm the received Tcg response is intended for user that received it.

  @param [in]        ParseStruct        Structure used to parse received TCG response.
  @param [in/out]    ComId              comID retrieved from received ComPacket.
  @param [in/out]    ComIdExtension     Extended comID retrieved from received ComPacket

**/
TCG_RESULT
EFIAPI
TcgGetComIds (
  const TCG_PARSE_STRUCT  *ParseStruct,
  UINT16                  *ComId,
  UINT16                  *ComIdExtension
  )
{
  NULL_CHECK (ParseStruct);
  NULL_CHECK (ComId);
  NULL_CHECK (ComIdExtension);

  if (ParseStruct->ComPacket == NULL) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p\n", ParseStruct->ComPacket));
    return TcgResultFailureInvalidAction;
  }

  *ComId          = SwapBytes16 (ParseStruct->ComPacket->ComIDBE);
  *ComIdExtension = SwapBytes16 (ParseStruct->ComPacket->ComIDExtensionBE);

  return TcgResultSuccess;
}

/**
  Checks if the ComIDs of the response match the expected values.

  @param[in]   ParseStruct               Structure used to parse received TCG response
  @param[in]   ExpectedComId             Expected comID
  @param[in]   ExpectedComIdExtension    Expected extended comID

**/
TCG_RESULT
EFIAPI
TcgCheckComIds (
  const TCG_PARSE_STRUCT  *ParseStruct,
  UINT16                  ExpectedComId,
  UINT16                  ExpectedComIdExtension
  )
{
  UINT16  ParseComId;
  UINT16  ParseComIdExtension;

  ERROR_CHECK (TcgGetComIds (ParseStruct, &ParseComId, &ParseComIdExtension));
  if ((ParseComId != ExpectedComId) || (ParseComIdExtension != ExpectedComIdExtension)) {
    DEBUG ((DEBUG_INFO, "Com ID: Actual 0x%02X Expected 0x%02X\n", ParseComId, ExpectedComId));
    DEBUG ((DEBUG_INFO, "Extended Com ID: 0x%02X Expected 0x%02X\n", ParseComIdExtension, ExpectedComIdExtension));
    return TcgResultFailure;
  }

  return TcgResultSuccess;
}

/**
 Returns the method status of the current subpacket.  Does not affect the current position
 in the ComPacket.  In other words, it can be called whenever you have a valid SubPacket.

 @param [in/out]  ParseStruct       Structure used to parse received TCG response
 @param [in/out]  MethodStatus      Method status retrieved of the current SubPacket

**/
TCG_RESULT
EFIAPI
TcgGetMethodStatus (
  const TCG_PARSE_STRUCT  *ParseStruct,
  UINT8                   *MethodStatus
  )
{
  TCG_PARSE_STRUCT  TmpParseStruct;
  TCG_TOKEN         TcgToken;
  UINT8             Reserved1, Reserved2;

  NULL_CHECK (ParseStruct);
  NULL_CHECK (MethodStatus);

  if ((ParseStruct->ComPacket == NULL) ||
      (ParseStruct->CurPacket == NULL) ||
      (ParseStruct->CurSubPacket == NULL)
      )
  {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", ParseStruct->ComPacket, ParseStruct->CurPacket, ParseStruct->CurSubPacket));
    return TcgResultFailureInvalidAction;
  }

  // duplicate ParseStruct, then don't need to "reset" location cur ptr
  CopyMem (&TmpParseStruct, ParseStruct, sizeof (TCG_PARSE_STRUCT));

  // method status list exists after the end method call in the subpacket
  // skip tokens until ENDDATA is found
  do {
    ERROR_CHECK (TcgGetNextToken (&TmpParseStruct, &TcgToken));
  } while (TcgToken.Type != TcgTokenTypeEndOfData);

  // only reach here if enddata is found
  // at this point, the curptr is pointing at method status list beginning
  ERROR_CHECK (TcgGetNextStartList (&TmpParseStruct));
  ERROR_CHECK (TcgGetNextUINT8 (&TmpParseStruct, MethodStatus));
  ERROR_CHECK (TcgGetNextUINT8 (&TmpParseStruct, &Reserved1));
  ERROR_CHECK (TcgGetNextUINT8 (&TmpParseStruct, &Reserved2));
  ERROR_CHECK (TcgGetNextEndList (&TmpParseStruct));

  if (Reserved1 != 0) {
    DEBUG ((DEBUG_INFO, "Method status reserved1 = 0x%02X (expected 0)\n", Reserved1));
    return TcgResultFailure;
  }

  if (Reserved2 != 0) {
    DEBUG ((DEBUG_INFO, "Method status reserved2 = 0x%02X (expected 0)\n", Reserved1));
    return TcgResultFailure;
  }

  return TcgResultSuccess;
}

/**
  Return the toke type string info.

  @param    Type       Input the type info.

  @retval   Return the string for this type.

**/
CHAR8 *
EFIAPI
TcgTokenTypeString (
  TCG_TOKEN_TYPE  Type
  )
{
  switch (Type) {
    case TcgTokenTypeReserved: return "Reserved";
    case TcgTokenTypeTinyAtom: return "Tiny Atom";
    case TcgTokenTypeShortAtom: return "Short Atom";
    case TcgTokenTypeMediumAtom: return "Medium Atom";
    case TcgTokenTypeLongAtom: return "Long Atom";
    case TcgTokenTypeStartList: return "Start List";
    case TcgTokenTypeEndList: return "End List";
    case TcgTokenTypeStartName: return "Start Name";
    case TcgTokenTypeEndName: return "End Name";
    case TcgTokenTypeCall: return "Call";
    case TcgTokenTypeEndOfData: return "End of Data";
    case TcgTokenTypeEndOfSession: return "End of Session";
    case TcgTokenTypeStartTransaction: return "Start Transaction";
    case TcgTokenTypeEndTransaction: return "End Transaction";
    case TcgTokenTypeEmptyAtom: return "Empty atom";
  }

  return "Unknown";
}

/**

  Adds Start Session call to the data structure.  This creates the entire ComPacket structure and
  returns the size of the entire compacket in the size parameter.

  @param [in/out]    CreateStruct               Structure used to add the start session call
  @param [in/out]    Size                       Describes the size of the entire ComPacket (header and payload). Filled out by function.
  @param [in]        ComId                      ComID for the ComPacket
  @param [in]        ComIdExtension             Extended ComID for the ComPacket
  @param [in]        HostSessionId              Host Session ID
  @param [in]        SpId                       Security Provider to start session with
  @param [in]        Write                      Write option for start session.  TRUE = start session requests write access
  @param [in]        HostChallengeLength        Length of the host challenge.  Length should be 0 if hostChallenge is NULL
  @param [in]        HostChallenge              Host challenge for Host Signing Authority.  If NULL, then no Host Challenge shall be sent.
  @param [in]        HostSigningAuthority       Host Signing Authority used for start session.  If NULL, then no Host Signing Authority shall be sent.

**/
TCG_RESULT
EFIAPI
TcgCreateStartSession (
  TCG_CREATE_STRUCT  *CreateStruct,
  UINT32             *Size,
  UINT16             ComId,
  UINT16             ComIdExtension,
  UINT32             HostSessionId,
  TCG_UID            SpId,
  BOOLEAN            Write,
  UINT32             HostChallengeLength,
  const VOID         *HostChallenge,
  TCG_UID            HostSigningAuthority
  )
{
  ERROR_CHECK (TcgStartComPacket (CreateStruct, ComId, ComIdExtension));
  ERROR_CHECK (TcgStartPacket (CreateStruct, 0x0, 0x0, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (CreateStruct, TCG_UID_SMUID, TCG_UID_SM_START_SESSION));
  ERROR_CHECK (TcgStartParameters (CreateStruct));
  ERROR_CHECK (TcgAddUINT32 (CreateStruct, HostSessionId));
  ERROR_CHECK (TcgAddTcgUid (CreateStruct, SpId));
  ERROR_CHECK (TcgAddBOOLEAN (CreateStruct, Write));

  // optional parameters
  if ((HostChallenge != NULL) && (HostChallengeLength != 0)) {
    ERROR_CHECK (TcgAddStartName (CreateStruct));
    ERROR_CHECK (TcgAddUINT8 (CreateStruct, 0x00));   // TODO Create Enum for Method Optional Parameters?
    ERROR_CHECK (TcgAddByteSequence (CreateStruct, HostChallenge, HostChallengeLength, FALSE));
    ERROR_CHECK (TcgAddEndName (CreateStruct));
  }

  // optional parameters
  if (HostSigningAuthority != 0) {
    ERROR_CHECK (TcgAddStartName (CreateStruct));
    ERROR_CHECK (TcgAddUINT8 (CreateStruct, 0x03));   // TODO Create Enum for Method Optional Parameters?
    ERROR_CHECK (TcgAddTcgUid (CreateStruct, HostSigningAuthority));
    ERROR_CHECK (TcgAddEndName (CreateStruct));
  }

  ERROR_CHECK (TcgEndParameters (CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (CreateStruct));
  ERROR_CHECK (TcgEndPacket (CreateStruct));
  ERROR_CHECK (TcgEndComPacket (CreateStruct, Size));

  return TcgResultSuccess;
}

/**
 Parses the Sync Session response contained in the parseStruct to retrieve Tper session ID.  If the Sync Session response
 parameters do not match the comID, extended ComID and host session ID then a failure is returned.

 @param[in/out]   ParseStruct          Structure used to parse received TCG response, contains Sync Session response.
 @param[in]       ComId                Expected ComID that is compared to actual ComID of response
 @param[in]       ComIdExtension       Expected Extended ComID that is compared to actual Extended ComID of response
 @param[in]       HostSessionId        Expected Host Session ID that is compared to actual  Host Session ID of response
 @param[in/out]   TperSessionId        Tper Session ID retrieved from the Sync Session response.

**/
TCG_RESULT
EFIAPI
TcgParseSyncSession (
  const TCG_PARSE_STRUCT  *ParseStruct,
  UINT16                  ComId,
  UINT16                  ComIdExtension,
  UINT32                  HostSessionId,
  UINT32                  *TperSessionId
  )
{
  UINT8             MethodStatus;
  TCG_PARSE_STRUCT  TmpParseStruct;
  UINT16            ParseComId;
  UINT16            ParseExtComId;
  TCG_UID           InvokingUID;
  TCG_UID           MethodUID;
  UINT32            RecvHostSessionId;

  NULL_CHECK (ParseStruct);
  NULL_CHECK (TperSessionId);

  CopyMem (&TmpParseStruct, ParseStruct, sizeof (TCG_PARSE_STRUCT));

  // verify method status is good
  ERROR_CHECK (TcgGetMethodStatus (&TmpParseStruct, &MethodStatus));
  METHOD_STATUS_ERROR_CHECK (MethodStatus, TcgResultFailure);

  // verify comids
  ERROR_CHECK (TcgGetComIds (&TmpParseStruct, &ParseComId, &ParseExtComId));

  if ((ComId != ParseComId) || (ComIdExtension != ParseExtComId)) {
    DEBUG ((DEBUG_INFO, "unmatched comid (exp: 0x%X recv: 0x%X) or comid extension (exp: 0x%X recv: 0x%X)\n", ComId, ParseComId, ComIdExtension, ParseExtComId));
    return TcgResultFailure;
  }

  ERROR_CHECK (TcgGetNextCall (&TmpParseStruct));
  ERROR_CHECK (TcgGetNextTcgUid (&TmpParseStruct, &InvokingUID));
  ERROR_CHECK (TcgGetNextTcgUid (&TmpParseStruct, &MethodUID));
  ERROR_CHECK (TcgGetNextStartList (&TmpParseStruct));
  ERROR_CHECK (TcgGetNextUINT32 (&TmpParseStruct, &RecvHostSessionId));
  ERROR_CHECK (TcgGetNextUINT32 (&TmpParseStruct, TperSessionId));
  ERROR_CHECK (TcgGetNextEndList (&TmpParseStruct));
  ERROR_CHECK (TcgGetNextEndOfData (&TmpParseStruct));

  if (InvokingUID != TCG_UID_SMUID) {
    DEBUG ((DEBUG_INFO, "Invoking UID did not match UID_SMUID\n"));
    return TcgResultFailure;
  }

  if (MethodUID != TCG_UID_SM_SYNC_SESSION) {
    DEBUG ((DEBUG_INFO, "Method UID did not match UID_SM_SYNC_SESSION\n"));
    return TcgResultFailure;
  }

  if (HostSessionId != RecvHostSessionId) {
    DEBUG ((DEBUG_INFO, "unmatched HostSessionId (exp: 0x%X recv: 0x%X)\n", HostSessionId, RecvHostSessionId));
    return TcgResultFailure;
  }

  return TcgResultSuccess;
}

/**

  Creates ComPacket with EndSession.
  This assumes a start session has already been opened.

  @param  [in/out]    CreateStruct        Structure used to add Endsession
  @param  [in/out]    Size                Describes the size of the entire ComPacket (header and payload). Filled out by function.
  @param  [in]        ComId               ComID for the ComPacket
  @param  [in]        ComIdExtension      Extended ComID for the ComPacket
  @param  [in]        HostSessionId         Host Session ID for the Packet
  @param  [in]        TpSessionId         Tper Session ID for the Packet

**/
TCG_RESULT
EFIAPI
TcgCreateEndSession (
  TCG_CREATE_STRUCT  *CreateStruct,
  UINT32             *Size,
  UINT16             ComId,
  UINT16             ComIdExtension,
  UINT32             HostSessionId,
  UINT32             TpSessionId
  )
{
  ERROR_CHECK (TcgStartComPacket (CreateStruct, ComId, ComIdExtension));
  ERROR_CHECK (TcgStartPacket (CreateStruct, TpSessionId, HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (CreateStruct, 0x0));
  ERROR_CHECK (TcgAddEndOfSession (CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (CreateStruct));
  ERROR_CHECK (TcgEndPacket (CreateStruct));
  ERROR_CHECK (TcgEndComPacket (CreateStruct, Size));

  return TcgResultSuccess;
}

/**
  Set start method.

  @param     CreateStruct   Input create structure.
  @param     Row            Input the row info.
  @param     ColumnNumber   the column info.

**/
TCG_RESULT
EFIAPI
TcgStartMethodSet (
  TCG_CREATE_STRUCT  *CreateStruct,
  TCG_UID            Row,
  UINT32             ColumnNumber
  )
{
  ERROR_CHECK (TcgStartMethodCall (CreateStruct, Row, TCG_UID_METHOD_SET));
  ERROR_CHECK (TcgStartParameters (CreateStruct));
  ERROR_CHECK (TcgAddStartName (CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (CreateStruct, 0x01)); // "Values"
  ERROR_CHECK (TcgAddStartList (CreateStruct));
  ERROR_CHECK (TcgAddStartName (CreateStruct));
  ERROR_CHECK (TcgAddUINT32 (CreateStruct, ColumnNumber));
  return TcgResultSuccess;
}

/**
  Set end method.

  @param     CreateStruct  Input create structure.

**/
TCG_RESULT
EFIAPI
TcgEndMethodSet (
  TCG_CREATE_STRUCT  *CreateStruct
  )
{
  ERROR_CHECK (TcgAddEndName (CreateStruct));
  ERROR_CHECK (TcgAddEndList (CreateStruct));
  ERROR_CHECK (TcgAddEndName (CreateStruct));
  ERROR_CHECK (TcgEndParameters (CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (CreateStruct));
  return TcgResultSuccess;
}

/**
  Creates ComPacket with a Method call that sets the PIN column for the row specified.
  This assumes a start session has already been opened with the desired SP.

  @param [in/out]   CreateStruct           Structure used to add method call.
  @param [in/out]   Size                   Describes the size of the entire ComPacket (header and payload). Filled out by function.
  @param [in]       ComId                  ComID for the ComPacket
  @param [in]       ComIdExtension         Extended ComID for the ComPacket
  @param [in]       TperSession            Tper Session ID for the Packet
  @param [in]       HostSession            Host Session ID for the Packet
  @param [in]       SidRow                 UID of row of current SP to set PIN column
  @param [in]       Password               value of PIN to set
  @param [in]       PasswordSize           Size of PIN

**/
TCG_RESULT
EFIAPI
TcgCreateSetCPin (
  TCG_CREATE_STRUCT  *CreateStruct,
  UINT32             *Size,
  UINT16             ComId,
  UINT16             ComIdExtension,
  UINT32             TperSession,
  UINT32             HostSession,
  TCG_UID            SidRow,
  const VOID         *Password,
  UINT32             PasswordSize
  )
{
  // set new SID Password
  ERROR_CHECK (TcgStartComPacket (CreateStruct, ComId, ComIdExtension));
  ERROR_CHECK (TcgStartPacket (CreateStruct, TperSession, HostSession, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodSet (CreateStruct, SidRow, 0x03)); // "PIN"
  ERROR_CHECK (TcgAddByteSequence (CreateStruct, Password, PasswordSize, FALSE));
  ERROR_CHECK (TcgEndMethodSet (CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (CreateStruct));
  ERROR_CHECK (TcgEndPacket (CreateStruct));
  ERROR_CHECK (TcgEndComPacket (CreateStruct, Size));
  return TcgResultSuccess;
}

/**
 Creates ComPacket with a Method call that sets the "Enabled" column for the row specified using the value specified.
 This assumes a start session has already been opened with the desired SP.

 @param [in/out]  CreateStruct          Structure used to add method call
 @param [in/out]  Size                  Describes the size of the entire ComPacket (header and payload). Filled out by function.
 @param [in]      ComId                 ComID for the ComPacket
 @param [in]      ComIdExtension        Extended ComID for the ComPacket
 @param [in]      TperSession           Tper Session ID for the Packet
 @param [in]      HostSession           Host Session ID for the Packet
 @param [in]      AuthorityUid          Authority UID to modify the "Enabled" column for
 @param [in]      Enabled               Value to set the "Enabled" column to

**/
TCG_RESULT
EFIAPI
TcgSetAuthorityEnabled (
  TCG_CREATE_STRUCT  *CreateStruct,
  UINT32             *Size,
  UINT16             ComId,
  UINT16             ComIdExtension,
  UINT32             TperSession,
  UINT32             HostSession,
  TCG_UID            AuthorityUid,
  BOOLEAN            Enabled
  )
{
  ERROR_CHECK (TcgStartComPacket (CreateStruct, ComId, ComIdExtension));
  ERROR_CHECK (TcgStartPacket (CreateStruct, TperSession, HostSession, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodSet (CreateStruct, AuthorityUid, 0x05)); // "Enabled"
  ERROR_CHECK (TcgAddBOOLEAN (CreateStruct, Enabled));
  ERROR_CHECK (TcgEndMethodSet (CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (CreateStruct));
  ERROR_CHECK (TcgEndPacket (CreateStruct));
  ERROR_CHECK (TcgEndComPacket (CreateStruct, Size));
  return TcgResultSuccess;
}

/**
  Create set ace.

  @param     CreateStruct      Input create structure.
  @param     Size              size info.
  @param     ComId             ComId info.
  @param     ComIdExtension    ComId extension info.
  @param     TperSession       Tper session data.
  @param     HostSession       Host session data.
  @param     AceRow            Ace row info.
  @param     Authority1        Authority 1 info.
  @param     LogicalOperator   Logical operator info.
  @param     Authority2        Authority 2 info.

  @retval    Return the action result.

**/
TCG_RESULT
EFIAPI
TcgCreateSetAce (
  TCG_CREATE_STRUCT  *CreateStruct,
  UINT32             *Size,
  UINT16             ComId,
  UINT16             ComIdExtension,
  UINT32             TperSession,
  UINT32             HostSession,
  TCG_UID            AceRow,
  TCG_UID            Authority1,
  BOOLEAN            LogicalOperator,
  TCG_UID            Authority2
  )
{
  UINT8  HalfUidAuthorityObjectRef[4];
  UINT8  HalfUidBooleanAce[4];

  HalfUidAuthorityObjectRef[0] = 0x0;
  HalfUidAuthorityObjectRef[1] = 0x0;
  HalfUidAuthorityObjectRef[2] = 0xC;
  HalfUidAuthorityObjectRef[3] = 0x5;

  HalfUidBooleanAce[0] = 0x0;
  HalfUidBooleanAce[1] = 0x0;
  HalfUidBooleanAce[2] = 0x4;
  HalfUidBooleanAce[3] = 0xE;

  ERROR_CHECK (TcgStartComPacket (CreateStruct, ComId, ComIdExtension));
  ERROR_CHECK (TcgStartPacket (CreateStruct, TperSession, HostSession, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodSet (CreateStruct, AceRow, 0x03));     // "BooleanExpr"
  ERROR_CHECK (TcgAddStartList (CreateStruct));
  ERROR_CHECK (TcgAddStartName (CreateStruct));
  ERROR_CHECK (TcgAddByteSequence (CreateStruct, HalfUidAuthorityObjectRef, sizeof (HalfUidAuthorityObjectRef), FALSE));
  ERROR_CHECK (TcgAddTcgUid (CreateStruct, Authority1));
  ERROR_CHECK (TcgAddEndName (CreateStruct));
  ERROR_CHECK (TcgAddStartName (CreateStruct));
  ERROR_CHECK (TcgAddByteSequence (CreateStruct, HalfUidAuthorityObjectRef, sizeof (HalfUidAuthorityObjectRef), FALSE));
  ERROR_CHECK (TcgAddTcgUid (CreateStruct, Authority2));
  ERROR_CHECK (TcgAddEndName (CreateStruct));

  ERROR_CHECK (TcgAddStartName (CreateStruct));
  ERROR_CHECK (TcgAddByteSequence (CreateStruct, HalfUidBooleanAce, sizeof (HalfUidBooleanAce), FALSE));
  ERROR_CHECK (TcgAddBOOLEAN (CreateStruct, LogicalOperator));
  ERROR_CHECK (TcgAddEndName (CreateStruct));
  ERROR_CHECK (TcgAddEndList (CreateStruct));
  ERROR_CHECK (TcgEndMethodSet (CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (CreateStruct));
  ERROR_CHECK (TcgEndPacket (CreateStruct));
  ERROR_CHECK (TcgEndComPacket (CreateStruct, Size));
  return TcgResultSuccess;
}

/**
  Enum level 0 discovery.

  @param     DiscoveryHeader   Discovery header.
  @param     Callback          Callback function.
  @param     Context           The context for the function.

  @retval    return true if the callback return TRUE, else return FALSE.

**/
BOOLEAN
EFIAPI
TcgEnumLevel0Discovery (
  const TCG_LEVEL0_DISCOVERY_HEADER  *DiscoveryHeader,
  TCG_LEVEL0_ENUM_CALLBACK           Callback,
  VOID                               *Context
  )
{
  UINT32                                BytesLeft;
  const UINT8                           *DiscoveryBufferPtr;
  UINT32                                FeatLength;
  TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER  *Feat;

  //
  // Total bytes including descriptors but not including the Length field
  //
  BytesLeft = SwapBytes32 (DiscoveryHeader->LengthBE);

  //
  // If discovery Header is not valid, exit
  //
  if (BytesLeft == 0) {
    return FALSE;
  }

  //
  // Subtract the Length of the Header, except the Length field, which is not included
  //
  BytesLeft -= (sizeof (TCG_LEVEL0_DISCOVERY_HEADER) - sizeof (DiscoveryHeader->LengthBE));

  //
  // Move ptr to first descriptor
  //
  DiscoveryBufferPtr = (const UINT8 *)DiscoveryHeader + sizeof (TCG_LEVEL0_DISCOVERY_HEADER);

  while (BytesLeft > sizeof (TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER)) {
    //
    // Pointer to beginning of descriptor (including common Header)
    //
    Feat = (TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER *)DiscoveryBufferPtr;

    FeatLength = Feat->Length + sizeof (TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER);

    //
    // Not enough bytes left for Feature descriptor
    //
    if (BytesLeft < FeatLength) {
      break;
    }

    //
    // Report the Feature to the callback
    //
    if (Callback (DiscoveryHeader, Feat, FeatLength, Context)) {
      return TRUE;
    }

    //
    // Descriptor Length only describes Data after common Header
    //
    BytesLeft          -= FeatLength;
    DiscoveryBufferPtr += FeatLength;
  }

  return FALSE;
}

/**
  The callback function for Get Feature function.

  @param     DiscoveryHeader   Input discovery header.
  @param     Feature           Input Feature.
  @param     FeatureSize       Input Feature size.
  @param     Context           The context.

**/
BOOLEAN
EFIAPI
TcgFindFeatureCallback (
  const TCG_LEVEL0_DISCOVERY_HEADER     *DiscoveryHeader,
  TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER  *Feature,
  UINTN                                 FeatureSize,
  VOID                                  *Context
  )
{
  TCG_FIND_FEATURE_CTX  *FindCtx;

  FindCtx = (TCG_FIND_FEATURE_CTX *)Context;
  if ( SwapBytes16 (Feature->FeatureCode_BE) == FindCtx->FeatureCode ) {
    FindCtx->Feature     = Feature;
    FindCtx->FeatureSize = FeatureSize;
    return TRUE; // done enumerating features
  }

  return FALSE; // continue enumerating
}

/**
  Get Feature code from the header.

  @param     DiscoveryHeader    The discovery header.
  @param     FeatureCode        return the Feature code.
  @param     FeatureSize        return the Feature size.

  @retval    return the Feature code data.
**/
TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER *
EFIAPI
TcgGetFeature (
  const TCG_LEVEL0_DISCOVERY_HEADER  *DiscoveryHeader,
  UINT16                             FeatureCode,
  UINTN                              *FeatureSize
  )
{
  TCG_FIND_FEATURE_CTX  FindCtx;

  FindCtx.FeatureCode = FeatureCode;
  FindCtx.Feature     = NULL;
  FindCtx.FeatureSize = 0;

  TcgEnumLevel0Discovery (DiscoveryHeader, TcgFindFeatureCallback, &FindCtx);
  if (FeatureSize != NULL) {
    *FeatureSize = FindCtx.FeatureSize;
  }

  return FindCtx.Feature;
}

/**
  Determines if the protocol provided is part of the provided supported protocol list.

  @param[in]  ProtocolList     Supported protocol list to investigate
  @param[in]  Protocol         Protocol value to determine if supported

  @return TRUE = protocol is supported, FALSE = protocol is not supported
**/
BOOLEAN
EFIAPI
TcgIsProtocolSupported (
  const TCG_SUPPORTED_SECURITY_PROTOCOLS  *ProtocolList,
  UINT16                                  Protocol
  )
{
  UINT16  Index;
  UINT16  ListLength;

  ListLength = SwapBytes16 (ProtocolList->ListLength_BE);

  if (ListLength > sizeof (ProtocolList->List)) {
    DEBUG ((DEBUG_INFO, "WARNING: list Length is larger than max allowed Value; truncating\n"));
    ListLength = sizeof (ProtocolList->List);
  }

  for (Index = 0; Index < ListLength; Index++) {
    if (ProtocolList->List[Index] == Protocol) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Check whether lock or not.

  @param     Discovery

  @retval    TRUE if lock, FALSE if not lock.
**/
BOOLEAN
EFIAPI
TcgIsLocked (
  const TCG_LEVEL0_DISCOVERY_HEADER  *Discovery
  )
{
  UINTN                           Size;
  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockDescriptor;

  Size           = 0;
  LockDescriptor = (TCG_LOCKING_FEATURE_DESCRIPTOR *)TcgGetFeature (Discovery, TCG_FEATURE_LOCKING, &Size);

  if ((LockDescriptor != NULL) && (Size >= sizeof (*LockDescriptor))) {
    DEBUG ((DEBUG_INFO, "locked: %d\n", LockDescriptor->Locked));
    return LockDescriptor->Locked;
  }

  //
  // Descriptor was not found
  //
  return FALSE;
}
