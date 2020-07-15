/** @file
  EFI UDPv4 protocol implementation.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UDP4_IMPL_H_
#define _UDP4_IMPL_H_

#include <Uefi.h>

#include <Protocol/Ip4.h>
#include <Protocol/Udp4.h>

#include <Library/IpIoLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/DpcLib.h>
#include <Library/PrintLib.h>

#include "Udp4Driver.h"


extern EFI_COMPONENT_NAME_PROTOCOL     gUdp4ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL    gUdp4ComponentName2;
extern EFI_UNICODE_STRING_TABLE        *gUdpControllerNameTable;
extern EFI_SERVICE_BINDING_PROTOCOL    mUdp4ServiceBinding;
extern EFI_UDP4_PROTOCOL               mUdp4Protocol;
extern UINT16                          mUdp4RandomPort;

#define ICMP_ERROR_PACKET_LENGTH  8

#define UDP4_TIMEOUT_INTERVAL (50 * TICKS_PER_MS)  // 50 milliseconds

#define UDP4_HEADER_SIZE      sizeof (EFI_UDP_HEADER)
#define UDP4_MAX_DATA_SIZE    65507

#define UDP4_PORT_KNOWN       1024

#define UDP4_SERVICE_DATA_SIGNATURE  SIGNATURE_32('U', 'd', 'p', '4')

#define UDP4_SERVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  UDP4_SERVICE_DATA, \
  ServiceBinding, \
  UDP4_SERVICE_DATA_SIGNATURE \
  )

typedef struct _UDP4_SERVICE_DATA_ {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;
  EFI_HANDLE                    ImageHandle;
  EFI_HANDLE                    ControllerHandle;
  LIST_ENTRY                    ChildrenList;
  UINTN                         ChildrenNumber;
  IP_IO                         *IpIo;

  EFI_EVENT                     TimeoutEvent;
} UDP4_SERVICE_DATA;

#define UDP4_INSTANCE_DATA_SIGNATURE  SIGNATURE_32('U', 'd', 'p', 'I')

#define UDP4_INSTANCE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  UDP4_INSTANCE_DATA, \
  Udp4Proto, \
  UDP4_INSTANCE_DATA_SIGNATURE \
  )

typedef struct _UDP4_INSTANCE_DATA_ {
  UINT32                Signature;
  LIST_ENTRY            Link;

  UDP4_SERVICE_DATA     *Udp4Service;
  EFI_UDP4_PROTOCOL     Udp4Proto;
  EFI_UDP4_CONFIG_DATA  ConfigData;
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
} UDP4_INSTANCE_DATA;

typedef struct _UDP4_RXDATA_WRAP_ {
  LIST_ENTRY             Link;
  NET_BUF                *Packet;
  UINT32                 TimeoutTick;
  EFI_UDP4_RECEIVE_DATA  RxData;
} UDP4_RXDATA_WRAP;

typedef struct {
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;
} UDP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT;

/**
  Reads the current operational settings.

  The GetModeData() function copies the current operational settings of this EFI
  UDPv4 Protocol instance into user-supplied buffers. This function is used
  optionally to retrieve the operational mode data of underlying networks or
  drivers.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[out] Udp4ConfigData    Pointer to the buffer to receive the current configuration data.
  @param[out] Ip4ModeData       Pointer to the EFI IPv4 Protocol mode data structure.
  @param[out] MnpConfigData     Pointer to the managed network configuration data structure.
  @param[out] SnpModeData       Pointer to the simple network mode data structure.

  @retval EFI_SUCCESS           The mode data was read.
  @retval EFI_NOT_STARTED       When Udp4ConfigData is queried, no configuration data is
                                available because this instance has not been started.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
EFI_STATUS
EFIAPI
Udp4GetModeData (
  IN  EFI_UDP4_PROTOCOL                *This,
  OUT EFI_UDP4_CONFIG_DATA             *Udp4ConfigData OPTIONAL,
  OUT EFI_IP4_MODE_DATA                *Ip4ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  );

/**
  Initializes, changes, or resets the operational parameters for this instance of the EFI UDPv4
  Protocol.

  The Configure() function is used to do the following:
  * Initialize and start this instance of the EFI UDPv4 Protocol.
  * Change the filtering rules and operational parameters.
  * Reset this instance of the EFI UDPv4 Protocol.
  Until these parameters are initialized, no network traffic can be sent or
  received by this instance. This instance can be also reset by calling Configure()
  with UdpConfigData set to NULL. Once reset, the receiving queue and transmitting
  queue are flushed and no traffic is allowed through this instance.
  With different parameters in UdpConfigData, Configure() can be used to bind
  this instance to specified port.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  UdpConfigData     Pointer to the buffer to receive the current configuration data.

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
EFI_STATUS
EFIAPI
Udp4Configure (
  IN EFI_UDP4_PROTOCOL     *This,
  IN EFI_UDP4_CONFIG_DATA  *UdpConfigData OPTIONAL
  );

/**
  Joins and leaves multicast groups.

  The Groups() function is used to enable and disable the multicast group
  filtering. If the JoinFlag is FALSE and the MulticastAddress is NULL, then all
  currently joined groups are left.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  JoinFlag          Set to TRUE to join a multicast group. Set to FALSE to leave one
                                or all multicast groups.
  @param[in]  MulticastAddress  Pointer to multicast group address to join or leave.

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
EFI_STATUS
EFIAPI
Udp4Groups (
  IN EFI_UDP4_PROTOCOL  *This,
  IN BOOLEAN            JoinFlag,
  IN EFI_IPv4_ADDRESS   *MulticastAddress OPTIONAL
  );

/**
  Adds and deletes routing table entries.

  The Routes() function adds a route to or deletes a route from the routing table.
  Routes are determined by comparing the SubnetAddress with the destination IP
  address and arithmetically AND-ing it with the SubnetMask. The gateway address
  must be on the same subnet as the configured station address.
  The default route is added with SubnetAddress and SubnetMask both set to 0.0.0.0.
  The default route matches all destination IP addresses that do not match any
  other routes.
  A zero GatewayAddress is a nonroute. Packets are sent to the destination IP
  address if it can be found in the Address Resolution Protocol (ARP) cache or
  on the local subnet. One automatic nonroute entry will be inserted into the
  routing table for outgoing packets that are addressed to a local subnet
  (gateway address of 0.0.0.0).
  Each instance of the EFI UDPv4 Protocol has its own independent routing table.
  Instances of the EFI UDPv4 Protocol that use the default IP address will also
  have copies of the routing table provided by the EFI_IP4_CONFIG_PROTOCOL. These
  copies will be updated automatically whenever the IP driver reconfigures its
  instances; as a result, the previous modification to these copies will be lost.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  DeleteRoute       Set to TRUE to delete this route from the routing table.
                                Set to FALSE to add this route to the routing table.
  @param[in]  SubnetAddress     The destination network address that needs to be routed.
  @param[in]  SubnetMask        The subnet mask of SubnetAddress.
  @param[in]  GatewayAddress    The gateway IP address for this route.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_NOT_STARTED       The EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                - RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND         This route is not in the routing table.
  @retval EFI_ACCESS_DENIED     The route is already defined in the routing table.

**/
EFI_STATUS
EFIAPI
Udp4Routes (
  IN EFI_UDP4_PROTOCOL  *This,
  IN BOOLEAN            DeleteRoute,
  IN EFI_IPv4_ADDRESS   *SubnetAddress,
  IN EFI_IPv4_ADDRESS   *SubnetMask,
  IN EFI_IPv4_ADDRESS   *GatewayAddress
  );

