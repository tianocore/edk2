/** @file
  Udp6 driver's whole implementation and internal data structures.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UDP6_IMPL_H_
#define _UDP6_IMPL_H_

#include <Uefi.h>

#include <Protocol/Ip6.h>
#include <Protocol/Udp6.h>

#include <Library/IpIoLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DpcLib.h>
#include <Library/PrintLib.h>

#include "Udp6Driver.h"

extern EFI_COMPONENT_NAME2_PROTOCOL   gUdp6ComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL    gUdp6ComponentName;
extern EFI_UNICODE_STRING_TABLE       *gUdp6ControllerNameTable;
extern EFI_SERVICE_BINDING_PROTOCOL   mUdp6ServiceBinding;
extern EFI_UDP6_PROTOCOL              mUdp6Protocol;
extern UINT16                         mUdp6RandomPort;

//
// Define time out 50 milliseconds
//
#define UDP6_TIMEOUT_INTERVAL (50 * TICKS_PER_MS)
#define UDP6_HEADER_SIZE      sizeof (EFI_UDP_HEADER)
#define UDP6_MAX_DATA_SIZE    65507
#define UDP6_PORT_KNOWN       1024

#define UDP6_SERVICE_DATA_SIGNATURE SIGNATURE_32 ('U', 'd', 'p', '6')
#define UDP6_INSTANCE_DATA_SIGNATURE  SIGNATURE_32 ('U', 'd', 'p', 'S')

#define UDP6_SERVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  UDP6_SERVICE_DATA, \
  ServiceBinding, \
  UDP6_SERVICE_DATA_SIGNATURE \
  )

#define UDP6_INSTANCE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  UDP6_INSTANCE_DATA, \
  Udp6Proto, \
  UDP6_INSTANCE_DATA_SIGNATURE \
  )
//
// Udp6 service contest data
//
typedef struct _UDP6_SERVICE_DATA {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;
  EFI_HANDLE                    ImageHandle;
  EFI_HANDLE                    ControllerHandle;
  LIST_ENTRY                    ChildrenList;
  UINTN                         ChildrenNumber;
  IP_IO                         *IpIo;
  EFI_EVENT                     TimeoutEvent;
 } UDP6_SERVICE_DATA;

typedef struct _UDP6_INSTANCE_DATA {
  UINT32                Signature;
  LIST_ENTRY            Link;
  UDP6_SERVICE_DATA     *Udp6Service;
  EFI_UDP6_PROTOCOL     Udp6Proto;
  EFI_UDP6_CONFIG_DATA  ConfigData;
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
  BOOLEAN               InDestroy;
} UDP6_INSTANCE_DATA;

typedef struct _UDP6_RXDATA_WRAP {
  LIST_ENTRY             Link;
  NET_BUF                *Packet;
  UINT32                 TimeoutTick;
  EFI_UDP6_RECEIVE_DATA  RxData;
} UDP6_RXDATA_WRAP;

typedef struct {
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;
} UDP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT;

/**
  Clean the Udp service context data.

  @param[in, out]  Udp6Service      Pointer to the UDP6_SERVICE_DATA.

**/
VOID
Udp6CleanService (
  IN OUT UDP6_SERVICE_DATA  *Udp6Service
  );

/**
  Create the Udp service context data.

  @param[in]  Udp6Service            Pointer to the UDP6_SERVICE_DATA.
  @param[in]  ImageHandle            The image handle of this udp6 driver.
  @param[in]  ControllerHandle       The controller handle this udp6 driver binds on.

  @retval EFI_SUCCESS            The udp6 service context data was created and
                                 initialized.
  @retval EFI_OUT_OF_RESOURCES   Cannot allocate memory.
  @retval Others                 An error condition occurred.

**/
EFI_STATUS
Udp6CreateService (
  IN UDP6_SERVICE_DATA  *Udp6Service,
  IN EFI_HANDLE         ImageHandle,
  IN EFI_HANDLE         ControllerHandle
  );
 
