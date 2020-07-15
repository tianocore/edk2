/** @file
  Ip4 internal functions and type definitions.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_IP4_IMPL_H__
#define __EFI_IP4_IMPL_H__

#include <Uefi.h>

#include <Protocol/IpSec.h>
#include <Protocol/Ip4.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/Arp.h>
#include <Protocol/ManagedNetwork.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>

#include <IndustryStandard/Dhcp.h>

#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DpcLib.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>

#include "Ip4Common.h"
#include "Ip4Driver.h"
#include "Ip4If.h"
#include "Ip4Icmp.h"
#include "Ip4Option.h"
#include "Ip4Igmp.h"
#include "Ip4Route.h"
#include "Ip4Input.h"
#include "Ip4Output.h"
#include "Ip4Config2Impl.h"
#include "Ip4Config2Nv.h"
#include "Ip4NvData.h"

#define IP4_PROTOCOL_SIGNATURE  SIGNATURE_32 ('I', 'P', '4', 'P')
#define IP4_SERVICE_SIGNATURE   SIGNATURE_32 ('I', 'P', '4', 'S')

//
// The state of IP4 protocol. It starts from UNCONFIGED. if it is
// successfully configured, it goes to CONFIGED. if configure NULL
// is called, it becomes UNCONFIGED again.
//
#define IP4_STATE_UNCONFIGED    0
#define IP4_STATE_CONFIGED      1

//
// The state of IP4 service. It starts from UNSTARTED. It transits
// to STARTED if autoconfigure is started. If default address is
// configured, it becomes CONFIGED. and if partly destroyed, it goes
// to DESTROY.
//
#define IP4_SERVICE_UNSTARTED   0
#define IP4_SERVICE_STARTED     1
#define IP4_SERVICE_CONFIGED    2
#define IP4_SERVICE_DESTROY     3


///
/// IP4_TXTOKEN_WRAP wraps the upper layer's transmit token.
/// The user's data is kept in the Packet. When fragment is
/// needed, each fragment of the Packet has a reference to the
/// Packet, no data is actually copied. The Packet will be
/// released when all the fragments of it have been recycled by
/// MNP. Upon then, the IP4_TXTOKEN_WRAP will be released, and
/// user's event signalled.
///
typedef struct {
  IP4_PROTOCOL              *IpInstance;
  EFI_IP4_COMPLETION_TOKEN  *Token;
  EFI_EVENT                 IpSecRecycleSignal;
  NET_BUF                   *Packet;
  BOOLEAN                   Sent;
  INTN                      Life;
} IP4_TXTOKEN_WRAP;

///
/// IP4_IPSEC_WRAP wraps the packet received from MNP layer. The packet
/// will be released after it has been processed by the receiver. Upon then,
/// the IP4_IPSEC_WRAP will be released, and the IpSecRecycleSignal will be signaled
/// to notice IPsec to free the resources.
///
typedef struct {
  EFI_EVENT                 IpSecRecycleSignal;
  NET_BUF                   *Packet;
} IP4_IPSEC_WRAP;

///
/// IP4_RXDATA_WRAP wraps the data IP4 child delivers to the
/// upper layers. The received packet is kept in the Packet.
/// The Packet itself may be constructured from some fragments.
/// All the fragments of the Packet is organized by a
/// IP4_ASSEMBLE_ENTRY structure. If the Packet is recycled by
/// the upper layer, the assemble entry and its associated
/// fragments will be freed at last.
///
typedef struct {
  LIST_ENTRY                Link;
  IP4_PROTOCOL              *IpInstance;
  NET_BUF                   *Packet;
  EFI_IP4_RECEIVE_DATA      RxData;
} IP4_RXDATA_WRAP;


struct _IP4_PROTOCOL {
  UINT32                    Signature;

  EFI_IP4_PROTOCOL          Ip4Proto;
  EFI_HANDLE                Handle;
  INTN                      State;

  BOOLEAN                   InDestroy;

  IP4_SERVICE               *Service;
  LIST_ENTRY                Link;       // Link to all the IP protocol from the service

  //
  // User's transmit/receive tokens, and received/delivered packets
  //
  NET_MAP                   RxTokens;
  NET_MAP                   TxTokens;   // map between (User's Token, IP4_TXTOKE_WRAP)
  LIST_ENTRY                Received;   // Received but not delivered packet
  LIST_ENTRY                Delivered;  // Delivered and to be recycled packets
  EFI_LOCK                  RecycleLock;

  //
  // Instance's address and route tables. There are two route tables.
  // RouteTable is used by the IP4 driver to route packet. EfiRouteTable
  // is used to communicate the current route info to the upper layer.
  //
  IP4_INTERFACE             *Interface;
  LIST_ENTRY                AddrLink;   // Ip instances with the same IP address.
  IP4_ROUTE_TABLE           *RouteTable;

  EFI_IP4_ROUTE_TABLE       *EfiRouteTable;
  UINT32                    EfiRouteCount;

  //
  // IGMP data for this instance
  //
  IP4_ADDR                  *Groups;    // stored in network byte order
  UINT32                    GroupCount;

  EFI_IP4_CONFIG_DATA       ConfigData;

};

struct _IP4_SERVICE {
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

  IP4_INTERFACE                   *DefaultInterface;
  IP4_ROUTE_TABLE                 *DefaultRouteTable;

  //
  // Ip reassemble utilities, and IGMP data
  //
  IP4_ASSEMBLE_TABLE              Assemble;
  IGMP_SERVICE_DATA               IgmpCtrl;

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
  EFI_EVENT                       ReconfigCheckTimer;
  EFI_EVENT                       ReconfigEvent;

  BOOLEAN                         Reconfig;

  //
  // Underlying media present status.
  //
  BOOLEAN                         MediaPresent;

  //
  // IPv4 Configuration II Protocol instance
  //
  IP4_CONFIG2_INSTANCE            Ip4Config2Instance;

  CHAR16                          *MacString;

  UINT32                          MaxPacketSize;
  UINT32                          OldMaxPacketSize; ///< The MTU before IPsec enable.
};

#define IP4_INSTANCE_FROM_PROTOCOL(Ip4) \
          CR ((Ip4), IP4_PROTOCOL, Ip4Proto, IP4_PROTOCOL_SIGNATURE)

#define IP4_SERVICE_FROM_PROTOCOL(Sb)   \
          CR ((Sb), IP4_SERVICE, ServiceBinding, IP4_SERVICE_SIGNATURE)

#define IP4_SERVICE_FROM_CONFIG2_INSTANCE(This) \
  CR (This, IP4_SERVICE, Ip4Config2Instance, IP4_SERVICE_SIGNATURE)


#define IP4_NO_MAPPING(IpInstance) (!(IpInstance)->Interface->Configured)

extern EFI_IP4_PROTOCOL mEfiIp4ProtocolTemplete;

/**
  Config the MNP parameter used by IP. The IP driver use one MNP
  child to transmit/receive frames. By default, it configures MNP
  to receive unicast/multicast/broadcast. And it will enable/disable
  the promiscous receive according to whether there is IP child
  enable that or not. If Force is FALSE, it will iterate through
  all the IP children to check whether the promiscuous receive
  setting has been changed. If it hasn't been changed, it won't
  reconfigure the MNP. If Force is TRUE, the MNP is configured no
  matter whether that is changed or not.

  @param[in]  IpSb               The IP4 service instance that is to be changed.
  @param[in]  Force              Force the configuration or not.

  @retval EFI_SUCCESS            The MNP is successfully configured/reconfigured.
  @retval Others                 Configuration failed.

**/
EFI_STATUS
Ip4ServiceConfigMnp (
  IN IP4_SERVICE            *IpSb,
  IN BOOLEAN                Force
  );

