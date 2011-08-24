/** @file
  TCP4 protocol services header file.

Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCP4_MAIN_H_
#define _TCP4_MAIN_H_

#include "Socket.h"

#include "Tcp4Proto.h"
#include "Tcp4Func.h"
#include "Tcp4Driver.h"


extern UINT16  mTcp4RandomPort;
extern CHAR16  *mTcpStateName[];

//
// Driver Produced Protocol Prototypes
//

//
// Function prototype for the Tcp4 socket request handler
//
/**
  The procotol handler provided to the socket layer, used to
  dispatch the socket level requests by calling the corresponding
  TCP layer functions.

  @param  Sock                   Pointer to the socket of this TCP instance.
  @param  Request                The code of this operation request.
  @param  Data                   Pointer to the operation specific data passed in
                                 together with the operation request.

  @retval EFI_SUCCESS            The socket request is completed successfully.
  @retval other                  The error status returned by the corresponding TCP
                                 layer function.

**/
EFI_STATUS
Tcp4Dispatcher (
  IN SOCKET                  *Sock,
  IN UINT8                   Request,
  IN VOID                    *Data    OPTIONAL
  );
  
///
/// TCP mode data
///
typedef struct _TCP4_MODE_DATA {
  EFI_TCP4_CONNECTION_STATE       *Tcp4State;
  EFI_TCP4_CONFIG_DATA            *Tcp4ConfigData;
  EFI_IP4_MODE_DATA               *Ip4ModeData;
  EFI_MANAGED_NETWORK_CONFIG_DATA *MnpConfigData;
  EFI_SIMPLE_NETWORK_MODE         *SnpModeData;
} TCP4_MODE_DATA;

///
/// TCP route infomation data
///
typedef struct _TCP4_ROUTE_INFO {
  BOOLEAN           DeleteRoute;
  EFI_IPv4_ADDRESS  *SubnetAddress;
  EFI_IPv4_ADDRESS  *SubnetMask;
  EFI_IPv4_ADDRESS  *GatewayAddress;
} TCP4_ROUTE_INFO;

