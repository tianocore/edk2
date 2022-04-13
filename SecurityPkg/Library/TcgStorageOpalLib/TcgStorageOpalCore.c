/** @file
  Public API for Opal Core library.

Copyright (c) 2016 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TcgStorageOpalLib.h>

#include "TcgStorageOpalLibInternal.h"

#pragma pack(1)
typedef struct {
  UINT8    HardwareReset : 1;
  UINT8    Reserved      : 7;
} TCG_BLOCK_SID_CLEAR_EVENTS;
#pragma pack()

#define TRUSTED_COMMAND_TIMEOUT_NS  ((UINT64) 5 * ((UINT64)(1000000)) * 1000)     // 5 seconds
#define BUFFER_SIZE                 512

/**
  The function performs a Trusted Send of a Buffer containing a TCG_COM_PACKET.

  @param[in]      Sscp                  The input Ssc Protocol.
  @param[in]      MediaId               The input Media id info used by Ssc Protocol.
  @param[in]      SecurityProtocol      Security Protocol
  @param[in]      SpSpecific            Security Protocol Specific
  @param[in]      TransferLength        Transfer Length of Buffer (in bytes) - always a multiple of 512
  @param[in]      Buffer                Address of Data to transfer
  @param[in]      BufferSize            Full Size of Buffer, including space that may be used for padding.

**/
TCG_RESULT
OpalTrustedSend (
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL  *Sscp,
  UINT32                                 MediaId,
  UINT8                                  SecurityProtocol,
  UINT16                                 SpSpecific,
  UINTN                                  TransferLength,
  VOID                                   *Buffer,
  UINTN                                  BufferSize
  )
{
  UINTN       TransferLength512;
  EFI_STATUS  Status;

  //
  // Round transferLength up to a 512-byte multiple
  //
  TransferLength512 = (TransferLength + 511) & ~(UINTN)511;

  if (TransferLength512 > BufferSize) {
    return TcgResultFailureBufferTooSmall;
  }

  ZeroMem ((UINT8 *)Buffer + TransferLength, TransferLength512 - TransferLength);

  Status = Sscp->SendData (
                   Sscp,
                   MediaId,
                   TRUSTED_COMMAND_TIMEOUT_NS,
                   SecurityProtocol,
                   SwapBytes16 (SpSpecific),
                   TransferLength512,
                   Buffer
                   );

  return Status == EFI_SUCCESS ? TcgResultSuccess : TcgResultFailure;
}

/**

  The function performs a Trusted Receive of a Buffer containing a TCG_COM_PACKET.

  @param[in]      Sscp                  The input Ssc Protocol.
  @param[in]      MediaId               The input Media id info used by Ssc Protocol.
  @param[in]      SecurityProtocol      Security Protocol
  @param[in]      SpSpecific            Security Protocol Specific
  @param[in]      Buffer                Address of Data to transfer
  @param[in]      BufferSize            Full Size of Buffer, including space that may be used for padding.
  @param[in]      EstimateTimeCost      Estimate the time needed.

**/
TCG_RESULT
OpalTrustedRecv (
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL  *Sscp,
  UINT32                                 MediaId,
  UINT8                                  SecurityProtocol,
  UINT16                                 SpSpecific,
  VOID                                   *Buffer,
  UINTN                                  BufferSize,
  UINT32                                 EstimateTimeCost
  )
{
  UINTN           TransferLength512;
  UINT32          Tries;
  TCG_COM_PACKET  *ComPacket;
  UINT32          Length;
  UINT32          OutstandingData;
  EFI_STATUS      Status;
  UINTN           TransferSize;

  //
  // Round Buffer Size down to a 512-byte multiple
  //
  TransferLength512 = BufferSize & ~(UINTN)511;
  Tries             = 0;
  ComPacket         = NULL;
  Length            = 0;
  OutstandingData   = 0;

  if (TransferLength512 < sizeof (TCG_COM_PACKET)) {
    DEBUG ((DEBUG_INFO, "transferLength %u too small for ComPacket\n", TransferLength512));
    return TcgResultFailureBufferTooSmall;
  }

  //
  // Some devices respond with Length = 0 and OutstandingData = 1 to indicate that processing is not yet completed,
  // so we need to retry the IF-RECV to get the actual Data.
  // See TCG Core Spec v2 Table 45 IF-RECV ComPacket Field Values Summary
  // This is an arbitrary number of retries, not from the spec.
  //
  // if user input estimate time cost(second level) value bigger than 10s, base on user input value to wait.
  // Else, Use a max timeout of 10 seconds to wait, 5000 tries * 2ms = 10s
  //
  if (EstimateTimeCost > 10) {
    Tries = EstimateTimeCost * 500; // 500 = 1000 * 1000 / 2000;
  } else {
    Tries = 5000;
  }

  while ((Tries--) > 0) {
    ZeroMem (Buffer, BufferSize);
    TransferSize = 0;

    Status = Sscp->ReceiveData (
                     Sscp,
                     MediaId,
                     TRUSTED_COMMAND_TIMEOUT_NS,
                     SecurityProtocol,
                     SwapBytes16 (SpSpecific),
                     TransferLength512,
                     Buffer,
                     &TransferSize
                     );
    if (EFI_ERROR (Status)) {
      return TcgResultFailure;
    }

    if ((SecurityProtocol != TCG_OPAL_SECURITY_PROTOCOL_1) && (SecurityProtocol != TCG_OPAL_SECURITY_PROTOCOL_2)) {
      return TcgResultSuccess;
    }

    if (SpSpecific == TCG_SP_SPECIFIC_PROTOCOL_LEVEL0_DISCOVERY) {
      return TcgResultSuccess;
    }

    ComPacket       = (TCG_COM_PACKET *)Buffer;
    Length          = SwapBytes32 (ComPacket->LengthBE);
    OutstandingData = SwapBytes32 (ComPacket->OutstandingDataBE);

    if ((Length != 0) && (OutstandingData == 0)) {
      return TcgResultSuccess;
    }

    //
    // Delay for 2 ms
    //
    MicroSecondDelay (2000);
  }

  return TcgResultFailure;
}

/**
  The function performs send, recv, check comIDs, check method status action.

  @param[in]      Session           OPAL_SESSION related to this method..
  @param[in]      SendSize          Transfer Length of Buffer (in bytes) - always a multiple of 512
  @param[in]      Buffer            Address of Data to transfer
  @param[in]      BufferSize        Full Size of Buffer, including space that may be used for padding.
  @param[in]      ParseStruct       Structure used to parse received TCG response.
  @param[in]      MethodStatus      Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.
  @param[in]      EstimateTimeCost  Estimate the time need to for the method.
**/
TCG_RESULT
EFIAPI
OpalPerformMethod (
  OPAL_SESSION      *Session,
  UINT32            SendSize,
  VOID              *Buffer,
  UINT32            BufferSize,
  TCG_PARSE_STRUCT  *ParseStruct,
  UINT8             *MethodStatus,
  UINT32            EstimateTimeCost
  )
{
  NULL_CHECK (Session);
  NULL_CHECK (MethodStatus);

  ERROR_CHECK (
    OpalTrustedSend (
      Session->Sscp,
      Session->MediaId,
      TCG_OPAL_SECURITY_PROTOCOL_1,
      Session->OpalBaseComId,
      SendSize,
      Buffer,
      BufferSize
      )
    );

  ERROR_CHECK (
    OpalTrustedRecv (
      Session->Sscp,
      Session->MediaId,
      TCG_OPAL_SECURITY_PROTOCOL_1,
      Session->OpalBaseComId,
      Buffer,
      BufferSize,
      EstimateTimeCost
      )
    );

  ERROR_CHECK (TcgInitTcgParseStruct (ParseStruct, Buffer, BufferSize));
  ERROR_CHECK (TcgCheckComIds (ParseStruct, Session->OpalBaseComId, Session->ComIdExtension));
  ERROR_CHECK (TcgGetMethodStatus (ParseStruct, MethodStatus));

  return TcgResultSuccess;
}

/**
  Trig the block sid action.

  @param[in]      Session            OPAL_SESSION related to this method..
  @param[in]      HardwareReset      Whether need to do hardware reset.

**/
TCG_RESULT
EFIAPI
OpalBlockSid (
  OPAL_SESSION  *Session,
  BOOLEAN       HardwareReset
  )
{
  UINT8                       Buffer[BUFFER_SIZE];
  TCG_BLOCK_SID_CLEAR_EVENTS  *ClearEvents;

  NULL_CHECK (Session);

  //
  // Set Hardware Reset bit
  //
  ClearEvents = (TCG_BLOCK_SID_CLEAR_EVENTS *)&Buffer[0];

  ClearEvents->Reserved      = 0;
  ClearEvents->HardwareReset = HardwareReset;

  return (OpalTrustedSend (
            Session->Sscp,
            Session->MediaId,
            TCG_OPAL_SECURITY_PROTOCOL_2,
            TCG_BLOCKSID_COMID,         // hardcode ComID 0x0005
            1,
            Buffer,
            BUFFER_SIZE
            ));
}

