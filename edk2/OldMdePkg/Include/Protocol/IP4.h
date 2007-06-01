/** @file
  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  IP4.h

**/

#ifndef __EFI_IP4_PROTOCOL_H__
#define __EFI_IP4_PROTOCOL_H__

#define EFI_IP4_SERVICE_BINDING_PROTOCOL_GUID \
  { \
    0xc51711e7, 0xb4bf, 0x404a, {0xbf, 0xb8, 0x0a, 0x04, 0x8e, 0xf1, 0xff, 0xe4 } \
  }

#define EFI_IP4_PROTOCOL_GUID \
  { \
    0x41d94cd2, 0x35b6, 0x455a, {0x82, 0x58, 0xd4, 0xe5, 0x13, 0x34, 0xaa, 0xdd } \
  }

typedef struct _EFI_IP4_PROTOCOL EFI_IP4_PROTOCOL;
  
typedef struct {
  EFI_HANDLE              InstanceHandle;
  EFI_IPv4_ADDRESS        Ip4Address;
  EFI_IPv4_ADDRESS        SubnetMask;
} EFI_IP4_ADDRESS_PAIR; 

typedef struct {
  EFI_HANDLE              DriverHandle;
  UINT32                  AddressCount;
  EFI_IP4_ADDRESS_PAIR    AddressPairs[1];
} EFI_IP4_VARIABLE_DATA;

typedef struct {
  UINT8                   DefaultProtocol;
  BOOLEAN                 AcceptAnyProtocol;
  BOOLEAN                 AcceptIcmpErrors;
  BOOLEAN                 AcceptBroadcast;
  BOOLEAN                 AcceptPromiscuous;
  BOOLEAN                 UseDefaultAddress;
  EFI_IPv4_ADDRESS        StationAddress;
  EFI_IPv4_ADDRESS        SubnetMask;
  UINT8                   TypeOfService;
  UINT8                   TimeToLive;
  BOOLEAN                 DoNotFragment;
  BOOLEAN                 RawData;
  UINT32                  ReceiveTimeout;
  UINT32                  TransmitTimeout;
} EFI_IP4_CONFIG_DATA;


typedef struct {
  EFI_IPv4_ADDRESS        SubnetAddress;
  EFI_IPv4_ADDRESS        SubnetMask;
  EFI_IPv4_ADDRESS        GatewayAddress;
} EFI_IP4_ROUTE_TABLE;

typedef struct {
  UINT8                   Type;
  UINT8                   Code;
} EFI_IP4_ICMP_TYPE;

typedef struct {
  BOOLEAN                 IsStarted;
  EFI_IP4_CONFIG_DATA     ConfigData;
  BOOLEAN                 IsConfigured;
  UINT32                  GroupCount;
  EFI_IPv4_ADDRESS        *GroupTable;
  UINT32                  RouteCount;
  EFI_IP4_ROUTE_TABLE     *RouteTable;
  UINT32                  IcmpTypeCount;
  EFI_IP4_ICMP_TYPE       *IcmpTypeList;
} EFI_IP4_MODE_DATA;

#pragma pack(1)

typedef struct {
  UINT8                   HeaderLength:4;
  UINT8                   Version:4;
  UINT8                   TypeOfService;
  UINT16                  TotalLength;
  UINT16                  Identification;
  UINT16                  Fragmentation;
  UINT8                   TimeToLive;
  UINT8                   Protocol;
  UINT16                  Checksum;
  EFI_IPv4_ADDRESS        SourceAddress;
  EFI_IPv4_ADDRESS        DestinationAddress;
} EFI_IP4_HEADER;
#pragma pack()


typedef struct {
  UINT32                  FragmentLength;
  VOID                    *FragmentBuffer;
} EFI_IP4_FRAGMENT_DATA;


typedef struct {
  EFI_TIME               TimeStamp;
  EFI_EVENT              RecycleSignal;
  UINT32                 HeaderLength;
  EFI_IP4_HEADER         *Header;
  UINT32                 OptionsLength;
  VOID                   *Options;
  UINT32                 DataLength;
  UINT32                 FragmentCount;
  EFI_IP4_FRAGMENT_DATA  FragmentTable[1];
} EFI_IP4_RECEIVE_DATA;


typedef struct {
  EFI_IPv4_ADDRESS       SourceAddress;
  EFI_IPv4_ADDRESS       GatewayAddress;
  UINT8                  Protocol;
  UINT8                  TypeOfService;
  UINT8                  TimeToLive;
  BOOLEAN                DoNotFragment;
} EFI_IP4_OVERRIDE_DATA;

