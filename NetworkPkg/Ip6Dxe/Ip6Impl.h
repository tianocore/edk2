/** @file
  Implementation of EFI_IP6_PROTOCOL protocol interfaces and type definitions.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP6_IMPL_H__
#define __EFI_IP6_IMPL_H__

#include <Uefi.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/ManagedNetwork.h>
#include <Protocol/IpSec.h>
#include <Protocol/Ip6.h>
#include <Protocol/Ip6Config.h>
#include <Protocol/Dhcp6.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DpcLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>

#include <Guid/MdeModuleHii.h>

#include "Ip6Common.h"
#include "Ip6Driver.h"
#include "Ip6Icmp.h"
#include "Ip6If.h"
#include "Ip6Input.h"
#include "Ip6Mld.h"
#include "Ip6Nd.h"
#include "Ip6Option.h"
#include "Ip6Output.h"
#include "Ip6Route.h"
#include "Ip6ConfigNv.h"
#include "Ip6ConfigImpl.h"

#define IP6_PROTOCOL_SIGNATURE SIGNATURE_32 ('I', 'P', '6', 'P')
#define IP6_SERVICE_SIGNATURE  SIGNATURE_32 ('I', 'P', '6', 'S')

//
// The state of IP6 protocol. It starts from UNCONFIGED. if it is
// successfully configured, it goes to CONFIGED. if configure NULL
// is called, it becomes UNCONFIGED again. If (partly) destroyed, it
// becomes DESTROY.
//
#define IP6_STATE_UNCONFIGED   0
#define IP6_STATE_CONFIGED     1

//
// The state of IP6 service. It starts from UNSTARTED. It transits
// to STARTED if autoconfigure is started. If default address is
// configured, it becomes CONFIGED. and if partly destroyed, it goes
// to DESTROY.
//
#define IP6_SERVICE_UNSTARTED  0
#define IP6_SERVICE_STARTED    1
#define IP6_SERVICE_CONFIGED   2
#define IP6_SERVICE_DESTROY    3

#define IP6_INSTANCE_FROM_PROTOCOL(Ip6) \
          CR ((Ip6), IP6_PROTOCOL, Ip6Proto, IP6_PROTOCOL_SIGNATURE)

#define IP6_SERVICE_FROM_PROTOCOL(Sb)   \
          CR ((Sb), IP6_SERVICE, ServiceBinding, IP6_SERVICE_SIGNATURE)

#define IP6_NO_MAPPING(IpInstance) (!(IpInstance)->Interface->Configured)

extern EFI_IPSEC2_PROTOCOL *mIpSec;
extern BOOLEAN             mIpSec2Installed;

//
// IP6_TXTOKEN_WRAP wraps the upper layer's transmit token.
// The user's data is kept in the Packet. When fragment is
// needed, each fragment of the Packet has a reference to the
// Packet, no data is actually copied. The Packet will be
// released when all the fragments of it have been recycled by
// MNP. Upon then, the IP6_TXTOKEN_WRAP will be released, and
// user's event signalled.
//
typedef struct {
  IP6_PROTOCOL              *IpInstance;
  EFI_IP6_COMPLETION_TOKEN  *Token;
  EFI_EVENT                 IpSecRecycleSignal;
  NET_BUF                   *Packet;
  BOOLEAN                   Sent;
  INTN                      Life;
} IP6_TXTOKEN_WRAP;

typedef struct {
  EFI_EVENT                 IpSecRecycleSignal;
  NET_BUF                   *Packet;
} IP6_IPSEC_WRAP;

//
// IP6_RXDATA_WRAP wraps the data IP6 child delivers to the
// upper layers. The received packet is kept in the Packet.
// The Packet itself may be constructured from some fragments.
// All the fragments of the Packet is organized by a
// IP6_ASSEMBLE_ENTRY structure. If the Packet is recycled by
// the upper layer, the assemble entry and its associated
// fragments will be freed at last.
//
typedef struct {
  LIST_ENTRY                Link;
  IP6_PROTOCOL              *IpInstance;
  NET_BUF                   *Packet;
  EFI_IP6_RECEIVE_DATA      RxData;
} IP6_RXDATA_WRAP;

struct _IP6_PROTOCOL {
  UINT32                    Signature;

  EFI_IP6_PROTOCOL          Ip6Proto;
  EFI_HANDLE                Handle;
  INTN                      State;

  IP6_SERVICE               *Service;
  LIST_ENTRY                Link; // Link to all the IP protocol from the service

  UINT8                     PrefixLength; // PrefixLength of the configured station address.
  //
  // User's transmit/receive tokens, and received/deliverd packets
  //
  NET_MAP                   RxTokens;
  NET_MAP                   TxTokens;   // map between (User's Token, IP6_TXTOKE_WRAP)
  LIST_ENTRY                Received;   // Received but not delivered packet
  LIST_ENTRY                Delivered;  // Delivered and to be recycled packets
  EFI_LOCK                  RecycleLock;

  IP6_INTERFACE             *Interface;
  LIST_ENTRY                AddrLink;   // Ip instances with the same IP address.

  EFI_IPv6_ADDRESS          *GroupList; // stored in network order.
  UINT32                    GroupCount;

  EFI_IP6_CONFIG_DATA       ConfigData;
  BOOLEAN                   InDestroy;
};

struct _IP6_SERVICE {
  UINT32                          Signature;
  EFI_SERVICE_BINDING_PROTOCOL    ServiceBinding;
  INTN                            State;

  //
  // List of all the IP instances and interfaces, and default
  // interface and route table and caches.
  //
  UINTN                           NumChildren;
  LIST_ENTRY                      Children;

  LIST_ENTRY                      Interfaces;

  IP6_INTERFACE                   *DefaultInterface;
  IP6_ROUTE_TABLE                 *RouteTable;

  IP6_LINK_RX_TOKEN               RecvRequest;

  //
  // Ip reassemble utilities and MLD data
  //
  IP6_ASSEMBLE_TABLE              Assemble;
  IP6_MLD_SERVICE_DATA            MldCtrl;

  EFI_IPv6_ADDRESS                LinkLocalAddr;
  BOOLEAN                         LinkLocalOk;
  BOOLEAN                         LinkLocalDadFail;
  BOOLEAN                         Dhcp6NeedStart;
  BOOLEAN                         Dhcp6NeedInfoRequest;

  //
  // ND data
  //
  UINT8                           CurHopLimit;
  UINT32                          LinkMTU;
  UINT32                          BaseReachableTime;
  UINT32                          ReachableTime;
  UINT32                          RetransTimer;
  LIST_ENTRY                      NeighborTable;

  LIST_ENTRY                      OnlinkPrefix;
  LIST_ENTRY                      AutonomousPrefix;

  LIST_ENTRY                      DefaultRouterList;
  UINT32                          RoundRobin;

  UINT8                           InterfaceIdLen;
  UINT8                           *InterfaceId;

  BOOLEAN                         RouterAdvertiseReceived;
  UINT8                           SolicitTimer;
  UINT32                          Ticks;

  //
  // Low level protocol used by this service instance
  //
  EFI_HANDLE                      Image;
  EFI_HANDLE                      Controller;

  EFI_HANDLE                      MnpChildHandle;
  EFI_MANAGED_NETWORK_PROTOCOL    *Mnp;

  EFI_MANAGED_NETWORK_CONFIG_DATA MnpConfigData;
  EFI_SIMPLE_NETWORK_MODE         SnpMode;

  EFI_EVENT                       Timer;
  EFI_EVENT                       FasterTimer;

  //
  // IPv6 Configuration Protocol instance
  //
  IP6_CONFIG_INSTANCE             Ip6ConfigInstance;

  //
  // The string representation of the current mac address of the
  // NIC this IP6_SERVICE works on.
  //
  CHAR16                          *MacString;
  UINT32                          MaxPacketSize;
  UINT32                          OldMaxPacketSize;
};

/**
  The callback function for the net buffer which wraps the user's
  transmit token. Although this function seems simple,
  there are some subtle aspects.
  When a user requests the IP to transmit a packet by passing it a
  token, the token is wrapped in an IP6_TXTOKEN_WRAP and the data
  is wrapped in a net buffer. The net buffer's Free function is
  set to Ip6FreeTxToken. The Token and token wrap are added to the
  IP child's TxToken map. Then the buffer is passed to Ip6Output for
  transmission. If an error occurs before that, the buffer
  is freed, which in turn frees the token wrap. The wrap may
  have been added to the TxToken map or not, and the user's event
  shouldn't be signaled because we are still in the EfiIp6Transmit. If
  the buffer has been sent by Ip6Output, it should be removed from
  the TxToken map and the user's event signaled. The token wrap and buffer
  are bound together. Refer to the comments in Ip6Output for information
  about IP fragmentation.

  @param[in]  Context                The token's wrap.

**/
VOID
EFIAPI
Ip6FreeTxToken (
  IN VOID                   *Context
  );

