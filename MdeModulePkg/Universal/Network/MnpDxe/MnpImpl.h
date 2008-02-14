/** @file

Copyright (c) 2005 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MnpImpl.h

Abstract:


**/

#ifndef _MNP_IMPL_H_
#define _MNP_IMPL_H_

#include "MnpDriver.h"

#define NET_ETHER_FCS_SIZE            4

#define MNP_SYS_POLL_INTERVAL         (50 * TICKS_PER_MS)   // 50 milliseconds
#define MNP_TIMEOUT_CHECK_INTERVAL    (50 * TICKS_PER_MS)   // 50 milliseconds
#define MNP_TX_TIMEOUT_TIME           (500 * TICKS_PER_MS)  // 500 milliseconds
#define MNP_INIT_NET_BUFFER_NUM       512
#define MNP_NET_BUFFER_INCREASEMENT   64
#define MNP_MAX_NET_BUFFER_NUM        65536

#define MNP_MAX_RCVD_PACKET_QUE_SIZE  256

#define MNP_RECEIVE_UNICAST           0x01
#define MNP_RECEIVE_BROADCAST         0x02

#define UNICAST_PACKET                MNP_RECEIVE_UNICAST
#define BROADCAST_PACKET              MNP_RECEIVE_BROADCAST

#define MNP_INSTANCE_DATA_SIGNATURE   EFI_SIGNATURE_32 ('M', 'n', 'p', 'I')

#define MNP_INSTANCE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  MNP_INSTANCE_DATA, \
  ManagedNetwork, \
  MNP_INSTANCE_DATA_SIGNATURE \
  )

typedef struct _MNP_INSTANCE_DATA {
  UINT32                          Signature;

  MNP_SERVICE_DATA                *MnpServiceData;

  EFI_HANDLE                      Handle;

  LIST_ENTRY                      InstEntry;

  EFI_MANAGED_NETWORK_PROTOCOL    ManagedNetwork;

  BOOLEAN                         Configured;
  BOOLEAN                         Destroyed;

  LIST_ENTRY                      GroupCtrlBlkList;

  NET_MAP                         RxTokenMap;

  LIST_ENTRY                      RxDeliveredPacketQueue;
  LIST_ENTRY                      RcvdPacketQueue;
  UINTN                           RcvdPacketQueueSize;

  EFI_MANAGED_NETWORK_CONFIG_DATA ConfigData;

  UINT8                           ReceiveFilter;
} MNP_INSTANCE_DATA;

typedef struct _MNP_GROUP_ADDRESS {
  LIST_ENTRY      AddrEntry;
  EFI_MAC_ADDRESS Address;
  INTN            RefCnt;
} MNP_GROUP_ADDRESS;

typedef struct _MNP_GROUP_CONTROL_BLOCK {
  LIST_ENTRY        CtrlBlkEntry;
  MNP_GROUP_ADDRESS *GroupAddress;
} MNP_GROUP_CONTROL_BLOCK;

typedef struct _MNP_RXDATA_WRAP {
  LIST_ENTRY                        WrapEntry;
  MNP_INSTANCE_DATA                 *Instance;
  EFI_MANAGED_NETWORK_RECEIVE_DATA  RxData;
  NET_BUF                           *Nbuf;
  UINT64                            TimeoutTick;
} MNP_RXDATA_WRAP;

EFI_STATUS
MnpInitializeServiceData (
  IN MNP_SERVICE_DATA  *MnpServiceData,
  IN EFI_HANDLE        ImageHandle,
  IN EFI_HANDLE        ControllerHandle
  );

VOID
MnpFlushServiceData (
  MNP_SERVICE_DATA  *MnpServiceData
  );

VOID
MnpInitializeInstanceData (
  IN MNP_SERVICE_DATA   *MnpServiceData,
  IN MNP_INSTANCE_DATA  *Instance
  );

EFI_STATUS
MnpTokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg
  );

EFI_STATUS
MnpCancelTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg
  );

VOID
MnpFlushRcvdDataQueue (
  IN MNP_INSTANCE_DATA  *Instance
  );

EFI_STATUS
MnpConfigureInstance (
  IN MNP_INSTANCE_DATA                *Instance,
  IN EFI_MANAGED_NETWORK_CONFIG_DATA  *ConfigData OPTIONAL
  );

EFI_STATUS
MnpGroupOp (
  IN MNP_INSTANCE_DATA        *Instance,
  IN BOOLEAN                  JoinFlag,
  IN EFI_MAC_ADDRESS          *MacAddr OPTIONAL,
  IN MNP_GROUP_CONTROL_BLOCK  *CtrlBlk OPTIONAL
  );

BOOLEAN
MnpIsValidTxToken (
  IN MNP_INSTANCE_DATA                     *Instance,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

VOID
MnpBuildTxPacket (
  IN  MNP_SERVICE_DATA                   *MnpServiceData,
  IN  EFI_MANAGED_NETWORK_TRANSMIT_DATA  *TxData,
  OUT UINT8                              **PktBuf,
  OUT UINT32                             *PktLen
  );

EFI_STATUS
MnpSyncSendPacket (
  IN MNP_SERVICE_DATA                      *MnpServiceData,
  IN UINT8                                 *Packet,
  IN UINT32                                Length,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

EFI_STATUS
MnpInstanceDeliverPacket (
  IN MNP_INSTANCE_DATA  *Instance
  );

VOID
EFIAPI
MnpRecycleRxData (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

EFI_STATUS
MnpReceivePacket (
  IN MNP_SERVICE_DATA  *MnpServiceData
  );

NET_BUF *
MnpAllocNbuf (
  IN MNP_SERVICE_DATA  *MnpServiceData
  );

VOID
MnpFreeNbuf (
  IN MNP_SERVICE_DATA  *MnpServiceData,
  IN NET_BUF           *Nbuf
  );

VOID
EFIAPI
MnpCheckPacketTimeout (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
MnpSystemPoll (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

EFI_STATUS
EFIAPI
MnpGetModeData (
  IN  EFI_MANAGED_NETWORK_PROTOCOL     *This,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData OPTIONAL
  );

EFI_STATUS
EFIAPI
MnpConfigure (
  IN EFI_MANAGED_NETWORK_PROTOCOL     *This,
  IN EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData OPTIONAL
  );

EFI_STATUS
EFIAPI
MnpMcastIpToMac (
  IN  EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN  BOOLEAN                       Ipv6Flag,
  IN  EFI_IP_ADDRESS                *IpAddress,
  OUT EFI_MAC_ADDRESS               *MacAddress
  );

EFI_STATUS
EFIAPI
MnpGroups (
  IN EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                       JoinFlag,
  IN EFI_MAC_ADDRESS               *MacAddress OPTIONAL
  );

EFI_STATUS
EFIAPI
MnpTransmit (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

EFI_STATUS
EFIAPI
MnpCancel (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token OPTIONAL
  );

EFI_STATUS
EFIAPI
MnpReceive (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

EFI_STATUS
EFIAPI
MnpPoll (
  IN EFI_MANAGED_NETWORK_PROTOCOL  *This
  );

#endif