/**

  Reverts device using Admin SP Revert method.

  @param[in]  AdminSpSession      OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY to perform PSID revert.

**/
TCG_RESULT
EFIAPI
OpalPsidRevert (
  OPAL_SESSION  *AdminSpSession
  )
{
  //
  // Now that base comid is known, start Session
  // we'll attempt to start Session as PSID authority
  // verify PSID Authority is defined in ADMIN SP authority table... is this possible?
  //
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;
  UINT8              Buffer[BUFFER_SIZE];
  UINT8              MethodStatus;

  NULL_CHECK (AdminSpSession);

  //
  // Send Revert action on Admin SP
  //
  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buffer, BUFFER_SIZE));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, AdminSpSession->OpalBaseComId, AdminSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, AdminSpSession->TperSessionId, AdminSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, OPAL_UID_ADMIN_SP, OPAL_ADMIN_SP_REVERT_METHOD));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  //
  // Send Revert Method Call
  //
  ERROR_CHECK (OpalPerformMethod (AdminSpSession, Size, Buffer, BUFFER_SIZE, &ParseStruct, &MethodStatus, 0));
  METHOD_STATUS_ERROR_CHECK (MethodStatus, TcgResultFailure);

  return TcgResultSuccess;
}

/**

  Reverts device using Admin SP Revert method.

  @param[in]  AdminSpSession      OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY to perform PSID revert.
  @param[in]  EstimateTimeCost    Estimate the time needed.

**/
TCG_RESULT
OpalPyrite2PsidRevert (
  OPAL_SESSION  *AdminSpSession,
  UINT32        EstimateTimeCost
  )
{
  //
  // Now that base comid is known, start Session
  // we'll attempt to start Session as PSID authority
  // verify PSID Authority is defined in ADMIN SP authority table... is this possible?
  //
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;
  UINT8              Buffer[BUFFER_SIZE];
  UINT8              MethodStatus;

  NULL_CHECK (AdminSpSession);

  //
  // Send Revert action on Admin SP
  //
  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buffer, BUFFER_SIZE));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, AdminSpSession->OpalBaseComId, AdminSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, AdminSpSession->TperSessionId, AdminSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, OPAL_UID_ADMIN_SP, OPAL_ADMIN_SP_REVERT_METHOD));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  //
  // Send Revert Method Call
  //
  ERROR_CHECK (OpalPerformMethod (AdminSpSession, Size, Buffer, BUFFER_SIZE, &ParseStruct, &MethodStatus, EstimateTimeCost));
  METHOD_STATUS_ERROR_CHECK (MethodStatus, TcgResultFailure);

  return TcgResultSuccess;
}

/**

  The function fills in the provided Buffer with the level 0 discovery Header
  of the device specified.

  @param[in]        Session         OPAL_SESSION data.
  @param[in]        BufferSize      Size of Buffer provided (in bytes)
  @param[in]        BuffAddress     Buffer address to fill with Level 0 Discovery response

**/
TCG_RESULT
EFIAPI
OpalRetrieveLevel0DiscoveryHeader (
  OPAL_SESSION  *Session,
  UINTN         BufferSize,
  VOID          *BuffAddress
  )
{
  return (OpalTrustedRecv (
            Session->Sscp,
            Session->MediaId,
            TCG_OPAL_SECURITY_PROTOCOL_1,              // SP
            TCG_SP_SPECIFIC_PROTOCOL_LEVEL0_DISCOVERY, // SP_Specific
            BuffAddress,
            BufferSize,
            0
            ));
}

/**

  The function fills in the provided Buffer with the supported protocol list
  of the device specified.

  @param[in]        Session         OPAL_SESSION data.
  @param[in]        BufferSize      Size of Buffer provided (in bytes)
  @param[in]        BuffAddress     Buffer address to fill with security protocol list

**/
TCG_RESULT
EFIAPI
OpalRetrieveSupportedProtocolList (
  OPAL_SESSION  *Session,
  UINTN         BufferSize,
  VOID          *BuffAddress
  )
{
  return (OpalTrustedRecv (
            Session->Sscp,
            Session->MediaId,
            TCG_SECURITY_PROTOCOL_INFO,    // SP
            TCG_SP_SPECIFIC_PROTOCOL_LIST, // SP_Specific
            BuffAddress,
            BufferSize,
            0
            ));
}

/**
  Starts a session with a security provider (SP).

  If a session is started successfully, the caller must end the session with OpalEndSession when finished
  performing Opal actions.

  @param[in/out]  Session                 OPAL_SESSION to initialize.
  @param[in]      SpId                    Security provider ID to start the session with.
  @param[in]      Write                   Whether the session should be read-only (FALSE) or read/write (TRUE).
  @param[in]      HostChallengeLength     Length of the host challenge.  Length should be 0 if hostChallenge is NULL
  @param[in]      HostChallenge           Host challenge for Host Signing Authority.  If NULL, then no Host Challenge will be sent.
  @param[in]      HostSigningAuthority    Host Signing Authority used for start session.  If NULL, then no Host Signing Authority will be sent.
  @param[in/out]  MethodStatus            Status of the StartSession method; only valid if TcgResultSuccess is returned.

  @return TcgResultSuccess indicates that the function completed without any internal errors.
  The caller must inspect the MethodStatus field to determine whether the method completed successfully.

**/
TCG_RESULT
EFIAPI
OpalStartSession (
  OPAL_SESSION  *Session,
  TCG_UID       SpId,
  BOOLEAN       Write,
  UINT32        HostChallengeLength,
  const VOID    *HostChallenge,
  TCG_UID       HostSigningAuthority,
  UINT8         *MethodStatus
  )
{
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;
  UINT8              Buf[BUFFER_SIZE];
  UINT16             ComIdExtension;
  UINT32             HostSessionId;

  ComIdExtension = 0;
  HostSessionId  = 1;

  NULL_CHECK (Session);
  NULL_CHECK (MethodStatus);

  Session->ComIdExtension = ComIdExtension;
  Session->HostSessionId  = HostSessionId;

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (
    TcgCreateStartSession (
      &CreateStruct,
      &Size,
      Session->OpalBaseComId,
      ComIdExtension,
      HostSessionId,
      SpId,
      Write,
      HostChallengeLength,
      HostChallenge,
      HostSigningAuthority
      )
    );
  ERROR_CHECK (OpalPerformMethod (Session, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));
  if (*MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    return TcgResultSuccess; // return early if method failed - user must check MethodStatus
  }

  if (TcgParseSyncSession (&ParseStruct, Session->OpalBaseComId, ComIdExtension, HostSessionId, &Session->TperSessionId) != TcgResultSuccess) {
    OpalEndSession (Session);
    return TcgResultFailure;
  }

  return TcgResultSuccess;
}

/**
  Close a session opened with OpalStartSession.

  @param[in/out]  Session                 OPAL_SESSION to end.

**/
TCG_RESULT
EFIAPI
OpalEndSession (
  OPAL_SESSION  *Session
  )
{
  UINT8              Buffer[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  UINT32             Size;
  TCG_PARSE_STRUCT   ParseStruct;

  NULL_CHECK (Session);
  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buffer, sizeof (Buffer)));
  ERROR_CHECK (
    TcgCreateEndSession (
      &CreateStruct,
      &Size,
      Session->OpalBaseComId,
      Session->ComIdExtension,
      Session->HostSessionId,
      Session->TperSessionId
      )
    );

  ERROR_CHECK (
    OpalTrustedSend (
      Session->Sscp,
      Session->MediaId,
      TCG_OPAL_SECURITY_PROTOCOL_1,
      Session->OpalBaseComId,
      Size,
      Buffer,
      sizeof (Buffer)
      )
    );

  ERROR_CHECK (
    OpalTrustedRecv (
      Session->Sscp,
      Session->MediaId,
      TCG_OPAL_SECURITY_PROTOCOL_1,
      Session->OpalBaseComId,
      Buffer,
      sizeof (Buffer),
      0
      )
    );

  ERROR_CHECK (TcgInitTcgParseStruct (&ParseStruct, Buffer, sizeof (Buffer)));
  ERROR_CHECK (TcgCheckComIds (&ParseStruct, Session->OpalBaseComId, Session->ComIdExtension));

  ERROR_CHECK (TcgGetNextEndOfSession (&ParseStruct));
  return TcgResultSuccess;
}

