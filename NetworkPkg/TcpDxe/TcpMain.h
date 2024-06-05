/** @file
  Declaration of protocol interfaces in EFI_TCP4_PROTOCOL and EFI_TCP6_PROTOCOL.
  It is the common head file for all Tcp*.c in TCP driver.

  Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCP_MAIN_H_
#define _TCP_MAIN_H_

#include <Protocol/ServiceBinding.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/Hash2.h>
#include <Library/IpIoLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>

#include "Socket.h"
#include "TcpProto.h"
#include "TcpDriver.h"
#include "TcpFunc.h"

extern UINT16                        mTcp4RandomPort;
extern UINT16                        mTcp6RandomPort;
extern CHAR16                        *mTcpStateName[];
extern EFI_COMPONENT_NAME_PROTOCOL   gTcpComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gTcpComponentName2;
extern EFI_UNICODE_STRING_TABLE      *gTcpControllerNameTable;

extern LIST_ENTRY  mTcpRunQue;
extern LIST_ENTRY  mTcpListenQue;
extern TCP_SEQNO   mTcpGlobalSecret;
extern UINT32      mTcpTick;

///
/// 30 seconds.
///
#define TCP6_KEEP_NEIGHBOR_TIME  30
///
/// 5 seconds, since 1 tick equals 200ms.
///
#define TCP6_REFRESH_NEIGHBOR_TICK  25

#define TCP_EXPIRE_TIME  65535

typedef union {
  EFI_TCP4_CONFIG_DATA    Tcp4CfgData;
  EFI_TCP6_CONFIG_DATA    Tcp6CfgData;
} TCP_CONFIG_DATA;

typedef union {
  EFI_TCP4_ACCESS_POINT    Tcp4Ap;
  EFI_TCP6_ACCESS_POINT    Tcp6Ap;
} TCP_ACCESS_POINT;

typedef struct _TCP4_MODE_DATA {
  EFI_TCP4_CONNECTION_STATE          *Tcp4State;
  EFI_TCP4_CONFIG_DATA               *Tcp4ConfigData;
  EFI_IP4_MODE_DATA                  *Ip4ModeData;
  EFI_MANAGED_NETWORK_CONFIG_DATA    *MnpConfigData;
  EFI_SIMPLE_NETWORK_MODE            *SnpModeData;
} TCP4_MODE_DATA;

typedef struct _TCP6_MODE_DATA {
  EFI_TCP6_CONNECTION_STATE          *Tcp6State;
  EFI_TCP6_CONFIG_DATA               *Tcp6ConfigData;
  EFI_IP6_MODE_DATA                  *Ip6ModeData;
  EFI_MANAGED_NETWORK_CONFIG_DATA    *MnpConfigData;
  EFI_SIMPLE_NETWORK_MODE            *SnpModeData;
} TCP6_MODE_DATA;

typedef struct _TCP4_ROUTE_INFO {
  BOOLEAN             DeleteRoute;
  EFI_IPv4_ADDRESS    *SubnetAddress;
  EFI_IPv4_ADDRESS    *SubnetMask;
  EFI_IPv4_ADDRESS    *GatewayAddress;
} TCP4_ROUTE_INFO;

typedef struct {
  EFI_SERVICE_BINDING_PROTOCOL    *ServiceBinding;
  UINTN                           NumberOfChildren;
  EFI_HANDLE                      *ChildHandleBuffer;
} TCP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT;

//
// EFI_TCP4_PROTOCOL definitions.
//

/**
  Get the current operational status.

  @param[in]   This                Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[out]  Tcp4State           Pointer to the buffer to receive the current TCP
                                   state. Optional parameter that may be NULL.
  @param[out]  Tcp4ConfigData      Pointer to the buffer to receive the current TCP
                                   configuration. Optional parameter that may be NULL.
  @param[out]  Ip4ModeData         Pointer to the buffer to receive the current
                                   IPv4 configuration. Optional parameter that may be NULL.
  @param[out]  MnpConfigData       Pointer to the buffer to receive the current MNP
                                   configuration data indirectly used by the TCPv4
                                   Instance. Optional parameter that may be NULL.
  @param[out]  SnpModeData         Pointer to the buffer to receive the current SNP
                                   configuration data indirectly used by the TCPv4
                                   Instance. Optional parameter that may be NULL.

  @retval EFI_SUCCESS              The mode data was read.
  @retval EFI_NOT_STARTED          No configuration data is available because this
                                   instance hasn't been started.
  @retval EFI_INVALID_PARAMETER    This is NULL.

**/
EFI_STATUS
EFIAPI
Tcp4GetModeData (
  IN   EFI_TCP4_PROTOCOL                *This,
  OUT  EFI_TCP4_CONNECTION_STATE        *Tcp4State      OPTIONAL,
  OUT  EFI_TCP4_CONFIG_DATA             *Tcp4ConfigData OPTIONAL,
  OUT  EFI_IP4_MODE_DATA                *Ip4ModeData    OPTIONAL,
  OUT  EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT  EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  );