/**
  Config the MNP parameter used by IP. The IP driver use one MNP
  child to transmit/receive frames. By default, it configures MNP
  to receive unicast/multicast/broadcast. And it will enable/disable
  the promiscuous receive according to whether there is IP child
  enable that or not. If Force is FALSE, it will iterate through
  all the IP children to check whether the promiscuous receive
  setting has been changed. If it hasn't been changed, it won't
  reconfigure the MNP. If Force is TRUE, the MNP is configured
  whether that is changed or not.

  @param[in]  IpSb               The IP6 service instance that is to be changed.
  @param[in]  Force              Force the configuration or not.

  @retval EFI_SUCCESS            The MNP successfully configured/reconfigured.
  @retval Others                 The configuration failed.

**/
EFI_STATUS
Ip6ServiceConfigMnp (
  IN IP6_SERVICE            *IpSb,
  IN BOOLEAN                Force
  );

/**
  Cancel the user's receive/transmit request. It is the worker function of
  EfiIp6Cancel API.

  @param[in]  IpInstance         The IP6 child.
  @param[in]  Token              The token to cancel. If NULL, all tokens will be
                                 cancelled.

  @retval EFI_SUCCESS            The token was cancelled.
  @retval EFI_NOT_FOUND          The token isn't found on either the
                                 transmit or receive queue.
  @retval EFI_DEVICE_ERROR       Not all tokens are cancelled when Token is NULL.

**/
EFI_STATUS
Ip6Cancel (
  IN IP6_PROTOCOL             *IpInstance,
  IN EFI_IP6_COMPLETION_TOKEN *Token          OPTIONAL
  );