/**

  The function retrieves the MSID from the device specified

  @param[in]  AdminSpSession      OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY to perform PSID revert.
  @param[in]  MsidBufferSize      Allocated Buffer Size (in bytes) for MSID allocated by caller
  @param[in]  Msid                Variable Length byte sequence representing MSID of device
  @param[in]  MsidLength          Actual Length of MSID retrieved from device

**/
TCG_RESULT
EFIAPI
OpalGetMsid (
  OPAL_SESSION  *AdminSpSession,
  UINT32        MsidBufferSize,
  UINT8         *Msid,
  UINT32        *MsidLength
  )
{
  //
  // now that base comid is known, start Session
  // we'll attempt to start Session as PSID authority
  // verify PSID Authority is defined in ADMIN SP authority table... is this possible?
  //
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;
  UINT8              MethodStatus;
  UINT32             Col;
  const VOID         *RecvMsid;
  UINT8              Buffer[BUFFER_SIZE];

  NULL_CHECK (AdminSpSession);
  NULL_CHECK (Msid);
  NULL_CHECK (MsidLength);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buffer, BUFFER_SIZE));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, AdminSpSession->OpalBaseComId, AdminSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, AdminSpSession->TperSessionId, AdminSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, OPAL_UID_ADMIN_SP_C_PIN_MSID, TCG_UID_METHOD_GET));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgAddStartList (&CreateStruct));
  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, TCG_CELL_BLOCK_START_COLUMN_NAME));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, OPAL_ADMIN_SP_PIN_COL));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));
  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, TCG_CELL_BLOCK_END_COLUMN_NAME));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, OPAL_ADMIN_SP_PIN_COL));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));
  ERROR_CHECK (TcgAddEndList (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  //
  // Send MSID Method Call
  //
  ERROR_CHECK (OpalPerformMethod (AdminSpSession, Size, Buffer, BUFFER_SIZE, &ParseStruct, &MethodStatus, 0));
  METHOD_STATUS_ERROR_CHECK (MethodStatus, TcgResultFailure);

  ERROR_CHECK (TcgGetNextStartList (&ParseStruct));
  ERROR_CHECK (TcgGetNextStartList (&ParseStruct));
  ERROR_CHECK (TcgGetNextStartName (&ParseStruct));
  ERROR_CHECK (TcgGetNextUINT32 (&ParseStruct, &Col));
  ERROR_CHECK (TcgGetNextByteSequence (&ParseStruct, &RecvMsid, MsidLength));
  ERROR_CHECK (TcgGetNextEndName (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndList (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndList (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndOfData (&ParseStruct));

  if (Col != OPAL_ADMIN_SP_PIN_COL) {
    DEBUG ((DEBUG_INFO, "ERROR: got col %u, expected %u\n", Col, OPAL_ADMIN_SP_PIN_COL));
    return TcgResultFailure;
  }

  if (RecvMsid == NULL) {
    return TcgResultFailure;
  }

  if (MsidBufferSize < *MsidLength) {
    DEBUG ((DEBUG_INFO, "Buffer too small MsidBufferSize: %d MsidLength: %d\n", MsidBufferSize, *MsidLength));
    return TcgResultFailureBufferTooSmall;
  }

  //
  // copy msid into Buffer
  //
  CopyMem (Msid, RecvMsid, *MsidLength);
  return TcgResultSuccess;
}

/**

  The function retrieves the MSID from the device specified

  @param[in]  AdminSpSession              OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_ANYBODY_AUTHORITY
  @param[out] ActiveDataRemovalMechanism  Active Data Removal Mechanism that the device will use for Revert/RevertSP calls.

**/
TCG_RESULT
OpalPyrite2GetActiveDataRemovalMechanism (
  IN  OPAL_SESSION  *AdminSpSession,
  OUT UINT8         *ActiveDataRemovalMechanism
  )
{
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;
  UINT8              MethodStatus;
  UINT32             Col;
  UINT8              RecvActiveDataRemovalMechanism;
  UINT8              Buffer[BUFFER_SIZE];

  NULL_CHECK (AdminSpSession);
  NULL_CHECK (ActiveDataRemovalMechanism);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buffer, BUFFER_SIZE));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, AdminSpSession->OpalBaseComId, AdminSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, AdminSpSession->TperSessionId, AdminSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, OPAL_UID_ADMIN_SP_DATA_REMOVAL_MECHANISM, TCG_UID_METHOD_GET));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgAddStartList (&CreateStruct));
  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, TCG_CELL_BLOCK_START_COLUMN_NAME));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, OPAL_ADMIN_SP_ACTIVE_DATA_REMOVAL_MECHANISM_COL));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));
  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, TCG_CELL_BLOCK_END_COLUMN_NAME));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, OPAL_ADMIN_SP_ACTIVE_DATA_REMOVAL_MECHANISM_COL));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));
  ERROR_CHECK (TcgAddEndList (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  //
  // Send Get Active Data Removal Mechanism Method Call
  //
  ERROR_CHECK (OpalPerformMethod (AdminSpSession, Size, Buffer, BUFFER_SIZE, &ParseStruct, &MethodStatus, 0));
  METHOD_STATUS_ERROR_CHECK (MethodStatus, TcgResultFailure);

  ERROR_CHECK (TcgGetNextStartList (&ParseStruct));
  ERROR_CHECK (TcgGetNextStartList (&ParseStruct));
  ERROR_CHECK (TcgGetNextStartName (&ParseStruct));
  ERROR_CHECK (TcgGetNextUINT32 (&ParseStruct, &Col));
  ERROR_CHECK (TcgGetNextUINT8 (&ParseStruct, &RecvActiveDataRemovalMechanism));
  ERROR_CHECK (TcgGetNextEndName (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndList (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndList (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndOfData (&ParseStruct));

  if (Col != OPAL_ADMIN_SP_ACTIVE_DATA_REMOVAL_MECHANISM_COL) {
    DEBUG ((DEBUG_INFO, "ERROR: got col %u, expected %u\n", Col, OPAL_ADMIN_SP_ACTIVE_DATA_REMOVAL_MECHANISM_COL));
    return TcgResultFailure;
  }

  if (RecvActiveDataRemovalMechanism >= ResearvedMechanism) {
    return TcgResultFailure;
  }

  //
  // Copy active data removal mechanism into Buffer
  //
  CopyMem (ActiveDataRemovalMechanism, &RecvActiveDataRemovalMechanism, sizeof (RecvActiveDataRemovalMechanism));
  return TcgResultSuccess;
}

/**

  The function calls the Admin SP RevertSP method on the Locking SP.  If KeepUserData is True, then the optional parameter
  to keep the user Data is set to True, otherwise the optional parameter is not provided.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY to revertSP
  @param[in]      KeepUserData        Specifies whether or not to keep user Data when performing RevertSP action. True = keeps user Data.
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalAdminRevert (
  OPAL_SESSION  *LockingSpSession,
  BOOLEAN       KeepUserData,
  UINT8         *MethodStatus
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  UINT32             Size;
  TCG_PARSE_STRUCT   ParseStruct;
  TCG_RESULT         Ret;

  NULL_CHECK (LockingSpSession);
  NULL_CHECK (MethodStatus);

  //
  // ReadLocked or WriteLocked must be False (per Opal spec) to guarantee revertSP can keep user Data
  //
  if (KeepUserData) {
    //
    // set readlocked and writelocked to false
    //
    Ret = OpalUpdateGlobalLockingRange (
            LockingSpSession,
            FALSE,
            FALSE,
            MethodStatus
            );

    if ((Ret != TcgResultSuccess) || (*MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
      //
      // bail out
      //
      return Ret;
    }
  }

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, LockingSpSession->OpalBaseComId, LockingSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, LockingSpSession->TperSessionId, LockingSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, TCG_UID_THIS_SP, OPAL_LOCKING_SP_REVERTSP_METHOD));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));

  if (KeepUserData) {
    //
    // optional parameter to keep Data after revert
    //
    ERROR_CHECK (TcgAddStartName (&CreateStruct));
    ERROR_CHECK (TcgAddUINT32 (&CreateStruct, 0x060000));      // weird Value but that's what spec says
    ERROR_CHECK (TcgAddBOOLEAN (&CreateStruct, KeepUserData));
    ERROR_CHECK (TcgAddEndName (&CreateStruct));
  }

  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  //
  // Send RevertSP method call
  //
  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  //
  // Session is immediately ended by device after successful revertsp, so no need to end Session
  //
  if (*MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS) {
    //
    // Caller should take ownership again
    //
    return TcgResultSuccess;
  } else {
    //
    // End Session
    //
    METHOD_STATUS_ERROR_CHECK (*MethodStatus, TcgResultSuccess);     // exit with success on method failure - user must inspect MethodStatus
  }

  return TcgResultSuccess;
}

/**

  The function calls the Admin SP RevertSP method on the Locking SP.  If KeepUserData is True, then the optional parameter
  to keep the user Data is set to True, otherwise the optional parameter is not provided.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY to revertSP
  @param[in]      KeepUserData        Specifies whether or not to keep user Data when performing RevertSP action. True = keeps user Data.
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.
  @param[in]      EstimateTimeCost    Estimate the time needed.

**/
TCG_RESULT
OpalPyrite2AdminRevert (
  OPAL_SESSION  *LockingSpSession,
  BOOLEAN       KeepUserData,
  UINT8         *MethodStatus,
  UINT32        EstimateTimeCost
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  UINT32             Size;
  TCG_PARSE_STRUCT   ParseStruct;
  TCG_RESULT         Ret;

  NULL_CHECK (LockingSpSession);
  NULL_CHECK (MethodStatus);

  //
  // ReadLocked or WriteLocked must be False (per Opal spec) to guarantee revertSP can keep user Data
  //
  if (KeepUserData) {
    //
    // set readlocked and writelocked to false
    //
    Ret = OpalUpdateGlobalLockingRange (
            LockingSpSession,
            FALSE,
            FALSE,
            MethodStatus
            );

    if ((Ret != TcgResultSuccess) || (*MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
      //
      // bail out
      //
      return Ret;
    }
  }

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, LockingSpSession->OpalBaseComId, LockingSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, LockingSpSession->TperSessionId, LockingSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, TCG_UID_THIS_SP, OPAL_LOCKING_SP_REVERTSP_METHOD));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));

  if (KeepUserData) {
    //
    // optional parameter to keep Data after revert
    //
    ERROR_CHECK (TcgAddStartName (&CreateStruct));
    ERROR_CHECK (TcgAddUINT32 (&CreateStruct, 0x060000));      // weird Value but that's what spec says
    ERROR_CHECK (TcgAddBOOLEAN (&CreateStruct, KeepUserData));
    ERROR_CHECK (TcgAddEndName (&CreateStruct));
  }

  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  //
  // Send RevertSP method call
  //
  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, EstimateTimeCost));

  //
  // Session is immediately ended by device after successful revertsp, so no need to end Session
  //
  if (*MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS) {
    //
    // Caller should take ownership again
    //
    return TcgResultSuccess;
  } else {
    //
    // End Session
    //
    METHOD_STATUS_ERROR_CHECK (*MethodStatus, TcgResultSuccess);     // exit with success on method failure - user must inspect MethodStatus
  }

  return TcgResultSuccess;
}