/**
  Queues outgoing data packets into the transmit queue.

  The Transmit() function places a sending request to this instance of the EFI
  UDPv4 Protocol, alongside the transmit data that was filled by the user. Whenever
  the packet in the token is sent out or some errors occur, the Token.Event will
  be signaled and Token.Status is updated. Providing a proper notification function
  and context for the event will enable the user to receive the notification and
  transmitting status.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  Token             Pointer to the completion token that will be placed into the
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
EFI_STATUS
EFIAPI
Udp4Transmit (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
  );

/**
  Places an asynchronous receive request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue.
  This function is always asynchronous.
  The caller must fill in the Token.Event field in the completion token, and this
  field cannot be NULL. When the receive operation completes, the EFI UDPv4 Protocol
  driver updates the Token.Status and Token.Packet.RxData fields and the Token.Event
  is signaled. Providing a proper notification function and context for the event
  will enable the user to receive the notification and receiving status. That
  notification function is guaranteed to not be re-entered.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  Token             Pointer to a token that is associated with
                                the receive data descriptor.

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
EFI_STATUS
EFIAPI
Udp4Receive (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
  );

/**
  Aborts an asynchronous transmit or receive request.

  The Cancel() function is used to abort a pending transmit or receive request.
  If the token is in the transmit or receive request queues, after calling this
  function, Token.Status will be set to EFI_ABORTED and then Token.Event will be
  signaled. If the token is not in one of the queues, which usually means that
  the asynchronous operation has completed, this function will not signal the
  token and EFI_NOT_FOUND is returned.

  @param[in]  This  Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  Token Pointer to a token that has been issued by
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
EFI_STATUS
EFIAPI
Udp4Cancel (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token OPTIONAL
  );

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to increase
  the rate that data packets are moved between the communications device and the
  transmit and receive queues.
  In some systems, the periodic timer event in the managed network driver may not
  poll the underlying communications device fast enough to transmit and/or receive
  all data packets without missing incoming packets or dropping outgoing packets.
  Drivers and applications that are experiencing packet loss should try calling
  the Poll() function more often.

  @param[in]  This  Pointer to the EFI_UDP4_PROTOCOL instance.

  @retval EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.

**/
EFI_STATUS
EFIAPI
Udp4Poll (
  IN EFI_UDP4_PROTOCOL  *This
  );