/**
  Initialize the IP6_PROTOCOL structure to the unconfigured states.

  @param[in]       IpSb                   The IP6 service instance.
  @param[in, out]  IpInstance             The IP6 child instance.

**/
VOID
Ip6InitProtocol (
  IN IP6_SERVICE            *IpSb,
  IN OUT IP6_PROTOCOL       *IpInstance
  );

/**
  Clean up the IP6 child, release all the resources used by it.

  @param[in, out]  IpInstance    The IP6 child to clean up.

  @retval EFI_SUCCESS            The IP6 child was cleaned up
  @retval EFI_DEVICE_ERROR       Some resources failed to be released.

**/
EFI_STATUS
Ip6CleanProtocol (
  IN OUT IP6_PROTOCOL            *IpInstance
  );

//
// EFI_IP6_PROTOCOL interface prototypes
//

/**
  Gets the current operational settings for this instance of the EFI IPv6 Protocol driver.

  The GetModeData() function returns the current operational mode data for this driver instance.
  The data fields in EFI_IP6_MODE_DATA are read only. This function is used optionally to
  retrieve the operational mode data of underlying networks or drivers.

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.
  @param[out] Ip6ModeData        The pointer to the EFI IPv6 Protocol mode data structure.
  @param[out] MnpConfigData      The pointer to the managed network configuration data structure.
  @param[out] SnpModeData        The pointer to the simple network mode data structure.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_OUT_OF_RESOURCES   The required mode data could not be allocated.

**/
EFI_STATUS
EFIAPI
EfiIp6GetModeData (
  IN EFI_IP6_PROTOCOL                 *This,
  OUT EFI_IP6_MODE_DATA               *Ip6ModeData     OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA *MnpConfigData   OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE         *SnpModeData     OPTIONAL
  );