/**

  The function activates the Locking SP.
  Once activated, per Opal spec, the ADMIN SP SID PIN is copied over to the ADMIN1 LOCKING SP PIN.
  If the Locking SP is already enabled, then TcgResultSuccess is returned and no action occurs.

  @param[in]      AdminSpSession      OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_SID_AUTHORITY to activate Locking SP
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalActivateLockingSp (
  OPAL_SESSION  *AdminSpSession,
  UINT8         *MethodStatus
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  UINT32             Size;
  TCG_PARSE_STRUCT   ParseStruct;

  NULL_CHECK (AdminSpSession);
  NULL_CHECK (MethodStatus);

  //
  // Call Activate method on Locking SP
  //
  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, AdminSpSession->OpalBaseComId, AdminSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, AdminSpSession->TperSessionId, AdminSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, OPAL_UID_LOCKING_SP, OPAL_ADMIN_SP_ACTIVATE_METHOD));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  //
  // Send Activate method call
  //
  ERROR_CHECK (OpalPerformMethod (AdminSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));
  METHOD_STATUS_ERROR_CHECK (*MethodStatus, TcgResultSuccess); // exit with success on method failure - user must inspect MethodStatus

  return TcgResultSuccess;
}

/**

  The function sets the PIN column of the specified cpinRowUid (authority) with the newPin Value.

  @param[in/out]  Session                 OPAL_SESSION to set password
  @param[in]      CpinRowUid              UID of row (authority) to update PIN column
  @param[in]      NewPin                  New Pin to set for cpinRowUid specified
  @param[in]      NewPinLength            Length in bytes of newPin
  @param[in/out]  MethodStatus            Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalSetPassword (
  OPAL_SESSION  *Session,
  TCG_UID       CpinRowUid,
  const VOID    *NewPin,
  UINT32        NewPinLength,
  UINT8         *MethodStatus
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;

  NULL_CHECK (Session);
  NULL_CHECK (NewPin);
  NULL_CHECK (MethodStatus);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (
    TcgCreateSetCPin (
      &CreateStruct,
      &Size,
      Session->OpalBaseComId,
      Session->ComIdExtension,
      Session->TperSessionId,
      Session->HostSessionId,
      CpinRowUid,
      NewPin,
      NewPinLength
      )
    );

  ERROR_CHECK (OpalPerformMethod (Session, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));
  // exit with success on method failure - user must inspect MethodStatus
  METHOD_STATUS_ERROR_CHECK (*MethodStatus, TcgResultSuccess);

  return TcgResultSuccess;
}

/**

  The function sets the Enabled column to TRUE for the authorityUid provided and updates the PIN column for the cpinRowUid provided
  using the newPin provided.  AuthorityUid and cpinRowUid should describe the same authority.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY to update
  @param[in]      CpinRowUid          Row UID of C_PIN table of Locking SP to update PIN
  @param[in]      AuthorityUid        UID of Locking SP authority to update Pin column with
  @param[in]      NewPin              New Password used to set Pin column
  @param[in]      NewPinLength        Length in bytes of new password
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalSetLockingSpAuthorityEnabledAndPin (
  OPAL_SESSION  *LockingSpSession,
  TCG_UID       CpinRowUid,
  TCG_UID       AuthorityUid,
  const VOID    *NewPin,
  UINT32        NewPinLength,
  UINT8         *MethodStatus
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;
  TCG_UID            ActiveKey;
  TCG_RESULT         Ret;

  NULL_CHECK (LockingSpSession);
  NULL_CHECK (NewPin);
  NULL_CHECK (MethodStatus);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (
    TcgSetAuthorityEnabled (
      &CreateStruct,
      &Size,
      LockingSpSession->OpalBaseComId,
      LockingSpSession->ComIdExtension,
      LockingSpSession->TperSessionId,
      LockingSpSession->HostSessionId,
      AuthorityUid,
      TRUE
      )
    );

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  if (*MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Send Set Authority error\n"));
    return TcgResultFailure;
  }

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));

  ERROR_CHECK (
    TcgCreateSetCPin (
      &CreateStruct,
      &Size,
      LockingSpSession->OpalBaseComId,
      LockingSpSession->ComIdExtension,
      LockingSpSession->TperSessionId,
      LockingSpSession->HostSessionId,
      CpinRowUid,
      NewPin,
      NewPinLength
      )
    );

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  //
  // allow user1 to set global range to unlocked/locked by modifying ACE_Locking_GlobalRange_SetRdLocked/SetWrLocked
  //
  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (
    TcgCreateSetAce (
      &CreateStruct,
      &Size,
      LockingSpSession->OpalBaseComId,
      LockingSpSession->ComIdExtension,
      LockingSpSession->TperSessionId,
      LockingSpSession->HostSessionId,
      OPAL_LOCKING_SP_ACE_LOCKING_GLOBALRANGE_SET_RDLOCKED,
      OPAL_LOCKING_SP_USER1_AUTHORITY,
      TCG_ACE_EXPRESSION_OR,
      OPAL_LOCKING_SP_ADMINS_AUTHORITY
      )
    );

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  if (*MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Update ACE for RDLOCKED failed\n"));
    return TcgResultFailure;
  }

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (
    TcgCreateSetAce (
      &CreateStruct,
      &Size,
      LockingSpSession->OpalBaseComId,
      LockingSpSession->ComIdExtension,
      LockingSpSession->TperSessionId,
      LockingSpSession->HostSessionId,
      OPAL_LOCKING_SP_ACE_LOCKING_GLOBALRANGE_SET_WRLOCKED,
      OPAL_LOCKING_SP_USER1_AUTHORITY,
      TCG_ACE_EXPRESSION_OR,
      OPAL_LOCKING_SP_ADMINS_AUTHORITY
      )
    );

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  if (*MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Update ACE for WRLOCKED failed\n"));
    return TcgResultFailure;
  }

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (OpalCreateRetrieveGlobalLockingRangeActiveKey (LockingSpSession, &CreateStruct, &Size));
  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  //
  // For Pyrite type SSC, it not supports Active Key.
  // So here add check logic before enable it.
  //
  Ret = OpalParseRetrieveGlobalLockingRangeActiveKey (&ParseStruct, &ActiveKey);
  if (Ret == TcgResultSuccess) {
    ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
    ERROR_CHECK (
      TcgCreateSetAce (
        &CreateStruct,
        &Size,
        LockingSpSession->OpalBaseComId,
        LockingSpSession->ComIdExtension,
        LockingSpSession->TperSessionId,
        LockingSpSession->HostSessionId,
        (ActiveKey == OPAL_LOCKING_SP_K_AES_256_GLOBALRANGE_KEY) ? OPAL_LOCKING_SP_ACE_K_AES_256_GLOBALRANGE_GENKEY : OPAL_LOCKING_SP_ACE_K_AES_128_GLOBALRANGE_GENKEY,
        OPAL_LOCKING_SP_USER1_AUTHORITY,
        TCG_ACE_EXPRESSION_OR,
        OPAL_LOCKING_SP_ADMINS_AUTHORITY
        )
      );

    ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

    if (*MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
      DEBUG ((DEBUG_INFO, "Update ACE for GLOBALRANGE_GENKEY failed\n"));
      //
      // Disable user1 if all permissions are not granted.
      //
      return TcgResultFailure;
    }
  }

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (
    TcgCreateSetAce (
      &CreateStruct,
      &Size,
      LockingSpSession->OpalBaseComId,
      LockingSpSession->ComIdExtension,
      LockingSpSession->TperSessionId,
      LockingSpSession->HostSessionId,
      OPAL_LOCKING_SP_ACE_LOCKING_GLOBALRANGE_GET_ALL,
      OPAL_LOCKING_SP_USER1_AUTHORITY,
      TCG_ACE_EXPRESSION_OR,
      OPAL_LOCKING_SP_ADMINS_AUTHORITY
      )
    );

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  if (*MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Update ACE for OPAL_LOCKING_SP_ACE_LOCKING_GLOBALRANGE_GET_ALL failed\n"));
    return TcgResultFailure;
  }

  return TcgResultSuccess;
}

/**

  The function sets the Enabled column to FALSE for the USER1 authority.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY to disable User1
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalDisableUser (
  OPAL_SESSION  *LockingSpSession,
  UINT8         *MethodStatus
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;

  NULL_CHECK (LockingSpSession);
  NULL_CHECK (MethodStatus);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (
    TcgSetAuthorityEnabled (
      &CreateStruct,
      &Size,
      LockingSpSession->OpalBaseComId,
      LockingSpSession->ComIdExtension,
      LockingSpSession->TperSessionId,
      LockingSpSession->HostSessionId,
      OPAL_LOCKING_SP_USER1_AUTHORITY,
      FALSE
      )
    );

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  return TcgResultSuccess;
}

/**

  The function retrieves the active key of the global locking range
  and calls the GenKey method on the active key retrieved.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP to generate key
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalGlobalLockingRangeGenKey (
  OPAL_SESSION  *LockingSpSession,
  UINT8         *MethodStatus
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;
  TCG_UID            ActiveKey;

  NULL_CHECK (LockingSpSession);
  NULL_CHECK (MethodStatus);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  //
  // retrieve the activekey in order to know which globalrange key to generate
  //
  ERROR_CHECK (OpalCreateRetrieveGlobalLockingRangeActiveKey (LockingSpSession, &CreateStruct, &Size));
  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  METHOD_STATUS_ERROR_CHECK (*MethodStatus, TcgResultSuccess);

  ERROR_CHECK (OpalParseRetrieveGlobalLockingRangeActiveKey (&ParseStruct, &ActiveKey));

  //
  // call genkey on ActiveKey UID
  //
  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, LockingSpSession->OpalBaseComId, LockingSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, LockingSpSession->TperSessionId, LockingSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, ActiveKey, TCG_UID_METHOD_GEN_KEY));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));

  return TcgResultSuccess;
}

/**

  The function updates the ReadLocked and WriteLocked columns of the Global Locking Range.
  This function is required for a user1 authority, since a user1 authority shall only have access to ReadLocked and WriteLocked columns
  (not ReadLockEnabled and WriteLockEnabled columns).

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP to generate key
  @param[in]      ReadLocked          Value to set ReadLocked column for Global Locking Range
  @param[in]      WriteLocked         Value to set WriteLocked column for Global Locking Range
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalUpdateGlobalLockingRange (
  OPAL_SESSION  *LockingSpSession,
  BOOLEAN       ReadLocked,
  BOOLEAN       WriteLocked,
  UINT8         *MethodStatus
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;

  NULL_CHECK (LockingSpSession);
  NULL_CHECK (MethodStatus);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));

  //
  // set global locking range values
  //
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, LockingSpSession->OpalBaseComId, LockingSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, LockingSpSession->TperSessionId, LockingSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, OPAL_LOCKING_SP_LOCKING_GLOBALRANGE, TCG_UID_METHOD_SET));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x01));                       // "Values"
  ERROR_CHECK (TcgAddStartList (&CreateStruct));

  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x07));                       // "ReadLocked"
  ERROR_CHECK (TcgAddBOOLEAN (&CreateStruct, ReadLocked));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));

  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x08));                       // "WriteLocked"
  ERROR_CHECK (TcgAddBOOLEAN (&CreateStruct, WriteLocked));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));

  ERROR_CHECK (TcgAddEndList (&CreateStruct));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));
  METHOD_STATUS_ERROR_CHECK (*MethodStatus, TcgResultSuccess);

  return TcgResultSuccess;
}

/**

  The function updates the RangeStart, RangeLength, ReadLockedEnabled, WriteLockedEnabled, ReadLocked and WriteLocked columns
  of the specified Locking Range.  This function requires admin authority of a locking SP session.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP to generate key
  @param[in]      LockingRangeUid     Locking range UID to set values
  @param[in]      RangeStart          Value to set RangeStart column for Locking Range
  @param[in]      RangeLength         Value to set RangeLength column for Locking Range
  @param[in]      ReadLockEnabled     Value to set readLockEnabled column for Locking Range
  @param[in]      WriteLockEnabled    Value to set writeLockEnabled column for Locking Range
  @param[in]      ReadLocked          Value to set ReadLocked column for Locking Range
  @param[in]      WriteLocked         Value to set WriteLocked column for Locking Range
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalSetLockingRange (
  OPAL_SESSION  *LockingSpSession,
  TCG_UID       LockingRangeUid,
  UINT64        RangeStart,
  UINT64        RangeLength,
  BOOLEAN       ReadLockEnabled,
  BOOLEAN       WriteLockEnabled,
  BOOLEAN       ReadLocked,
  BOOLEAN       WriteLocked,
  UINT8         *MethodStatus
  )
{
  UINT8              Buf[BUFFER_SIZE];
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;

  NULL_CHECK (LockingSpSession);
  NULL_CHECK (MethodStatus);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));

  //
  // set locking range values
  //
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, LockingSpSession->OpalBaseComId, LockingSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, LockingSpSession->TperSessionId, LockingSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, LockingRangeUid, TCG_UID_METHOD_SET));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x01));                        // "Values"
  ERROR_CHECK (TcgAddStartList (&CreateStruct));

  //
  // range start and range Length only apply to non-global locking ranges
  //
  if (LockingRangeUid != OPAL_LOCKING_SP_LOCKING_GLOBALRANGE) {
    ERROR_CHECK (TcgAddStartName (&CreateStruct));
    ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x03));                       // "RangeStart"
    ERROR_CHECK (TcgAddUINT64 (&CreateStruct, RangeStart));
    ERROR_CHECK (TcgAddEndName (&CreateStruct));

    ERROR_CHECK (TcgAddStartName (&CreateStruct));
    ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x04));                       // "RangeLength"
    ERROR_CHECK (TcgAddUINT64 (&CreateStruct, RangeLength));
    ERROR_CHECK (TcgAddEndName (&CreateStruct));
  }

  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x05));                       // "ReadLockEnabled"
  ERROR_CHECK (TcgAddBOOLEAN (&CreateStruct, ReadLockEnabled));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));

  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x06));                       // "WriteLockEnabled"
  ERROR_CHECK (TcgAddBOOLEAN (&CreateStruct, WriteLockEnabled));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));

  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x07));                       // "ReadLocked"
  ERROR_CHECK (TcgAddBOOLEAN (&CreateStruct, ReadLocked));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));

  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, 0x08));                       // "WriteLocked"
  ERROR_CHECK (TcgAddBOOLEAN (&CreateStruct, WriteLocked));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));

  ERROR_CHECK (TcgAddEndList (&CreateStruct));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, MethodStatus, 0));
  // Exit with success on method failure - user must inspect MethodStatus
  METHOD_STATUS_ERROR_CHECK (*MethodStatus, TcgResultSuccess);

  return TcgResultSuccess;
}

/**

  The function populates the CreateStruct with a payload that will retrieve the global locking range active key.
  It is intended to be called with a session that is already started with a valid credential.
  The function does not send the payload.

  @param[in]      Session        OPAL_SESSION to populate command for, needs ComId
  @param[in/out]  CreateStruct   Structure to populate with encoded TCG command
  @param[in/out]  Size           Size in bytes of the command created.

**/
TCG_RESULT
EFIAPI
OpalCreateRetrieveGlobalLockingRangeActiveKey (
  const OPAL_SESSION  *Session,
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT32              *Size
  )
{
  NULL_CHECK (Session);
  NULL_CHECK (CreateStruct);
  NULL_CHECK (Size);

  // Retrieve the activekey in order to know which globalrange key to generate
  ERROR_CHECK (TcgStartComPacket (CreateStruct, Session->OpalBaseComId, Session->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (CreateStruct, Session->TperSessionId, Session->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (CreateStruct, OPAL_LOCKING_SP_LOCKING_GLOBALRANGE, TCG_UID_METHOD_GET));
  ERROR_CHECK (TcgStartParameters (CreateStruct));
  ERROR_CHECK (TcgAddStartList (CreateStruct));
  ERROR_CHECK (TcgAddStartName (CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (CreateStruct, TCG_CELL_BLOCK_START_COLUMN_NAME));
  ERROR_CHECK (TcgAddUINT8 (CreateStruct, 0x0A));         // ActiveKey
  ERROR_CHECK (TcgAddEndName (CreateStruct));
  ERROR_CHECK (TcgAddStartName (CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (CreateStruct, TCG_CELL_BLOCK_END_COLUMN_NAME));
  ERROR_CHECK (TcgAddUINT8 (CreateStruct, 0x0A));
  ERROR_CHECK (TcgAddEndName (CreateStruct));
  ERROR_CHECK (TcgAddEndList (CreateStruct));
  ERROR_CHECK (TcgEndParameters (CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (CreateStruct));
  ERROR_CHECK (TcgEndPacket (CreateStruct));
  ERROR_CHECK (TcgEndComPacket (CreateStruct, Size));

  return TcgResultSuccess;
}

/**

  The function acquires the activeKey specified for the Global Locking Range from the ParseStruct.

  @param[in]      ParseStruct    Structure that contains the device's response with the activekey
  @param[in/out]  ActiveKey      The UID of the active key retrieved

**/
TCG_RESULT
EFIAPI
OpalParseRetrieveGlobalLockingRangeActiveKey (
  TCG_PARSE_STRUCT  *ParseStruct,
  TCG_UID           *ActiveKey
  )
{
  UINT32  ColumnName;

  NULL_CHECK (ParseStruct);
  NULL_CHECK (ActiveKey);

  // parse response
  ERROR_CHECK (TcgGetNextStartList (ParseStruct));
  ERROR_CHECK (TcgGetNextStartList (ParseStruct));
  ERROR_CHECK (TcgGetNextStartName (ParseStruct));
  ERROR_CHECK (TcgGetNextUINT32 (ParseStruct, &ColumnName));
  ERROR_CHECK (TcgGetNextTcgUid (ParseStruct, ActiveKey));
  ERROR_CHECK (TcgGetNextEndName (ParseStruct));
  ERROR_CHECK (TcgGetNextEndList (ParseStruct));
  ERROR_CHECK (TcgGetNextEndList (ParseStruct));
  ERROR_CHECK (TcgGetNextEndOfData (ParseStruct));

  if (ColumnName != 0x0A) {
    DEBUG ((DEBUG_INFO, "Unexpected column name %u (exp 0x0A)\n", ColumnName));
    return TcgResultFailure;
  }

  if ((*ActiveKey != OPAL_LOCKING_SP_K_AES_256_GLOBALRANGE_KEY) && (*ActiveKey != OPAL_LOCKING_SP_K_AES_128_GLOBALRANGE_KEY)) {
    DEBUG ((DEBUG_INFO, "Unexpected gen key %u (exp %u or %u)\n", *ActiveKey, OPAL_LOCKING_SP_K_AES_256_GLOBALRANGE_KEY, OPAL_LOCKING_SP_K_AES_128_GLOBALRANGE_KEY));
    return TcgResultFailure;
  }

  return TcgResultSuccess;
}

/**

  The function retrieves the TryLimit column for the specified rowUid (authority).

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP to retrieve try limit
  @param[in]      RowUid              Row UID of the Locking SP C_PIN table to retrieve TryLimit column
  @param[in/out]  TryLimit            Value from TryLimit column

**/
TCG_RESULT
EFIAPI
OpalGetTryLimit (
  OPAL_SESSION  *LockingSpSession,
  TCG_UID       RowUid,
  UINT32        *TryLimit
  )
{
  TCG_CREATE_STRUCT  CreateStruct;
  TCG_PARSE_STRUCT   ParseStruct;
  UINT32             Size;
  UINT8              MethodStatus;
  UINT8              Buf[BUFFER_SIZE];
  UINT32             Col;

  NULL_CHECK (LockingSpSession);
  NULL_CHECK (TryLimit);

  ERROR_CHECK (TcgInitTcgCreateStruct (&CreateStruct, Buf, sizeof (Buf)));
  ERROR_CHECK (TcgStartComPacket (&CreateStruct, LockingSpSession->OpalBaseComId, LockingSpSession->ComIdExtension));
  ERROR_CHECK (TcgStartPacket (&CreateStruct, LockingSpSession->TperSessionId, LockingSpSession->HostSessionId, 0x0, 0x0, 0x0));
  ERROR_CHECK (TcgStartSubPacket (&CreateStruct, 0x0));
  ERROR_CHECK (TcgStartMethodCall (&CreateStruct, RowUid, TCG_UID_METHOD_GET));
  ERROR_CHECK (TcgStartParameters (&CreateStruct));
  ERROR_CHECK (TcgAddStartList (&CreateStruct));
  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, TCG_CELL_BLOCK_START_COLUMN_NAME));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, OPAL_LOCKING_SP_C_PIN_TRYLIMIT_COL));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));
  ERROR_CHECK (TcgAddStartName (&CreateStruct));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, TCG_CELL_BLOCK_END_COLUMN_NAME));
  ERROR_CHECK (TcgAddUINT8 (&CreateStruct, OPAL_LOCKING_SP_C_PIN_TRYLIMIT_COL));
  ERROR_CHECK (TcgAddEndName (&CreateStruct));
  ERROR_CHECK (TcgAddEndList (&CreateStruct));
  ERROR_CHECK (TcgEndParameters (&CreateStruct));
  ERROR_CHECK (TcgEndMethodCall (&CreateStruct));
  ERROR_CHECK (TcgEndSubPacket (&CreateStruct));
  ERROR_CHECK (TcgEndPacket (&CreateStruct));
  ERROR_CHECK (TcgEndComPacket (&CreateStruct, &Size));

  ERROR_CHECK (OpalPerformMethod (LockingSpSession, Size, Buf, sizeof (Buf), &ParseStruct, &MethodStatus, 0));
  METHOD_STATUS_ERROR_CHECK (MethodStatus, TcgResultFailure);

  ERROR_CHECK (TcgGetNextStartList (&ParseStruct));
  ERROR_CHECK (TcgGetNextStartList (&ParseStruct));
  ERROR_CHECK (TcgGetNextStartName (&ParseStruct));
  ERROR_CHECK (TcgGetNextUINT32 (&ParseStruct, &Col));
  ERROR_CHECK (TcgGetNextUINT32 (&ParseStruct, TryLimit));
  ERROR_CHECK (TcgGetNextEndName (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndList (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndList (&ParseStruct));
  ERROR_CHECK (TcgGetNextEndOfData (&ParseStruct));

  if (Col != OPAL_LOCKING_SP_C_PIN_TRYLIMIT_COL) {
    DEBUG ((DEBUG_INFO, "ERROR: got col %u, expected %u\n", Col, OPAL_LOCKING_SP_C_PIN_TRYLIMIT_COL));
    return TcgResultFailure;
  }

  return TcgResultSuccess;
}

/**

  Get the support attribute info.

  @param[in]      Session             OPAL_SESSION with OPAL_UID_LOCKING_SP to retrieve info.
  @param[out]     SupportedAttributes Return the support attribute info.
  @param[out]     OpalBaseComId       Return the base com id info.

**/
TCG_RESULT
EFIAPI
OpalGetSupportedAttributesInfo (
  IN  OPAL_SESSION                 *Session,
  OUT OPAL_DISK_SUPPORT_ATTRIBUTE  *SupportedAttributes,
  OUT UINT16                       *OpalBaseComId
  )
{
  UINT8                             Buffer[BUFFER_SIZE];
  TCG_SUPPORTED_SECURITY_PROTOCOLS  *SupportedProtocols;
  TCG_LEVEL0_DISCOVERY_HEADER       *DiscoveryHeader;
  OPAL_LEVEL0_FEATURE_DESCRIPTOR    *Feat;
  OPAL_LEVEL0_FEATURE_DESCRIPTOR    *Feat2;
  UINTN                             Size;
  UINTN                             Size2;

  NULL_CHECK (Session);
  NULL_CHECK (SupportedAttributes);
  NULL_CHECK (OpalBaseComId);

  ZeroMem (Buffer, BUFFER_SIZE);
  ZeroMem (SupportedAttributes, sizeof (OPAL_DISK_SUPPORT_ATTRIBUTE));
  ASSERT (sizeof (Buffer) >= sizeof (TCG_SUPPORTED_SECURITY_PROTOCOLS));

  //
  // Retrieve supported protocols verify security protocol 1 is supported
  //
  SupportedProtocols = (TCG_SUPPORTED_SECURITY_PROTOCOLS *)Buffer;

  //
  // Get list of supported protocols
  //
  if (OpalRetrieveSupportedProtocolList (Session, sizeof (TCG_SUPPORTED_SECURITY_PROTOCOLS), SupportedProtocols) == TcgResultFailure) {
    DEBUG ((DEBUG_INFO, "OpalRetrieveSupportedProtocolList failed\n"));
    return TcgResultFailure;
  }

  SupportedAttributes->Sp1        = TcgIsProtocolSupported (SupportedProtocols, TCG_OPAL_SECURITY_PROTOCOL_1);
  SupportedAttributes->Sp2        = TcgIsProtocolSupported (SupportedProtocols, TCG_OPAL_SECURITY_PROTOCOL_2);
  SupportedAttributes->SpIeee1667 = TcgIsProtocolSupported (SupportedProtocols, TCG_SECURITY_PROTOCOL_IEEE_1667);

  DEBUG ((
    DEBUG_INFO,
    "Supported Protocols: Sp1 %d Sp2: %d SpIeee1667 %d \n",
    SupportedAttributes->Sp1,
    SupportedAttributes->Sp2,
    SupportedAttributes->SpIeee1667
    ));

  //
  // Perform level 0 discovery and assign desired feature info to Opal Disk structure
  //
  ZeroMem (Buffer, BUFFER_SIZE);
  if (OpalRetrieveLevel0DiscoveryHeader (Session, BUFFER_SIZE, Buffer) == TcgResultFailure) {
    DEBUG ((DEBUG_INFO, "OpalRetrieveLevel0DiscoveryHeader failed\n"));
    return TcgResultFailure;
  }

  //
  // Check for required feature descriptors
  //
  DiscoveryHeader = (TCG_LEVEL0_DISCOVERY_HEADER *)Buffer;

  Size                          = 0;
  Feat                          = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_OPAL_SSC_V2_0_0, &Size);
  SupportedAttributes->OpalSsc2 = (Feat != NULL);

  *OpalBaseComId = TCG_RESERVED_COMID;

  //
  // Check Opal SCC V2 has valid settings for SID C_PIN on revert
  //
  if (SupportedAttributes->OpalSsc2 && (Size >= sizeof (OPAL_SSCV2_FEATURE_DESCRIPTOR))) {
    //
    // Want opposite polarity b/c Value is greater than a bit, but we only care about non-zero vs zero
    //
    SupportedAttributes->InitCpinIndicator = (Feat->OpalSscV2.InitialCPINSIDPIN == 0);
    SupportedAttributes->CpinUponRevert    = (Feat->OpalSscV2.CPINSIDPINRevertBehavior == 0);
    DEBUG ((
      DEBUG_INFO,
      "Opal SSC V2 InitCpinIndicator %d  CpinUponRevert %d \n",
      SupportedAttributes->InitCpinIndicator,
      SupportedAttributes->CpinUponRevert
      ));
    *OpalBaseComId = SwapBytes16 (Feat->OpalSscV2.BaseComdIdBE);
  }

  Size                             = 0;
  Feat                             = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_OPAL_SSC_LITE, &Size);
  SupportedAttributes->OpalSscLite = (Feat != NULL);

  if ((Feat != NULL) && (Size >= sizeof (OPAL_SSCLITE_FEATURE_DESCRIPTOR))) {
    if (*OpalBaseComId == TCG_RESERVED_COMID) {
      //
      // Pin values used always match up with ComId used
      //
      *OpalBaseComId                         = SwapBytes16 (Feat->OpalSscLite.BaseComdIdBE);
      SupportedAttributes->InitCpinIndicator = (Feat->OpalSscV2.InitialCPINSIDPIN == 0);
      SupportedAttributes->CpinUponRevert    = (Feat->OpalSscV2.CPINSIDPINRevertBehavior == 0);
      DEBUG ((
        DEBUG_INFO,
        "Opal SSC Lite InitCpinIndicator %d  CpinUponRevert %d \n",
        SupportedAttributes->InitCpinIndicator,
        SupportedAttributes->CpinUponRevert
        ));
    }
  }

  //
  // For some pyrite 2.0 device, it contains both pyrite 1.0 and 2.0 feature data.
  // so here try to get data from pyrite 2.0 feature data first.
  //
  Size  = 0;
  Feat  = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_PYRITE_SSC, &Size);
  Size2 = 0;
  Feat2 = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_PYRITE_SSC_V2_0_0, &Size2);
  if ((Feat2 != NULL) && (Size2 >= sizeof (PYRITE_SSCV2_FEATURE_DESCRIPTOR))) {
    SupportedAttributes->PyriteSscV2 = TRUE;
    if (*OpalBaseComId == TCG_RESERVED_COMID) {
      *OpalBaseComId                         = SwapBytes16 (Feat2->PyriteSscV2.BaseComdIdBE);
      SupportedAttributes->InitCpinIndicator = (Feat2->PyriteSscV2.InitialCPINSIDPIN == 0);
      SupportedAttributes->CpinUponRevert    = (Feat2->PyriteSscV2.CPINSIDPINRevertBehavior == 0);
      DEBUG ((
        DEBUG_INFO,
        "Pyrite SSC V2 InitCpinIndicator %d  CpinUponRevert %d \n",
        SupportedAttributes->InitCpinIndicator,
        SupportedAttributes->CpinUponRevert
        ));
    }
  } else {
    SupportedAttributes->PyriteSsc = (Feat != NULL);
    if ((Feat != NULL) && (Size >= sizeof (PYRITE_SSC_FEATURE_DESCRIPTOR))) {
      if (*OpalBaseComId == TCG_RESERVED_COMID) {
        *OpalBaseComId                         = SwapBytes16 (Feat->PyriteSsc.BaseComdIdBE);
        SupportedAttributes->InitCpinIndicator = (Feat->PyriteSsc.InitialCPINSIDPIN == 0);
        SupportedAttributes->CpinUponRevert    = (Feat->PyriteSsc.CPINSIDPINRevertBehavior == 0);
        DEBUG ((
          DEBUG_INFO,
          "Pyrite SSC InitCpinIndicator %d  CpinUponRevert %d \n",
          SupportedAttributes->InitCpinIndicator,
          SupportedAttributes->CpinUponRevert
          ));
      }
    }
  }

  Size                          = 0;
  Feat                          = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_OPAL_SSC_V1_0_0, &Size);
  SupportedAttributes->OpalSsc1 = (Feat != NULL);
  if ((Feat != NULL) && (Size >= sizeof (OPAL_SSCV1_FEATURE_DESCRIPTOR))) {
    if (*OpalBaseComId == TCG_RESERVED_COMID) {
      *OpalBaseComId = SwapBytes16 (Feat->OpalSscV1.BaseComdIdBE);
    }
  }

  Size = 0;
  Feat = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_LOCKING, &Size);
  if ((Feat != NULL) && (Size >= sizeof (TCG_LOCKING_FEATURE_DESCRIPTOR))) {
    SupportedAttributes->MediaEncryption = Feat->Locking.MediaEncryption;
    DEBUG ((DEBUG_INFO, "SupportedAttributes->MediaEncryption 0x%X \n", SupportedAttributes->MediaEncryption));
  }

  Size = 0;
  Feat = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_BLOCK_SID, &Size);
  if ((Feat != NULL) && (Size >= sizeof (TCG_BLOCK_SID_FEATURE_DESCRIPTOR))) {
    SupportedAttributes->BlockSid = TRUE;
    DEBUG ((DEBUG_INFO, "BlockSid Supported!!! Current Status is 0x%X \n", Feat->BlockSid.SIDBlockedState));
  } else {
    DEBUG ((DEBUG_INFO, "BlockSid Unsupported!!!"));
  }

  Size = 0;
  Feat = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_DATA_REMOVAL, &Size);
  if ((Feat != NULL) && (Size >= sizeof (DATA_REMOVAL_FEATURE_DESCRIPTOR))) {
    SupportedAttributes->DataRemoval = TRUE;
    DEBUG ((DEBUG_INFO, "DataRemoval Feature Supported!\n"));
    DEBUG ((DEBUG_INFO, "Operation Processing = 0x%x\n", Feat->DataRemoval.OperationProcessing));
    DEBUG ((DEBUG_INFO, "RemovalMechanism = 0x%x\n", Feat->DataRemoval.RemovalMechanism));
    DEBUG ((DEBUG_INFO, "BIT0 :: Format = 0x%x, Time = 0x%x\n", Feat->DataRemoval.FormatBit0, SwapBytes16 (Feat->DataRemoval.TimeBit0)));
    DEBUG ((DEBUG_INFO, "BIT1 :: Format = 0x%x, Time = 0x%x\n", Feat->DataRemoval.FormatBit1, SwapBytes16 (Feat->DataRemoval.TimeBit1)));
    DEBUG ((DEBUG_INFO, "BIT2 :: Format = 0x%x, Time = 0x%x\n", Feat->DataRemoval.FormatBit2, SwapBytes16 (Feat->DataRemoval.TimeBit2)));
    DEBUG ((DEBUG_INFO, "BIT3 :: Format = 0x%x, Time = 0x%x\n", Feat->DataRemoval.FormatBit3, SwapBytes16 (Feat->DataRemoval.TimeBit3)));
    DEBUG ((DEBUG_INFO, "BIT4 :: Format = 0x%x, Time = 0x%x\n", Feat->DataRemoval.FormatBit4, SwapBytes16 (Feat->DataRemoval.TimeBit4)));
  }

  DEBUG ((DEBUG_INFO, "Base COMID 0x%04X \n", *OpalBaseComId));

  return TcgResultSuccess;
}

