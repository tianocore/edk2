/** @file
  EFI_MANAGED_NETWORK_SERVICE_BINDING_PROTOCOL as defined in UEFI 2.0.
  EFI_MANAGED_NETWORK_PROTOCOL as defined in UEFI 2.0.

  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_MANAGED_NETWORK_PROTOCOL_H__
#define __EFI_MANAGED_NETWORK_PROTOCOL_H__

#include <Protocol/SimpleNetwork.h>

#define EFI_MANAGED_NETWORK_SERVICE_BINDING_PROTOCOL_GUID \
  { \
    0xf36ff770, 0xa7e1, 0x42cf, {0x9e, 0xd2, 0x56, 0xf0, 0xf2, 0x71, 0xf4, 0x4c } \
  }

#define EFI_MANAGED_NETWORK_PROTOCOL_GUID \
  { \
    0x7ab33a91, 0xace5, 0x4326, { 0xb5, 0x72, 0xe7, 0xee, 0x33, 0xd3, 0x9f, 0x16 } \
  }

typedef struct _EFI_MANAGED_NETWORK_PROTOCOL EFI_MANAGED_NETWORK_PROTOCOL;

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
  EFI_MAC_ADDRESS                   *DestinationAddress; //OPTIONAL
  EFI_MAC_ADDRESS                   *SourceAddress;      //OPTIONAL
  UINT16                            ProtocolType;        //OPTIONAL
  UINT32                            DataLength;
  UINT16                            HeaderLength;        //OPTIONAL
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

/**
  Returns the operational parameters for the current MNP child driver.

  @param  This          Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param  MnpConfigData Pointer to storage for MNP operational parameters.
  @param  SnpModeData   Pointer to storage for SNP operational parameters.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this MNP implementation.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been configured. The default
                                values are returned in MnpConfigData if it is not NULL.
  @retval Other                 The mode data could not be read.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_GET_MODE_DATA)(
  IN  EFI_MANAGED_NETWORK_PROTOCOL     *This,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  )
;

/**
  Sets or clears the operational parameters for the MNP child driver.

  @param  This          Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param  MnpConfigData Pointer to configuration data that will be assigned to the MNP
                        child driver instance. If NULL, the MNP child driver instance is
                        reset to startup defaults and all pending transmit and receive
                        requests are flushed.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  Required system resources (usually memory) could not be
                                allocated.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this [MNP]
                                implementation.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred.
  @retval Other                 The MNP child driver instance has been reset to startup defaults.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_CONFIGURE)(
  IN EFI_MANAGED_NETWORK_PROTOCOL     *This,
  IN EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL
  )
;

/**
  Translates an IP multicast address to a hardware (MAC) multicast address.

  @param  This       Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param  Ipv6Flag   Set to TRUE to if IpAddress is an IPv6 multicast address.
                     Set to FALSE if IpAddress is an IPv4 multicast address.
  @param  IpAddress  Pointer to the multicast IP address (in network byte order) to convert.
  @param  MacAddress Pointer to the resulting multicast MAC address.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER One of the following conditions is TRUE:
                                - This is NULL.
                                - IpAddress is NULL.
                                - *IpAddress is not a valid multicast IP address.
                                - MacAddress is NULL.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been configured.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this MNP implementation.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred.
  @retval Other                 The address could not be converted.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_MCAST_IP_TO_MAC)(
  IN  EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN  BOOLEAN                       Ipv6Flag,
  IN  EFI_IP_ADDRESS                *IpAddress,
  OUT EFI_MAC_ADDRESS               *MacAddress
  )
;

/**
  Enables and disables receive filters for multicast address.

  @param  This       Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param  JoinFlag   Set to TRUE to join this multicast group.
                     Set to FALSE to leave this multicast group.
  @param  MacAddress Pointer to the multicast MAC group (address) to join or leave.

  @retval EFI_SUCCESS           The requested operation completed successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                - This is NULL.
                                - JoinFlag is TRUE and MacAddress is NULL.
                                - *MacAddress is not a valid multicast MAC address.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been configured.
  @retval EFI_ALREADY_STARTED   The supplied multicast group is already joined.
  @retval EFI_NOT_FOUND         The supplied multicast group is not joined.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this MNP implementation.
  @retval Other                 The requested operation could not be completed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_GROUPS)(
  IN EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                       JoinFlag,
  IN EFI_MAC_ADDRESS               *MacAddress  OPTIONAL
  )
;

/**
  Places asynchronous outgoing data packets into the transmit queue.

  @param  This  Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param  Token Pointer to a token associated with the transmit data descriptor.

  @retval EFI_SUCCESS           The transmit completion token was cached.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been configured.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_ACCESS_DENIED     The transmit completion token is already in the transmit queue.
  @retval EFI_OUT_OF_RESOURCES  The transmit data could not be queued due to a lack of system resources
                                (usually memory).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_NOT_READY         The transmit request could not be queued because the transmit queue is full.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_TRANSMIT)(
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  )
;

/**
  Places an asynchronous receiving request into the receiving queue.

  @param  This  Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param  Token Pointer to a token associated with the receive data descriptor.

  @retval EFI_SUCCESS           The receive completion token was cached.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been configured.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                - This is NULL.
                                - Token is NULL.
                                - Token.Event is NULL
  @retval EFI_OUT_OF_RESOURCES  The transmit data could not be queued due to a lack of system resources
                                (usually memory).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_ACCESS_DENIED     The receive completion token was already in the receive queue.
  @retval EFI_NOT_READY         The receive request could not be queued because the receive queue is full.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_RECEIVE)(
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  )
;


/**
  Aborts an asynchronous transmit or receive request.

  @param  This  Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param  Token Pointer to a token that has been issued by
                EFI_MANAGED_NETWORK_PROTOCOL.Transmit() or
                EFI_MANAGED_NETWORK_PROTOCOL.Receive(). If
                NULL, all pending tokens are aborted.

  @retval  EFI_SUCCESS           The asynchronous I/O request was aborted and Token.Event
                                 was signaled. When Token is NULL, all pending requests were
                                 aborted and their events were signaled.
  @retval  EFI_NOT_STARTED       This MNP child driver instance has not been configured.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_NOT_FOUND         When Token is not NULL, the asynchronous I/O request was
                                 not found in the transmit or receive queue. It has either completed
                                 or was not issued by Transmit() and Receive().

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_CANCEL)(
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token  OPTIONAL
  )
;

/**
  Polls for incoming data packets and processes outgoing data packets.

  @param  This Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.

  @retval EFI_SUCCESS      Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED  This MNP child driver instance has not been configured.
  @retval EFI_DEVICE_ERROR An unexpected system or network error occurred.
  @retval EFI_NOT_READY    No incoming or outgoing data was processed. Consider increasing
                           the polling rate.
  @retval EFI_TIMEOUT      Data was dropped out of the transmit and/or receive queue.
                            Consider increasing the polling rate.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MANAGED_NETWORK_POLL)(
  IN EFI_MANAGED_NETWORK_PROTOCOL    *This
  )
;

/**  
  @par Protocol Description:
  The MNP is used by network applications (and drivers) to 
  perform raw (unformatted) asynchronous network packet I/O.
 
  @param GetModeData
  Returns the current MNP child driver operational parameters. 
  May also support returning underlying Simple Network Protocol (SNP) driver mode data. 

  @param Configure
  Sets and clears operational parameters for an MNP child driver. 

  @param McastIpToMac
  Translates a software (IP) multicast address to a hardware (MAC) multicast address. 
  This function may be unsupported in some MNP implementations. 

  @param Groups
  Enables and disables receive filters for multicast addresses. 
  This function may be unsupported in some MNP implementations. 

  @param Transmit
  Places asynchronous outgoing data packets into the transmit queue. 

  @param Receive
  Places an asynchronous receiving request into the receiving queue. 

  @param Cancel
  Aborts a pending transmit or receive request. 

  @param Poll
  Polls for incoming data packets and processes outgoing data packets. 

**/
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