/**
  Assigns an IPv6 address and subnet mask to this EFI IPv6 Protocol driver instance.

  The Configure() function is used to set, change, or reset the operational parameters and filter
  settings for this EFI IPv6 Protocol instance. Until these parameters have been set, no network traffic
  can be sent or received by this instance. Once the parameters have been reset (by calling this
  function with Ip6ConfigData set to NULL), no more traffic can be sent or received until these
  parameters have been set again. Each EFI IPv6 Protocol instance can be started and stopped
  independently of each other by enabling or disabling their receive filter settings with the
  Configure() function.

  If Ip6ConfigData.StationAddress is a valid non-zero IPv6 unicast address, it is required
  to be one of the currently configured IPv6 addresses list in the EFI IPv6 drivers, or else
  EFI_INVALID_PARAMETER will be returned. If Ip6ConfigData.StationAddress is
  unspecified, the IPv6 driver will bind a source address according to the source address selection
  algorithm. Clients could frequently call GetModeData() to check get a currently configured IPv6.
  If both Ip6ConfigData.StationAddress and Ip6ConfigData.Destination are unspecified, when
  transmitting the packet afterwards, the source address filled in each outgoing IPv6 packet
  is decided based on the destination of this packet.

  If operational parameters are reset or changed, any pending transmit and receive requests will be
  cancelled. Their completion token status will be set to EFI_ABORTED, and their events will be
  signaled.

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  Ip6ConfigData      The pointer to the EFI IPv6 Protocol configuration data structure.
                                 If NULL, reset the configuration data.

  @retval EFI_SUCCESS            The driver instance was successfully opened.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - Ip6ConfigData.StationAddress is neither zero nor
                                   a unicast IPv6 address.
                                 - Ip6ConfigData.StationAddress is neither zero nor
                                   one of the configured IP addresses in the EFI IPv6 driver.
                                 - Ip6ConfigData.DefaultProtocol is illegal.
  @retval EFI_OUT_OF_RESOURCES   The EFI IPv6 Protocol driver instance data could not be allocated.
  @retval EFI_NO_MAPPING         The IPv6 driver was responsible for choosing a source address for
                                 this instance, but no source address was available for use.
  @retval EFI_ALREADY_STARTED    The interface is already open and must be stopped before the IPv6
                                 address or prefix length can be changed.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred. The EFI IPv6
                                 Protocol driver instance was not opened.
  @retval EFI_UNSUPPORTED        Default protocol specified through
                                 Ip6ConfigData.DefaulProtocol isn't supported.

**/
EFI_STATUS
EFIAPI
EfiIp6Configure (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_CONFIG_DATA       *Ip6ConfigData OPTIONAL
  );