/**
  Get the current operational status of a TCP instance.
  
  The GetModeData() function copies the current operational settings of this 
  EFI TCPv4 Protocol instance into user-supplied buffers. This function can 
  also be used to retrieve the operational setting of underlying drivers 
  such as IPv4, MNP, or SNP.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  Tcp4State                Pointer to the buffer to receive the current TCP
                                   state.
  @param  Tcp4ConfigData           Pointer to the buffer to receive the current TCP
                                   configuration.
  @param  Ip4ModeData              Pointer to the buffer to receive the current IPv4 
                                   configuration data used by the TCPv4 instance.
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
  IN      EFI_TCP4_PROTOCOL                  *This,
     OUT  EFI_TCP4_CONNECTION_STATE          *Tcp4State       OPTIONAL,
     OUT  EFI_TCP4_CONFIG_DATA               *Tcp4ConfigData  OPTIONAL,
     OUT  EFI_IP4_MODE_DATA                  *Ip4ModeData     OPTIONAL,
     OUT  EFI_MANAGED_NETWORK_CONFIG_DATA    *MnpConfigData   OPTIONAL,
     OUT  EFI_SIMPLE_NETWORK_MODE            *SnpModeData     OPTIONAL
  );


/**
  Initialize or brutally reset the operational parameters for
  this EFI TCPv4 instance.
  
  The Configure() function does the following:
  * Initialize this EFI TCPv4 instance, i.e., initialize the communication end 
  setting, specify active open or passive open for an instance.
  * Reset this TCPv4 instance brutally, i.e., cancel all pending asynchronous 
  tokens, flush transmission and receiving buffer directly without informing 
  the communication peer.
  No other TCPv4 Protocol operation can be executed by this instance 
  until it is configured properly. For an active TCP4 instance, after a proper 
  configuration it may call Connect() to initiates the three-way handshake. 
  For a passive TCP4 instance, its state will transit to Tcp4StateListen after 
  configuration, and Accept() may be called to listen the incoming TCP connection 
  request. If TcpConfigData is set to NULL, the instance is reset. Resetting 
  process will be done brutally, the state machine will be set to Tcp4StateClosed 
  directly, the receive queue and transmit queue will be flushed, and no traffic is 
  allowed through this instance.

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
  IN EFI_TCP4_PROTOCOL        *This,
  IN EFI_TCP4_CONFIG_DATA     *TcpConfigData     OPTIONAL
  );

/**
  Add or delete routing entries.
  
  The Routes() function adds or deletes a route from the instance's routing table.
  The most specific route is selected by comparing the SubnetAddress with the 
  destination IP address's arithmetical AND to the SubnetMask.
  The default route is added with both SubnetAddress and SubnetMask set to 0.0.0.0. 
  The default route matches all destination IP addresses if there is no more specific route.
  Direct route is added with GatewayAddress set to 0.0.0.0. Packets are sent to 
  the destination host if its address can be found in the Address Resolution Protocol (ARP) 
  cache or it is on the local subnet. If the instance is configured to use default 
  address, a direct route to the local network will be added automatically.
  Each TCP instance has its own independent routing table. Instance that uses the 
  default IP address will have a copy of the EFI_IP4_CONFIG_PROTOCOL's routing table. 
  The copy will be updated automatically whenever the IP driver reconfigures its 
  instance. As a result, the previous modification to the instance's local copy 
  will be lost. The priority of checking the route table is specific with IP 
  implementation and every IP implementation must comply with RFC 1122.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  DeleteRoute              If TRUE, delete the specified route from routing
                                   table; if FALSE, add the specified route to
                                   routing table.
                                   DestinationAddress and SubnetMask are used as 
                                   the keywords to search route entry.
  @param  SubnetAddress            The destination network.
  @param  SubnetMask               The subnet mask for the destination network.
  @param  GatewayAddress           The gateway address for this route. 
                                   It must be on the same subnet with the station 
                                   address unless a direct route is specified.
                                   
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
  );

/**
  Initiate a nonblocking TCP connection request for an active TCP instance.

  The Connect() function will initiate an active open to the remote peer configured 
  in current TCP instance if it is configured active. If the connection succeeds 
  or fails due to any error, the ConnectionToken->CompletionToken.Event will be 
  signaled and ConnectionToken->CompletionToken.Status will be updated accordingly. 
  This function can only be called for the TCP instance in Tcp4StateClosed state. 
  The instance will transfer into Tcp4StateSynSent if the function returns EFI_SUCCESS. 
  If TCP three way handshake succeeds, its state will become Tcp4StateEstablished, 
  otherwise, the state will return to Tcp4StateClosed.
  
  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance
  @param  ConnectionToken          Pointer to the connection token to return when
                                   the TCP three way handshake finishes.

  @retval EFI_SUCCESS              The connection request is successfully initiated 
                                   and the state of this TCPv4 instance has 
                                   been changed to Tcp4StateSynSent.
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
  );

/**
  Listen on the passive instance to accept an incoming connection request.

  The Accept() function initiates an asynchronous accept request to wait for an 
  incoming connection on the passive TCP instance. If a remote peer successfully 
  establishes a connection with this instance, a new TCP instance will be created 
  and its handle will be returned in ListenToken->NewChildHandle. The newly created 
  instance is configured by inheriting the passive instance's configuration and is 
  ready for use upon return. The instance is in the Tcp4StateEstablished state.
  The ListenToken->CompletionToken.Event will be signaled when a new connection 
  is accepted, user aborts the listen or connection is reset. This function only 
  can be called when current TCP instance is in Tcp4StateListen state.

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
  @retval EFI_DEVICE_ERROR         Any unexpected and not belonged to above category error.

**/
EFI_STATUS
EFIAPI
Tcp4Accept (
  IN EFI_TCP4_PROTOCOL             *This,
  IN EFI_TCP4_LISTEN_TOKEN         *ListenToken
  );