/**

  Get the support attribute info.

  @param[in]      Session             OPAL_SESSION with OPAL_UID_LOCKING_SP to retrieve info.
  @param[in/out]  LockingFeature      Return the Locking info.

**/
TCG_RESULT
EFIAPI
OpalGetLockingInfo (
  OPAL_SESSION                    *Session,
  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockingFeature
  )
{
  UINT8                           Buffer[BUFFER_SIZE];
  TCG_LEVEL0_DISCOVERY_HEADER     *DiscoveryHeader;
  OPAL_LEVEL0_FEATURE_DESCRIPTOR  *Feat;
  UINTN                           Size;

  NULL_CHECK (Session);
  NULL_CHECK (LockingFeature);

  ZeroMem (Buffer, BUFFER_SIZE);
  ASSERT (sizeof (Buffer) >= sizeof (TCG_SUPPORTED_SECURITY_PROTOCOLS));

  if (OpalRetrieveLevel0DiscoveryHeader (Session, BUFFER_SIZE, Buffer) == TcgResultFailure) {
    DEBUG ((DEBUG_INFO, "OpalRetrieveLevel0DiscoveryHeader failed\n"));
    return TcgResultFailure;
  }

  DiscoveryHeader = (TCG_LEVEL0_DISCOVERY_HEADER *)Buffer;

  Size = 0;
  Feat = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, TCG_FEATURE_LOCKING, &Size);
  if ((Feat != NULL) && (Size >= sizeof (TCG_LOCKING_FEATURE_DESCRIPTOR))) {
    CopyMem (LockingFeature, &Feat->Locking, sizeof (TCG_LOCKING_FEATURE_DESCRIPTOR));
  }

  return TcgResultSuccess;
}