/**
  Joins and leaves multicast groups.

  The Groups() function is used to join and leave multicast group sessions. Joining a group will
  enable reception of matching multicast packets. Leaving a group will disable reception of matching
  multicast packets. Source-Specific Multicast isn't required to be supported.

  If JoinFlag is FALSE and GroupAddress is NULL, all joined groups will be left.

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  JoinFlag           Set to TRUE to join the multicast group session and FALSE to leave.
  @param[in]  GroupAddress       The pointer to the IPv6 multicast address.
                                 This is an optional parameter that may be NULL.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more of the following is TRUE:
                                 - This is NULL.
                                 - JoinFlag is TRUE and GroupAddress is NULL.
                                 - GroupAddress is not NULL and *GroupAddress is
                                   not a multicast IPv6 address.
                                 - GroupAddress is not NULL and *GroupAddress is in the
                                   range of SSM destination address.
  @retval EFI_NOT_STARTED        This instance has not been started.
  @retval EFI_OUT_OF_RESOURCES   System resources could not be allocated.
  @retval EFI_UNSUPPORTED        This EFI IPv6 Protocol implementation does not support multicast groups.
  @retval EFI_ALREADY_STARTED    The group address is already in the group table (when
                                 JoinFlag is TRUE).
  @retval EFI_NOT_FOUND          The group address is not in the group table (when JoinFlag is FALSE).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp6Groups (
  IN EFI_IP6_PROTOCOL  *This,
  IN BOOLEAN           JoinFlag,
  IN EFI_IPv6_ADDRESS  *GroupAddress  OPTIONAL
  );

/**
  Adds and deletes routing table entries.

  The Routes() function adds a route to or deletes a route from the routing table.

  Routes are determined by comparing the leftmost PrefixLength bits of Destination with
  the destination IPv6 address arithmetically. The gateway address must be on the same subnet as the
  configured station address.

  The default route is added with Destination and PrefixLegth both set to all zeros. The
  default route matches all destination IPv6 addresses that do not match any other routes.

  All EFI IPv6 Protocol instances share a routing table.

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  DeleteRoute        Set to TRUE to delete this route from the routing table. Set to
                                 FALSE to add this route to the routing table. Destination,
                                 PrefixLength and Gateway are used as the key to each
                                 route entry.
  @param[in]  Destination        The address prefix of the subnet that needs to be routed.
                                 This is an optional parameter that may be NULL.
  @param[in]  PrefixLength       The prefix length of Destination. Ignored if Destination
                                 is NULL.
  @param[in]  GatewayAddress     The unicast gateway IPv6 address for this route.
                                 This is an optional parameter that may be NULL.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The driver instance has not been started.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - When DeleteRoute is TRUE, both Destination and
                                   GatewayAddress are NULL.
                                 - When DeleteRoute is FALSE, either Destination or
                                   GatewayAddress is NULL.
                                 - *GatewayAddress is not a valid unicast IPv6 address.
                                 - *GatewayAddress is one of the local configured IPv6
                                   addresses.
  @retval EFI_OUT_OF_RESOURCES   Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND          This route is not in the routing table (when DeleteRoute is TRUE).
  @retval EFI_ACCESS_DENIED      The route is already defined in the routing table (when
                                 DeleteRoute is FALSE).

**/
EFI_STATUS
EFIAPI
EfiIp6Routes (
  IN EFI_IP6_PROTOCOL    *This,
  IN BOOLEAN             DeleteRoute,
  IN EFI_IPv6_ADDRESS    *Destination    OPTIONAL,
  IN UINT8               PrefixLength,
  IN EFI_IPv6_ADDRESS    *GatewayAddress OPTIONAL
  );

