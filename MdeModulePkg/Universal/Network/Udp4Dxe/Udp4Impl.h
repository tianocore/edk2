/** @file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Udp4Impl.h

Abstract:

  EFI UDPv4 protocol implementation


**/

#ifndef _UDP4_IMPL_H_
#define _UDP4_IMPL_H_

#include <PiDxe.h>

#include <Protocol/Ip4.h>
#include <Protocol/Udp4.h>

#include <Library/IpIoLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Udp4Driver.h"


extern EFI_COMPONENT_NAME_PROTOCOL     gUdp4ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL    gUdp4ComponentName2;
extern EFI_SERVICE_BINDING_PROTOCOL    mUdp4ServiceBinding;
extern EFI_UDP4_PROTOCOL               mUdp4Protocol;
extern UINT16                          mUdp4RandomPort;

#define ICMP_ERROR_PACKET_LENGTH  8

#define UDP4_TIMEOUT_INTERVAL (50 * TICKS_PER_MS)  // 50 milliseconds

#define UDP4_HEADER_SIZE      sizeof (EFI_UDP4_HEADER)
#define UDP4_MAX_DATA_SIZE    65507

#define UDP4_PORT_KNOWN       1024

#define UDP4_SERVICE_DATA_SIGNATURE  EFI_SIGNATURE_32('U', 'd', 'p', '4')

#define UDP4_SERVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  UDP4_SERVICE_DATA, \
  ServiceBinding, \
  UDP4_SERVICE_DATA_SIGNATURE \
  )

typedef struct _UDP4_SERVICE_DATA_ {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;
  EFI_HANDLE                    ImageHandle;
  EFI_HANDLE                    ControllerHandle;
  LIST_ENTRY                    ChildrenList;
  UINTN                         ChildrenNumber;
  IP_IO                         *IpIo;

  EFI_EVENT                     TimeoutEvent;

  CHAR16                        *MacString;
} UDP4_SERVICE_DATA;

#define UDP4_INSTANCE_DATA_SIGNATURE  EFI_SIGNATURE_32('U', 'd', 'p', 'I')

#define UDP4_INSTANCE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  UDP4_INSTANCE_DATA, \
  Udp4Proto, \
  UDP4_INSTANCE_DATA_SIGNATURE \
  )

typedef struct _UDP4_INSTANCE_DATA_ {
  UINT32                Signature;
  LIST_ENTRY            Link;

  UDP4_SERVICE_DATA     *Udp4Service;
  EFI_UDP4_PROTOCOL     Udp4Proto;
  EFI_UDP4_CONFIG_DATA  ConfigData;
  EFI_HANDLE            ChildHandle;
  BOOLEAN               Configured;
  BOOLEAN               IsNoMapping;

  NET_MAP               TxTokens;
  NET_MAP               RxTokens;

  NET_MAP               McastIps;

  LIST_ENTRY            RcvdDgramQue;
  LIST_ENTRY            DeliveredDgramQue;

  UINT16                HeadSum;

  EFI_STATUS            IcmpError;

  IP_IO_IP_INFO         *IpInfo;

  BOOLEAN               Destroyed;
} UDP4_INSTANCE_DATA;

typedef struct _UDP4_RXDATA_WRAP_ {
  LIST_ENTRY             Link;
  NET_BUF                *Packet;
  UINT32                 TimeoutTick;
  EFI_UDP4_RECEIVE_DATA  RxData;
} UDP4_RXDATA_WRAP;

EFI_STATUS
EFIAPI
Udp4GetModeData (
  IN  EFI_UDP4_PROTOCOL                *This,
  OUT EFI_UDP4_CONFIG_DATA             *Udp4ConfigData OPTIONAL,
  OUT EFI_IP4_MODE_DATA                *Ip4ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  );

EFI_STATUS
EFIAPI
Udp4Configure (
  IN EFI_UDP4_PROTOCOL     *This,
  IN EFI_UDP4_CONFIG_DATA  *UdpConfigData OPTIONAL
  );

EFI_STATUS
EFIAPI
Udp4Groups (
  IN EFI_UDP4_PROTOCOL  *This,
  IN BOOLEAN            JoinFlag,
  IN EFI_IPv4_ADDRESS   *MulticastAddress OPTIONAL
  );

EFI_STATUS
EFIAPI
Udp4Routes (
  IN EFI_UDP4_PROTOCOL  *This,
  IN BOOLEAN            DeleteRoute,
  IN EFI_IPv4_ADDRESS   *SubnetAddress,
  IN EFI_IPv4_ADDRESS   *SubnetMask,
  IN EFI_IPv4_ADDRESS   *GatewayAddress
  );

EFI_STATUS
EFIAPI
Udp4Transmit (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
  );

EFI_STATUS
EFIAPI
Udp4Receive (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
  );

EFI_STATUS
EFIAPI
Udp4Cancel (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token OPTIONAL
  );

EFI_STATUS
EFIAPI
Udp4Poll (
  IN EFI_UDP4_PROTOCOL  *This
  );

EFI_STATUS
Udp4CreateService (
  IN UDP4_SERVICE_DATA  *Udp4Service,
  IN EFI_HANDLE         ImageHandle,
  IN EFI_HANDLE         ControllerHandle
  );

VOID
Udp4CleanService (
  IN UDP4_SERVICE_DATA  *Udp4Service
  );

VOID
Udp4InitInstance (
  IN UDP4_SERVICE_DATA   *Udp4Service,
  IN UDP4_INSTANCE_DATA  *Instance
  );

VOID
Udp4CleanInstance (
  IN UDP4_INSTANCE_DATA  *Instance
  );

EFI_STATUS
Udp4Bind (
  IN LIST_ENTRY            *InstanceList,
  IN EFI_UDP4_CONFIG_DATA  *ConfigData
  );

BOOLEAN
Udp4IsReconfigurable (
  IN EFI_UDP4_CONFIG_DATA  *OldConfigData,
  IN EFI_UDP4_CONFIG_DATA  *NewConfigData
  );

VOID
Udp4BuildIp4ConfigData (
  IN EFI_UDP4_CONFIG_DATA  *Udp4ConfigData,
  IN EFI_IP4_CONFIG_DATA   *Ip4ConfigData
  );

EFI_STATUS
Udp4ValidateTxToken (
  IN UDP4_INSTANCE_DATA         *Instance,
  IN EFI_UDP4_COMPLETION_TOKEN  *TxToken
  );

EFI_STATUS
Udp4TokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  );

UINT16
Udp4Checksum (
  IN NET_BUF *Packet,
  IN UINT16  HeadSum
  );

EFI_STATUS
Udp4RemoveToken (
  IN NET_MAP                    *TokenMap,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
  );

EFI_STATUS
Udp4LeaveGroup (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  );

VOID
Udp4FlushRcvdDgram (
  IN UDP4_INSTANCE_DATA  *Instance
  );

EFI_STATUS
Udp4InstanceCancelToken (
  IN UDP4_INSTANCE_DATA         *Instance,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token OPTIONAL
  );

VOID
Udp4InstanceDeliverDgram (
  IN UDP4_INSTANCE_DATA  *Instance
  );

VOID
Udp4ReportIcmpError (
  IN UDP4_INSTANCE_DATA  *Instance
  );

VOID
Udp4NetVectorExtFree (
  VOID  *Context
  );

EFI_STATUS
Udp4SetVariableData (
  IN UDP4_SERVICE_DATA  *Udp4Service
  );

VOID
Udp4ClearVariableData (
  IN UDP4_SERVICE_DATA  *Udp4Service
  );

#endif

