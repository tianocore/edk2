/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Tcp4.h

Abstract:

  UEFI TCPv4 Protocol Definition

--*/

#ifndef _TCP4_H_
#define _TCP4_H_

#include EFI_PROTOCOL_DEFINITION (ServiceBinding)
#include EFI_PROTOCOL_DEFINITION (Ip4)

#define EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID \
  { \
    0x00720665, 0x67EB, 0x4a99, {0xBA, 0xF7, 0xD3, 0xC3, 0x3A, 0x1C, 0x7C, 0xC9} \
  }

extern EFI_GUID gEfiTcp4ServiceBindingProtocolGuid;

#define EFI_TCP4_PROTOCOL_GUID \
  { \
    0x65530BC7, 0xA359, 0x410f, {0xB0, 0x10, 0x5A, 0xAD, 0xC7, 0xEC, 0x2B, 0x62} \
  }

extern EFI_GUID gEfiTcp4ProtocolGuid;

EFI_FORWARD_DECLARATION (EFI_TCP4_PROTOCOL);

typedef struct {
  EFI_HANDLE        InstanceHandle;
  EFI_IPv4_ADDRESS  LocalAddress;
  UINT16            LocalPort;
  EFI_IPv4_ADDRESS  RemoteAddress;
  UINT16            RemotePort;
} EFI_TCP4_SERVICE_POINT;

typedef struct {
  EFI_HANDLE              DriverHandle;
  UINT32                  ServiceCount;
  EFI_TCP4_SERVICE_POINT  Services[1];
} EFI_TCP4_VARIABLE_DATA;

typedef struct {
  BOOLEAN           UseDefaultAddress;
  EFI_IPv4_ADDRESS  StationAddress;
  EFI_IPv4_ADDRESS  SubnetMask;
  UINT16            StationPort;
  EFI_IPv4_ADDRESS  RemoteAddress;
  UINT16            RemotePort;
  BOOLEAN           ActiveFlag;
} EFI_TCP4_ACCESS_POINT;

typedef struct {
  UINT32   ReceiveBufferSize;
  UINT32   SendBufferSize;
  UINT32   MaxSynBackLog;
  UINT32   ConnectionTimeout;
  UINT32   DataRetries;
  UINT32   FinTimeout;
  UINT32   TimeWaitTimeout;
  UINT32   KeepAliveProbes;
  UINT32   KeepAliveTime;
  UINT32   KeepAliveInterval;
  BOOLEAN  EnableNagle;
  BOOLEAN  EnableTimeStamp;
  BOOLEAN  EnableWindowScaling;
  BOOLEAN  EnableSelectiveAck;
  BOOLEAN  EnablePathMtuDiscovery;
} EFI_TCP4_OPTION;

typedef struct {
  UINT8                 TypeOfService;
  UINT8                 TimeToLive;
  EFI_TCP4_ACCESS_POINT AccessPoint;
  EFI_TCP4_OPTION       *ControlOption;
} EFI_TCP4_CONFIG_DATA;

typedef enum {
  Tcp4StateClosed     = 0,
  Tcp4StateListen     = 1,
  Tcp4StateSynSent    = 2,
  Tcp4StateSynReceived= 3,
  Tcp4StateEstablished= 4,
  Tcp4StateFinWait1   = 5,
  Tcp4StateFinWait2   = 6,
  Tcp4StateClosing    = 7,
  Tcp4StateTimeWait   = 8,
  Tcp4StateCloseWait  = 9,
  Tcp4StateLastAck    = 10
} EFI_TCP4_CONNECTION_STATE;

typedef struct {
  EFI_EVENT   Event;
  EFI_STATUS  Status;
} EFI_TCP4_COMPLETION_TOKEN;

typedef struct {
  EFI_TCP4_COMPLETION_TOKEN CompletionToken;
} EFI_TCP4_CONNECTION_TOKEN;

