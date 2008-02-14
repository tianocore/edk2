/** @file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  ArpImpl.h

Abstract:


**/

#ifndef _ARP_IMPL_H_
#define _ARP_IMPL_H_


#include <PiDxe.h>

#include <Protocol/Arp.h>
#include <Protocol/ManagedNetwork.h>
#include <Protocol/ServiceBinding.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>


#define ARP_ETHER_PROTO_TYPE         0x0806
#define IPv4_ETHER_PROTO_TYPE        0x0800
#define IPv6_ETHER_PROTO_TYPE        0x86DD

#define ARP_OPCODE_REQUEST           0x0001
#define ARP_OPCODE_REPLY             0x0002

#define ARP_DEFAULT_TIMEOUT_VALUE    (400 * TICKS_PER_SECOND)
#define ARP_DEFAULT_RETRY_COUNT      2
#define ARP_DEFAULT_RETRY_INTERVAL   (5   * TICKS_PER_MS)
#define ARP_PERIODIC_TIMER_INTERVAL  (500 * TICKS_PER_MS)

#pragma pack(1)
typedef struct _ARP_HEAD {
  UINT16  HwType;
  UINT16  ProtoType;
  UINT8   HwAddrLen;
  UINT8   ProtoAddrLen;
  UINT16  OpCode;
} ARP_HEAD;
#pragma pack()

typedef struct _ARP_ADDRESS {
  UINT8  *SenderHwAddr;
  UINT8  *SenderProtoAddr;
  UINT8  *TargetHwAddr;
  UINT8  *TargetProtoAddr;
} ARP_ADDRESS;

#define MATCH_SW_ADDRESS  0x1
#define MATCH_HW_ADDRESS  0x2

typedef enum {
  ByNone         = 0,
  ByProtoAddress = MATCH_SW_ADDRESS,
  ByHwAddress    = MATCH_HW_ADDRESS,
  ByBoth         = MATCH_SW_ADDRESS | MATCH_HW_ADDRESS
} FIND_OPTYPE;

#define ARP_INSTANCE_DATA_SIGNATURE  EFI_SIGNATURE_32('A', 'R', 'P', 'I')

#define ARP_INSTANCE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  ARP_INSTANCE_DATA, \
  ArpProto, \
  ARP_INSTANCE_DATA_SIGNATURE \
  )

typedef struct _ARP_SERVICE_DATA  ARP_SERVICE_DATA;

typedef struct _ARP_INSTANCE_DATA {
  UINT32               Signature;
  ARP_SERVICE_DATA     *ArpService;
  EFI_HANDLE           Handle;
  EFI_ARP_PROTOCOL     ArpProto;
  LIST_ENTRY           List;
  EFI_ARP_CONFIG_DATA  ConfigData;
  BOOLEAN              Configured;
  BOOLEAN              Destroyed;
} ARP_INSTANCE_DATA;

#define ARP_SERVICE_DATA_SIGNATURE  EFI_SIGNATURE_32('A', 'R', 'P', 'S')

#define ARP_SERVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  ARP_SERVICE_DATA, \
  ServiceBinding, \
  ARP_SERVICE_DATA_SIGNATURE \
  )

struct _ARP_SERVICE_DATA {
  UINT32                           Signature;
  EFI_SERVICE_BINDING_PROTOCOL     ServiceBinding;

  EFI_HANDLE                       MnpChildHandle;
  EFI_HANDLE                       ImageHandle;
  EFI_HANDLE                       ControllerHandle;

  EFI_MANAGED_NETWORK_PROTOCOL          *Mnp;
  EFI_MANAGED_NETWORK_CONFIG_DATA       MnpConfigData;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  RxToken;

  EFI_SIMPLE_NETWORK_MODE          SnpMode;

  UINTN                            ChildrenNumber;
  LIST_ENTRY                       ChildrenList;

  LIST_ENTRY                       PendingRequestTable;
  LIST_ENTRY                       DeniedCacheTable;
  LIST_ENTRY                       ResolvedCacheTable;

  EFI_EVENT                        PeriodicTimer;
};

typedef struct _USER_REQUEST_CONTEXT {
  LIST_ENTRY         List;
  ARP_INSTANCE_DATA  *Instance;
  EFI_EVENT          UserRequestEvent;
  VOID               *UserHwAddrBuffer;
} USER_REQUEST_CONTEXT;

#define ARP_MAX_PROTOCOL_ADDRESS_LEN  sizeof(EFI_IP_ADDRESS)
#define ARP_MAX_HARDWARE_ADDRESS_LEN  sizeof(EFI_MAC_ADDRESS)

typedef struct _NET_ARP_ADDRESS {
  UINT16  Type;
  UINT8   Length;
  UINT8   *AddressPtr;
  union {
    UINT8  ProtoAddress[ARP_MAX_PROTOCOL_ADDRESS_LEN];
    UINT8  HwAddress[ARP_MAX_HARDWARE_ADDRESS_LEN];
  } Buffer;
} NET_ARP_ADDRESS;

typedef enum {
  Hardware,
  Protocol
} ARP_ADDRESS_TYPE;

typedef struct _ARP_CACHE_ENTRY {
  LIST_ENTRY      List;

  UINT32          RetryCount;
  UINT32          DefaultDecayTime;
  UINT32          DecayTime;
  UINT32          NextRetryTime;

  NET_ARP_ADDRESS  Addresses[2];

  LIST_ENTRY      UserRequestList;
} ARP_CACHE_ENTRY;

