/** @file
  Implementation of EFI_TCP4_PROTOCOL and EFI_TCP6_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TcpMain.h"

/**
  Check the integrity of the data buffer.

  @param[in]  DataLen              The total length of the data buffer.
  @param[in]  FragmentCount        The fragment count of the fragment table.
  @param[in]  FragmentTable        Pointer to the fragment table of the data
                                   buffer.

  @retval EFI_SUCCESS              The integrity check passed.
  @retval EFI_INVALID_PARAMETER    The integrity check failed.

**/
EFI_STATUS
TcpChkDataBuf (
  IN UINT32                 DataLen,
  IN UINT32                 FragmentCount,
  IN EFI_TCP4_FRAGMENT_DATA *FragmentTable
  )
{
  UINT32 Index;

  UINT32 Len;

  for (Index = 0, Len = 0; Index < FragmentCount; Index++) {
    if (FragmentTable[Index].FragmentBuffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    Len = Len + FragmentTable[Index].FragmentLength;
  }

  if (DataLen != Len) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

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
  IN   EFI_TCP4_PROTOCOL                  *This,
  OUT  EFI_TCP4_CONNECTION_STATE          *Tcp4State      OPTIONAL,
  OUT  EFI_TCP4_CONFIG_DATA               *Tcp4ConfigData OPTIONAL,
  OUT  EFI_IP4_MODE_DATA                  *Ip4ModeData    OPTIONAL,
  OUT  EFI_MANAGED_NETWORK_CONFIG_DATA    *MnpConfigData  OPTIONAL,
  OUT  EFI_SIMPLE_NETWORK_MODE            *SnpModeData    OPTIONAL
  )
{
  TCP4_MODE_DATA  TcpMode;
  SOCKET          *Sock;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  Sock                    = SOCK_FROM_THIS (This);

  TcpMode.Tcp4State       = Tcp4State;
  TcpMode.Tcp4ConfigData  = Tcp4ConfigData;
  TcpMode.Ip4ModeData     = Ip4ModeData;
  TcpMode.MnpConfigData   = MnpConfigData;
  TcpMode.SnpModeData     = SnpModeData;

  return SockGetMode (Sock, &TcpMode);
}

/**
  Initialize or brutally reset the operational parameters for
  this EFI TCPv4 instance.

  @param[in]   This                Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]   TcpConfigData       Pointer to the configure data to configure the
                                   instance. Optional parameter that may be NULL.

  @retval EFI_SUCCESS              The operational settings were set, changed, or
                                   reset successfully.
  @retval EFI_NO_MAPPING           When using a default address, configuration
                                   (through DHCP, BOOTP, RARP, etc.) is not
                                   finished.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_ACCESS_DENIED        Configuring TCP instance when it is already
                                   configured.
  @retval EFI_DEVICE_ERROR         An unexpected network or system error occurred.
  @retval EFI_UNSUPPORTED          One or more of the control options are not
                                   supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate enough system resources.

**/
EFI_STATUS
EFIAPI
Tcp4Configure (
  IN EFI_TCP4_PROTOCOL        * This,
  IN EFI_TCP4_CONFIG_DATA     * TcpConfigData OPTIONAL
  )
{
  EFI_TCP4_OPTION  *Option;
  SOCKET           *Sock;
  EFI_STATUS       Status;
  IP4_ADDR         Ip;
  IP4_ADDR         SubnetMask;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Tcp protocol related parameter check will be conducted here
  //
  if (NULL != TcpConfigData) {

    CopyMem (&Ip, &TcpConfigData->AccessPoint.RemoteAddress, sizeof (IP4_ADDR));
    if (IP4_IS_LOCAL_BROADCAST (NTOHL (Ip))) {
      return EFI_INVALID_PARAMETER;
    }

    if (TcpConfigData->AccessPoint.ActiveFlag && (0 == TcpConfigData->AccessPoint.RemotePort || (Ip == 0))) {
      return EFI_INVALID_PARAMETER;
    }

    if (!TcpConfigData->AccessPoint.UseDefaultAddress) {

      CopyMem (&Ip, &TcpConfigData->AccessPoint.StationAddress, sizeof (IP4_ADDR));
      CopyMem (&SubnetMask, &TcpConfigData->AccessPoint.SubnetMask, sizeof (IP4_ADDR));
      if (!IP4_IS_VALID_NETMASK (NTOHL (SubnetMask)) ||
          (SubnetMask != 0 && !NetIp4IsUnicast (NTOHL (Ip), NTOHL (SubnetMask)))) {
        return EFI_INVALID_PARAMETER;
      }
    }

    Option = TcpConfigData->ControlOption;
    if ((NULL != Option) && (Option->EnableSelectiveAck || Option->EnablePathMtuDiscovery)) {
      return EFI_UNSUPPORTED;
    }
  }

  Sock = SOCK_FROM_THIS (This);

  if (NULL == TcpConfigData) {
    return SockFlush (Sock);
  }

  Status = SockConfigure (Sock, TcpConfigData);

  if (EFI_NO_MAPPING == Status) {
    Sock->ConfigureState = SO_NO_MAPPING;
  }

  return Status;
}

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
  IN EFI_TCP4_PROTOCOL           *This,
  IN BOOLEAN                     DeleteRoute,
  IN EFI_IPv4_ADDRESS            *SubnetAddress,
  IN EFI_IPv4_ADDRESS            *SubnetMask,
  IN EFI_IPv4_ADDRESS            *GatewayAddress
  )
{
  SOCKET          *Sock;
  TCP4_ROUTE_INFO RouteInfo;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  Sock                      = SOCK_FROM_THIS (This);

  RouteInfo.DeleteRoute     = DeleteRoute;
  RouteInfo.SubnetAddress   = SubnetAddress;
  RouteInfo.SubnetMask      = SubnetMask;
  RouteInfo.GatewayAddress  = GatewayAddress;

  return SockRoute (Sock, &RouteInfo);
}