/**
  Initialize the IP4_PROTOCOL structure to the unconfigured states.

  @param  IpSb                   The IP4 service instance.
  @param  IpInstance             The IP4 child instance.

**/
VOID
Ip4InitProtocol (
  IN     IP4_SERVICE            *IpSb,
  IN OUT IP4_PROTOCOL           *IpInstance
  );

/**
  Clean up the IP4 child, release all the resources used by it.

  @param[in]  IpInstance         The IP4 child to clean up.

  @retval EFI_SUCCESS            The IP4 child is cleaned up.
  @retval EFI_DEVICE_ERROR       Some resources failed to be released.

**/
EFI_STATUS
Ip4CleanProtocol (
  IN  IP4_PROTOCOL          *IpInstance
  );

/**
  Cancel the user's receive/transmit request.

  @param[in]  IpInstance         The IP4 child.
  @param[in]  Token              The token to cancel. If NULL, all token will be
                                 cancelled.

  @retval EFI_SUCCESS            The token is cancelled.
  @retval EFI_NOT_FOUND          The token isn't found on either the
                                 transmit/receive queue.
  @retval EFI_DEVICE_ERROR       Not all token is cancelled when Token is NULL.

**/
EFI_STATUS
Ip4Cancel (
  IN IP4_PROTOCOL             *IpInstance,
  IN EFI_IP4_COMPLETION_TOKEN *Token          OPTIONAL
  );