/**
  This function cleans the udp instance.

  @param[in, out]  Instance       Pointer to the UDP6_INSTANCE_DATA to clean.

**/
VOID
Udp6CleanInstance (
  IN OUT UDP6_INSTANCE_DATA  *Instance
  );
 
/**
  This function intializes the new created udp instance.

  @param[in]      Udp6Service      Pointer to the UDP6_SERVICE_DATA.
  @param[in, out]  Instance         Pointer to the un-initialized UDP6_INSTANCE_DATA.

**/
VOID
Udp6InitInstance (
  IN UDP6_SERVICE_DATA       *Udp6Service,
  IN OUT UDP6_INSTANCE_DATA  *Instance
  );

/**
  This function reports the received ICMP error.

  @param[in]  Instance          Pointer to the udp instance context data.

**/
VOID
Udp6ReportIcmpError (
  IN UDP6_INSTANCE_DATA  *Instance
  );

/**
  This function copies the current operational settings of this EFI UDPv6 Protocol
  instance into user-supplied buffers. This function is used optionally to retrieve
  the operational mode data of underlying networks or drivers.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[out] Udp6ConfigData     The buffer in which the current UDP configuration
                                 data is returned. This parameter is optional and
                                 may be NULL.
  @param[out] Ip6ModeData        The buffer in which the current EFI IPv6 Protocol
                                 mode data is returned. This parameter is optional
                                 and may be NULL.
  @param[out] MnpConfigData      The buffer in which the current managed network
                                 configuration data is returned. This parameter
                                 is optional and may be NULL.
  @param[out] SnpModeData        The buffer in which the simple network mode data
                                 is returned. This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The mode data was read.
  @retval EFI_NOT_STARTED        When Udp6ConfigData is queried, no configuration
                                 data is  available because this instance has not
                                 been started.
  @retval EFI_INVALID_PARAMETER  This is NULL.

**/
EFI_STATUS
EFIAPI
Udp6GetModeData (
  IN  EFI_UDP6_PROTOCOL                *This,
  OUT EFI_UDP6_CONFIG_DATA             *Udp6ConfigData OPTIONAL,
  OUT EFI_IP6_MODE_DATA                *Ip6ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  );

/**
  This function is used to do the following:
  Initialize and start this instance of the EFI UDPv6 Protocol.
  Change the filtering rules and operational parameters.
  Reset this instance of the EFI UDPv6 Protocol.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  UdpConfigData      Pointer to the buffer to set the configuration
                                 data. This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The configuration settings were set, changed, or
                                 reset successfully.
  @retval EFI_NO_MAPPING         When the UdpConifgData.UseAnyStationAddress is set
                                 to true  and there is no address available for IP6
                                 driver to binding  source address to this
                                 instance.
  @retval EFI_INVALID_PARAMETER  One or more following conditions are TRUE:
                                 This is NULL.
                                 UdpConfigData.StationAddress is not a valid
                                 unicast IPv6 address.
                                 UdpConfigData.RemoteAddress is not a valid unicast
                                 IPv6  address, if it is not zero.
  @retval EFI_ALREADY_STARTED    The EFI UDPv6 Protocol instance is already
                                 started/configured and must be stopped/reset
                                 before it can be reconfigured. Only TrafficClass,
                                 HopLimit, ReceiveTimeout, and TransmitTimeout can
                                 be reconfigured without stopping the current
                                 instance of the EFI UDPv6 Protocol.
  @retval EFI_ACCESS_DENIED      UdpConfigData.AllowDuplicatePort is FALSE, and
                                 UdpConfigData.StationPort is already used by another
                                 instance.
  @retval EFI_OUT_OF_RESOURCES   The EFI UDPv6 Protocol driver cannot allocate
                                 memory for this EFI UDPv6 Protocol instance.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred, and
                                 this instance was not opened.

**/
EFI_STATUS
EFIAPI
Udp6Configure (
  IN EFI_UDP6_PROTOCOL     *This,
  IN EFI_UDP6_CONFIG_DATA  *UdpConfigData OPTIONAL
  );