/**
  Queues outgoing data into the transmit queue.

  The Transmit() function queues a sending request to this TCPv4 instance along 
  with the user data. The status of the token is updated and the event in the token 
  will be signaled once the data is sent out or some error occurs.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance
  @param  Token                    Pointer to the completion token to queue to the
                                   transmit queue

  @retval EFI_SUCCESS              The data has been queued for transmission.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_NO_MAPPING           When using a default address, configuration
                                   (DHCP, BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER    One or more parameters are invalid.
  @retval EFI_ACCESS_DENIED        One or more of the following conditions is TRUE:
                                   * A transmit completion token with the same
                                     Token-> CompletionToken.Event was already in the
                                     transmission queue. 
                                   * The current instance is in Tcp4StateClosed state 
                                   * The current instance is a passive one and 
                                     it is in Tcp4StateListen state. 
                                   * User has called Close() to disconnect this 
                                     connection.
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
  );

/**
  Place an asynchronous receive request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue. 
  This function is always asynchronous. The caller must allocate the 
  Token->CompletionToken.Event and the FragmentBuffer used to receive data. He also 
  must fill the DataLength which represents the whole length of all FragmentBuffer. 
  When the receive operation completes, the EFI TCPv4 Protocol driver updates the 
  Token->CompletionToken.Status and Token->Packet.RxData fields and the 
  Token->CompletionToken.Event is signaled. If got data the data and its length 
  will be copy into the FragmentTable, in the same time the full length of received 
  data will be recorded in the DataLength fields. Providing a proper notification 
  function and context for the event will enable the user to receive the notification 
  and receiving status. That notification function is guaranteed to not be re-entered.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  Token                    Pointer to a token that is associated with the
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
                                   The EFI TCPv4 Protocol instance has been reset 
                                   to startup defaults.
  @retval EFI_ACCESS_DENIED        One or more of the following conditions is TRUE:
                                   * A receive completion token with the same
                                     Token->CompletionToken.Event was already in 
                                     the receive queue. 
                                   * The current instance is in Tcp4StateClosed state. 
                                   * The current instance is a passive one and it 
                                     is in Tcp4StateListen state. 
                                   * User has called Close() to disconnect this 
                                     connection.
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
  );

/**
  Disconnecting a TCP connection gracefully or reset a TCP connection.

  Initiate an asynchronous close token to TCP driver. After Close() is called, 
  any buffered transmission data will be sent by TCP driver and the current 
  instance will have a graceful close working flow described as RFC 793 if 
  AbortOnClose is set to FALSE, otherwise, a rest packet will be sent by TCP 
  driver to fast disconnect this connection. When the close operation completes 
  successfully the TCP instance is in Tcp4StateClosed state, all pending 
  asynchronous operation is signaled and any buffers used for TCP network traffic 
  is flushed.

  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  CloseToken               Pointer to the close token to return when
                                   operation finishes.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_NOT_STARTED          The EFI_TCP4_PROTOCOL instance hasn't been
                                   configured.
  @retval EFI_ACCESS_DENIED        One or more of the following are TRUE: 
                                   * Configure() has been called with TcpConfigData
                                     set to NULL and this function has not returned.
                                   * Previous Close() call on this instance has not
                                     finished.
  @retval EFI_INVALID_PARAMETER    One ore more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate enough resource to finish the
                                   operation.
  @retval EFI_DEVICE_ERROR         Any unexpected and not belonged to above
                                   category error.

**/
EFI_STATUS
EFIAPI
Tcp4Close (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_CLOSE_TOKEN        *CloseToken
  );

/**
  Abort an asynchronous connection, listen, transmission or receive request.

  The Cancel() function aborts a pending connection, listen, transmit or receive 
  request. If Token is not NULL and the token is in the connection, listen, 
  transmission or receive queue when it is being cancelled, its Token->Status 
  will be set to EFI_ABORTED and then Token->Event will be signaled. If the token 
  is not in one of the queues, which usually means that the asynchronous operation 
  has completed, EFI_NOT_FOUND is returned. If Token is NULL all asynchronous token 
  issued by Connect(), Accept(), Transmit() and Receive()will be aborted.
  NOTE: It has not been implemented currently.
    
  @param  This                     Pointer to the EFI_TCP4_PROTOCOL instance.
  @param  Token                    Pointer to a token that has been issued by
                                   Connect(), Accept(), Transmit() or Receive(). If
                                   NULL, all pending tokens issued by above four
                                   functions will be aborted.
                                   
  @retval  EFI_SUCCESS             The asynchronous I/O request is aborted and Token->Event
                                   is signaled.
  @retval  EFI_INVALID_PARAMETER   This is NULL.
  @retval  EFI_NOT_STARTED         This instance hasn's been configured.
  @retval  EFI_NO_MAPPING          When using the default address, configuration
                                   (DHCP, BOOTP,RARP, etc.) hasn's finished yet.
  @retval  EFI_NOT_FOUND           The asynchronous I/O request isn's found in the 
                                   transmission or receive queue. It has either 
                                   completed or wasn's issued by Transmit() and Receive().
  @retval  EFI_UNSUPPORTED         The operation is not supported in current
                                   implementation.
  
**/
EFI_STATUS
EFIAPI
Tcp4Cancel (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_COMPLETION_TOKEN   *Token    OPTIONAL
  );

/**
  Poll to receive incoming data and transmit outgoing segments.

  The Poll() function increases the rate that data is moved between the network 
  and application and can be called when the TCP instance is created successfully. 
  Its use is optional. In some implementations, the periodical timer in the MNP 
  driver may not poll the underlying communications device fast enough to avoid 
  drop packets. Drivers and applications that are experiencing packet loss should 
  try calling the Poll() function in a high frequency.

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
  );

#endif