/**
  Initialize or brutally reset the operational parameters for
  this EFI TCPv4 instance.

  @param[in]   This                Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]   TcpConfigData       Pointer to the configure data to configure the
                                   instance. Optional parameter that may be NULL.

  @retval EFI_SUCCESS              The operational settings are set, changed, or
                                   reset successfully.
  @retval EFI_NO_MAPPING           When using a default address, configuration
                                   (through DHCP, BOOTP, RARP, etc.) is not
                                   finished.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_ACCESS_DENIED        Configuring the TCP instance when it is already
                                   configured.
  @retval EFI_DEVICE_ERROR         An unexpected network or system error occurred.
  @retval EFI_UNSUPPORTED          One or more of the control options are not
                                   supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate enough system resources.

**/
EFI_STATUS
EFIAPI
Tcp4Configure (
  IN EFI_TCP4_PROTOCOL     *This,
  IN EFI_TCP4_CONFIG_DATA  *TcpConfigData OPTIONAL
  );

/**
  Add or delete routing entries.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  DeleteRoute          If TRUE, delete the specified route from routing
                                   table; if FALSE, add the specified route to
                                   routing table.
  @param[in]  SubnetAddress        The destination network.
  @param[in]  SubnetMask           The subnet mask for the destination network.
  @param[in]  GatewayAddress       The gateway address for this route.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance has not been
                                   configured.
  @retval EFI_NO_MAPPING           When using a default address, configuration
                                   (through DHCP, BOOTP, RARP, etc.) is not
                                   finished.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate enough resources to add the
                                   entry to the routing table.
  @retval EFI_NOT_FOUND            This route is not in the routing table.
  @retval EFI_ACCESS_DENIED        This route is already in the routing table.
  @retval EFI_UNSUPPORTED          The TCP driver does not support this operation.

**/
EFI_STATUS
EFIAPI
Tcp4Routes (
  IN EFI_TCP4_PROTOCOL  *This,
  IN BOOLEAN            DeleteRoute,
  IN EFI_IPv4_ADDRESS   *SubnetAddress,
  IN EFI_IPv4_ADDRESS   *SubnetMask,
  IN EFI_IPv4_ADDRESS   *GatewayAddress
  );