/**
  This function places a sending request to this instance of the EFI UDPv6 Protocol,
  alongside the transmit data that was filled by the user.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  Token              Pointer to the completion token that will be
                                 placed into the transmit queue.

  @retval EFI_SUCCESS            The data has been queued for transmission.
  @retval EFI_NOT_STARTED        This EFI UDPv6 Protocol instance has not been
                                 started.
  @retval EFI_NO_MAPPING         The under-layer IPv6 driver was responsible for
                                 choosing a source address for this instance, but
                                 no  source address was available for use.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                 This is NULL. Token is NULL. Token.Event is NULL.
                                 Token.Packet.TxData is NULL.
                                 Token.Packet.TxData.FragmentCount is zero.
                                 Token.Packet.TxData.DataLength is not equal to the
                                 sum of fragment lengths.
                                 One or more of the
                                 Token.Packet.TxData.FragmentTable[]
                                 .FragmentLength fields is zero.
                                 One or more of the
                                 Token.Packet.TxData.FragmentTable[]
                                 .FragmentBuffer fields is NULL.
                                 One or more of the
                                 Token.Packet.TxData.UdpSessionData.
                                 DestinationAddres are not valid unicast IPv6
                                 addresses, if the  UdpSessionData is not NULL.
                                 Token.Packet.TxData.UdpSessionData.
                                 DestinationAddres is NULL
                                 Token.Packet.TxData.UdpSessionData.
                                 DestinatioPort is zero.
                                 Token.Packet.TxData.UdpSessionData is
                                 NULL and this  instance's
                                 UdpConfigData.RemoteAddress is unspecified.
  @retval EFI_ACCESS_DENIED      The transmit completion token with the same
                                 Token.Event is already in the transmit queue.
  @retval EFI_NOT_READY          The completion token could not be queued because
                                 the transmit queue is full.
  @retval EFI_OUT_OF_RESOURCES   Could not queue the transmit data.
  @retval EFI_NOT_FOUND          There is no route to the destination network or
                                 address.
  @retval EFI_BAD_BUFFER_SIZE    The data length is greater than the maximum UDP
                                 packet size. Or the length of the IP header + UDP
                                 header + data length is greater than MTU if
                                 DoNotFragment is TRUE.

**/
EFI_STATUS
EFIAPI
Udp6Transmit (
  IN EFI_UDP6_PROTOCOL          *This,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token
  );

/**
  This function places a completion token into the receive packet queue. This function
  is always asynchronous.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  Token              Pointer to a token that is associated with the
                                 receive data descriptor.

  @retval EFI_SUCCESS            The receive completion token is cached.
  @retval EFI_NOT_STARTED        This EFI UDPv6 Protocol instance has not been
                                 started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL.
                                 Token is NULL.
                                 Token.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES   The receive completion token could not be queued
                                 due to a lack of system resources (usually
                                 memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The EFI UDPv6 Protocol instance has been reset to
                                 startup defaults.
  @retval EFI_ACCESS_DENIED      A receive completion token with the same
                                 Token.Event is already in the receive queue.
  @retval EFI_NOT_READY          The receive request could not be queued because
                                 the receive  queue is full.

**/
EFI_STATUS
EFIAPI
Udp6Receive (
  IN EFI_UDP6_PROTOCOL          *This,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token
  );

/**
  This function is used to abort a pending transmit or receive request.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  Token              Pointer to a token that has been issued by
                                 EFI_UDP6_PROTOCOL.Transmit() or
                                 EFI_UDP6_PROTOCOL.Receive(). This parameter is
                                 optional and may be NULL.

  @retval EFI_SUCCESS            The asynchronous I/O request is aborted and
                                 Token.Event is  signaled. When Token is NULL, all
                                 pending requests are aborted and their events are
                                 signaled.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        This instance has not been started.
  @retval EFI_NO_MAPPING         When using the default address, configuration
                                 (DHCP, BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_NOT_FOUND          When Token is not NULL, the asynchronous I/O
                                 request is not found in the transmit or receive
                                 queue. It either completed or was not issued by
                                 Transmit() or Receive().

**/
EFI_STATUS
EFIAPI
Udp6Cancel (
  IN EFI_UDP6_PROTOCOL          *This,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token OPTIONAL
  );

