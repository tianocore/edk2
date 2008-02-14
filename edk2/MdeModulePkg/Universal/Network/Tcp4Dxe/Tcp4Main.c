/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Tcp4Main.c

Abstract:

  Implementation of TCP4 protocol services.


**/

#include "Tcp4Main.h"


/**
  Check the integrity of the data buffer.

  @param  DataLen                  The total length of the data buffer.
  @param  FragmentCount            The fragment count of the fragment table.
  @param  FragmentTable            Pointer to the fragment table of the data
                                   buffer.

  @retval EFI_SUCCESS              The integrity check is passed.
  @retval EFI_INVALID_PARAMETER    The integrity check is failed.

**/
STATIC
EFI_STATUS
Tcp4ChkDataBuf (
  IN UINT32                 DataLen,
  IN UINT32                 FragmentCount,
  IN EFI_TCP4_FRAGMENT_DATA *FragmentTable
  )
{
  UINT32 Index;

  UINT32 Len;

  for (Index = 0, Len = 0; Index < FragmentCount; Index++) {
    Len = Len + (UINT32) FragmentTable[Index].FragmentLength;
  }

  if (DataLen != Len) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


/**
  Get the current operational status.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  Tcp4State                Pointer to the buffer to receive the current TCP
                                   state.
  @param  Tcp4ConfigData           Pointer to the buffer to receive the current TCP
                                   configuration.
  @param  Ip4ModeData              Pointer to the buffer to receive the current
                                   IPv4 configuration.
  @param  MnpConfigData            Pointer to the buffer to receive the current MNP
                                   configuration data indirectly used by the TCPv4
                                   Instance.
  @param  SnpModeData              Pointer to the buffer to receive the current SNP
                                   configuration data indirectly used by the TCPv4
                                   Instance.

  @retval EFI_SUCCESS              The mode data was read.
  @retval EFI_NOT_STARTED          No configuration data is available because this
                                   instance hasn't been started.
  @retval EFI_INVALID_PARAMETER    This is NULL.

**/
EFI_STATUS
EFIAPI
Tcp4GetModeData (
  IN  CONST EFI_TCP4_PROTOCOL                  * This,
  OUT       EFI_TCP4_CONNECTION_STATE          * Tcp4State OPTIONAL,
  OUT       EFI_TCP4_CONFIG_DATA               * Tcp4ConfigData OPTIONAL,
  OUT       EFI_IP4_MODE_DATA                  * Ip4ModeData OPTIONAL,
  OUT       EFI_MANAGED_NETWORK_CONFIG_DATA    * MnpConfigData OPTIONAL,
  OUT       EFI_SIMPLE_NETWORK_MODE            * SnpModeData OPTIONAL
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

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  TcpConfigData            Pointer to the configure data to configure the
                                   instance.

  @retval EFI_SUCCESS              The operational settings are set, changed, or
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
    if ((Ip != 0) && !Ip4IsUnicast (NTOHL (Ip), 0)) {
      return EFI_INVALID_PARAMETER;
    }

    if (TcpConfigData->AccessPoint.ActiveFlag &&
      (0 == TcpConfigData->AccessPoint.RemotePort || (Ip == 0))) {
      return EFI_INVALID_PARAMETER;
    }

    if (!TcpConfigData->AccessPoint.UseDefaultAddress) {

      CopyMem (&Ip, &TcpConfigData->AccessPoint.StationAddress, sizeof (IP4_ADDR));
      CopyMem (&SubnetMask, &TcpConfigData->AccessPoint.SubnetMask, sizeof (IP4_ADDR));
      if (!Ip4IsUnicast (NTOHL (Ip), 0) || !IP4_IS_VALID_NETMASK (NTOHL (SubnetMask))) {
        return EFI_INVALID_PARAMETER;
      }
    }

    Option = TcpConfigData->ControlOption;
    if ((NULL != Option) &&
        (Option->EnableSelectiveAck || Option->EnablePathMtuDiscovery)) {
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

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  DeleteRoute              If TRUE, delete the specified route from routing
                                   table; if FALSE, add the specified route to
                                   routing table.
  @param  SubnetAddress            The destination network.
  @param  SubnetMask               The subnet mask for the destination network.
  @param  GatewayAddress           The gateway address for this route.

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
  Initiate a nonblocking TCP connection request for an active TCP instance.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance
  @param  ConnectionToken          Pointer to the connection token to return when
                                   the TCP three way handshake finishes.

  @retval EFI_SUCCESS              The connection request is successfully
                                   initiated.
  @retval EFI_NOT_STARTED          This EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        The instance is not configured as an active one
                                   or it is not in Tcp4StateClosed state.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     The driver can't allocate enough resource to
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

  if (NULL == This ||
      NULL == ConnectionToken ||
      NULL == ConnectionToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockConnect (Sock, ConnectionToken);
}


/**
  Listen on the passive instance to accept an incoming connection request.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance
  @param  ListenToken              Pointer to the listen token to return when
                                   operation finishes.

  @retval EFI_SUCCESS              The listen token has been queued successfully.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        The instatnce is not a passive one or it is not
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

  if (NULL == This ||
      NULL == ListenToken ||
      NULL == ListenToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockAccept (Sock, ListenToken);
}


/**
  Queues outgoing data into the transmit queue

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance
  @param  Token                    Pointer to the completion token to queue to the
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
  @retval EFI_OUT_OF_RESOURCES     Could not queue the transmit data because of
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

  Status = Tcp4ChkDataBuf (
            (UINT32) Token->Packet.TxData->DataLength,
            (UINT32) Token->Packet.TxData->FragmentCount,
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

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  Token                    Pointer to a token that is associated with the
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
  @retval EFI_CONNECTION_FIN       The communication peer has closed the connection
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

  Status = Tcp4ChkDataBuf (
            (UINT32) Token->Packet.RxData->DataLength,
            (UINT32) Token->Packet.RxData->FragmentCount,
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

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance
  @param  CloseToken               Pointer to the close token to return when
                                   operation finishes.

  @retval EFI_SUCCESS              The operation completed successfully
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        One or more of the following are TRUE: *
                                   Configure() has been called with TcpConfigData
                                   set to NULL and this function has not returned.
                                   * Previous Close() call on this instance has not
                                   finished.
  @retval EFI_INVALID_PARAMETER    One ore more parameters are invalid
  @retval EFI_OUT_OF_RESOURCES     Could not allocate enough resource to finish the
                                   operation
  @retval EFI_DEVICE_ERROR         Any unexpected and not belonged to above
                                   category error.

**/
EFI_STATUS
EFIAPI
Tcp4Close (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_CLOSE_TOKEN        *CloseToken
  )
{
  SOCKET  *Sock;

  if (NULL == This ||
      NULL == CloseToken ||
      NULL == CloseToken->CompletionToken.Event) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = SOCK_FROM_THIS (This);

  return SockClose (Sock, CloseToken, CloseToken->AbortOnClose);
}


/**
  Abort an asynchronous connection, listen, transmission or receive request.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  Token                    Pointer to a token that has been issued by
                                   Connect(), Accept(), Transmit() or Receive(). If
                                   NULL, all pending tokens issued by above four
                                   functions will be aborted.

  @retval EFI_UNSUPPORTED          The operation is not supported in current
                                   implementation.

**/
EFI_STATUS
EFIAPI
Tcp4Cancel (
  IN EFI_TCP4_PROTOCOL           * This,
  IN EFI_TCP4_COMPLETION_TOKEN   * Token OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Poll to receive incoming data and transmit outgoing segments.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.

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

  Sock    = SOCK_FROM_THIS (This);

  Status  = Sock->ProtoHandler (Sock, SOCK_POLL, NULL);

  return Status;
}