/**
  Add or delete Neighbor cache entries.

  The Neighbors() function is used to add, update, or delete an entry from a neighbor cache.
  IPv6 neighbor cache entries are typically inserted and updated by the network protocol driver as
  network traffic is processed. Most neighbor cache entries will timeout and be deleted if the network
  traffic stops. Neighbor cache entries that were inserted by Neighbors() may be static (will not
  timeout) or dynamic (will timeout).

  The implementation should follow the neighbor cache timeout mechanism defined in
  RFC4861. The default neighbor cache timeout value should be tuned for the expected network
  environment.

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  DeleteFlag         Set to TRUE to delete the specified cache entry. Set to FALSE to
                                 add (or update, if it already exists and Override is TRUE) the
                                 specified cache entry. TargetIp6Address is used as the key
                                 to find the requested cache entry.
  @param[in]  TargetIp6Address   The pointer to the Target IPv6 address.
  @param[in]  TargetLinkAddress  The pointer to link-layer address of the target. Ignored if NULL.
  @param[in]  Timeout            Time in 100-ns units that this entry will remain in the neighbor
                                 cache, it will be deleted after Timeout. A value of zero means that
                                 the entry is permanent. A non-zero value means that the entry is
                                 dynamic.
  @param[in]  Override           If TRUE, the cached link-layer address of the matching entry will
                                 be overridden and updated; if FALSE, EFI_ACCESS_DENIED
                                 will be returned if a corresponding cache entry already exists.

  @retval  EFI_SUCCESS           The data has been queued for transmission.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - TargetIpAddress is NULL.
                                 - *TargetLinkAddress is invalid when not NULL.
                                 - *TargetIpAddress is not a valid unicast IPv6 address.
                                 - *TargetIpAddress is one of the local configured IPv6
                                   addresses.
  @retval  EFI_OUT_OF_RESOURCES  Could not add the entry to the neighbor cache.
  @retval  EFI_NOT_FOUND         This entry is not in the neighbor cache (when DeleteFlag  is
                                 TRUE or when DeleteFlag  is FALSE while
                                 TargetLinkAddress is NULL.).
  @retval  EFI_ACCESS_DENIED     The to-be-added entry is already defined in the neighbor cache,
                                 and that entry is tagged as un-overridden (when Override
                                 is FALSE).

**/
EFI_STATUS
EFIAPI
EfiIp6Neighbors (
  IN EFI_IP6_PROTOCOL          *This,
  IN BOOLEAN                   DeleteFlag,
  IN EFI_IPv6_ADDRESS          *TargetIp6Address,
  IN EFI_MAC_ADDRESS           *TargetLinkAddress OPTIONAL,
  IN UINT32                    Timeout,
  IN BOOLEAN                   Override
  );

/**
  Places outgoing data packets into the transmit queue.

  The Transmit() function places a sending request in the transmit queue of this
  EFI IPv6 Protocol instance. Whenever the packet in the token is sent out or some
  errors occur, the event in the token will be signaled and the status is updated.

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  Token              The pointer to the transmit token.

  @retval  EFI_SUCCESS           The data has been queued for transmission.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_NO_MAPPING        The IPv6 driver was responsible for choosing
                                 a source address for this transmission,
                                 but no source address was available for use.
  @retval  EFI_INVALID_PARAMETER One or more of the following is TRUE:
                                 - This is NULL.
                                 - Token is NULL.
                                 - Token.Event is NULL.
                                 - Token.Packet.TxData is NULL.
                                 - Token.Packet.ExtHdrsLength is not zero and
                                   Token.Packet.ExtHdrs is NULL.
                                 - Token.Packet.FragmentCount is zero.
                                 - One or more of the Token.Packet.TxData.
                                   FragmentTable[].FragmentLength fields is zero.
                                 - One or more of the Token.Packet.TxData.
                                   FragmentTable[].FragmentBuffer fields is NULL.
                                 - Token.Packet.TxData.DataLength is zero or not
                                   equal to the sum of fragment lengths.
                                 - Token.Packet.TxData.DestinationAddress is non-
                                   zero when DestinationAddress is configured as
                                   non-zero when doing Configure() for this
                                   EFI IPv6 protocol instance.
                                 - Token.Packet.TxData.DestinationAddress is
                                   unspecified when DestinationAddress is unspecified
                                   when doing Configure() for this EFI IPv6 protocol
                                   instance.
  @retval  EFI_ACCESS_DENIED     The transmit completion token with the same Token.
                                 The event was already in the transmit queue.
  @retval  EFI_NOT_READY         The completion token could not be queued because
                                 the transmit queue is full.
  @retval  EFI_NOT_FOUND         Not route is found to the destination address.
  @retval  EFI_OUT_OF_RESOURCES  Could not queue the transmit data.
  @retval  EFI_BUFFER_TOO_SMALL  Token.Packet.TxData.TotalDataLength is too
                                 short to transmit.
  @retval  EFI_BAD_BUFFER_SIZE   If Token.Packet.TxData.DataLength is beyond the
                                 maximum that which can be described through the
                                 Fragment Offset field in Fragment header when
                                 performing fragmentation.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp6Transmit (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_COMPLETION_TOKEN  *Token
  );

/**
  Places a receiving request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue.
  This function is always asynchronous.

  The Token.Event field in the completion token must be filled in by the caller
  and cannot be NULL. When the receive operation completes, the EFI IPv6 Protocol
  driver updates the Token.Status and Token.Packet.RxData fields and the Token.Event
  is signaled.

  Current Udp implementation creates an IP child for each Udp child.
  It initates a asynchronous receive immediately whether or not
  there is no mapping. Therefore, disable the returning EFI_NO_MAPPING for now.
  To enable it, the following check must be performed:

  if (NetIp6IsUnspecifiedAddr (&Config->StationAddress) && IP6_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  Token              The pointer to a token that is associated with the
                                 receive data descriptor.

  @retval EFI_SUCCESS            The receive completion token was cached.
  @retval EFI_NOT_STARTED        This EFI IPv6 Protocol instance has not been started.
  @retval EFI_NO_MAPPING         When IP6 driver responsible for binding source address to this instance,
                                 while no source address is available for use.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - Token is NULL.
                                 - Token.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES   The receive completion token could not be queued due to a lack of system
                                 resources (usually memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The EFI IPv6 Protocol instance has been reset to startup defaults.
  @retval EFI_ACCESS_DENIED      The receive completion token with the same Token.Event was already
                                 in the receive queue.
  @retval EFI_NOT_READY          The receive request could not be queued because the receive queue is full.

**/
EFI_STATUS
EFIAPI
EfiIp6Receive (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_COMPLETION_TOKEN  *Token
  );