/**
  This function can be used by network drivers and applications to increase the rate that
  data packets are moved between the communications device and the transmit/receive queues.

  @param[in] This                Pointer to the EFI_UDP6_PROTOCOL instance.

  @retval EFI_SUCCESS            Incoming or outgoing data was processed.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  @retval EFI_TIMEOUT            Data was dropped out of the transmit and/or
                                 receive queue.

**/
EFI_STATUS
EFIAPI
Udp6Poll (
  IN EFI_UDP6_PROTOCOL  *This
  );

/**
  This function is used to enable and disable the multicast group filtering.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  JoinFlag           Set to TRUE to join a multicast group. Set to
                                 FALSE to leave one or all multicast groups.
  @param[in]  MulticastAddress   Pointer to multicast group address to join or
                                 leave. This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The EFI UDPv6 Protocol instance has not been
                                 started.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate resources to join the group.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. JoinFlag is TRUE and
                                 MulticastAddress is NULL. JoinFlag is TRUE and
                                 *MulticastAddress is not a valid  multicast
                                 address.
  @retval EFI_ALREADY_STARTED    The group address is already in the group table
                                 (when JoinFlag is TRUE).
  @retval EFI_NOT_FOUND          The group address is not in the group table (when
                                 JoinFlag is FALSE).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
Udp6Groups (
  IN EFI_UDP6_PROTOCOL  *This,
  IN BOOLEAN            JoinFlag,
  IN EFI_IPv6_ADDRESS   *MulticastAddress OPTIONAL
  );

/**
  This function tries to bind the udp instance according to the configured port
  allocation stragety.

  @param[in]  InstanceList       Pointer to the head of the list linking the udp
                                 instances.
  @param[in]  ConfigData         Pointer to the ConfigData of the instance to be
                                 bound.

  @retval EFI_SUCCESS            The bound operation completed successfully.
  @retval EFI_ACCESS_DENIED      The <Address, Port> specified by the ConfigData is
                                 already used by another instance.
  @retval EFI_OUT_OF_RESOURCES   No available port resources.

**/
EFI_STATUS
Udp6Bind (
  IN LIST_ENTRY            *InstanceList,
  IN EFI_UDP6_CONFIG_DATA  *ConfigData
  );

/**
  This function builds the Ip6 configdata from the Udp6ConfigData.

  @param[in]       Udp6ConfigData         Pointer to the EFI_UDP6_CONFIG_DATA.
  @param[in, out]  Ip6ConfigData          Pointer to the EFI_IP6_CONFIG_DATA.

**/
VOID
Udp6BuildIp6ConfigData (
  IN EFI_UDP6_CONFIG_DATA      *Udp6ConfigData,
  IN OUT EFI_IP6_CONFIG_DATA   *Ip6ConfigData
  );

/**
  This function checks whether the specified Token duplicates with the one in the Map.

  @param[in]  Map                Pointer to the NET_MAP.
  @param[in]  Item               Pointer to the NET_MAP_ITEM contain the pointer to
                                 the Token.
  @param[in]  Context            Pointer to the Token to be checked.

  @retval EFI_SUCCESS            The Token specified by Context differs from the
                                 one in the Item.
  @retval EFI_ACCESS_DENIED      The Token duplicates with the one in the Item.

**/
EFI_STATUS
EFIAPI
Udp6TokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  );

/**
  This function removes the specified Token from the TokenMap.

  @param[in]  TokenMap           Pointer to the NET_MAP containing the tokens.
  @param[in]  Token              Pointer to the Token to be removed.

  @retval EFI_SUCCESS            The specified Token is removed from the TokenMap.
  @retval EFI_NOT_FOUND          The specified Token is not found in the TokenMap.

**/
EFI_STATUS
Udp6RemoveToken (
  IN NET_MAP                    *TokenMap,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token
  );

