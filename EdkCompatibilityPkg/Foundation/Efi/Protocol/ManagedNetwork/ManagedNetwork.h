/*++ 

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ManagedNetwork.h

Abstract:

  UEFI Managed Network protocol definition.

--*/

#ifndef _MANAGED_NETWORK_H_
#define _MANAGED_NETWORK_H_

#include EFI_PROTOCOL_DEFINITION (SimpleNetwork)
#include EFI_PROTOCOL_DEFINITION (ServiceBinding)

#define EFI_MANAGED_NETWORK_SERVICE_BINDING_PROTOCOL_GUID \
  { 0xf36ff770, 0xa7e1, 0x42cf, {0x9e, 0xd2, 0x56, 0xf0, 0xf2, 0x71, 0xf4, 0x4c} }

#define EFI_MANAGED_NETWORK_PROTOCOL_GUID \
  { 0x7ab33a91, 0xace5, 0x4326, {0xb5, 0x72, 0xe7, 0xee, 0x33, 0xd3, 0x9f, 0x16} }

EFI_FORWARD_DECLARATION (EFI_MANAGED_NETWORK_PROTOCOL);

typedef struct {
  UINT32     ReceivedQueueTimeoutValue;
  UINT32     TransmitQueueTimeoutValue;
  UINT16     ProtocolTypeFilter;
  BOOLEAN    EnableUnicastReceive;
  BOOLEAN    EnableMulticastReceive;
  BOOLEAN    EnableBroadcastReceive;
  BOOLEAN    EnablePromiscuousReceive;
  BOOLEAN    FlushQueuesOnReset;
  BOOLEAN    EnableReceiveTimestamps;
  BOOLEAN    DisableBackgroundPolling;
} EFI_MANAGED_NETWORK_CONFIG_DATA;

typedef struct {
  EFI_TIME      Timestamp;
  EFI_EVENT     RecycleEvent;
  UINT32        PacketLength;
  UINT32        HeaderLength;
  UINT32        AddressLength;
  UINT32        DataLength;
  BOOLEAN       BroadcastFlag;
  BOOLEAN       MulticastFlag;
  BOOLEAN       PromiscuousFlag;
  UINT16        ProtocolType;
  VOID          *DestinationAddress;
  VOID          *SourceAddress;
  VOID          *MediaHeader;
  VOID          *PacketData;
} EFI_MANAGED_NETWORK_RECEIVE_DATA;

typedef struct {
  UINT32        FragmentLength;
  VOID          *FragmentBuffer;
} EFI_MANAGED_NETWORK_FRAGMENT_DATA;

typedef struct {
  EFI_MAC_ADDRESS                   *DestinationAddress;
  EFI_MAC_ADDRESS                   *SourceAddress;
  UINT16                            ProtocolType;
  UINT32                            DataLength;
  UINT16                            HeaderLength;
  UINT16                            FragmentCount;
  EFI_MANAGED_NETWORK_FRAGMENT_DATA FragmentTable[1];
} EFI_MANAGED_NETWORK_TRANSMIT_DATA;


typedef struct {
  EFI_EVENT                             Event;
  EFI_STATUS                            Status;
  union {
    EFI_MANAGED_NETWORK_RECEIVE_DATA    *RxData;
    EFI_MANAGED_NETWORK_TRANSMIT_DATA   *TxData;
  } Packet;
} EFI_MANAGED_NETWORK_COMPLETION_TOKEN;

typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_GET_MODE_DATA) (
  IN  EFI_MANAGED_NETWORK_PROTOCOL     *This,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData  OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_CONFIGURE) (
  IN EFI_MANAGED_NETWORK_PROTOCOL     *This,
  IN EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_MCAST_IP_TO_MAC) (
  IN  EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN  BOOLEAN                       Ipv6Flag,
  IN  EFI_IP_ADDRESS                *IpAddress,
  OUT EFI_MAC_ADDRESS               *MacAddress
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_GROUPS) (
  IN EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                       JoinFlag,
  IN EFI_MAC_ADDRESS               *MacAddress  OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_TRANSMIT) (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_RECEIVE) (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_CANCEL) (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token  OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_POLL) (
  IN EFI_MANAGED_NETWORK_PROTOCOL    *This
  );

struct _EFI_MANAGED_NETWORK_PROTOCOL {
  EFI_MANAGED_NETWORK_GET_MODE_DATA       GetModeData;
  EFI_MANAGED_NETWORK_CONFIGURE           Configure;
  EFI_MANAGED_NETWORK_MCAST_IP_TO_MAC     McastIpToMac;
  EFI_MANAGED_NETWORK_GROUPS              Groups;
  EFI_MANAGED_NETWORK_TRANSMIT            Transmit;
  EFI_MANAGED_NETWORK_RECEIVE             Receive;
  EFI_MANAGED_NETWORK_CANCEL              Cancel;
  EFI_MANAGED_NETWORK_POLL                Poll;
};

extern EFI_GUID gEfiManagedNetworkServiceBindingProtocolGuid;
extern EFI_GUID gEfiManagedNetworkProtocolGuid;

#endif