/**
  Initiate a nonblocking TCP connection request for an active TCP instance.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  ConnectionToken      Pointer to the connection token to return when
                                   the TCP three way handshake finishes.

  @retval EFI_SUCCESS              The connection request is successfully
                                   initiated.
  @retval EFI_NOT_STARTED          This EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        The instance is not configured as an active one
                                   or it is not in Tcp4StateClosed state.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     The driver can't allocate enough resources to
                                   initiate the active open.
  @retval EFI_DEVICE_ERROR         An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
Tcp4Connect (
  IN EFI_TCP4_PROTOCOL          *This,
  IN EFI_TCP4_CONNECTION_TOKEN  *ConnectionToken
  );

/**
  Listen on the passive instance to accept an incoming connection request.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  ListenToken          Pointer to the listen token to return when
                                   operation finishes.

  @retval EFI_SUCCESS              The listen token has been queued successfully.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        The instance is not a passive one or it is not
                                   in Tcp4StateListen state, or a same listen token
                                   has already existed in the listen token queue of
                                   this TCP instance.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate enough resources to finish
                                   the operation.
  @retval EFI_DEVICE_ERROR         An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
Tcp4Accept (
  IN EFI_TCP4_PROTOCOL      *This,
  IN EFI_TCP4_LISTEN_TOKEN  *ListenToken
  );

/**
  Queues outgoing data into the transmit queue

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance
  @param[in]  Token                Pointer to the completion token to queue to the
                                   transmit queue

  @retval EFI_SUCCESS              The data has been queued for transmission
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_NO_MAPPING           When using a default address, configuration
                                   (DHCP, BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid
  @retval EFI_ACCESS_DENIED        One or more of the following conditions is TRUE:
                                   * A transmit completion token with the same
                                   Token-> CompletionToken.Event was already in the
                                   transmission queue. * The current instance is in
                                   Tcp4StateClosed state * The current instance is
                                   a passive one and it is in Tcp4StateListen
                                   state. * User has called Close() to disconnect
                                   this connection.
  @retval EFI_NOT_READY            The completion token could not be queued because
                                   the transmit queue is full.
  @retval EFI_OUT_OF_RESOURCES     Could not queue the transmit data because of a
                                   resource shortage.
  @retval EFI_NETWORK_UNREACHABLE  There is no route to the destination network or
                                   address.

**/
EFI_STATUS
EFIAPI
Tcp4Transmit (
  IN EFI_TCP4_PROTOCOL  *This,
  IN EFI_TCP4_IO_TOKEN  *Token
  );

/**
  Place an asynchronous receive request into the receiving queue.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  Token                Pointer to a token that is associated with the
                                   receive data descriptor.

  @retval EFI_SUCCESS              The receive completion token was cached.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_NO_MAPPING           When using a default address, configuration
                                   (DHCP, BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     The receive completion token could not be queued
                                   due to a lack of system resources.
  @retval EFI_DEVICE_ERROR         An unexpected system or network error occurred.
  @retval EFI_ACCESS_DENIED        One or more of the following conditions is TRUE:
                                   * A receive completion token with the same
                                   Token->CompletionToken.Event was already in the
                                   receive queue. * The current instance is in
                                   Tcp4StateClosed state. * The current instance is
                                   a passive one and it is in Tcp4StateListen
                                   state. * User has called Close() to disconnect
                                   this connection.
  @retval EFI_CONNECTION_FIN       The communication peer has closed the connection
                                   and there is no buffered data in the receive
                                   buffer of this instance.
  @retval EFI_NOT_READY            The receive request could not be queued because
                                   the receive queue is full.

**/
EFI_STATUS
EFIAPI
Tcp4Receive (
  IN EFI_TCP4_PROTOCOL  *This,
  IN EFI_TCP4_IO_TOKEN  *Token
  );

/**
  Disconnecting a TCP connection gracefully or reset a TCP connection.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  CloseToken           Pointer to the close token to return when
                                   operation finishes.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        One or more of the following are TRUE: *
                                   Configure() has been called with TcpConfigData
                                   set to NULL and this function has not returned.
                                   * Previous Close() call on this instance has not
                                   finished.
  @retval EFI_INVALID_PARAMETER    One ore more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate enough resources to finish the
                                   operation.
  @retval EFI_DEVICE_ERROR         Any unexpected error not belonging to the error
                                   categories given above.

**/
EFI_STATUS
EFIAPI
Tcp4Close (
  IN EFI_TCP4_PROTOCOL     *This,
  IN EFI_TCP4_CLOSE_TOKEN  *CloseToken
  );

