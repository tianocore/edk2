/** @file
  The shared head file for iSCSI driver.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISCSI_IMPL_H_
#define _ISCSI_IMPL_H_

#include <Uefi.h>

#include <IndustryStandard/Dhcp.h>

#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiConfigAccess.h>

#include <Protocol/Ip6.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Dhcp6.h>
#include <Protocol/Dns4.h>
#include <Protocol/Dns6.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Tcp6.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/Ip6Config.h>

#include <Protocol/AuthenticationInfo.h>
#include <Protocol/IScsiInitiatorName.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/AdapterInformation.h>
#include <Protocol/NetworkInterfaceIdentifier.h>

#include <Library/BaseCryptLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Library/SafeIntLib.h>
#include <Library/TcpIoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/EventGroup.h>
#include <Guid/Acpi.h>

#include "IScsiConfigNVDataStruc.h"
#include "IScsiDriver.h"
#include "IScsiProto.h"
#include "IScsiCHAP.h"
#include "IScsiDhcp.h"
#include "IScsiDhcp6.h"

#include "IScsiIbft.h"
#include "IScsiMisc.h"
#include "IScsiDns.h"
#include "IScsiConfig.h"

#define ISCSI_AUTH_INITIAL        0

#define ISCSI_SESSION_SIGNATURE   SIGNATURE_32 ('I', 'S', 'S', 'N')
///
/// 10 seconds
///
#define ISCSI_GET_MAPPING_TIMEOUT 100000000U
///
/// 3 seconds
///
#define ISCSI_WAIT_IPSEC_TIMEOUT  30000000U

struct _ISCSI_SESSION {
  UINT32                      Signature;

  ISCSI_DRIVER_DATA           *Private;
  ISCSI_ATTEMPT_CONFIG_NVDATA *ConfigData;

  UINT8                       AuthType;
  union {
    ISCSI_CHAP_AUTH_DATA      CHAP;
  } AuthData;

  UINT8                       State;

  UINT8                       Isid[6];
  UINT16                      Tsih;

  UINT32                      CmdSN;
  UINT32                      ExpCmdSN;
  UINT32                      MaxCmdSN;

  UINT32                      InitiatorTaskTag;
  UINT16                      NextCid;

  LIST_ENTRY                  Conns;
  UINT32                      NumConns;

  LIST_ENTRY                  TcbList;

  //
  // Session-wide parameters
  //
  UINT16                      TargetPortalGroupTag;
  UINT32                      MaxConnections;
  BOOLEAN                     InitialR2T;
  BOOLEAN                     ImmediateData;
  UINT32                      MaxBurstLength;
  UINT32                      FirstBurstLength;
  UINT32                      DefaultTime2Wait;
  UINT32                      DefaultTime2Retain;
  UINT16                      MaxOutstandingR2T;
  BOOLEAN                     DataPDUInOrder;
  BOOLEAN                     DataSequenceInOrder;
  UINT8                       ErrorRecoveryLevel;
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

  UINT8             AuthStep;

  BOOLEAN           PartialReqSent;
  BOOLEAN           PartialRspRcvd;

  BOOLEAN           TransitInitiated;
  BOOLEAN           ParamNegotiated;

  UINT16            Cid;
  UINT32            ExpStatSN;

  //
  // Queues...
  //
  NET_BUF_QUEUE     RspQue;

  BOOLEAN           Ipv6Flag;
  TCP_IO            TcpIo;

  //
  // Connection-only parameters.
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

struct _ISCSI_DRIVER_DATA {
  UINT32                          Signature;
  EFI_HANDLE                      Image;
  EFI_HANDLE                      Controller;
  ISCSI_PRIVATE_PROTOCOL          IScsiIdentifier;

  EFI_EVENT                       ExitBootServiceEvent;

  EFI_EXT_SCSI_PASS_THRU_PROTOCOL IScsiExtScsiPassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE     ExtScsiPassThruMode;
  EFI_HANDLE                      ExtScsiPassThruHandle;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_HANDLE                      ChildHandle;
  ISCSI_SESSION                   *Session;
};

#endif