typedef struct {
  EFI_IPv4_ADDRESS       DestinationAddress;
  EFI_IP4_OVERRIDE_DATA  *OverrideData;      //OPTIONAL
  UINT32                 OptionsLength;      //OPTIONAL
  VOID                   *OptionsBuffer;     //OPTIONAL
  UINT32                 TotalDataLength;
  UINT32                 FragmentCount;
  EFI_IP4_FRAGMENT_DATA  FragmentTable[1];
} EFI_IP4_TRANSMIT_DATA;

typedef struct {
  EFI_EVENT                Event;
  EFI_STATUS               Status;
  union {
    EFI_IP4_RECEIVE_DATA   *RxData;
    EFI_IP4_TRANSMIT_DATA  *TxData;
  } Packet;
} EFI_IP4_COMPLETION_TOKEN;

/**
  Gets the current operational settings for this instance of the EFI IPv4 Protocol driver.

  @param  This          Pointer to the EFI_IP4_PROTOCOL instance.
  @param  Ip4ModeData   Pointer to the EFI IPv4 Protocol mode data structure.
  @param  MnpConfigData Pointer to the managed network configuration data structure.
  @param  SnpData       Pointer to the simple network mode data structure.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_OUT_OF_RESOURCES  The required mode data could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IP4_GET_MODE_DATA) (
  IN  EFI_IP4_PROTOCOL                *This,
  OUT EFI_IP4_MODE_DATA               *Ip4ModeData     OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA *MnpConfigData   OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE         *SnpModeData     OPTIONAL
  )
;  

/**
  Assigns an IPv4 address and subnet mask to this EFI IPv4 Protocol driver instance.

  @param  This         Pointer to the EFI_IP4_PROTOCOL instance.
  @param  IpConfigData Pointer to the EFI IPv4 Protocol configuration data structure.

  @retval EFI_SUCCESS           The driver instance was successfully opened.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
  @retval EFI_UNSUPPORTED       One or more of the following conditions is TRUE:
                                A configuration protocol (DHCP, BOOTP, RARP, etc.) could
                                not be located when clients choose to use the default IPv4
                                address. This EFI IPv4 Protocol implementation does not
                                support this requested filter or timeout setting.
  @retval EFI_OUT_OF_RESOURCES  The EFI IPv4 Protocol driver instance data could not be allocated.
  @retval EFI_ALREADY_STARTED   The interface is already open and must be stopped before the
                                IPv4 address or subnet mask can be changed. The interface must
                                also be stopped when switching to/from raw packet mode.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred. The EFI IPv4
                                 Protocol driver instance is not opened.

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IP4_CONFIGURE) (
  IN EFI_IP4_PROTOCOL    *This,
  IN EFI_IP4_CONFIG_DATA *IpConfigData     OPTIONAL
  )
;  

/**
  Joins and leaves multicast groups.

  @param  This                  Pointer to the EFI_IP4_PROTOCOL instance.
  @param  JoinFlag              Set to TRUE to join the multicast group session and FALSE to leave.
  @param  GroupAddress          Pointer to the IPv4 multicast address.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following is TRUE:
                                - This is NULL.
                                - JoinFlag is TRUE and GroupAddress is NULL.
                                - GroupAddress is not NULL and *GroupAddress is
                                not a multicast IPv4 address.
  @retval EFI_NOT_STARTED       This instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_OUT_OF_RESOURCES  System resources could not be allocated.
  @retval EFI_UNSUPPORTED       This EFI IPv4 Protocol implementation does not support multicast groups.
  @retval EFI_ALREADY_STARTED   The group address is already in the group table (when
                                JoinFlag is TRUE).
  @retval EFI_NOT_FOUND         The group address is not in the group table (when JoinFlag is FALSE).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IP4_GROUPS) (
  IN EFI_IP4_PROTOCOL    *This,
  IN BOOLEAN             JoinFlag,
  IN EFI_IPv4_ADDRESS    *GroupAddress  OPTIONAL
  )
;    

/**
  Adds and deletes routing table entries.

  @param  This                   Pointer to the EFI_IP4_PROTOCOL instance.
  @param  DeleteRoute            Set to TRUE to delete this route from the routing table. Set to
                                 FALSE to add this route to the routing table. SubnetAddress
                                 and SubnetMask are used as the key to each route entry.
  @param  SubnetAddress          The address of the subnet that needs to be routed.
  @param  SubnetMask             The subnet mask of SubnetAddress.
  @param  GatewayAddress         The unicast gateway IPv4 address for this route.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The driver instance has not been started.
  @retval EFI_NO_MAPPING         When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - SubnetAddress is NULL.
                                 - SubnetMask is NULL.
                                 - GatewayAddress is NULL.
                                 - *SubnetAddress is not a valid subnet address.
                                 - *SubnetMask is not a valid subnet mask.
                                 - *GatewayAddress is not a valid unicast IPv4 address.
  @retval EFI_OUT_OF_RESOURCES   Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND          This route is not in the routing table (when DeleteRoute is TRUE).
  @retval EFI_ACCESS_DENIED      The route is already defined in the routing table (when
                                  DeleteRoute is FALSE).
                                 
**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IP4_ROUTES) (
  IN EFI_IP4_PROTOCOL    *This,
  IN BOOLEAN             DeleteRoute,
  IN EFI_IPv4_ADDRESS    *SubnetAddress,
  IN EFI_IPv4_ADDRESS    *SubnetMask,
  IN EFI_IPv4_ADDRESS    *GatewayAddress  
  )
;  

/**
  Places outgoing data packets into the transmit queue.

  @param  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param  Token Pointer to the transmit token.

  @retval  EFI_SUCCESS           The data has been queued for transmission.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval  EFI_INVALID_PARAMETER One or more pameters are invalid.
  @retval  EFI_ACCESS_DENIED     The transmit completion token with the same Token.Event
                                 was already in the transmit queue.
  @retval  EFI_NOT_READY         The completion token could not be queued because the transmit
                                 queue is full. 
  @retval  EFI_NOT_FOUND         Not route is found to destination address.
  @retval  EFI_OUT_OF_RESOURCES  Could not queue the transmit data.
  @retval  EFI_BUFFER_TOO_SMALL  Token.Packet.TxData.TotalDataLength is too
                                 short to transmit.
  @retval  EFI_BAD_BUFFER_SIZE   The length of the IPv4 header + option length + total data length is
                                 greater than MTU (or greater than the maximum packet size if
                                 Token.Packet.TxData.OverrideData.
                                 DoNotFragment is TRUE.)

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IP4_TRANSMIT) (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token
  )
;    

/**
  Places a receiving request into the receiving queue.

  @param  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param  Token Pointer to a token that is associated with the receive data descriptor.

  @retval EFI_SUCCESS           The receive completion token was cached.
  @retval EFI_NOT_STARTED       This EFI IPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP, RARP, etc.)
                                is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                - This is NULL.
                                - Token is NULL.
                                - Token.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES  The receive completion token could not be queued due to a lack of system
                                resources (usually memory).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
                                The EFI IPv4 Protocol instance has been reset to startup defaults.
                                EFI_ACCESS_DENIED The receive completion token with the same Token.Event was already
                                in the receive queue.
  @retval EFI_NOT_READY         The receive request could not be queued because the receive queue is full.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received.

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IP4_RECEIVE) (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token
  )
;      

/**
  Abort an asynchronous transmit or receive request.

  @param  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param  Token Pointer to a token that has been issued by
                EFI_IP4_PROTOCOL.Transmit() or
                EFI_IP4_PROTOCOL.Receive(). If NULL, all pending
                tokens are aborted. Type EFI_IP4_COMPLETION_TOKEN is
                defined in EFI_IP4_PROTOCOL.Transmit().

  @retval EFI_SUCCESS           The asynchronous I/O request was aborted and
                                Token.->Event was signaled. When Token is NULL, all
                                pending requests were aborted and their events were signaled.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_NOT_STARTED       This instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_NOT_FOUND         When Token is not NULL, the asynchronous I/O request was
                                not found in the transmit or receive queue. It has either completed
                                or was not issued by Transmit() and Receive().

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IP4_CANCEL) (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token OPTIONAL
  )
;      
  
/**
  Polls for incoming data packets and processes outgoing data packets.

  @param  This Pointer to the EFI_IP4_PROTOCOL instance.

  @retval  EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval  EFI_NOT_STARTED       This EFI IPv4 Protocol instance has not been started.
  @retval  EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval  EFI_NOT_READY         No incoming or outgoing data is processed.
  @retval  EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.
                                 Consider increasing the polling rate.

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IP4_POLL) (
  IN EFI_IP4_PROTOCOL          *This
  )
;  

struct _EFI_IP4_PROTOCOL {
  EFI_IP4_GET_MODE_DATA        GetModeData;
  EFI_IP4_CONFIGURE            Configure;
  EFI_IP4_GROUPS               Groups;
  EFI_IP4_ROUTES               Routes;
  EFI_IP4_TRANSMIT             Transmit;
  EFI_IP4_RECEIVE              Receive;
  EFI_IP4_CANCEL               Cancel;
  EFI_IP4_POLL                 Poll;
};

extern EFI_GUID gEfiIp4ServiceBindingProtocolGuid;
extern EFI_GUID gEfiIp4ProtocolGuid;

#endif