/**
  Abort an asynchronous connection, listen, transmission or receive request.

  @param  This  The pointer to the EFI_TCP4_PROTOCOL instance.
  @param  Token The pointer to a token that has been issued by
                EFI_TCP4_PROTOCOL.Connect(),
                EFI_TCP4_PROTOCOL.Accept(),
                EFI_TCP4_PROTOCOL.Transmit() or
                EFI_TCP4_PROTOCOL.Receive(). If NULL, all pending
                tokens issued by above four functions will be aborted. Type
                EFI_TCP4_COMPLETION_TOKEN is defined in
                EFI_TCP4_PROTOCOL.Connect().

  @retval  EFI_SUCCESS             The asynchronous I/O request is aborted and Token->Event
                                   is signaled.
  @retval  EFI_INVALID_PARAMETER   This is NULL.
  @retval  EFI_NOT_STARTED         This instance hasn't been configured.
  @retval  EFI_NO_MAPPING          When using the default address, configuration
                                   (DHCP, BOOTP,RARP, etc.) hasn't finished yet.
  @retval  EFI_NOT_FOUND           The asynchronous I/O request isn't found in the
                                   transmission or receive queue. It has either
                                   completed or wasn't issued by Transmit() and Receive().

**/
EFI_STATUS
EFIAPI
Tcp4Cancel (
  IN EFI_TCP4_PROTOCOL          *This,
  IN EFI_TCP4_COMPLETION_TOKEN  *Token OPTIONAL
  );

/**
  Poll to receive incoming data and transmit outgoing segments.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.

  @retval EFI_SUCCESS              Incoming or outgoing data was processed.
  @retval EFI_INVALID_PARAMETER    This is NULL.
  @retval EFI_DEVICE_ERROR         An unexpected system or network error occurred.
  @retval EFI_NOT_READY            No incoming or outgoing data was processed.
  @retval EFI_TIMEOUT              Data was dropped out of the transmission or
                                   receive queue. Consider increasing the polling
                                   rate.

**/
EFI_STATUS
EFIAPI
Tcp4Poll (
  IN EFI_TCP4_PROTOCOL  *This
  );

//
// EFI_TCP6_PROTOCOL definitions.
//

