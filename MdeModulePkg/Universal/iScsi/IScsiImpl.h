/*++

Copyright (c)  2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IScsiImpl.h

Abstract:

--*/

#ifndef _ISCSI_IMPL_H_
#define _ISCSI_IMPL_H_

#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include "IScsiCommon.h"
#include "IScsiDriver.h"
#include "IScsiConfigNVDataStruc.h"
#include "IScsiExtScsiPassThru.h"
#include "IScsiProto.h"
#include "IScsiCHAP.h"
#include "IScsiDhcp.h"
#include "IScsiTcp4Io.h"
#include "IScsiIbft.h"
#include "IScsiMisc.h"
#include "IScsiConfig.h"

#define ISCSI_SESSION_SIGNATURE EFI_SIGNATURE_32 ('I', 'S', 'S', 'N')

typedef struct _ISCSI_SESSION {
  UINT32                    Signature;

  ISCSI_SESSION_CONFIG_DATA ConfigData;
  ISCSI_CHAP_AUTH_DATA      AuthData;

  CHAR8                     InitiatorName[ISCSI_NAME_MAX_SIZE];
  UINTN                     InitiatorNameLength;
  UINT8                     State;

  UINT8                     ISID[6];
  UINT16                    TSIH;

  UINT32                    CmdSN;
  UINT32                    ExpCmdSN;
  UINT32                    MaxCmdSN;

  UINT32                    InitiatorTaskTag;
  UINT16                    NextCID;

  NET_LIST_ENTRY            Conns;
  UINT32                    NumConns;

  NET_LIST_ENTRY            TcbList;

  //
  // session-wide parameters
  //
  UINT16                    TargetPortalGroupTag;
  UINT32                    MaxConnections;
  BOOLEAN                   InitialR2T;
  BOOLEAN                   ImmediateData;
  UINT32                    MaxBurstLength;
  UINT32                    FirstBurstLength;
  UINT32                    DefaultTime2Wait;
  UINT32                    DefaultTime2Retain;
  UINT16                    MaxOutstandingR2T;
  BOOLEAN                   DataPDUInOrder;
  BOOLEAN                   DataSequenceInOrder;
  UINT8                     ErrorRecoveryLevel;
} ISCSI_SESSION;

#define ISCSI_CONNECTION_SIGNATURE  EFI_SIGNATURE_32 ('I', 'S', 'C', 'N')

typedef struct _ISCSI_CONNECTION {
  UINT32            Signature;
  NET_LIST_ENTRY    Link;

  EFI_EVENT         TimeoutEvent;

  ISCSI_SESSION     *Session;

  UINT8             State;
  UINT8             CurrentStage;
  UINT8             NextStage;

  UINT8             CHAPStep;

  BOOLEAN           PartialReqSent;
  BOOLEAN           PartialRspRcvd;

  BOOLEAN           TransitInitiated;

  UINT16            CID;
  UINT32            ExpStatSN;

  //
  // queues...
  //
  NET_BUF_QUEUE     RspQue;

  TCP4_IO           Tcp4Io;

  //
  // connection-only parameters
  //
  UINT32            MaxRecvDataSegmentLength;
  ISCSI_DIGEST_TYPE HeaderDigest;
  ISCSI_DIGEST_TYPE DataDigest;
} ISCSI_CONNECTION;

#define ISCSI_DRIVER_DATA_SIGNATURE EFI_SIGNATURE_32 ('I', 'S', 'D', 'A')

#define ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU(PassThru) \
  CR ( \
  PassThru, \
  ISCSI_DRIVER_DATA, \
  IScsiExtScsiPassThru, \
  ISCSI_DRIVER_DATA_SIGNATURE \
  )
#define ISCSI_DRIVER_DATA_FROM_IDENTIFIER(Identifier) \
  CR ( \
  Identifier, \
  ISCSI_DRIVER_DATA, \
  IScsiIdentifier, \
  ISCSI_DRIVER_DATA_SIGNATURE \
  )
#define ISCSI_DRIVER_DATA_FROM_SESSION(s) \
  CR ( \
  s, \
  ISCSI_DRIVER_DATA, \
  Session, \
  ISCSI_DRIVER_DATA_SIGNATURE \
  )

typedef struct _ISCSI_DRIVER_DATA {
  UINT32                          Signature;
  EFI_HANDLE                      Image;
  EFI_HANDLE                      Controller;
  ISCSI_PRIVATE_PROTOCOL          IScsiIdentifier;

  EFI_EVENT                       ExitBootServiceEvent;

  EFI_EXT_SCSI_PASS_THRU_PROTOCOL IScsiExtScsiPassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE     ExtScsiPassThruMode;
  EFI_HANDLE                      ExtScsiPassThruHandle;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;

  ISCSI_SESSION                   Session;
} ISCSI_DRIVER_DATA;

#endif