/**
  Create the Udp service context data.

  @param[in, out] Udp4Service      Pointer to the UDP4_SERVICE_DATA.
  @param[in]      ImageHandle      The image handle of this udp4 driver.
  @param[in]      ControllerHandle The controller handle this udp4 driver binds on.

  @retval EFI_SUCCESS              The udp4 service context data is created and
                                   initialized.
  @retval EFI_OUT_OF_RESOURCES     Cannot allocate memory.
  @retval other                    Other error occurs.

**/
EFI_STATUS
Udp4CreateService (
  IN OUT UDP4_SERVICE_DATA  *Udp4Service,
  IN     EFI_HANDLE         ImageHandle,
  IN     EFI_HANDLE         ControllerHandle
  );

/**
  Clean the Udp service context data.

  @param[in]  Udp4Service            Pointer to the UDP4_SERVICE_DATA.

**/
VOID
Udp4CleanService (
  IN UDP4_SERVICE_DATA  *Udp4Service
  );

/**
  This function initializes the new created udp instance.

  @param[in]      Udp4Service       Pointer to the UDP4_SERVICE_DATA.
  @param[in, out] Instance          Pointer to the un-initialized UDP4_INSTANCE_DATA.

**/
VOID
Udp4InitInstance (
  IN     UDP4_SERVICE_DATA   *Udp4Service,
  IN OUT UDP4_INSTANCE_DATA  *Instance
  );

/**
  This function cleans the udp instance.

  @param[in]  Instance               Pointer to the UDP4_INSTANCE_DATA to clean.

**/
VOID
Udp4CleanInstance (
  IN UDP4_INSTANCE_DATA  *Instance
  );

/**
  This function tries to bind the udp instance according to the configured port
  allocation strategy.

  @param[in]      InstanceList   Pointer to the head of the list linking the udp
                                 instances.
  @param[in, out] ConfigData     Pointer to the ConfigData of the instance to be
                                 bound. ConfigData->StationPort will be assigned
                                 with an available port value on success.

  @retval EFI_SUCCESS            The bound operation is completed successfully.
  @retval EFI_ACCESS_DENIED      The <Address, Port> specified by the ConfigData is
                                 already used by other instance.
  @retval EFI_OUT_OF_RESOURCES   No available port resources.

**/
EFI_STATUS
Udp4Bind (
  IN     LIST_ENTRY            *InstanceList,
  IN OUT EFI_UDP4_CONFIG_DATA  *ConfigData
  );

/**
  This function is used to check whether the NewConfigData has any un-reconfigurable
  parameters changed compared to the OldConfigData.

  @param[in]  OldConfigData      Pointer to the current ConfigData the udp instance
                                 uses.
  @param[in]  NewConfigData      Pointer to the new ConfigData.

  @retval TRUE     The instance is reconfigurable.
  @retval FALSE    Otherwise.

**/
BOOLEAN
Udp4IsReconfigurable (
  IN EFI_UDP4_CONFIG_DATA  *OldConfigData,
  IN EFI_UDP4_CONFIG_DATA  *NewConfigData
  );

/**
  This function builds the Ip4 configdata from the Udp4ConfigData.

  @param[in]       Udp4ConfigData    Pointer to the EFI_UDP4_CONFIG_DATA.
  @param[in, out]  Ip4ConfigData     Pointer to the EFI_IP4_CONFIG_DATA.

**/
VOID
Udp4BuildIp4ConfigData (
  IN     EFI_UDP4_CONFIG_DATA  *Udp4ConfigData,
  IN OUT EFI_IP4_CONFIG_DATA   *Ip4ConfigData
  );