/**
  Get the current operational status.

  The GetModeData() function copies the current operational settings of this EFI TCPv6
  Protocol instance into user-supplied buffers. This function can also be used to retrieve
  the operational setting of underlying drivers such as IPv6, MNP, or SNP.

  @param[in]  This              Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[out] Tcp6State         The buffer in which the current TCP state is
                                returned. Optional parameter that may be NULL.
  @param[out] Tcp6ConfigData    The buffer in which the current TCP configuration
                                is returned. Optional parameter that may be NULL.
  @param[out] Ip6ModeData       The buffer in which the current IPv6 configuration
                                data used by the TCP instance is returned.
                                Optional parameter that may be NULL.
  @param[out] MnpConfigData     The buffer in which the current MNP configuration
                                data used indirectly by the TCP instance is returned.
                                Optional parameter that may be NULL.
  @param[out] SnpModeData       The buffer in which the current SNP mode data
                                 used indirectly by the TCP instance is returned.
                                Optional parameter that may be NULL.

  @retval EFI_SUCCESS           The mode data was read.
  @retval EFI_NOT_STARTED       No configuration data is available because this instance hasn't
                                been started.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
EFI_STATUS
EFIAPI
Tcp6GetModeData (
  IN  EFI_TCP6_PROTOCOL                *This,
  OUT EFI_TCP6_CONNECTION_STATE        *Tcp6State      OPTIONAL,
  OUT EFI_TCP6_CONFIG_DATA             *Tcp6ConfigData OPTIONAL,
  OUT EFI_IP6_MODE_DATA                *Ip6ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  );

/**
  Initialize or brutally reset the operational parameters for this EFI TCPv6 instance.

  The Configure() function does the following:
  - Initialize this TCP instance, i.e., initialize the communication end settings and
    specify active open or passive open for an instance.
  - Reset this TCP instance brutally, i.e., cancel all pending asynchronous tokens, flush
    transmission and receiving buffer directly without informing the communication peer.

  No other TCPv6 Protocol operation except Poll() can be executed by this instance until
  it is configured properly. For an active TCP instance, after a proper configuration it
  may call Connect() to initiates the three-way handshake. For a passive TCP instance,
  its state will transit to Tcp6StateListen after configuration, and Accept() may be
  called to listen the incoming TCP connection requests. If Tcp6ConfigData is set to NULL,
  the instance is reset. Resetting process will be done brutally, the state machine will
  be set to Tcp6StateClosed directly, the receive queue and transmit queue will be flushed,
  and no traffic is allowed through this instance.

  @param[in] This               Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] Tcp6ConfigData     Pointer to the configure data to configure the instance.
                                If Tcp6ConfigData is set to NULL, the instance is reset.

  @retval EFI_SUCCESS           The operational settings were set, changed, or reset
                                successfully.
  @retval EFI_NO_MAPPING        The underlying IPv6 driver was responsible for choosing a source
                                address for this instance, but no source address was available for
                                use.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions are TRUE:
                                - This is NULL.
                                - Tcp6ConfigData->AccessPoint.StationAddress is neither zero nor
                                  one of the configured IP addresses in the underlying IPv6 driver.
                                - Tcp6ConfigData->AccessPoint.RemoteAddress isn't a valid unicast
                                  IPv6 address.
                                - Tcp6ConfigData->AccessPoint.RemoteAddress is zero or
                                  Tcp6ConfigData->AccessPoint.RemotePort is zero when
                                  Tcp6ConfigData->AccessPoint.ActiveFlag is TRUE.
                                - A same access point has been configured in other TCP
                                  instance properly.
  @retval EFI_ACCESS_DENIED     Configuring TCP instance when it is configured without
                                calling Configure() with NULL to reset it.
  @retval EFI_UNSUPPORTED       One or more of the control options are not supported in
                                the implementation.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate enough system resources when
                                executing Configure().
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred.

**/
EFI_STATUS
EFIAPI
Tcp6Configure (
  IN EFI_TCP6_PROTOCOL     *This,
  IN EFI_TCP6_CONFIG_DATA  *Tcp6ConfigData OPTIONAL
  );

/**
  Initiate a nonblocking TCP connection request for an active TCP instance.

  The Connect() function will initiate an active open to the remote peer configured
  in current TCP instance if it is configured active. If the connection succeeds or
  fails due to an error, the ConnectionToken->CompletionToken.Event will be signaled,
  and ConnectionToken->CompletionToken.Status will be updated accordingly. This
  function can only be called for the TCP instance in Tcp6StateClosed state. The
  instance will transfer into Tcp6StateSynSent if the function returns EFI_SUCCESS.
  If TCP three-way handshake succeeds, its state will become Tcp6StateEstablished;
  otherwise, the state will return to Tcp6StateClosed.

  @param[in] This                Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] ConnectionToken     Pointer to the connection token to return when the TCP
                                 three-way handshake finishes.

  @retval EFI_SUCCESS            The connection request successfully initiated and the state of
                                 this TCP instance has been changed to Tcp6StateSynSent.
  @retval EFI_NOT_STARTED        This EFI TCPv6 Protocol instance has not been configured.
  @retval EFI_ACCESS_DENIED      One or more of the following conditions are TRUE:
                                 - This instance is not configured as an active instance.
                                 - This instance is not in Tcp6StateClosed state.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                 - This is NULL.
                                 - ConnectionToken is NULL.
                                 - ConnectionToken->CompletionToken.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES   The driver can't allocate enough resources to initiate the active open.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
Tcp6Connect (
  IN EFI_TCP6_PROTOCOL          *This,
  IN EFI_TCP6_CONNECTION_TOKEN  *ConnectionToken
  );

/**
  Listen on the passive instance to accept an incoming connection request. This is a
  nonblocking operation.

  The Accept() function initiates an asynchronous accept request to wait for an incoming
  connection on the passive TCP instance. If a remote peer successfully establishes a
  connection with this instance, a new TCP instance will be created and its handle will
  be returned in ListenToken->NewChildHandle. The newly created instance is configured
  by inheriting the passive instance's configuration, and is ready for use upon return.
  The new instance is in the Tcp6StateEstablished state.

  The ListenToken->CompletionToken.Event will be signaled when a new connection is
  accepted, user aborts the listen or connection is reset.

  This function only can be called when the current TCP instance is in Tcp6StateListen state.

  @param[in] This                Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] ListenToken         Pointer to the listen token to return when the operation finishes.


  @retval EFI_SUCCESS            The listen token was been queued successfully.
  @retval EFI_NOT_STARTED        This EFI TCPv6 Protocol instance has not been configured.
  @retval EFI_ACCESS_DENIED      One or more of the following are TRUE:
                                 - This instance is not a passive instance.
                                 - This instance is not in Tcp6StateListen state.
                                 - The same listen token has already existed in the listen
                                   token queue of this TCP instance.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                 - This is NULL.
                                 - ListenToken is NULL.
                                 - ListenToken->CompletionToken.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate enough resources to finish the operation.
  @retval EFI_DEVICE_ERROR       Any unexpected error not belonging to the error
                                 categories given above.

**/
EFI_STATUS
EFIAPI
Tcp6Accept (
  IN EFI_TCP6_PROTOCOL      *This,
  IN EFI_TCP6_LISTEN_TOKEN  *ListenToken
  );