typedef struct {
  EFI_TCP4_COMPLETION_TOKEN CompletionToken;
  EFI_HANDLE                NewChildHandle;
} EFI_TCP4_LISTEN_TOKEN;

typedef struct {
  UINT32 FragmentLength;
  VOID   *FragmentBuffer;
} EFI_TCP4_FRAGMENT_DATA;

typedef struct {
  BOOLEAN                 UrgentFlag;
  UINT32                  DataLength;
  UINT32                  FragmentCount;
  EFI_TCP4_FRAGMENT_DATA  FragmentTable[1];
} EFI_TCP4_RECEIVE_DATA;

typedef struct {
  BOOLEAN                 Push;
  BOOLEAN                 Urgent;
  UINT32                  DataLength;
  UINT32                  FragmentCount;
  EFI_TCP4_FRAGMENT_DATA  FragmentTable[1];
} EFI_TCP4_TRANSMIT_DATA;

typedef struct {
  EFI_TCP4_COMPLETION_TOKEN CompletionToken;
  union {
    EFI_TCP4_RECEIVE_DATA   *RxData;
    EFI_TCP4_TRANSMIT_DATA  *TxData;
  } Packet;
} EFI_TCP4_IO_TOKEN;

typedef struct {
  EFI_TCP4_COMPLETION_TOKEN CompletionToken;
  BOOLEAN                   AbortOnClose;
} EFI_TCP4_CLOSE_TOKEN;

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_GET_MODE_DATA) (
  IN    EFI_TCP4_PROTOCOL                *This,
  OUT EFI_TCP4_CONNECTION_STATE          *Tcp4State OPTIONAL,
  OUT EFI_TCP4_CONFIG_DATA               *Tcp4ConfigData OPTIONAL,
  OUT EFI_IP4_MODE_DATA                  *Ip4ModeData OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA    *MnpConfigData OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE            *SnpModeData OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_CONFIGURE) (
  IN EFI_TCP4_PROTOCOL        *This,
  IN EFI_TCP4_CONFIG_DATA     *TcpConfigData OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_ROUTES) (
  IN EFI_TCP4_PROTOCOL           *This,
  IN BOOLEAN                     DeleteRoute,
  IN EFI_IPv4_ADDRESS            *SubnetAddress,
  IN EFI_IPv4_ADDRESS            *SubnetMask,
  IN EFI_IPv4_ADDRESS            *GatewayAddress
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_CONNECT) (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_CONNECTION_TOKEN   *ConnectionToken
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_ACCEPT) (
  IN EFI_TCP4_PROTOCOL             *This,
  IN EFI_TCP4_LISTEN_TOKEN         *ListenToken
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_TRANSMIT) (
  IN EFI_TCP4_PROTOCOL            *This,
  IN EFI_TCP4_IO_TOKEN            *Token
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_RECEIVE) (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_IO_TOKEN           *Token
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_CLOSE) (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_CLOSE_TOKEN        *CloseToken
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_CANCEL) (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_COMPLETION_TOKEN   *Token OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCP4_POLL) (
  IN EFI_TCP4_PROTOCOL        *This
  );

struct _EFI_TCP4_PROTOCOL {
  EFI_TCP4_GET_MODE_DATA  GetModeData;
  EFI_TCP4_CONFIGURE      Configure;
  EFI_TCP4_ROUTES         Routes;
  EFI_TCP4_CONNECT        Connect;
  EFI_TCP4_ACCEPT         Accept;
  EFI_TCP4_TRANSMIT       Transmit;
  EFI_TCP4_RECEIVE        Receive;
  EFI_TCP4_CLOSE          Close;
  EFI_TCP4_CANCEL         Cancel;
  EFI_TCP4_POLL           Poll;
};

#define EFI_CONNECTION_FIN      EFIERR (104)
#define EFI_CONNECTION_RESET    EFIERR (105)
#define EFI_CONNECTION_REFUSED  EFIERR (106)

#endif