/**
  This function is used to check whether the NewConfigData has any un-reconfigurable
  parameters changed compared to the OldConfigData.

  @param[in]  OldConfigData    Pointer to the current ConfigData the udp instance
                               uses.
  @param[in]  NewConfigData    Pointer to the new ConfigData.

  @retval TRUE     The instance is reconfigurable according to NewConfigData.
  @retval FALSE   The instance is not reconfigurable according to NewConfigData.

**/
BOOLEAN
Udp6IsReconfigurable (
  IN EFI_UDP6_CONFIG_DATA  *OldConfigData,
  IN EFI_UDP6_CONFIG_DATA  *NewConfigData
  );

/**
  This function removes the multicast group specified by Arg from the Map.

  @param[in]  Map                Pointer to the NET_MAP.
  @param[in]  Item               Pointer to the NET_MAP_ITEM.
  @param[in]  Arg                Pointer to the Arg. It is the pointer to a
                                 multicast IPv6 Address. This parameter is
                                 optional and may be NULL.

  @retval EFI_SUCCESS            The multicast address is removed.
  @retval EFI_ABORTED            The specified multicast address is removed, and the
                                 Arg is not NULL.

**/
EFI_STATUS
EFIAPI
Udp6LeaveGroup (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  );

/**
  This function validates the TxToken, it returns the error code according to the spec.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  TxToken            Pointer to the token to be checked.

  @retval EFI_SUCCESS            The TxToken is valid.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                 Token.Event is NULL.
                                 Token.Packet.TxData is NULL.
                                 Token.Packet.TxData.FragmentCount is zero.
                                 Token.Packet.TxData.DataLength is not equal to the
                                 sum of fragment lengths.
                                 One or more of the
                                 Token.Packet.TxData.FragmentTable[].FragmentLength
                                 fields is zero.
                                 One or more of the
                                 Token.Packet.TxData.FragmentTable[].FragmentBuffer
                                 fields is NULL.
                                 UdpSessionData.DestinationAddress are not valid
                                 unicast IPv6 addresses if the UdpSessionData is
                                 not NULL.
                                 UdpSessionData.DestinationPort and
                                 ConfigData.RemotePort are all zero if the
                                 UdpSessionData is not NULL.
  @retval EFI_BAD_BUFFER_SIZE    The data length is greater than the maximum UDP
                                 packet size.

**/
EFI_STATUS
Udp6ValidateTxToken (
  IN UDP6_INSTANCE_DATA         *Instance,
  IN EFI_UDP6_COMPLETION_TOKEN  *TxToken
  );

/**
  This function is a dummy ext-free function for the NET_BUF created for the output
  udp datagram.

  @param[in]  Context               Pointer to the context data.

**/
VOID
EFIAPI
Udp6NetVectorExtFree (
  IN VOID  *Context
  );

/**
  This function calculates the checksum for the Packet, utilizing the pre-calculated
  pseudo header to reduce overhead.

  @param[in]  Packet           Pointer to the NET_BUF contains the udp datagram.
  @param[in]  HeadSum          Checksum of the pseudo header execpt the length
                               field.

  @return The 16-bit checksum of this udp datagram.

**/
UINT16
Udp6Checksum (
  IN NET_BUF *Packet,
  IN UINT16  HeadSum
  );

/**
  This function delivers the received datagrams to the specified instance.

  @param[in]  Instance               Pointer to the instance context data.

**/
VOID
Udp6InstanceDeliverDgram (
  IN UDP6_INSTANCE_DATA  *Instance
  );

/**
  Cancel Udp6 tokens from the Udp6 instance.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  Token              Pointer to the token to be canceled. If NULL, all
                                 tokens in this instance will be cancelled.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The Token is cancelled.
  @retval EFI_NOT_FOUND          The Token is not found.

**/
EFI_STATUS
Udp6InstanceCancelToken (
  IN UDP6_INSTANCE_DATA         *Instance,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token OPTIONAL
  );

/**
  This function removes all the Wrap datas in the RcvdDgramQue.

  @param[in]  Instance    Pointer to the Udp6 Instance.

**/
VOID
Udp6FlushRcvdDgram (
  IN UDP6_INSTANCE_DATA  *Instance
  );

#endif