/**
  Queues outgoing data into the transmit queue.

  The Transmit() function queues a sending request to this TCP instance along with the
  user data. The status of the token is updated and the event in the token will be
  signaled once the data is sent out or some error occurs.

  @param[in] This                 Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] Token                Pointer to the completion token to queue to the transmit queue.

  @retval EFI_SUCCESS             The data has been queued for transmission.
  @retval EFI_NOT_STARTED         This EFI TCPv6 Protocol instance has not been configured.
  @retval EFI_NO_MAPPING          The underlying IPv6 driver was responsible for choosing a
                                  source address for this instance, but no source address was
                                  available for use.
  @retval EFI_INVALID_PARAMETER   One or more of the following are TRUE:
                                  - This is NULL.
                                  - Token is NULL.
                                  - Token->CompletionToken.Event is NULL.
                                  - Token->Packet.TxData is NULL.
                                  - Token->Packet.FragmentCount is zero.
                                  - Token->Packet.DataLength is not equal to the sum of fragment lengths.
  @retval EFI_ACCESS_DENIED       One or more of the following conditions are TRUE:
                                  - A transmit completion token with the same Token->
                                    CompletionToken.Event was already in the
                                    transmission queue.
                                  - The current instance is in Tcp6StateClosed state.
                                  - The current instance is a passive one and it is in
                                    Tcp6StateListen state.
                                  - User has called Close() to disconnect this connection.
  @retval EFI_NOT_READY           The completion token could not be queued because the
                                  transmit queue is full.
  @retval EFI_OUT_OF_RESOURCES    Could not queue the transmit data because of a resource
                                  shortage.
  @retval EFI_NETWORK_UNREACHABLE There is no route to the destination network or address.

**/
EFI_STATUS
EFIAPI
Tcp6Transmit (
  IN EFI_TCP6_PROTOCOL  *This,
  IN EFI_TCP6_IO_TOKEN  *Token
  );