/**
  Initiate a non-blocking TCP connection request for an active TCP instance.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  ConnectionToken      Pointer to the connection token to return when
                                   the TCP three way handshake finishes.

  @retval EFI_SUCCESS              The connection request successfully
                                   initiated.
  @retval EFI_NOT_STARTED          This EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        The instance is not configured as an active one,
                                   or it is not in Tcp4StateClosed state.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     The driver can't allocate enough resources to
                                   initiate the active open.
  @retval EFI_DEVICE_ERROR         An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
Tcp4Connect (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_CONNECTION_TOKEN   *ConnectionToken
  )
{
  SOCKET  *Sock;

  if (NULL == This || NULL == ConnectionToken || NULL == ConnectionToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockConnect (Sock, ConnectionToken);
}

/**
  Listen on the passive instance to accept an incoming connection request.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  ListenToken          Pointer to the listen token to return when
                                   operation finishes.

  @retval EFI_SUCCESS              The listen token was queued successfully.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        The instance is not a passive one or it is not
                                   in Tcp4StateListen state or a same listen token
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
  IN EFI_TCP4_PROTOCOL             *This,
  IN EFI_TCP4_LISTEN_TOKEN         *ListenToken
  )
{
  SOCKET  *Sock;

  if (NULL == This || NULL == ListenToken || NULL == ListenToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockAccept (Sock, ListenToken);
}

/**
  Queues outgoing data into the transmit queue

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  Token                Pointer to the completion token to queue to the
                                   transmit queue.

  @retval EFI_SUCCESS              The data has been queued for transmission.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_NO_MAPPING           When using a default address, configuration
                                   (DHCP, BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid
  @retval EFI_ACCESS_DENIED        One or more of the following conditions is TRUE:
                                   * A transmit completion token with the same
                                   Token-> CompletionToken.Event was already in the
                                   transmission queue. * The current instance is in
                                   Tcp4StateClosed state. * The current instance is
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
  IN EFI_TCP4_PROTOCOL            *This,
  IN EFI_TCP4_IO_TOKEN            *Token
  )
{
  SOCKET      *Sock;
  EFI_STATUS  Status;

  if (NULL == This ||
      NULL == Token ||
      NULL == Token->CompletionToken.Event ||
      NULL == Token->Packet.TxData ||
      0 == Token->Packet.TxData->FragmentCount ||
      0 == Token->Packet.TxData->DataLength
      ) {
    return EFI_INVALID_PARAMETER;
  }

  Status = TcpChkDataBuf (
            Token->Packet.TxData->DataLength,
            Token->Packet.TxData->FragmentCount,
            Token->Packet.TxData->FragmentTable
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockSend (Sock, Token);
}

/**
  Place an asynchronous receive request into the receiving queue.

  @param[in]  This                 Pointer to the EFI_TCP4_PROTOCOL instance.
  @param[in]  Token                Pointer to a token that is associated with the
                                   receive data descriptor.

  @retval EFI_SUCCESS              The receive completion token was cached
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
  @retval EFI_CONNECTION_FIN       The communication peer has closed the connection,
                                   and there is no any buffered data in the receive
                                   buffer of this instance.
  @retval EFI_NOT_READY            The receive request could not be queued because
                                   the receive queue is full.

**/
EFI_STATUS
EFIAPI
Tcp4Receive (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_IO_TOKEN           *Token
  )
{
  SOCKET      *Sock;
  EFI_STATUS  Status;

  if (NULL == This ||
      NULL == Token ||
      NULL == Token->CompletionToken.Event ||
      NULL == Token->Packet.RxData ||
      0 == Token->Packet.RxData->FragmentCount ||
      0 == Token->Packet.RxData->DataLength
      ) {
    return EFI_INVALID_PARAMETER;
  }

  Status = TcpChkDataBuf (
            Token->Packet.RxData->DataLength,
            Token->Packet.RxData->FragmentCount,
            Token->Packet.RxData->FragmentTable
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockRcv (Sock, Token);

}

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
                                   set to NULL, and this function has not returned.
                                   * Previous Close() call on this instance has not
                                   finished.
  @retval EFI_INVALID_PARAMETER    One ore more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate enough resources to finish the
                                   operation.
  @retval EFI_DEVICE_ERROR         Any unexpected category error not belonging to those
                                   listed above.

**/
EFI_STATUS
EFIAPI
Tcp4Close (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_CLOSE_TOKEN        *CloseToken
  )
{
  SOCKET  *Sock;

  if (NULL == This || NULL == CloseToken || NULL == CloseToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockClose (Sock, CloseToken, CloseToken->AbortOnClose);
}

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
  IN EFI_TCP4_PROTOCOL             *This,
  IN EFI_TCP4_COMPLETION_TOKEN     *Token OPTIONAL
  )
{
  SOCKET  *Sock;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockCancel (Sock, Token);
}

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
  IN EFI_TCP4_PROTOCOL        *This
  )
{
  SOCKET      *Sock;
  EFI_STATUS  Status;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  Sock   = SOCK_FROM_THIS (This);

  Status = Sock->ProtoHandler (Sock, SOCK_POLL, NULL);

  return Status;
}

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
                                data indirectly used by the TCP instance is returned.
                                Optional parameter that may be NULL.
  @param[out] SnpModeData       The buffer in which the current SNP mode data
                                indirectly used by the TCP instance is returned.
                                Optional parameter that may be NULL.

  @retval EFI_SUCCESS           The mode data was read.
  @retval EFI_NOT_STARTED       No configuration data is available because this instance hasn't
                                been started.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