/**
  Abort an asynchronous transmit or receive request.

  The Cancel() function is used to abort a pending transmit or receive request.
  If the token is in the transmit or receive request queues, after calling this
  function, Token->Status will be set to EFI_ABORTED, and then Token->Event will
  be signaled. If the token is not in one of the queues, which usually means the
  asynchronous operation has completed, this function will not signal the token,
  and EFI_NOT_FOUND is returned.

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  Token              The pointer to a token that has been issued by
                                 EFI_IP6_PROTOCOL.Transmit() or
                                 EFI_IP6_PROTOCOL.Receive(). If NULL, all pending
                                 tokens are aborted. Type EFI_IP6_COMPLETION_TOKEN is
                                 defined in EFI_IP6_PROTOCOL.Transmit().

  @retval EFI_SUCCESS            The asynchronous I/O request was aborted and
                                 Token->Event was signaled. When Token is NULL, all
                                 pending requests were aborted, and their events were signaled.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        This instance has not been started.
  @retval EFI_NOT_FOUND          When Token is not NULL, the asynchronous I/O request was
                                 not found in the transmit or receive queue. It has either completed
                                 or was not issued by Transmit() and Receive().
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp6Cancel (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_COMPLETION_TOKEN  *Token    OPTIONAL
  );

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function polls for incoming data packets and processes outgoing data
  packets. Network drivers and applications can call the EFI_IP6_PROTOCOL.Poll()
  function to increase the rate that data packets are moved between the communications
  device and the transmit and receive queues.

  In some systems the periodic timer event may not poll the underlying communications
  device fast enough to transmit and/or receive all data packets without missing
  incoming packets or dropping outgoing packets. Drivers and applications that are
  experiencing packet loss should try calling the EFI_IP6_PROTOCOL.Poll() function
  more often.

  @param[in]  This               The pointer to the EFI_IP6_PROTOCOL instance.

  @retval  EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval  EFI_NOT_STARTED       This EFI IPv6 Protocol instance has not been started.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval  EFI_NOT_READY         No incoming or outgoing data was processed.
  @retval  EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.
                                 Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
EfiIp6Poll (
  IN EFI_IP6_PROTOCOL          *This
  );

#endif