/**
  Places an asynchronous receive request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue. This
  function is always asynchronous. The caller must allocate the Token->CompletionToken.Event
  and the FragmentBuffer used to receive data. The caller also must fill the DataLength, which
  represents the whole length of all FragmentBuffer. When the receive operation completes, the
  EFI TCPv6 Protocol driver updates the Token->CompletionToken.Status and Token->Packet.RxData
  fields, and the Token->CompletionToken.Event is signaled. If data is obtained, the data and its length
  will be copied into the FragmentTable. At the same time the full length of received data will
  be recorded in the DataLength fields. Providing a proper notification function and context
  for the event enables the user to receive the notification and receiving status. That
  notification function is guaranteed to not be re-entered.

  @param[in] This               Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] Token              Pointer to a token that is associated with the receive data
                                descriptor.

  @retval EFI_SUCCESS            The receive completion token was cached.
  @retval EFI_NOT_STARTED        This EFI TCPv6 Protocol instance has not been configured.
  @retval EFI_NO_MAPPING         The underlying IPv6 driver was responsible for choosing a source
                                 address for this instance, but no source address was available for use.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - Token is NULL.
                                 - Token->CompletionToken.Event is NULL.
                                 - Token->Packet.RxData is NULL.
                                 - Token->Packet.RxData->DataLength is 0.
                                 - The Token->Packet.RxData->DataLength is not the
                                   sum of all FragmentBuffer length in FragmentTable.
  @retval EFI_OUT_OF_RESOURCES   The receive completion token could not be queued due to a lack of
                                 system resources (usually memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The EFI TCPv6 Protocol instance has been reset to startup defaults.
  @retval EFI_ACCESS_DENIED      One or more of the following conditions is TRUE:
                                 - A receive completion token with the same Token->CompletionToken.Event
                                   was already in the receive queue.
                                 - The current instance is in Tcp6StateClosed state.
                                 - The current instance is a passive one and it is in
                                   Tcp6StateListen state.
                                 - The user has called Close() to disconnect this connection.
  @retval EFI_CONNECTION_FIN     The communication peer has closed the connection, and there is no
                                 buffered data in the receive buffer of this instance.
  @retval EFI_NOT_READY          The receive request could not be queued because the receive queue is full.

**/
EFI_STATUS
EFIAPI
Tcp6Receive (
  IN EFI_TCP6_PROTOCOL  *This,
  IN EFI_TCP6_IO_TOKEN  *Token
  );

/**
  Disconnecting a TCP connection gracefully or reset a TCP connection. This function is a
  nonblocking operation.

  Initiate an asynchronous close token to the TCP driver. After Close() is called, any buffered
  transmission data will be sent by the TCP driver, and the current instance will have a graceful close
  working flow described as RFC 793 if AbortOnClose is set to FALSE, otherwise, a rest packet
  will be sent by TCP driver to fast disconnect this connection. When the close operation completes
  successfully the TCP instance is in Tcp6StateClosed state, all pending asynchronous
  operations are signaled, and any buffers used for TCP network traffic are flushed.

  @param[in] This                Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] CloseToken          Pointer to the close token to return when operation finishes.

  @retval EFI_SUCCESS            The Close() was called successfully.
  @retval EFI_NOT_STARTED        This EFI TCPv6 Protocol instance has not been configured.
  @retval EFI_ACCESS_DENIED      One or more of the following are TRUE:
                                 - CloseToken or CloseToken->CompletionToken.Event is already in use.
                                 - Previous Close() call on this instance has not finished.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                 - This is NULL.
                                 - CloseToken is NULL.
                                 - CloseToken->CompletionToken.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate enough resources to finish the operation.
  @retval EFI_DEVICE_ERROR       Any unexpected error not belonging to the error categories given above.

**/
EFI_STATUS
EFIAPI
Tcp6Close (
  IN EFI_TCP6_PROTOCOL     *This,
  IN EFI_TCP6_CLOSE_TOKEN  *CloseToken
  );