/**

  Get the descriptor for the specific feature code.

  @param[in]      Session             OPAL_SESSION with OPAL_UID_LOCKING_SP to retrieve info.
  @param[in]      FeatureCode         The feature code user request.
  @param[in, out] DataSize            The data size.
  @param[out]     Data                The data buffer used to save the feature descriptor.

**/
TCG_RESULT
OpalGetFeatureDescriptor (
  IN     OPAL_SESSION  *Session,
  IN     UINT16        FeatureCode,
  IN OUT UINTN         *DataSize,
  OUT    VOID          *Data
  )
{
  UINT8                           Buffer[BUFFER_SIZE];
  TCG_LEVEL0_DISCOVERY_HEADER     *DiscoveryHeader;
  OPAL_LEVEL0_FEATURE_DESCRIPTOR  *Feat;
  UINTN                           Size;

  NULL_CHECK (Session);
  NULL_CHECK (DataSize);
  NULL_CHECK (Data);

  ZeroMem (Buffer, BUFFER_SIZE);
  ASSERT (sizeof (Buffer) >= sizeof (TCG_SUPPORTED_SECURITY_PROTOCOLS));

  if (OpalRetrieveLevel0DiscoveryHeader (Session, BUFFER_SIZE, Buffer) == TcgResultFailure) {
    DEBUG ((DEBUG_INFO, "OpalRetrieveLevel0DiscoveryHeader failed\n"));
    return TcgResultFailure;
  }

  DiscoveryHeader = (TCG_LEVEL0_DISCOVERY_HEADER *)Buffer;

  Size = 0;
  Feat = (OPAL_LEVEL0_FEATURE_DESCRIPTOR *)TcgGetFeature (DiscoveryHeader, FeatureCode, &Size);
  if (Feat != NULL) {
    if (Size > *DataSize) {
      *DataSize = Size;
      return TcgResultFailureBufferTooSmall;
    }

    *DataSize = Size;
    CopyMem (Data, Feat, Size);
  }

  return TcgResultSuccess;
}

