/** @file
  UDP4 Service Binding Protocol as defined in UEFI specification.

  The EFI UDPv4 Protocol provides simple packet-oriented services 
  to transmit and receive UDP packets.  

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_UDP4_PROTOCOL_H__
#define __EFI_UDP4_PROTOCOL_H__

#include <Protocol/Ip4.h>
//
//GUID definitions
//
#define EFI_UDP4_SERVICE_BINDING_PROTOCOL_GUID \
  { \
    0x83f01464, 0x99bd, 0x45e5, {0xb3, 0x83, 0xaf, 0x63, 0x05, 0xd8, 0xe9, 0xe6 } \
  }

#define EFI_UDP4_PROTOCOL_GUID \
  { \
    0x3ad9df29, 0x4501, 0x478d, {0xb1, 0xf8, 0x7f, 0x7f, 0xe7, 0x0e, 0x50, 0xf3 } \
  }

typedef struct _EFI_UDP4_PROTOCOL EFI_UDP4_PROTOCOL; 
  
typedef struct {
  EFI_HANDLE              InstanceHandle;
  EFI_IPv4_ADDRESS        LocalAddress;
  UINT16                  LocalPort;
  EFI_IPv4_ADDRESS        RemoteAddress;
  UINT16                  RemotePort;
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


typedef struct {
  UINT32             FragmentLength;
  VOID               *FragmentBuffer;
} EFI_UDP4_FRAGMENT_DATA;

typedef struct {
  EFI_IPv4_ADDRESS   SourceAddress;
  UINT16             SourcePort;
  EFI_IPv4_ADDRESS   DestinationAddress;
  UINT16             DestinationPort;
} EFI_UDP4_SESSION_DATA;
typedef struct {
  //
  // Receiving Filters
  //
  BOOLEAN            AcceptBroadcast;
  BOOLEAN            AcceptPromiscuous;
  BOOLEAN            AcceptAnyPort;
  BOOLEAN            AllowDuplicatePort;
  //
  // I/O parameters
  //
  UINT8              TypeOfService;
  UINT8              TimeToLive;
  BOOLEAN            DoNotFragment;
  UINT32             ReceiveTimeout;
  UINT32             TransmitTimeout;
  //
  // Access Point
  //
  BOOLEAN            UseDefaultAddress;
  EFI_IPv4_ADDRESS   StationAddress;
  EFI_IPv4_ADDRESS   SubnetMask;
  UINT16             StationPort;
  EFI_IPv4_ADDRESS   RemoteAddress;
  UINT16             RemotePort;
} EFI_UDP4_CONFIG_DATA;

typedef struct {
  EFI_UDP4_SESSION_DATA     *UdpSessionData;       //OPTIONAL
  EFI_IPv4_ADDRESS          *GatewayAddress;       //OPTIONAL
  UINT32                    DataLength;
  UINT32                    FragmentCount; 
  EFI_UDP4_FRAGMENT_DATA    FragmentTable[1];
} EFI_UDP4_TRANSMIT_DATA;

typedef struct {
  EFI_TIME                  TimeStamp;
  EFI_EVENT                 RecycleSignal;
  EFI_UDP4_SESSION_DATA     UdpSession;
  UINT32                    DataLength;
  UINT32                    FragmentCount;
  EFI_UDP4_FRAGMENT_DATA    FragmentTable[1];
} EFI_UDP4_RECEIVE_DATA;


typedef struct {
  EFI_EVENT                 Event;
  EFI_STATUS                Status;
  union {
    EFI_UDP4_RECEIVE_DATA   *RxData;
    EFI_UDP4_TRANSMIT_DATA  *TxData;
  } Packet;
} EFI_UDP4_COMPLETION_TOKEN;

/**
  Reads the current operational settings.

  @param  This           Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Udp4ConfigData Pointer to the buffer to receive the current configuration data.
  @param  Ip4ModeData    Pointer to the EFI IPv4 Protocol mode data structure.
  @param  MnpConfigData  Pointer to the managed network configuration data structure.
  @param  SnpModeData    Pointer to the simple network mode data structure.

  @retval EFI_SUCCESS           The mode data was read.
  @retval EFI_NOT_STARTED       When Udp4ConfigData is queried, no configuration data is
                                available because this instance has not been started.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_GET_MODE_DATA)(
  IN  EFI_UDP4_PROTOCOL                *This,
  OUT EFI_UDP4_CONFIG_DATA             *Udp4ConfigData OPTIONAL,
  OUT EFI_IP4_MODE_DATA                *Ip4ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  )
;  
  

/**
  Initializes, changes, or resets the operational parameters for this instance of the EFI UDPv4
  Protocol.

  @param  This           Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Udp4ConfigData Pointer to the buffer to receive the current configuration data.

  @retval EFI_SUCCESS           The configuration settings were set, changed, or reset successfully.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more following conditions are TRUE:
  @retval EFI_ALREADY_STARTED   The EFI UDPv4 Protocol instance is already started/configured
                                and must be stopped/reset before it can be reconfigured.
  @retval EFI_ACCESS_DENIED     UdpConfigData. AllowDuplicatePort is FALSE
                                and UdpConfigData.StationPort is already used by
                                other instance.
  @retval EFI_OUT_OF_RESOURCES  The EFI UDPv4 Protocol driver cannot allocate memory for this
                                EFI UDPv4 Protocol instance.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred and this instance
                                 was not opened. 

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_CONFIGURE)(
  IN EFI_UDP4_PROTOCOL      *This,
  IN EFI_UDP4_CONFIG_DATA   *UdpConfigData OPTIONAL
  )
;  

/**
  Joins and leaves multicast groups.

  @param  This             Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  JoinFlag         Set to TRUE to join a multicast group. Set to FALSE to leave one
                           or all multicast groups.
  @param  MulticastAddress Pointer to multicast group address to join or leave.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_NOT_STARTED       The EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate resources to join the group.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                - This is NULL.
                                - JoinFlag is TRUE and MulticastAddress is NULL.
                                - JoinFlag is TRUE and *MulticastAddress is not
                                  a valid multicast address.
  @retval EFI_ALREADY_STARTED   The group address is already in the group table (when
                                JoinFlag is TRUE).
  @retval EFI_NOT_FOUND         The group address is not in the group table (when JoinFlag is
                                FALSE).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_GROUPS)(
  IN EFI_UDP4_PROTOCOL      *This,
  IN BOOLEAN                JoinFlag,
  IN EFI_IPv4_ADDRESS       *MulticastAddress    OPTIONAL
  )
;   

/**
  Adds and deletes routing table entries.

  @param  This           Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  DeleteRoute    Set to TRUE to delete this route from the routing table.
                         Set to FALSE to add this route to the routing table.
  @param  SubnetAddress  The destination network address that needs to be routed.
  @param  SubnetMask     The subnet mask of SubnetAddress.
  @param  GatewayAddress The gateway IP address for this route.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_NOT_STARTED       The EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                - RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND         This route is not in the routing table.
  @retval EFI_ACCESS_DENIED     The route is already defined in the routing table.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_ROUTES)(
  IN EFI_UDP4_PROTOCOL      *This,
  IN BOOLEAN                DeleteRoute,
  IN EFI_IPv4_ADDRESS       *SubnetAddress,
  IN EFI_IPv4_ADDRESS       *SubnetMask,
  IN EFI_IPv4_ADDRESS       *GatewayAddress
  )
;     

/**
  Polls for incoming data packets and processes outgoing data packets.

  @param  This Pointer to the EFI_UDP4_PROTOCOL instance.

  @retval EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_POLL)(
  IN EFI_UDP4_PROTOCOL      *This
  )
;   

/**
  Places an asynchronous receive request into the receiving queue.

  @param  This  Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Token Pointer to a token that is associated with the receive data
                descriptor.

  @retval EFI_SUCCESS           The receive completion token was cached.
  @retval EFI_NOT_STARTED       This EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP, RARP, etc.)
                                is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
  @retval EFI_OUT_OF_RESOURCES  The receive completion token could not be queued due to a lack of system
                                resources (usually memory).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_ACCESS_DENIED     A receive completion token with the same Token.Event was already in
                                the receive queue.
  @retval EFI_NOT_READY         The receive request could not be queued because the receive queue is full.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_RECEIVE)(
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
  )
;   

/**
  Queues outgoing data packets into the transmit queue.

  @param  This  Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Token Pointer to the completion token that will be placed into the
                transmit queue.

  @retval EFI_SUCCESS           The data has been queued for transmission.
  @retval EFI_NOT_STARTED       This EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_ACCESS_DENIED     The transmit completion token with the same
                                Token.Event was already in the transmit queue.
  @retval EFI_NOT_READY         The completion token could not be queued because the
                                transmit queue is full.
  @retval EFI_OUT_OF_RESOURCES  Could not queue the transmit data.
  @retval EFI_NOT_FOUND         There is no route to the destination network or address.
  @retval EFI_BAD_BUFFER_SIZE   The data length is greater than the maximum UDP packet
                                size. Or the length of the IP header + UDP header + data
                                length is greater than MTU if DoNotFragment is TRUE.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_TRANSMIT)(
  IN EFI_UDP4_PROTOCOL           *This,
  IN EFI_UDP4_COMPLETION_TOKEN   *Token
  )
;     

/**
  Aborts an asynchronous transmit or receive request.

  @param  This  Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Token Pointer to a token that has been issued by
                EFI_UDP4_PROTOCOL.Transmit() or
                EFI_UDP4_PROTOCOL.Receive().If NULL, all pending
                tokens are aborted.

  @retval  EFI_SUCCESS           The asynchronous I/O request was aborted and Token.Event
                                 was signaled. When Token is NULL, all pending requests are
                                 aborted and their events are signaled.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval  EFI_NOT_FOUND         When Token is not NULL, the asynchronous I/O request was
                                 not found in the transmit or receive queue. It has either completed
                                 or was not issued by Transmit() and Receive().

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UDP4_CANCEL)(
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token  OPTIONAL
  )
;       

/**  
  @par Protocol Description:
  The EFI_UDP4_PROTOCOL defines an EFI UDPv4 Protocol session that can be used 
  by any network drivers, applications, or daemons to transmit or receive UDP packets. 
  This protocol instance can either be bound to a specified port as a service or 
  connected to some remote peer as an active client. Each instance has its own settings, 
  such as the routing table and group table, which are independent from each other.

  @param GetModeData
  Reads the current operational settings. 

  @param Configure
  Initializes, changes, or resets operational settings for the EFI UDPv4 Protocol. 

  @param Groups
  Joins and leaves multicast groups. 

  @param Routes
  Add and deletes routing table entries. 

  @param Transmit
  Queues outgoing data packets into the transmit queue. This function is a nonblocked operation. 

  @param Receive
  Places a receiving request token into the receiving queue. This function is a nonblocked operation. 

  @param Cancel
  Aborts a pending transmit or receive request. 

  @param Poll
  Polls for incoming data packets and processes outgoing data packets.   
**/
struct _EFI_UDP4_PROTOCOL {
  EFI_UDP4_GET_MODE_DATA        GetModeData;
  EFI_UDP4_CONFIGURE            Configure;
  EFI_UDP4_GROUPS               Groups;
  EFI_UDP4_ROUTES               Routes;
  EFI_UDP4_TRANSMIT             Transmit;
  EFI_UDP4_RECEIVE              Receive;
  EFI_UDP4_CANCEL               Cancel;
  EFI_UDP4_POLL                 Poll;
};

extern EFI_GUID gEfiUdp4ServiceBindingProtocolGuid;
extern EFI_GUID gEfiUdp4ProtocolGuid;

#endif