/**
  Abort an asynchronous connection, listen, transmission or receive request.

  The Cancel() function aborts a pending connection, listen, transmit or
  receive request.

  If Token is not NULL and the token is in the connection, listen, transmission
  or receive queue when it is being cancelled, its Token->Status will be set
  to EFI_ABORTED and then Token->Event will be signaled.

  If the token is not in one of the queues, which usually means that the
  asynchronous operation has completed, EFI_NOT_FOUND is returned.

  If Token is NULL all asynchronous token issued by Connect(), Accept(),
  Transmit() and Receive() will be aborted.

  @param[in] This                Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] Token               Pointer to a token that has been issued by
                                 EFI_TCP6_PROTOCOL.Connect(),
                                 EFI_TCP6_PROTOCOL.Accept(),
                                 EFI_TCP6_PROTOCOL.Transmit() or
                                 EFI_TCP6_PROTOCOL.Receive(). If NULL, all pending
                                 tokens issued by above four functions will be aborted. Type
                                 EFI_TCP6_COMPLETION_TOKEN is defined in
                                 EFI_TCP_PROTOCOL.Connect().

  @retval EFI_SUCCESS            The asynchronous I/O request is aborted and Token->Event
                                 is signaled.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        This instance hasn't been configured.
  @retval EFI_NOT_FOUND          The asynchronous I/O request isn't found in the transmission or
                                 receive queue. It has either completed or wasn't issued by
                                 Transmit() and Receive().

**/
EFI_STATUS
EFIAPI
Tcp6Cancel (
  IN EFI_TCP6_PROTOCOL          *This,
  IN EFI_TCP6_COMPLETION_TOKEN  *Token OPTIONAL
  );

/**
  Poll to receive incoming data and transmit outgoing segments.

  The Poll() function increases the rate that data is moved between the network
  and application and can be called when the TCP instance is created successfully.
  Its use is optional.

  @param[in] This                Pointer to the EFI_TCP6_PROTOCOL instance.

  @retval EFI_SUCCESS            Incoming or outgoing data was processed.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  @retval EFI_NOT_READY          No incoming or outgoing data is processed.
  @retval EFI_TIMEOUT            Data was dropped out of the transmission or receive queue.
                                 Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
Tcp6Poll (
  IN EFI_TCP6_PROTOCOL  *This
  );

/**
  Retrieves the Initial Sequence Number (ISN) for a TCP connection identified by local
  and remote IP addresses and ports.

  This method is based on https://datatracker.ietf.org/doc/html/rfc9293#section-3.4.1
  Where the ISN is computed as follows:
    ISN = TimeStamp + MD5(LocalIP, LocalPort, RemoteIP, RemotePort, Secret)

  Otherwise:
    ISN = M + F(localip, localport, remoteip, remoteport, secretkey)

    "Here M is the 4 microsecond timer, and F() is a pseudorandom function (PRF) of the
    connection's identifying parameters ("localip, localport, remoteip, remoteport")
    and a secret key ("secretkey") (SHLD-1). F() MUST NOT be computable from the
    outside (MUST-9), or an attacker could still guess at sequence numbers from the
    ISN used for some other connection. The PRF could be implemented as a
    cryptographic hash of the concatenation of the TCP connection parameters and some
    secret data. For discussion of the selection of a specific hash algorithm and
    management of the secret key data."

  @param[in]       LocalIp        A pointer to the local IP address of the TCP connection.
  @param[in]       LocalIpSize    The size, in bytes, of the LocalIp buffer.
  @param[in]       LocalPort      The local port number of the TCP connection.
  @param[in]       RemoteIp       A pointer to the remote IP address of the TCP connection.
  @param[in]       RemoteIpSize   The size, in bytes, of the RemoteIp buffer.
  @param[in]       RemotePort     The remote port number of the TCP connection.
  @param[out]      Isn            A pointer to the variable that will receive the Initial
                                  Sequence Number (ISN).

  @retval EFI_SUCCESS             The operation completed successfully, and the ISN was
                                  retrieved.
  @retval EFI_INVALID_PARAMETER   One or more of the input parameters are invalid.
  @retval EFI_UNSUPPORTED         The operation is not supported.

**/
EFI_STATUS
TcpGetIsn (
  IN UINT8       *LocalIp,
  IN UINTN       LocalIpSize,
  IN UINT16      LocalPort,
  IN UINT8       *RemoteIp,
  IN UINTN       RemoteIpSize,
  IN UINT16      RemotePort,
  OUT TCP_SEQNO  *Isn
  );

#endif
