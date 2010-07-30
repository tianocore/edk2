/** @file
  The header file of IScsiImpl.c.

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_IMPL_H_
#define _ISCSI_IMPL_H_

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/EventGroup.h>
#include <Guid/Acpi.h>

#include "IScsiCommon.h"
#include "IScsiDriver.h"
#include "IScsiInitiatorName.h"
#include "ComponentName.h"
#include "IScsiConfigNVDataStruc.h"
#include "IScsiExtScsiPassThru.h"
#include "IScsiProto.h"
#include "IScsiMisc.h"
#include "IScsiCHAP.h"
#include "IScsiConfig.h"
#include "IScsiDhcp.h"
#include "IScsiTcp4Io.h"
#include "IScsiIbft.h"


#define ISCSI_SESSION_SIGNATURE SIGNATURE_32 ('I', 'S', 'S', 'N')

struct _ISCSI_SESSION {
  UINT32                    Signature;

  ISCSI_SESSION_CONFIG_DATA ConfigData;
  ISCSI_CHAP_AUTH_DATA      AuthData;

  CHAR8                     InitiatorName[ISCSI_NAME_MAX_SIZE];
  UINTN                     InitiatorNameLength;
  UINT8                     State;

  UINT8                     Isid[6];
  UINT16                    Tsih;

  UINT32                    CmdSN;
  UINT32                    ExpCmdSN;
  UINT32                    MaxCmdSN;

  UINT32                    InitiatorTaskTag;
  UINT16                    NextCid;

  LIST_ENTRY                Conns;
  UINT32                    NumConns;

  LIST_ENTRY                TcbList;

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
};

#define ISCSI_CONNECTION_SIGNATURE  SIGNATURE_32 ('I', 'S', 'C', 'N')

struct _ISCSI_CONNECTION {
  UINT32            Signature;
  LIST_ENTRY        Link;

  EFI_EVENT         TimeoutEvent;

  ISCSI_SESSION     *Session;

  UINT8             State;
  UINT8             CurrentStage;
  UINT8             NextStage;

  UINT8             CHAPStep;

  BOOLEAN           PartialReqSent;
  BOOLEAN           PartialRspRcvd;

  BOOLEAN           TransitInitiated;

  UINT16            Cid;
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
};

#define ISCSI_DRIVER_DATA_SIGNATURE SIGNATURE_32 ('I', 'S', 'D', 'A')

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

struct _ISCSI_DRIVER_DATA {
  UINT32                          Signature;
  EFI_HANDLE                      Image;
  EFI_HANDLE                      Controller;
  ISCSI_PRIVATE_PROTOCOL          IScsiIdentifier;
  EFI_HANDLE                      ChildHandle;
  EFI_EVENT                       ExitBootServiceEvent;

  EFI_EXT_SCSI_PASS_THRU_PROTOCOL IScsiExtScsiPassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE     ExtScsiPassThruMode;
  EFI_HANDLE                      ExtScsiPassThruHandle;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;

  ISCSI_SESSION                   Session;
};

#endif