EFI_STATUS
EFIAPI
ArpConfigure (
  IN EFI_ARP_PROTOCOL     *This,
  IN EFI_ARP_CONFIG_DATA  *ConfigData OPTIONAL
  );

EFI_STATUS
EFIAPI
ArpAdd (
  IN EFI_ARP_PROTOCOL  *This,
  IN BOOLEAN           DenyFlag,
  IN VOID              *TargetSwAddress OPTIONAL,
  IN VOID              *TargetHwAddress OPTIONAL,
  IN UINT32            TimeoutValue,
  IN BOOLEAN           Overwrite
  );

EFI_STATUS
EFIAPI
ArpFind (
  IN EFI_ARP_PROTOCOL    *This,
  IN BOOLEAN             BySwAddress,
  IN VOID                *AddressBuffer OPTIONAL,
  OUT UINT32             *EntryLength   OPTIONAL,
  OUT UINT32             *EntryCount    OPTIONAL,
  OUT EFI_ARP_FIND_DATA  **Entries      OPTIONAL,
  IN BOOLEAN             Refresh
  );

EFI_STATUS
EFIAPI
ArpDelete (
  IN EFI_ARP_PROTOCOL  *This,
  IN BOOLEAN           BySwAddress,
  IN VOID              *AddressBuffer OPTIONAL
  );

EFI_STATUS
EFIAPI
ArpFlush (
  IN EFI_ARP_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
ArpRequest (
  IN EFI_ARP_PROTOCOL  *This,
  IN VOID              *TargetSwAddress OPTIONAL,
  IN EFI_EVENT         ResolvedEvent    OPTIONAL,
  OUT VOID             *TargetHwAddress
  );

EFI_STATUS
EFIAPI
ArpCancel (
  IN EFI_ARP_PROTOCOL  *This,
  IN VOID              *TargetSwAddress OPTIONAL,
  IN EFI_EVENT         ResolvedEvent    OPTIONAL
  );

EFI_STATUS
ArpConfigureInstance (
  IN ARP_INSTANCE_DATA    *Instance,
  IN EFI_ARP_CONFIG_DATA  *ConfigData OPTIONAL
  );

ARP_CACHE_ENTRY *
ArpFindDeniedCacheEntry (
  IN ARP_SERVICE_DATA  *ArpService,
  IN NET_ARP_ADDRESS   *ProtocolAddress OPTIONAL,
  IN NET_ARP_ADDRESS   *HardwareAddress OPTIONAL
  );

ARP_CACHE_ENTRY *
ArpFindNextCacheEntryInTable (
  IN LIST_ENTRY        *CacheTable,
  IN LIST_ENTRY        *StartEntry,
  IN FIND_OPTYPE       FindOpType,
  IN NET_ARP_ADDRESS   *ProtocolAddress OPTIONAL,
  IN NET_ARP_ADDRESS   *HardwareAddress OPTIONAL
  );

ARP_CACHE_ENTRY *
ArpAllocCacheEntry (
  IN ARP_INSTANCE_DATA  *Instance
  );

VOID
ArpFillAddressInCacheEntry (
  IN ARP_CACHE_ENTRY  *CacheEntry,
  IN NET_ARP_ADDRESS  *HwAddr OPTIONAL,
  IN NET_ARP_ADDRESS  *SwAddr OPTIONAL
  );

UINTN
ArpAddressResolved (
  IN ARP_CACHE_ENTRY    *CacheEntry,
  IN ARP_INSTANCE_DATA  *Instance OPTIONAL,
  IN EFI_EVENT          UserEvent OPTIONAL
  );

UINTN
ArpDeleteCacheEntry (
  IN ARP_INSTANCE_DATA  *Instance,
  IN BOOLEAN            BySwAddress,
  IN UINT8              *AddressBuffer OPTIONAL,
  IN BOOLEAN            Force
  );

VOID
ArpSendFrame (
  IN ARP_INSTANCE_DATA  *Instance,
  IN ARP_CACHE_ENTRY    *CacheEntry,
  IN UINT16             ArpOpCode
  );

VOID
ArpInitInstance (
  IN ARP_SERVICE_DATA   *ArpService,
  IN ARP_INSTANCE_DATA  *Instance
  );

VOID
EFIAPI
ArpOnFrameRcvdDpc (
  IN VOID       *Context
  );

VOID
EFIAPI
ArpOnFrameRcvd (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
ArpOnFrameSentDpc (
  IN VOID       *Context
  );

VOID
EFIAPI
ArpOnFrameSent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
ArpTimerHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

UINTN
ArpCancelRequest (
  IN ARP_INSTANCE_DATA  *Instance,
  IN VOID               *TargetSwAddress OPTIONAL,
  IN EFI_EVENT          UserEvent        OPTIONAL
  );

EFI_STATUS
ArpFindCacheEntry (
  IN ARP_INSTANCE_DATA   *Instance,
  IN BOOLEAN             BySwAddress,
  IN VOID                *AddressBuffer OPTIONAL,
  OUT UINT32             *EntryLength   OPTIONAL,
  OUT UINT32             *EntryCount    OPTIONAL,
  OUT EFI_ARP_FIND_DATA  **Entries      OPTIONAL,
  IN BOOLEAN             Refresh
  );

#endif