EFI_STATUS
EFIAPI
Tcp6GetModeData (
  IN  EFI_TCP6_PROTOCOL                  *This,
  OUT EFI_TCP6_CONNECTION_STATE          *Tcp6State      OPTIONAL,
  OUT EFI_TCP6_CONFIG_DATA               *Tcp6ConfigData OPTIONAL,
  OUT EFI_IP6_MODE_DATA                  *Ip6ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA    *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE            *SnpModeData    OPTIONAL
  )
{
  TCP6_MODE_DATA  TcpMode;
  SOCKET          *Sock;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  Sock                   = SOCK_FROM_THIS (This);

  TcpMode.Tcp6State      = Tcp6State;
  TcpMode.Tcp6ConfigData = Tcp6ConfigData;
  TcpMode.Ip6ModeData    = Ip6ModeData;
  TcpMode.MnpConfigData  = MnpConfigData;
  TcpMode.SnpModeData    = SnpModeData;

  return SockGetMode (Sock, &TcpMode);
}

/**
  Initialize or brutally reset the operational parameters for this EFI TCPv6 instance.

  The Configure() function does the following:
  - Initialize this TCP instance, i.e., initialize the communication end settings and
    specify active open or passive open for an instance.
  - Reset this TCP instance brutally, i.e., cancel all pending asynchronous tokens, flush
    transmission and receiving buffer directly without informing the communication peer.

  No other TCPv6 Protocol operation except Poll() can be executed by this instance until
  it is configured properly. For an active TCP instance, after a proper configuration it
  may call Connect() to initiate a three-way handshake. For a passive TCP instance,
  its state transits to Tcp6StateListen after configuration, and Accept() may be
  called to listen the incoming TCP connection requests. If Tcp6ConfigData is set to NULL,
  the instance is reset. The resetting process will be done brutally, the state machine will
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
  @retval EFI_ACCESS_DENIED     Configuring a TCP instance when it is configured without
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
  IN EFI_TCP6_PROTOCOL        *This,
  IN EFI_TCP6_CONFIG_DATA     *Tcp6ConfigData OPTIONAL
  )
{
  EFI_TCP6_OPTION   *Option;
  SOCKET            *Sock;
  EFI_STATUS        Status;
  EFI_IPv6_ADDRESS  *Ip;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Tcp protocol related parameter check will be conducted here
  //
  if (NULL != Tcp6ConfigData) {

    Ip = &Tcp6ConfigData->AccessPoint.RemoteAddress;
    if (!NetIp6IsUnspecifiedAddr (Ip) && !NetIp6IsValidUnicast (Ip)) {
      return EFI_INVALID_PARAMETER;
    }

    if (Tcp6ConfigData->AccessPoint.ActiveFlag &&
        (0 == Tcp6ConfigData->AccessPoint.RemotePort || NetIp6IsUnspecifiedAddr (Ip))
        ) {
      return EFI_INVALID_PARAMETER;
    }

    Ip = &Tcp6ConfigData->AccessPoint.StationAddress;
    if (!NetIp6IsUnspecifiedAddr (Ip) && !NetIp6IsValidUnicast (Ip)) {
      return EFI_INVALID_PARAMETER;
    }

    Option = Tcp6ConfigData->ControlOption;
    if ((NULL != Option) && (Option->EnableSelectiveAck || Option->EnablePathMtuDiscovery)) {
      return EFI_UNSUPPORTED;
    }
  }

  Sock = SOCK_FROM_THIS (This);

  if (NULL == Tcp6ConfigData) {
    return SockFlush (Sock);
  }

  Status = SockConfigure (Sock, Tcp6ConfigData);

  if (EFI_NO_MAPPING == Status) {
    Sock->ConfigureState = SO_NO_MAPPING;
  }

  return Status;
}

/**
  Initiate a nonblocking TCP connection request for an active TCP instance.

  The Connect() function will initiate an active open to the remote peer configured
  in a current TCP instance if it is configured active. If the connection succeeds or
  fails due to any error, the ConnectionToken->CompletionToken.Event will be signaled
  and ConnectionToken->CompletionToken.Status will be updated accordingly. This
  function can only be called for the TCP instance in the Tcp6StateClosed state. The
  instance will transfer into Tcp6StateSynSent if the function returns EFI_SUCCESS.
  If a TCP three-way handshake succeeds, its state will become Tcp6StateEstablished.
  Otherwise, the state will return to Tcp6StateClosed.

  @param[in] This                Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] ConnectionToken     Pointer to the connection token to return when the TCP three
                                 way handshake finishes.

  @retval EFI_SUCCESS            The connection request successfully initiated and the state of
                                 this TCP instance has been changed to Tcp6StateSynSent.
  @retval EFI_NOT_STARTED        This EFI TCPv6 Protocol instance has not been configured.
  @retval EFI_ACCESS_DENIED      One or more of the following conditions are TRUE:
                                 - This instance is not configured as an active one.
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
  IN EFI_TCP6_PROTOCOL           *This,
  IN EFI_TCP6_CONNECTION_TOKEN   *ConnectionToken
  )
{
  SOCKET  *Sock;

  if (NULL == This || NULL == ConnectionToken || NULL == ConnectionToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockConnect (Sock, ConnectionToken);
}

/**
  Listen on the passive instance to accept an incoming connection request. This is a
  nonblocking operation.

  The Accept() function initiates an asynchronous accept request to wait for an incoming
  connection on the passive TCP instance. If a remote peer successfully establishes a
  connection with this instance, a new TCP instance will be created and its handle will
  be returned in ListenToken->NewChildHandle. The newly created instance is configured
  by inheriting the passive instance's configuration and is ready for use upon return.
  The new instance is in the Tcp6StateEstablished state.

  The ListenToken->CompletionToken.Event will be signaled when a new connection is
  accepted, when a user aborts the listen or when a connection is reset.

  This function only can be called when a current TCP instance is in Tcp6StateListen state.

  @param[in] This                Pointer to the EFI_TCP6_PROTOCOL instance.
  @param[in] ListenToken         Pointer to the listen token to return when operation finishes.


  @retval EFI_SUCCESS            The listen token queued successfully.
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
  @retval EFI_OUT_OF_RESOURCES   Could not allocate enough resource to finish the operation.
  @retval EFI_DEVICE_ERROR       Any unexpected error not belonging to a category listed above.

**/
EFI_STATUS
EFIAPI
Tcp6Accept (
  IN EFI_TCP6_PROTOCOL             *This,
  IN EFI_TCP6_LISTEN_TOKEN         *ListenToken
  )
{
  SOCKET  *Sock;

  if (NULL == This || NULL == ListenToken || NULL == ListenToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockAccept (Sock, ListenToken);
}

/**
  Queues outgoing data into the transmit queue.

  The Transmit() function queues a sending request to this TCP instance along with the
  user data. The status of the token is updated and the event in the token will be
  signaled once the data is sent out or an error occurs.

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
  @retval EFI_OUT_OF_RESOURCES    Could not queue the transmit data because of resource
                                  shortage.
  @retval EFI_NETWORK_UNREACHABLE There is no route to the destination network or address.

**/
EFI_STATUS
EFIAPI
Tcp6Transmit (
  IN EFI_TCP6_PROTOCOL            *This,
  IN EFI_TCP6_IO_TOKEN            *Token
  )
{
  SOCKET      *Sock;
  EFI_STATUS  Status;

  if (NULL == This ||
      NULL == Token ||
      NULL == Token->CompletionToken.Event ||
      NULL == Token->Packet.TxData ||
      0 == Token->Packet.TxData->FragmentCount ||
      0 == Token->Packet.TxData->DataLength
      ) {
    return EFI_INVALID_PARAMETER;
  }

  Status = TcpChkDataBuf (
            Token->Packet.TxData->DataLength,
            Token->Packet.TxData->FragmentCount,
            (EFI_TCP4_FRAGMENT_DATA *) Token->Packet.TxData->FragmentTable
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockSend (Sock, Token);
}

/**
  Places an asynchronous receive request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue. This
  function is always asynchronous. The caller must allocate the Token->CompletionToken.Event
  and the FragmentBuffer used to receive data. The caller also must fill the DataLength that
  represents the whole length of all FragmentBuffer. When the receive operation completes, the
  EFI TCPv6 Protocol driver updates the Token->CompletionToken.Status and Token->Packet.RxData
  fields, and the Token->CompletionToken.Event is signaled. If data obtained, the data and its length
  will be copied into the FragmentTable; at the same time the full length of received data will
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
                                 - User has called Close() to disconnect this connection.
  @retval EFI_CONNECTION_FIN     The communication peer has closed the connection and there is no
                                 buffered data in the receive buffer of this instance.
  @retval EFI_NOT_READY          The receive request could not be queued because the receive queue is full.

**/
EFI_STATUS
EFIAPI
Tcp6Receive (
  IN EFI_TCP6_PROTOCOL           *This,
  IN EFI_TCP6_IO_TOKEN           *Token
  )
{
  SOCKET      *Sock;
  EFI_STATUS  Status;

  if (NULL == This ||
      NULL == Token ||
      NULL == Token->CompletionToken.Event ||
      NULL == Token->Packet.RxData ||
      0 == Token->Packet.RxData->FragmentCount ||
      0 == Token->Packet.RxData->DataLength
      ) {
    return EFI_INVALID_PARAMETER;
  }

  Status = TcpChkDataBuf (
            Token->Packet.RxData->DataLength,
            Token->Packet.RxData->FragmentCount,
            (EFI_TCP4_FRAGMENT_DATA *) Token->Packet.RxData->FragmentTable
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockRcv (Sock, Token);
}

/**
  Disconnecting a TCP connection gracefully or reset a TCP connection. This function is a
  nonblocking operation.

  Initiate an asynchronous close token to the TCP driver. After Close() is called, any buffered
  transmission data will be sent by the TCP driver, and the current instance will have a graceful close
  working flow described as RFC 793 if AbortOnClose is set to FALSE. Otherwise, a rest packet
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
  @retval EFI_OUT_OF_RESOURCES   Could not allocate enough resource to finish the operation.
  @retval EFI_DEVICE_ERROR       Any unexpected error not belonging to error categories given above.

**/
EFI_STATUS
EFIAPI
Tcp6Close (
  IN EFI_TCP6_PROTOCOL           *This,
  IN EFI_TCP6_CLOSE_TOKEN        *CloseToken
  )
{
  SOCKET  *Sock;

  if (NULL == This || NULL == CloseToken || NULL == CloseToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockClose (Sock, CloseToken, CloseToken->AbortOnClose);
}

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
  IN EFI_TCP6_PROTOCOL           *This,
  IN EFI_TCP6_COMPLETION_TOKEN   *Token OPTIONAL
  )
{
  SOCKET  *Sock;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockCancel (Sock, Token);
}

/**
  Poll to receive incoming data and transmit outgoing segments.

  The Poll() function increases the rate that data is moved between the network
  and application, and can be called when the TCP instance is created successfully.
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
  IN EFI_TCP6_PROTOCOL        *This
  )
{
  SOCKET      *Sock;
  EFI_STATUS  Status;

  if (NULL == This) {
    return EFI_INVALID_PARAMETER;
  }

  Sock   = SOCK_FROM_THIS (This);

  Status = Sock->ProtoHandler (Sock, SOCK_POLL, NULL);

  return Status;
}