/**
  Change the IP4 child's multicast setting. The caller
  should make sure that the parameters is valid.

  @param[in]  IpInstance             The IP4 child to change the setting.
  @param[in]  JoinFlag               TRUE to join the group, otherwise leave it
  @param[in]  GroupAddress           The target group address

  @retval EFI_ALREADY_STARTED    Want to join the group, but already a member of it
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resources.
  @retval EFI_DEVICE_ERROR       Failed to set the group configuration
  @retval EFI_SUCCESS            Successfully updated the group setting.
  @retval EFI_NOT_FOUND          Try to leave the group which it isn't a member.

**/
EFI_STATUS
Ip4Groups (
  IN IP4_PROTOCOL           *IpInstance,
  IN BOOLEAN                JoinFlag,
  IN EFI_IPv4_ADDRESS       *GroupAddress       OPTIONAL
  );

/**
  This heart beat timer of IP4 service instance times out all of its IP4 children's
  received-but-not-delivered and transmitted-but-not-recycle packets, and provides
  time input for its IGMP protocol.

  @param[in]  Event                  The IP4 service instance's heart beat timer.
  @param[in]  Context                The IP4 service instance.

**/
VOID
EFIAPI
Ip4TimerTicking (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

/**
  This dedicated timer is used to poll underlying network media status. In case
  of cable swap or wireless network switch, a new round auto configuration will
  be initiated. The timer will signal the IP4 to run DHCP configuration again.
  IP4 driver will free old IP address related resource, such as route table and
  Interface, then initiate a DHCP process to acquire new IP, eventually create
  route table for new IP address.

  @param[in]  Event                  The IP4 service instance's heart beat timer.
  @param[in]  Context                The IP4 service instance.

**/
VOID
EFIAPI
Ip4TimerReconfigChecking (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

/**
  Decrease the life of the transmitted packets. If it is
  decreased to zero, cancel the packet. This function is
  called by Ip4PacketTimerTicking which time out both the
  received-but-not-delivered and transmitted-but-not-recycle
  packets.

  @param[in]  Map                    The IP4 child's transmit map.
  @param[in]  Item                   Current transmitted packet.
  @param[in]  Context                Not used.

  @retval EFI_SUCCESS            Always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
Ip4SentPacketTicking (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  );

/**
  The callback function for the net buffer which wraps the user's
  transmit token. Although it seems this function is pretty simple,
  there are some subtle things.
  When user requests the IP to transmit a packet by passing it a
  token, the token is wrapped in an IP4_TXTOKEN_WRAP and the data
  is wrapped in an net buffer. the net buffer's Free function is
  set to Ip4FreeTxToken. The Token and token wrap are added to the
  IP child's TxToken map. Then the buffer is passed to Ip4Output for
  transmission. If something error happened before that, the buffer
  is freed, which in turn will free the token wrap. The wrap may
  have been added to the TxToken map or not, and the user's event
  shouldn't be fired because we are still in the EfiIp4Transmit. If
  the buffer has been sent by Ip4Output, it should be removed from
  the TxToken map and user's event signaled. The token wrap and buffer
  are bound together. Check the comments in Ip4Output for information
  about IP fragmentation.

  @param[in]  Context                The token's wrap.

**/
VOID
EFIAPI
Ip4FreeTxToken (
  IN VOID                   *Context
  );

extern EFI_IPSEC2_PROTOCOL   *mIpSec;
extern BOOLEAN               mIpSec2Installed;

#endif