/**

  The function determines whether or not all of the requirements for the Opal Feature (not full specification)
  are met by the specified device.

  @param[in]      SupportedAttributes     Opal device attribute.

**/
BOOLEAN
EFIAPI
OpalFeatureSupported (
  OPAL_DISK_SUPPORT_ATTRIBUTE  *SupportedAttributes
  )
{
  NULL_CHECK (SupportedAttributes);

  if (SupportedAttributes->Sp1 == 0) {
    return FALSE;
  }

  if ((SupportedAttributes->OpalSscLite == 0) &&
      (SupportedAttributes->OpalSsc1 == 0) &&
      (SupportedAttributes->OpalSsc2 == 0) &&
      (SupportedAttributes->PyriteSsc == 0) &&
      (SupportedAttributes->PyriteSscV2 == 0)
      )
  {
    return FALSE;
  }

  return TRUE;
}

/**

  The function returns whether or not the device is Opal Enabled.
  TRUE means that the device is partially or fully locked.
  This will perform a Level 0 Discovery and parse the locking feature descriptor

  @param[in]      SupportedAttributes     Opal device attribute.
  @param[in]      LockingFeature          Opal device locking status.


**/
BOOLEAN
EFIAPI
OpalFeatureEnabled (
  OPAL_DISK_SUPPORT_ATTRIBUTE     *SupportedAttributes,
  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockingFeature
  )
{
  NULL_CHECK (SupportedAttributes);
  NULL_CHECK (LockingFeature);

  if (!OpalFeatureSupported (SupportedAttributes)) {
    return FALSE;
  }

  if (LockingFeature->LockingSupported && LockingFeature->LockingEnabled) {
    return TRUE;
  }

  return FALSE;
}

/**

  The function returns whether or not the device is Opal Locked.
  TRUE means that the device is partially or fully locked.
  This will perform a Level 0 Discovery and parse the locking feature descriptor

  @param[in]      SupportedAttributes     Opal device attribute.
  @param[in]      LockingFeature          Opal device locking status.

**/
BOOLEAN
OpalDeviceLocked (
  OPAL_DISK_SUPPORT_ATTRIBUTE     *SupportedAttributes,
  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockingFeature
  )
{
  NULL_CHECK (SupportedAttributes);
  NULL_CHECK (LockingFeature);

  if (!OpalFeatureEnabled (SupportedAttributes, LockingFeature)) {
    return FALSE;
  }

  return LockingFeature->Locked;
}