/**
  This function validates the TxToken, it returns the error code according to the spec.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  TxToken            Pointer to the token to be checked.

  @retval EFI_SUCCESS            The TxToken is valid.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE: This is
                                 NULL. Token is NULL. Token.Event is NULL.
                                 Token.Packet.TxData is NULL.
                                 Token.Packet.TxData.FragmentCount is zero.
                                 Token.Packet.TxData.DataLength is not equal to the
                                 sum of fragment lengths. One or more of the
                                 Token.Packet.TxData.FragmentTable[].
                                 FragmentLength fields is zero. One or more of the
                                 Token.Packet.TxData.FragmentTable[].
                                 FragmentBuffer fields is NULL.
                                 Token.Packet.TxData. GatewayAddress is not a
                                 unicast IPv4 address if it is not NULL. One or
                                 more IPv4 addresses in Token.Packet.TxData.
                                 UdpSessionData are not valid unicast IPv4
                                 addresses if the UdpSessionData is not NULL.
  @retval EFI_BAD_BUFFER_SIZE    The data length is greater than the maximum UDP
                                 packet size.

**/
EFI_STATUS
Udp4ValidateTxToken (
  IN UDP4_INSTANCE_DATA         *Instance,
  IN EFI_UDP4_COMPLETION_TOKEN  *TxToken
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
Udp4TokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  );

/**
  This function calculates the checksum for the Packet, utilizing the pre-calculated
  pseudo HeadSum to reduce some overhead.

  @param[in]  Packet             Pointer to the NET_BUF contains the udp datagram.
  @param[in]  HeadSum            Checksum of the pseudo header except the length
                                 field.

  @retval The 16-bit checksum of this udp datagram.

**/
UINT16
Udp4Checksum (
  IN NET_BUF *Packet,
  IN UINT16  HeadSum
  );

/**
  This function removes the specified Token from the TokenMap.

  @param[in, out] TokenMap       Pointer to the NET_MAP containing the tokens.
  @param[in]      Token          Pointer to the Token to be removed.

  @retval EFI_SUCCESS            The specified Token is removed from the TokenMap.
  @retval EFI_NOT_FOUND          The specified Token is not found in the TokenMap.

**/
EFI_STATUS
Udp4RemoveToken (
  IN OUT NET_MAP                    *TokenMap,
  IN     EFI_UDP4_COMPLETION_TOKEN  *Token
  );

/**
  This function removes the multicast group specified by Arg from the Map.

  @param[in, out] Map            Pointer to the NET_MAP.
  @param[in]      Item           Pointer to the NET_MAP_ITEM.
  @param[in]      Arg            Pointer to the Arg, it's the pointer to a
                                 multicast IPv4 Address.

  @retval EFI_SUCCESS            The multicast address is removed.
  @retval EFI_ABORTED            The specified multicast address is removed and the
                                 Arg is not NULL.

**/
EFI_STATUS
EFIAPI
Udp4LeaveGroup (
  IN OUT NET_MAP       *Map,
  IN     NET_MAP_ITEM  *Item,
  IN     VOID          *Arg OPTIONAL
  );

/**
  This function removes all the Wrap datas in the RcvdDgramQue.

  @param[in]  Instance           Pointer to the udp instance context data.

**/
VOID
Udp4FlushRcvdDgram (
  IN UDP4_INSTANCE_DATA  *Instance
  );

/**
  Cancel Udp4 tokens from the Udp4 instance.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  Token              Pointer to the token to be canceled, if NULL, all
                                 tokens in this instance will be cancelled.

  @retval EFI_SUCCESS            The Token is cancelled.
  @retval EFI_NOT_FOUND          The Token is not found.

**/
EFI_STATUS
Udp4InstanceCancelToken (
  IN UDP4_INSTANCE_DATA         *Instance,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token OPTIONAL
  );

/**
  This function delivers the received datagrams for the specified instance.

  @param[in]  Instance               Pointer to the instance context data.

**/
VOID
Udp4InstanceDeliverDgram (
  IN UDP4_INSTANCE_DATA  *Instance
  );

/**
  This function reports the received ICMP error.

  @param[in]  Instance               Pointer to the udp instance context data.

**/
VOID
Udp4ReportIcmpError (
  IN UDP4_INSTANCE_DATA  *Instance
  );

/**
  This function is a dummy ext-free function for the NET_BUF created for the output
  udp datagram.

  @param[in]  Context                Pointer to the context data.

**/
VOID
EFIAPI
Udp4NetVectorExtFree (
  VOID  *Context
  );

#endif
