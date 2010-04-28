/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Udp4.h

Abstract:

--*/

#ifndef _UDP4_H_
#define _UDP4_H_

#include EFI_PROTOCOL_DEFINITION (ServiceBinding)
#include EFI_PROTOCOL_DEFINITION (Ip4)

//
//GUID definitions
//
#define EFI_UDP4_SERVICE_BINDING_PROTOCOL_GUID \
  { 0x83f01464, 0x99bd, 0x45e5, {0xb3, 0x83, 0xaf, 0x63, 0x05, 0xd8, 0xe9, 0xe6} }

#define EFI_UDP4_PROTOCOL_GUID \
  { 0x3ad9df29, 0x4501, 0x478d, {0xb1, 0xf8, 0x7f, 0x7f, 0xe7, 0x0e, 0x50, 0xf3} }

typedef struct {
  EFI_HANDLE          InstanceHandle;
  EFI_IPv4_ADDRESS    LocalAddress;
  UINT16              LocalPort;
  EFI_IPv4_ADDRESS    RemoteAddress;
  UINT16              RemotePort;
} EFI_UDP4_SERVICE_POINT; 

typedef struct {
  EFI_HANDLE              DriverHandle;
  UINT32                  ServiceCount;
  EFI_UDP4_SERVICE_POINT  Services[1];
} EFI_UDP4_VARIABLE_DATA;

//
//ICMP error definitions
//
#define EFI_NETWORK_UNREACHABLE      EFIERR(100)
#define EFI_HOST_UNREACHABLE         EFIERR(101) 
#define EFI_PROTOCOL_UNREACHABLE     EFIERR(102)
#define EFI_PORT_UNREACHABLE         EFIERR(103)

EFI_FORWARD_DECLARATION (EFI_UDP4_PROTOCOL);

//*************************************************
//      EFI_UDP4_FRAGMENT_DATA                    *
//*************************************************
typedef struct {
  UINT32        FragmentLength;
  VOID         *FragmentBuffer;
} EFI_UDP4_FRAGMENT_DATA;

//*************************************************
//      EFI_UDP4_SESSION_DATA                     *
//*************************************************
typedef struct {
  EFI_IPv4_ADDRESS   SourceAddress;
  UINT16             SourcePort;
  EFI_IPv4_ADDRESS   DestinationAddress;
  UINT16             DestinationPort;
} EFI_UDP4_SESSION_DATA;

//************************************************
//      EFI_UDP4_CONFIG_DATA                     *
//************************************************
typedef struct {
  //Receiving Filters
  BOOLEAN        AcceptBroadcast;
  BOOLEAN        AcceptPromiscuous;
  BOOLEAN        AcceptAnyPort;
  BOOLEAN        AllowDuplicatePort;
  //I/O parameters
  UINT8          TypeOfService;
  UINT8          TimeToLive;
  BOOLEAN        DoNotFragment;
  UINT32         ReceiveTimeout;
  UINT32         TransmitTimeout;
  //Access Point
  BOOLEAN           UseDefaultAddress;
  EFI_IPv4_ADDRESS  StationAddress;
  EFI_IPv4_ADDRESS  SubnetMask;
  UINT16            StationPort;
  EFI_IPv4_ADDRESS  RemoteAddress;
  UINT16            RemotePort;
} EFI_UDP4_CONFIG_DATA;

//*******************************************************
//               EFI_UDP4_TRANSMIT_DATA                 *
//*******************************************************
typedef struct {
  EFI_UDP4_SESSION_DATA     *UdpSessionData;
  EFI_IPv4_ADDRESS          *GatewayAddress;
  UINT32                    DataLength;
  UINT32                    FragmentCount; 
  EFI_UDP4_FRAGMENT_DATA    FragmentTable[1];
} EFI_UDP4_TRANSMIT_DATA;

//*******************************************************
//           EFI_UDP4_RECEIVE_DATA                      *
//*******************************************************
typedef struct {
  EFI_TIME                  TimeStamp;
  EFI_EVENT                 RecycleSignal;
  EFI_UDP4_SESSION_DATA     UdpSession;
  UINT32                    DataLength;
  UINT32                    FragmentCount;
  EFI_UDP4_FRAGMENT_DATA    FragmentTable[1];
} EFI_UDP4_RECEIVE_DATA;


//*******************************************************
//           EFI_UDP4_COMPLETION_TOKEN                  *
//*******************************************************
typedef struct {
  EFI_EVENT                             Event;
  EFI_STATUS                            Status;
  union {
    EFI_UDP4_RECEIVE_DATA               *RxData;
    EFI_UDP4_TRANSMIT_DATA              *TxData;
  }                                     Packet;
} EFI_UDP4_COMPLETION_TOKEN;

typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_GET_MODE_DATA) (
  IN EFI_UDP4_PROTOCOL                 *This,
  OUT EFI_UDP4_CONFIG_DATA             *Udp4ConfigData OPTIONAL,
  OUT EFI_IP4_MODE_DATA                *Ip4ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_CONFIGURE) (
  IN EFI_UDP4_PROTOCOL     *This,
  IN EFI_UDP4_CONFIG_DATA  *UdpConfigData OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_GROUPS) (
  IN EFI_UDP4_PROTOCOL  *This,
  IN BOOLEAN            JoinFlag,
  IN EFI_IPv4_ADDRESS   *MulticastAddress OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_ROUTES) (
  IN EFI_UDP4_PROTOCOL  *This,
  IN BOOLEAN            DeleteRoute,
  IN EFI_IPv4_ADDRESS   *SubnetAddress,
  IN EFI_IPv4_ADDRESS   *SubnetMask,
  IN EFI_IPv4_ADDRESS   *GatewayAddress
);

typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_TRANSMIT) (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
);

typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_RECEIVE) (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
);

typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_CANCEL)(
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_POLL) (
  IN EFI_UDP4_PROTOCOL  *This
);

struct _EFI_UDP4_PROTOCOL {
  EFI_UDP4_GET_MODE_DATA  GetModeData;
  EFI_UDP4_CONFIGURE      Configure;
  EFI_UDP4_GROUPS         Groups;
  EFI_UDP4_ROUTES         Routes;
  EFI_UDP4_TRANSMIT       Transmit;
  EFI_UDP4_RECEIVE        Receive;
  EFI_UDP4_CANCEL         Cancel;
  EFI_UDP4_POLL           Poll;
};

extern EFI_GUID gEfiUdp4ServiceBindingProtocolGuid;
extern EFI_GUID gEfiUdp4ProtocolGuid;
extern EFI_GUID gEfiUdp4RegistryDataGuid;

#endif
