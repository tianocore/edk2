/** @file
  Implement the socket support for the socket layer.

  Socket States:
  * Bound - pSocket->PortList is not NULL
  * Listen - AcceptWait event is not NULL

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


  \section DataStructures Data Structures

  <code><pre>

                +---------------+   +-------------+   +-------------+
  Service Lists | ::ESL_SERVICE |-->| ESL_SERVICE |-->| ESL_SERVICE |--> NULL (pNext)
                +---------------+   +-------------+   +-------------+
                  ^                       | (pPortList)    |
    pUdp4List ^   | pTcp4List             |                |
              |   |                       |                |
          ^   |   |                       |                |
 pIp4List |   |   |                       |                |
        +---------------+                 |                |
        | ::ESL_LAYER   | ::mEslLayer     |                |
        +---------------+                 |                |
                  | (pSocketList)         |                |
    Socket List   V                       V                V
                +---------------+   +-------------+   +-------------+
                | ::ESL_SOCKET  |-->| ::ESL_PORT  |-->|   ESL_PORT  |--> NULL (pLinkSocket)
                +---------------+   +-------------+   +-------------+
                  |                       |                |
                  |                       |                V
                  V                       V               NULL
                +-------------+   +-------------+
                | ESL_SOCKET  |-->|   ESL_PORT  |--> NULL
                +-------------+   +-------------+
                  |    | | | |            |
                  V    | | | |            V
                 NULL  | | | |           NULL
               (pNext) | | | |     (pLinkService)
                       | | | |                                     pRxPacketListHead
                       | | | `-----------------------------------------------.
                       | | |                     pRxOobPacketListHead        |
                       | | `--------------------------------.                |
                       | |      pTxPacketListHead           |                |
                       | `---------------.                  |                |
  pTxOobPacketListHead |                 |                  |                |
                       V                 V                  V                V
                  +--------------+    +------------+    +------------+    +------------+
                  | ::ESL_PACKET |    | ESL_PACKET |    | ESL_PACKET |    | ESL_PACKET |
                  +--------------+    +------------+    +------------+    +------------+
                         |                 |                |                |
                         V                 V                V                V
                  +------------+    +------------+    +------------+    +------------+
                  | ESL_PACKET |    | ESL_PACKET |    | ESL_PACKET |    | ESL_PACKET |
                  +------------+    +------------+    +------------+    +------------+
                         |                 |                |                |
                         V                 V                V                V
                        NULL              NULL             NULL             NULL
                       (pNext)

  </pre></code>

  ::mEslLayer is the one and only ::ESL_LAYER structure.  It connects directly or
  indirectly to the other data structures.  The ESL_LAYER structure has a unique
  service list for each of the network protocol interfaces.

  ::ESL_SERVICE manages the network interfaces for a given transport type (IP4, TCP4, UDP4, etc.)

  ::ESL_SOCKET manages the activity for a single socket instance.  As such, it contains
  the ::EFI_SOCKET_PROTOCOL structure which the BSD socket library uses as the object
  reference and the API into the EFI socket library.

  ::ESL_PORT manages the connection with a single instance of the lower layer network.
  This structure is the socket equivalent of an IP connection or a TCP or UDP port.

  ::ESL_PACKET buffers data for transmit and receive.  There are four queues connected
  to the ::ESL_SOCKET that manage the data:
  <ul>
    <li>ESL_SOCKET::pRxPacketListHead - Normal (low) priority receive data</li>
    <li>ESL_SOCKET::pRxOobPacketListHead - High (out-of-band or urgent) priority receive data</li>
    <li>ESL_SOCKET::pTxPacketListHead - Normal (low) priority transmit data</li>
    <li>ESL_SOCKET::pTxOobPacketListHead - High (out-of-band or urgent) priority transmit data</li>
  </ul>
  The selection of the transmit queue is controlled by the MSG_OOB flag on the transmit
  request as well as the socket option SO_OOBINLINE.  The receive queue is selected by
  the URGENT data flag for TCP and the setting of the socket option SO_OOBINLINE.

  Data structure synchronization is done by raising TPL to TPL_SOCKET.  Modifying
  critical elements within the data structures must be done at this TPL.  TPL is then
  restored to the previous level.  Note that the code verifies that all callbacks are
  entering at TPL_SOCKETS for proper data structure synchronization.

  \section PortCloseStateMachine Port Close State Machine

  The port close state machine walks the port through the necessary
  states to stop activity on the port and get it into a state where
  the resources may be released.  The state machine consists of the
  following arcs and states:

  <code><pre>

      +--------------------------+
      |          Open            |
      +--------------------------+
                   |
                   |  ::EslSocketPortCloseStart
                   V
      +--------------------------+
      | PORT_STATE_CLOSE_STARTED |
      +--------------------------+
                   |
                   |  ::EslSocketPortCloseTxDone
                   V
      +--------------------------+
      | PORT_STATE_CLOSE_TX_DONE |
      +--------------------------+
                   |
                   |  ::EslSocketPortCloseComplete
                   V
      +--------------------------+
      |  PORT_STATE_CLOSE_DONE   |
      +--------------------------+
                   |
                   |  ::EslSocketPortCloseRxDone
                   V
      +--------------------------+
      | PORT_STATE_CLOSE_RX_DONE |
      +--------------------------+
                   |
                   |  ::EslSocketPortClose
                   V
      +--------------------------+
      |          Closed          |
      +--------------------------+

  </pre></code>

  <ul>
    <li>Arc: ::EslSocketPortCloseStart - Marks the port as closing and
      initiates the port close operation</li>
    <li>State: PORT_STATE_CLOSE_STARTED</li>
    <li>Arc: ::EslSocketPortCloseTxDone - Waits until all of the transmit
      operations to complete.  After all of the transmits are complete,
      this routine initiates the network specific close operation by calling
      through ESL_PROTOCOL_API::pfnPortCloseOp.  One such routine is
      ::EslTcp4PortCloseOp.
    </li>
    <li>State: PORT_STATE_CLOSE_TX_DONE</li>
    <li>Arc: ::EslSocketPortCloseComplete - Called when the close operation is
      complete.  After the transition to PORT_STATE_CLOSE_DONE,
      this routine calls ::EslSocketRxCancel to abort the pending receive operations.
    </li>
    <li>State: PORT_STATE_CLOSE_DONE</li>
    <li>Arc: ::EslSocketPortCloseRxDone - Waits until all of the receive
      operation have been cancelled.  After the transition to
      PORT_STATE_CLOSE_RX_DONE, this routine calls ::EslSocketPortClose.
    </li>
    <li>State: PORT_STATE_CLOSE_RX_DONE</li>
    <li>Arc: ::EslSocketPortClose - This routine discards any receive buffers
      using a network specific support routine via ESL_PROTOCOL_API::pfnPacketFree.
      This routine then releases the port resources allocated by ::EslSocketPortAllocate
      and calls the network specific port close routine (e.g. ::EslTcp4PortClose)
      via ESL_PROTOCOL_API::pfnPortClose to release any network specific resources.
    </li>
  </ul>


  \section ReceiveEngine Receive Engine

  The receive path accepts data from the network and queues (buffers) it for the
  application.  Flow control is applied once a maximum amount of buffering is reached
  and is released when the buffer usage drops below that limit.  Eventually the
  application requests data from the socket which removes entries from the queue and
  returns the data.

  The receive engine is the state machine which reads data from the network and
  fills the queue with received packets.  The receive engine uses two data structures
  to manage the network receive opeations and the buffers.

  At a high level, the ::ESL_IO_MGMT structures are managing the tokens and
  events for the interface to the UEFI network stack.  The ::ESL_PACKET
  structures are managing the receive data buffers.  The receive engine
  connects these two structures in the network specific receive completion
  routines.

<code><pre>

      +------------------+
      |     ::ESL_PORT     |
      |                  |
      +------------------+
      |    ::ESL_IO_MGMT   |
      +------------------+
      |    ESL_IO_MGMT   |
      +------------------+
      .                  .
      .    ESL_IO_MGMT   .
      .                  .
      +------------------+

</pre></code>

  The ::ESL_IO_MGMT structures are allocated as part of the ::ESL_PORT structure in
  ::EslSocketPortAllocate.  The ESL_IO_MGMT structures are separated and placed on
  the free list by calling ::EslSocketIoInit.  The ESL_IO_MGMT structure contains
  the network layer specific receive completion token and event.  The receive engine
  is eventually shutdown by ::EslSocketPortCloseTxDone and the resources in these
  structures are released in ::EslSocketPortClose by a call to ::EslSocketIoFree.

<code><pre>

         pPort->pRxActive
                |
                V
          +-------------+   +-------------+   +-------------+
  Active  | ESL_IO_MGMT |-->| ESL_IO_MGMT |-->| ESL_IO_MGMT |--> NULL
          +-------------+   +-------------+   +-------------+

          +-------------+   +-------------+   +-------------+
  Free    | ESL_IO_MGMT |-->| ESL_IO_MGMT |-->| ESL_IO_MGMT |--> NULL
          +-------------+   +-------------+   +-------------+
                ^
                |
          pPort->pRxFree
</pre></code>

  The receive engine is started by calling ::EslSocketRxStart.  Flow control pauses
  the receive engine by stopping the calls to EslSocketRxStart when the amount of
  receive data waiting for the application meets or exceeds MAX_RX_DATA.  After
  the application reads enough data that the amount of buffering drops below this
  limit, the calls to EslSockeRxStart continue which releases the flow control.

  Receive flow control is applied when the port is created, since no receive
  operation are pending to the low layer network driver.  The flow control gets
  released when the low layer network port is configured or the first receive
  operation is posted.  Flow control remains in the released state until the
  maximum buffer space is consumed.  During this time, ::EslSocketRxComplete
  calls ::EslSocketRxStart.  Flow control is applied in EslSocketRxComplete
  by skipping the call to EslSocketRxStart.  Flow control is eventually
  released in ::EslSocketReceive when the buffer space drops below the
  maximum amount causing EslSocketReceive to call EslSocketRxStart.

<code><pre>

                    +------------+   +------------+
    High     .----->| ESL_PACKET |-->| ESL_PACKET |--> NULL (pNext)
  Priority   |      +------------+   +------------+
             |
             | pRxOobPacketListHead
       +------------+
       | ::ESL_SOCKET |
       +------------+
             | pRxPacketListHead
    Low      |
  Priority   |      +------------+   +------------+   +------------+
             `----->| ::ESL_PACKET |-->| ESL_PACKET |-->| ESL_PACKET |--> NULL
                    +------------+   +------------+   +------------+

</pre></code>

  ::EslSocketRxStart connects an ::ESL_PACKET structure to the ::ESL_IO_MGMT structure
  and then calls the network layer to start the receive operation.  Upon
  receive completion, ::EslSocketRxComplete breaks the connection between these
  structrues and places the ESL_IO_MGMT structure onto the ESL_PORT::pRxFree list to
  make token and event available for another receive operation.  EslSocketRxComplete
  then queues the ESL_PACKET structure (data packet) to either the
  ESL_SOCKET::pRxOobPacketListTail or ESL_SOCKET::pRxPacketListTail depending on
  whether urgent or normal data was received.  Finally ::EslSocketRxComplete attempts
  to start another receive operation.

<code><pre>

  Setup for IP4 and UDP4

      +--------------------+
      | ESL_IO_MGMT        |
      |                    |
      |    +---------------+
      |    | Token         |
      |    |        RxData --> NULL
      +----+---------------+
         |
         V
      +--------------------+
      | ESL_PACKET         |
      |                    |
      |    +---------------+
      |    |       pRxData --> NULL
      +----+---------------+

  Completion for IP4 and UDP4

      +--------------------+   +----------------------+
      | ESL_IO_MGMT        |   |      Data Buffer     |
      |                    |   |     (Driver owned)   |
      |    +---------------+   +----------------------+
      |    | Token         |               ^
      |    |      Rx Event |               |
      |    |               |   +----------------------+
      |    |        RxData --> | EFI_IP4_RECEIVE_DATA |
      +----+---------------+   |    (Driver owned)    |
         |                     +----------------------+
         V                                 ^
      +--------------------+               .
      | ESL_PACKET         |               .
      |                    |               .
      |    +---------------+               .
      |    |       pRxData --> NULL  .......
      +----+---------------+


  Setup and completion for TCP4

      +--------------------+   +--------------------------+
      | ESL_IO_MGMT        |-->| ESL_PACKET               |
      |                    |   |                          |
      |    +---------------+   +----------------------+   |
      |    | Token         |   | EFI_IP4_RECEIVE_DATA |   |
      |    |        RxData --> |                      |   |
      |    |               |   +----------------------+---+
      |    |        Event  |   |       Data Buffer        |
      +----+---------------+   |                          |
                               |                          |
                               +--------------------------+

</pre></code>

  To minimize the number of buffer copies, the data is not copied until the
  application makes a receive call.  At this point socket performs a single copy
  in the receive path to move the data from the buffer filled by the network layer
  into the application's buffer.

  The IP4 and UDP4 drivers go one step further to reduce buffer copies.  They
  allow the socket layer to hold on to the actual receive buffer until the
  application has performed a receive operation or closes the socket.  Both
  of theses operations return the buffer to the lower layer network driver
  by calling ESL_PROTOCOL_API::pfnPacketFree.

  When a socket application wants to receive data it indirectly calls
  ::EslSocketReceive to remove data from one of the receive data queues.  This routine
  removes the next available packet from ESL_SOCKET::pRxOobPacketListHead or
  ESL_SOCKET::pRxPacketListHead and copies the data from the packet
  into the application's buffer.  For SOCK_STREAM sockets, if the packet
  contains more data then the ESL_PACKET structures remains at the head of the
  receive queue for the next application receive
  operation.  For SOCK_DGRAM, SOCK_RAW and SOCK_SEQ_PACKET sockets, the ::ESL_PACKET
  structure is removed from the head of the receive queue and any remaining data is
  discarded as the packet is placed on the free queue.

  During socket layer shutdown, ::EslSocketShutdown calls ::EslSocketRxCancel to
  cancel any pending receive operations.  EslSocketRxCancel calls the network specific
  cancel routine using ESL_PORT::pfnRxCancel.


  \section TransmitEngine Transmit Engine

  Application calls to ::EslSocketTransmit cause data to be copied into a buffer.
  The buffer exists as an extension to an ESL_PACKET structure and the structure
  is placed at the end of the transmit queue.

<code><pre>

     *ppQueueHead: pSocket->pRxPacketListHead or pSocket->pRxOobPacketListHead
          |
          V
        +------------+   +------------+   +------------+
  Data  | ESL_PACKET |-->| ESL_PACKET |-->| ESL_PACKET |--> NULL
        +------------+   +------------+   +------------+
                                                     ^
                                                     |
     *ppQueueTail: pSocket->pRxPacketListTail or pSocket->pRxOobPacketListTail

</pre></code>

  There are actually two transmit queues the normal or low priority queue which is
  the default and the urgent or high priority queue which is addressed by specifying
  the MSG_OOB flag during the transmit request.  Associated with each queue is a
  transmit engine which is responsible for sending the data in that queue.

  The transmit engine is the state machine which removes entries from the head
  of the transmit queue and causes the data to be sent over the network.

<code><pre>

      +--------------------+   +--------------------+
      | ESL_IO_MGMT        |   | ESL_PACKET         |
      |                    |   |                    |
      |    +---------------+   +----------------+   |
      |    | Token         |   | Buffer Length  |   |
      |    |        TxData --> | Buffer Address |   |
      |    |               |   +----------------+---+
      |    |        Event  |   | Data Buffer        |
      +----+---------------+   |                    |
                               +--------------------+
</pre></code>

  At a high level, the transmit engine uses a couple of data structures
  to manage the data flow.  The ::ESL_IO_MGMT structures manage the tokens and
  events for the interface to the UEFI network stack.  The ::ESL_PACKET
  structures manage the data buffers that get sent.  The transmit
  engine connects these two structures prior to transmission and disconnects
  them upon completion.

<code><pre>

         pPort->pTxActive or pTxOobActive
                |
                V
          +-------------+   +-------------+   +-------------+
  Active  | ESL_IO_MGMT |-->| ESL_IO_MGMT |-->| ESL_IO_MGMT |--> NULL
          +-------------+   +-------------+   +-------------+

          +-------------+   +-------------+   +-------------+
  Free    | ESL_IO_MGMT |-->| ESL_IO_MGMT |-->| ESL_IO_MGMT |--> NULL
          +-------------+   +-------------+   +-------------+
                ^
                |
          pPort->pTxFree or pTxOobFree

</pre></code>

  The transmit engine manages multiple transmit operations using the
  active and free lists shown above.  ::EslSocketPortAllocate allocates the
  ::ESL_IO_MGMT structures as an extension to the ::ESL_PORT structure.
  This routine places the ESL_IO_MGMT structures on the free list by calling
  ::EslSocketIoInit.  During their lifetime, the ESL_IO_MGMT structures
  will move from the free list to the active list and back again.  The
  active list contains the packets that are actively being processed by
  the UEFI network stack.  Eventually the ESL_IO_MGMT structures will be
  removed from the free list and be deallocated by the EslSocketPortClose
  routine.

  The network specific code calls the ::EslSocketTxStart routine
  to hand a packet to the network stack.  EslSocketTxStart connects
  the transmit packet (::ESL_PACKET) to an ::ESL_IO_MGMT structure
  and then queues the result to one of the active lists:
  ESL_PORT::pTxActive or ESL_PORT::pTxOobActive.  The routine then
  hands the packet to the network stack.

  Upon completion, the network specific TxComplete routine calls
  ::EslSocketTxComplete to disconnect the transmit packet from the
  ESL_IO_MGMT structure and frees the ::ESL_PACKET structure by calling
  ::EslSocketPacketFree.  The routine places the ::ESL_IO_MGMT structure
  into the free list either ESL_PORT::pTxFree or ESL_PORT::pTxOobFree.
  EslSocketTxComplete then starts the next transmit operation while
  the socket is active or calls the ::EslSocketPortCloseTxDone routine
  when the socket is shutting down.

**/

#include "Socket.h"


/** Socket driver connection points

  List the network stack connection points for the socket driver.
**/
CONST ESL_SOCKET_BINDING cEslSocketBinding[] = {
  { L"Ip4",
    &gEfiIp4ServiceBindingProtocolGuid,
    &gEfiIp4ProtocolGuid,
    &mEslIp4ServiceGuid,
    OFFSET_OF ( ESL_LAYER, pIp4List ),
    4,    //  RX buffers
    4,    //  TX buffers
    0 },  //  TX Oob buffers
  { L"Tcp4",
    &gEfiTcp4ServiceBindingProtocolGuid,
    &gEfiTcp4ProtocolGuid,
    &mEslTcp4ServiceGuid,
    OFFSET_OF ( ESL_LAYER, pTcp4List ),
    4,    //  RX buffers
    4,    //  TX buffers
    4 },  //  TX Oob buffers
  { L"Tcp6",
    &gEfiTcp6ServiceBindingProtocolGuid,
    &gEfiTcp6ProtocolGuid,
    &mEslTcp6ServiceGuid,
    OFFSET_OF ( ESL_LAYER, pTcp6List ),
    4,    //  RX buffers
    4,    //  TX buffers
    4 },  //  TX Oob buffers
  { L"Udp4",
    &gEfiUdp4ServiceBindingProtocolGuid,
    &gEfiUdp4ProtocolGuid,
    &mEslUdp4ServiceGuid,
    OFFSET_OF ( ESL_LAYER, pUdp4List ),
    4,    //  RX buffers
    4,    //  TX buffers
    0 },  //  TX Oob buffers
  { L"Udp6",
    &gEfiUdp6ServiceBindingProtocolGuid,
    &gEfiUdp6ProtocolGuid,
    &mEslUdp6ServiceGuid,
    OFFSET_OF ( ESL_LAYER, pUdp6List ),
    4,    //  RX buffers
    4,    //  TX buffers
    0 }   //  TX Oob buffers
};

CONST UINTN cEslSocketBindingEntries = DIM ( cEslSocketBinding );

/// APIs to support the various socket types for the v4 network stack.
CONST ESL_PROTOCOL_API * cEslAfInetApi[] = {
  NULL,             //  0
  &cEslTcp4Api,     //  SOCK_STREAM
  &cEslUdp4Api,     //  SOCK_DGRAM
  &cEslIp4Api,      //  SOCK_RAW
  NULL,             //  SOCK_RDM
  &cEslTcp4Api      //  SOCK_SEQPACKET
};

/// Number of entries in the v4 API array ::cEslAfInetApi.
CONST int cEslAfInetApiSize = DIM ( cEslAfInetApi );


/// APIs to support the various socket types for the v6 network stack.
CONST ESL_PROTOCOL_API * cEslAfInet6Api[] = {
  NULL,             //  0
  &cEslTcp6Api,     //  SOCK_STREAM
  &cEslUdp6Api,     //  SOCK_DGRAM
  NULL,             //  SOCK_RAW
  NULL,             //  SOCK_RDM
  &cEslTcp6Api      //  SOCK_SEQPACKET
};

/// Number of entries in the v6 API array ::cEslAfInet6Api.
CONST int cEslAfInet6ApiSize = DIM ( cEslAfInet6Api );


/// Global management structure for the socket layer.
ESL_LAYER mEslLayer;


/** Initialize an endpoint for network communication.

  This routine initializes the communication endpoint.

  The ::socket routine calls this routine indirectly to create
  the communication endpoint.

  @param[in] pSocketProtocol Address of the socket protocol structure.
  @param[in]  domain   Select the family of protocols for the client or server
                       application.  See the ::socket documentation for values.
  @param[in]  type     Specifies how to make the network connection.
                       See the ::socket documentation for values.
  @param[in]  protocol Specifies the lower layer protocol to use.
                       See the ::socket documentation for values.
  @param[out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created
  @retval EFI_INVALID_PARAMETER - Invalid domain value, errno = EAFNOSUPPORT
  @retval EFI_INVALID_PARAMETER - Invalid type value, errno = EINVAL
  @retval EFI_INVALID_PARAMETER - Invalid protocol value, errno = EINVAL
 **/
EFI_STATUS
EslSocket (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int domain,
  IN int type,
  IN int protocol,
  IN int * pErrno
  )
{
  CONST ESL_PROTOCOL_API * pApi;
  CONST ESL_PROTOCOL_API ** ppApiArray;
  CONST ESL_PROTOCOL_API ** ppApiArrayEnd;
  int ApiArraySize;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  int errno;

  DBG_ENTER ( );

  //  Locate the socket
  pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

  //  Set the default domain if necessary
  if ( AF_UNSPEC == domain ) {
    domain = AF_INET;
  }

  //  Assume success
  errno = 0;
  Status = EFI_SUCCESS;

  //  Use break instead of goto
  for ( ; ; ) {
    //  Validate the domain value
    if (( AF_INET != domain )
      && ( AF_INET6 != domain )
      && ( AF_LOCAL != domain )) {
      DEBUG (( DEBUG_ERROR | DEBUG_SOCKET,
                "ERROR - Invalid domain value\r\n" ));
      Status = EFI_INVALID_PARAMETER;
      errno = EAFNOSUPPORT;
      break;
    }

    //  Determine the protocol APIs
    ppApiArray = NULL;
    ApiArraySize = 0;
    if (( AF_INET == domain )
      || ( AF_LOCAL == domain )) {
      ppApiArray = &cEslAfInetApi[0];
      ApiArraySize = cEslAfInetApiSize;
    }
    else {
      ppApiArray = &cEslAfInet6Api[0];
      ApiArraySize = cEslAfInet6ApiSize;
    }

    //  Set the default type if necessary
    if ( 0 == type ) {
      type = SOCK_STREAM;
    }

    //  Validate the type value
    if (( type >= ApiArraySize )
      || ( NULL == ppApiArray )
      || ( NULL == ppApiArray[ type ])) {
      DEBUG (( DEBUG_ERROR | DEBUG_SOCKET,
                "ERROR - Invalid type value\r\n" ));
      //  The socket type is not supported
      Status = EFI_INVALID_PARAMETER;
      errno = EPROTOTYPE;
      break;
    }

    //  Set the default protocol if necessary
    pApi = ppApiArray[ type ];
    if ( 0 == protocol ) {
      protocol = pApi->DefaultProtocol;
    }

    //  Validate the protocol value
    if (( pApi->DefaultProtocol != protocol )
      && ( SOCK_RAW != type )) {
      Status = EFI_INVALID_PARAMETER;

      //  Assume that the driver supports this protocol
      ppApiArray = &cEslAfInetApi[0];
      ppApiArrayEnd = &ppApiArray [ cEslAfInetApiSize ];
      while ( ppApiArrayEnd > ppApiArray ) {
        pApi = *ppApiArray;
        if ( protocol == pApi->DefaultProtocol ) {
          break;
        }
        ppApiArray += 1;
      }
      if ( ppApiArrayEnd <= ppApiArray ) {
        //  Verify against the IPv6 table
        ppApiArray = &cEslAfInet6Api[0];
        ppApiArrayEnd = &ppApiArray [ cEslAfInet6ApiSize ];
        while ( ppApiArrayEnd > ppApiArray ) {
          pApi = *ppApiArray;
          if ( protocol == pApi->DefaultProtocol ) {
            break;
          }
          ppApiArray += 1;
        }
      }
      if ( ppApiArrayEnd <= ppApiArray ) {
        DEBUG (( DEBUG_ERROR | DEBUG_SOCKET,
                  "ERROR - The protocol is not supported!\r\n" ));
        errno = EPROTONOSUPPORT;
        break;
      }

      //  The driver does not support this protocol
      DEBUG (( DEBUG_ERROR | DEBUG_SOCKET,
                "ERROR - The protocol does not support this socket type!\r\n" ));
      errno = EPROTONOSUPPORT;
      errno = EPROTOTYPE;
      break;
    }
    //  Save the socket attributes
    pSocket->pApi = pApi;
    pSocket->Domain = domain;
    pSocket->Type = type;
    pSocket->Protocol = protocol;

    //  Done
    break;
  }
  //  Return the operation status
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Accept a network connection.

  This routine calls the network specific layer to remove the next
  connection from the FIFO.

  The ::accept calls this routine to poll for a network
  connection to the socket.  When a connection is available
  this routine returns the ::EFI_SOCKET_PROTOCOL structure address
  associated with the new socket and the remote network address
  if requested.

  @param[in]      pSocketProtocol   Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]      pSockAddr         Address of a buffer to receive the remote
                                    network address.
  @param[in,out]  pSockAddrLength   Length in bytes of the address buffer.
                                    On output specifies the length of the
                                    remote network address.
  @param[out]     ppSocketProtocol  Address of a buffer to receive the
                                    ::EFI_SOCKET_PROTOCOL instance
                                    associated with the new socket.
  @param[out]     pErrno            Address to receive the errno value upon completion.

  @retval EFI_SUCCESS   New connection successfully created
  @retval EFI_NOT_READY No connection is available
 **/
EFI_STATUS
EslSocketAccept (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN struct sockaddr * pSockAddr,
  IN OUT socklen_t * pSockAddrLength,
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol,
  IN int * pErrno
  )
{
  ESL_SOCKET * pNewSocket;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  pNewSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Verify the API
    //
    if ( NULL == pSocket->pApi->pfnAccept ) {
      Status = EFI_UNSUPPORTED;
      pSocket->errno = ENOTSUP;
    }
    else {
      //
      //  Validate the sockaddr
      //
      if (( NULL != pSockAddr )
        && ( NULL == pSockAddrLength )) {
        DEBUG (( DEBUG_ACCEPT,
                  "ERROR - pSockAddr is NULL!\r\n" ));
        Status = EFI_INVALID_PARAMETER;
        pSocket->errno = EFAULT;
      }
      else {
        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Verify that the socket is in the listen state
        //
        if ( SOCKET_STATE_LISTENING != pSocket->State ) {
          DEBUG (( DEBUG_ACCEPT,
                    "ERROR - Socket is not listening!\r\n" ));
          if ( NULL == pSocket->pApi->pfnAccept ) {
            //
            //  Socket does not support listen
            //
            pSocket->errno = EOPNOTSUPP;
            Status = EFI_UNSUPPORTED;
          }
          else {
            //
            //  Socket supports listen, but not in listen state
            //
            pSocket->errno = EINVAL;
            Status = EFI_NOT_STARTED;
          }
        }
        else {
          //
          //  Determine if a socket is available
          //
          if ( 0 == pSocket->FifoDepth ) {
            //
            //  No connections available
            //  Determine if any ports are available
            //
            if ( NULL == pSocket->pPortList ) {
              //
              //  No ports available
              //
              Status = EFI_DEVICE_ERROR;
              pSocket->errno = EINVAL;

              //
              //  Update the socket state
              //
              pSocket->State = SOCKET_STATE_NO_PORTS;
            }
            else {
              //
              //  Ports are available
              //  No connection requests at this time
              //
              Status = EFI_NOT_READY;
              pSocket->errno = EAGAIN;
            }
          }
          else {

            //
            //  Attempt to accept the connection and
            //  get the remote network address
            //
            pNewSocket = pSocket->pFifoHead;
            ASSERT ( NULL != pNewSocket );
            Status = pSocket->pApi->pfnAccept ( pNewSocket,
                                                pSockAddr,
                                                pSockAddrLength );
            if ( !EFI_ERROR ( Status )) {
              //
              //  Remove the new socket from the list
              //
              pSocket->pFifoHead = pNewSocket->pNextConnection;
              if ( NULL == pSocket->pFifoHead ) {
                pSocket->pFifoTail = NULL;
              }

              //
              //  Account for this socket
              //
              pSocket->FifoDepth -= 1;

              //
              //  Update the new socket's state
              //
              pNewSocket->State = SOCKET_STATE_CONNECTED;
              pNewSocket->bConfigured = TRUE;
              DEBUG (( DEBUG_ACCEPT,
                        "0x%08x: Socket connected\r\n",
                        pNewSocket ));
            }
          }
        }

        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );
      }
    }
  }

  //
  //  Return the new socket
  //
  if (( NULL != ppSocketProtocol )
    && ( NULL != pNewSocket )) {
    *ppSocketProtocol = &pNewSocket->SocketProtocol;
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Allocate and initialize a ESL_SOCKET structure.

  This support function allocates an ::ESL_SOCKET structure
  and installs a protocol on ChildHandle.  If pChildHandle is a
  pointer to NULL, then a new handle is created and returned in
  pChildHandle.  If pChildHandle is not a pointer to NULL, then
  the protocol installs on the existing pChildHandle.

  @param[in,out]  pChildHandle  Pointer to the handle of the child to create.
                                If it is NULL, then a new handle is created.
                                If it is a pointer to an existing UEFI handle,
                                then the protocol is added to the existing UEFI
                                handle.
  @param[in]      DebugFlags    Flags for debug messages
  @param[in,out]  ppSocket      The buffer to receive an ::ESL_SOCKET structure address.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child
  @retval other                 The child handle was not created
**/
EFI_STATUS
EFIAPI
EslSocketAllocate (
  IN OUT EFI_HANDLE * pChildHandle,
  IN     UINTN DebugFlags,
  IN OUT ESL_SOCKET ** ppSocket
  )
{
  UINTN LengthInBytes;
  ESL_LAYER * pLayer;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Create a socket structure
  //
  LengthInBytes = sizeof ( *pSocket );
  pSocket = (ESL_SOCKET *) AllocateZeroPool ( LengthInBytes );
  if ( NULL != pSocket ) {
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT,
              "0x%08x: Allocate pSocket, %d bytes\r\n",
              pSocket,
              LengthInBytes ));

    //
    //  Initialize the socket protocol
    //
    pSocket->Signature = SOCKET_SIGNATURE;
    pSocket->SocketProtocol.pfnAccept = EslSocketAccept;
    pSocket->SocketProtocol.pfnBind = EslSocketBind;
    pSocket->SocketProtocol.pfnClosePoll = EslSocketClosePoll;
    pSocket->SocketProtocol.pfnCloseStart = EslSocketCloseStart;
    pSocket->SocketProtocol.pfnConnect = EslSocketConnect;
    pSocket->SocketProtocol.pfnGetLocal = EslSocketGetLocalAddress;
    pSocket->SocketProtocol.pfnGetPeer = EslSocketGetPeerAddress;
    pSocket->SocketProtocol.pfnListen = EslSocketListen;
    pSocket->SocketProtocol.pfnOptionGet = EslSocketOptionGet;
    pSocket->SocketProtocol.pfnOptionSet = EslSocketOptionSet;
    pSocket->SocketProtocol.pfnPoll = EslSocketPoll;
    pSocket->SocketProtocol.pfnReceive = EslSocketReceive;
    pSocket->SocketProtocol.pfnShutdown = EslSocketShutdown;
    pSocket->SocketProtocol.pfnSocket = EslSocket;
    pSocket->SocketProtocol.pfnTransmit = EslSocketTransmit;

    pSocket->MaxRxBuf = MAX_RX_DATA;
    pSocket->MaxTxBuf = MAX_TX_DATA;

    //
    //  Install the socket protocol on the specified handle
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    pChildHandle,
                    &gEfiSocketProtocolGuid,
                    &pSocket->SocketProtocol,
                    NULL
                    );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                "Installed: gEfiSocketProtocolGuid on   0x%08x\r\n",
                *pChildHandle ));
      pSocket->SocketProtocol.SocketHandle = *pChildHandle;

      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Add this socket to the list
      //
      pLayer = &mEslLayer;
      pSocket->pNext = pLayer->pSocketList;
      pLayer->pSocketList = pSocket;

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );

      //
      //  Return the socket structure address
      //
      *ppSocket = pSocket;
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags | DEBUG_POOL | DEBUG_INIT,
                "ERROR - Failed to install gEfiSocketProtocolGuid on 0x%08x, Status: %r\r\n",
                *pChildHandle,
                Status ));
    }

    //
    //  Release the socket if necessary
    //
    if ( EFI_ERROR ( Status )) {
      gBS->FreePool ( pSocket );
      DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT,
                "0x%08x: Free pSocket, %d bytes\r\n",
                pSocket,
                sizeof ( *pSocket )));
      pSocket = NULL;
    }
  }
  else {
    Status = EFI_OUT_OF_RESOURCES;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Bind a name to a socket.

  This routine calls the network specific layer to save the network
  address of the local connection point.

  The ::bind routine calls this routine to connect a name
  (network address and port) to a socket on the local machine.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]  pSockAddr       Address of a sockaddr structure that contains the
                              connection point on the local machine.  An IPv4 address
                              of INADDR_ANY specifies that the connection is made to
                              all of the network stacks on the platform.  Specifying a
                              specific IPv4 address restricts the connection to the
                              network stack supporting that address.  Specifying zero
                              for the port causes the network layer to assign a port
                              number from the dynamic range.  Specifying a specific
                              port number causes the network layer to use that port.
  @param[in]  SockAddrLength  Specifies the length in bytes of the sockaddr structure.
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created
**/
EFI_STATUS
EslSocketBind (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN CONST struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength,
  OUT int * pErrno
  )
{
  EFI_HANDLE ChildHandle;
  UINT8 * pBuffer;
  ESL_PORT * pPort;
  ESL_SERVICE ** ppServiceListHead;
  ESL_SOCKET * pSocket;
  ESL_SERVICE * pService;
  EFI_SERVICE_BINDING_PROTOCOL * pServiceBinding;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Validate the structure pointer
    //
    pSocket->errno = 0;
    if ( NULL == pSockAddr ) {
      DEBUG (( DEBUG_BIND,
                "ERROR - pSockAddr is NULL!\r\n" ));
      Status = EFI_INVALID_PARAMETER;
      pSocket->errno = EFAULT;
    }

    //
    //  Validate the local address length
    //
    else if ( SockAddrLength < pSocket->pApi->MinimumAddressLength ) {
      DEBUG (( DEBUG_BIND,
                "ERROR - Invalid bind name length: %d\r\n",
                SockAddrLength ));
      Status = EFI_INVALID_PARAMETER;
      pSocket->errno = EINVAL;
    }

    //
    //  Validate the shutdown state
    //
    else if ( pSocket->bRxDisable || pSocket->bTxDisable ) {
      DEBUG (( DEBUG_BIND,
                "ERROR - Shutdown has been called on socket 0x%08x\r\n",
                pSocket ));
      pSocket->errno = EINVAL;
      Status = EFI_INVALID_PARAMETER;
    }

    //
    //  Verify the socket state
    //
    else if ( SOCKET_STATE_NOT_CONFIGURED != pSocket->State ) {
      DEBUG (( DEBUG_BIND,
                "ERROR - The socket 0x%08x is already configured!\r\n",
                pSocket ));
      pSocket->errno = EINVAL;
      Status = EFI_ALREADY_STARTED;
    }
    else {
      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Assume no ports are available
      //
      pSocket->errno = EADDRNOTAVAIL;
      Status = EFI_INVALID_PARAMETER;

      //
      //  Walk the list of services
      //
      pBuffer = (UINT8 *)&mEslLayer;
      pBuffer = &pBuffer[ pSocket->pApi->ServiceListOffset ];
      ppServiceListHead = (ESL_SERVICE **)pBuffer;
      pService = *ppServiceListHead;
      while ( NULL != pService ) {
        //
        //  Create the port
        //
        pServiceBinding = pService->pServiceBinding;
        ChildHandle = NULL;
        Status = pServiceBinding->CreateChild ( pServiceBinding,
                                                &ChildHandle );
        if ( !EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_BIND | DEBUG_POOL,
                    "0x%08x: %s port handle created\r\n",
                    ChildHandle,
                    pService->pSocketBinding->pName ));

          //
          //  Open the port
          //
          Status = EslSocketPortAllocate ( pSocket,
                                           pService,
                                           ChildHandle,
                                           pSockAddr,
                                           TRUE,
                                           DEBUG_BIND,
                                           &pPort );
        }
        else {
          DEBUG (( DEBUG_BIND | DEBUG_POOL,
                    "ERROR - Failed to open %s port handle, Status: %r\r\n",
                    pService->pSocketBinding->pName,
                    Status ));
        }

        //
        //  Set the next service
        //
        pService = pService->pNext;
      }

      //
      //  Verify that at least one network connection was found
      //
      if ( NULL != pSocket->pPortList ) {
        Status = EFI_SUCCESS;
      }
      else {
        if ( EADDRNOTAVAIL == pSocket->errno ) {
          DEBUG (( DEBUG_BIND | DEBUG_POOL | DEBUG_INIT,
                    "ERROR - Socket address is not available!\r\n" ));
        }
        if ( EADDRINUSE == pSocket->errno ) {
          DEBUG (( DEBUG_BIND | DEBUG_POOL | DEBUG_INIT,
                    "ERROR - Socket address is in use!\r\n" ));
        }
        Status = EFI_INVALID_PARAMETER;
      }

      //
      //  Mark this socket as bound if successful
      //
      if ( !EFI_ERROR ( Status )) {
        pSocket->State = SOCKET_STATE_BOUND;
        pSocket->errno = 0;
      }

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Test the bind configuration.

  @param[in] pPort        Address of the ::ESL_PORT structure.
  @param[in] ErrnoValue   errno value if test fails

  @retval EFI_SUCCESS   The connection was successfully established.
  @retval Others        The connection attempt failed.
**/
EFI_STATUS
EslSocketBindTest (
  IN ESL_PORT * pPort,
  IN int ErrnoValue
  )
{
  UINT8 * pBuffer;
  VOID * pConfigData;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Locate the configuration data
  //
  pBuffer = (UINT8 *)pPort;
  pBuffer = &pBuffer [ pPort->pSocket->pApi->ConfigDataOffset ];
  pConfigData = (VOID *)pBuffer;

  //
  //  Validate that the port is connected
  //
  Status = pPort->pSocket->pApi->pfnVerifyLocalIpAddress ( pPort, pBuffer );
  if ( EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_WARN | DEBUG_BIND,
              "WARNING - Port 0x%08x invalid IP address: %r\r\n",
              pPort,
              Status ));
    pPort->pSocket->errno = ErrnoValue;
  }
  else {
    //
    //  Attempt to use this configuration
    //
    Status = pPort->pfnConfigure ( pPort->pProtocol.v, pConfigData );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_WARN | DEBUG_BIND,
                "WARNING - Port 0x%08x failed configuration, Status: %r\r\n",
                pPort,
                Status ));
      pPort->pSocket->errno = ErrnoValue;
    }
    else {
      //
      //  Reset the port
      //
      Status = pPort->pfnConfigure ( pPort->pProtocol.v, NULL );
      if ( EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_ERROR | DEBUG_BIND,
                  "ERROR - Port 0x%08x failed configuration reset, Status: %r\r\n",
                  pPort,
                  Status ));
        ASSERT ( EFI_SUCCESS == Status );
      }
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Determine if the socket is closed.

  This routine checks the state of the socket to determine if
  the network specific layer has completed the close operation.

  The ::close routine polls this routine to determine when the
  close operation is complete.  The close operation needs to
  reverse the operations of the ::EslSocketAllocate routine.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     Socket successfully closed
  @retval EFI_NOT_READY   Close still in progress
  @retval EFI_ALREADY     Close operation already in progress
  @retval Other           Failed to close the socket
**/
EFI_STATUS
EslSocketClosePoll (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  )
{
  int errno;
  ESL_LAYER * pLayer;
  ESL_SOCKET * pNextSocket;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  errno = 0;
  Status = EFI_SUCCESS;

  //
  //  Synchronize with the socket layer
  //
  RAISE_TPL ( TplPrevious, TPL_SOCKETS );

  //
  //  Locate the socket
  //
  pLayer = &mEslLayer;
  pNextSocket = pLayer->pSocketList;
  pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
  while ( NULL != pNextSocket ) {
    if ( pNextSocket == pSocket ) {
      //
      //  Determine if the socket is in the closing state
      //
      if ( SOCKET_STATE_CLOSED == pSocket->State ) {
        //
        //  Walk the list of ports
        //
        if ( NULL == pSocket->pPortList ) {
          //
          //  All the ports are closed
          //  Close the WaitAccept event if necessary
          //
          if ( NULL != pSocket->WaitAccept ) {
            Status = gBS->CloseEvent ( pSocket->WaitAccept );
            if ( !EFI_ERROR ( Status )) {
              DEBUG (( DEBUG_SOCKET | DEBUG_CLOSE | DEBUG_POOL,
                        "0x%08x: Closed WaitAccept event\r\n",
                        pSocket->WaitAccept ));
              //
              //  Return the transmit status
              //
              Status = pSocket->TxError;
              if ( EFI_ERROR ( Status )) {
                pSocket->errno = EIO;
              }
            }
            else {
              DEBUG (( DEBUG_ERROR | DEBUG_SOCKET | DEBUG_CLOSE | DEBUG_POOL,
                        "ERROR - Failed to close the WaitAccept event, Status: %r\r\n",
                        Status ));
              ASSERT ( EFI_SUCCESS == Status );
            }
          }
        }
        else {
          //
          //  At least one port is still open
          //
          Status = EFI_NOT_READY;
          errno = EAGAIN;
        }
      }
      else {
        //
        //  SocketCloseStart was not called
        //
        Status = EFI_NOT_STARTED;
        errno = EPERM;
      }
      break;
    }

    //
    //  Set the next socket
    //
    pNextSocket = pNextSocket->pNext;
  }

  //
  //  Handle the error case where the socket was already closed
  //
  if ( NULL == pSocket ) {
    //
    //  Socket not found
    //
    Status = EFI_NOT_FOUND;
    errno = ENOTSOCK;
  }

  //
  //  Release the socket layer synchronization
  //
  RESTORE_TPL ( TplPrevious );

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Start the close operation on the socket.

  This routine calls the network specific layer to initiate the
  close state machine.  This routine then calls the network
  specific layer to determine if the close state machine has gone
  to completion.  The result from this poll is returned to the
  caller.

  The ::close routine calls this routine to start the close
  operation which reverses the operations of the
  ::EslSocketAllocate routine.  The close routine then polls
  the ::EslSocketClosePoll routine to determine when the
  socket is closed.

  @param[in] pSocketProtocol  Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in] bCloseNow        Boolean to control close behavior
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     Socket successfully closed
  @retval EFI_NOT_READY   Close still in progress
  @retval EFI_ALREADY     Close operation already in progress
  @retval Other           Failed to close the socket
**/
EFI_STATUS
EslSocketCloseStart (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN BOOLEAN bCloseNow,
  IN int * pErrno
  )
{
  int errno;
  ESL_PORT * pNextPort;
  ESL_PORT * pPort;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;
  errno = 0;

  //
  //  Synchronize with the socket layer
  //
  RAISE_TPL ( TplPrevious, TPL_SOCKETS );

  //
  //  Determine if the socket is already closed
  //
  pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
  if ( SOCKET_STATE_CLOSED > pSocket->State ) {
    //
    //  Update the socket state
    //
    pSocket->State = SOCKET_STATE_CLOSED;

    //
    //  Walk the list of ports
    //
    pPort = pSocket->pPortList;
    while ( NULL != pPort ) {
      //
      //  Start closing the ports
      //
      pNextPort = pPort->pLinkSocket;
      Status = EslSocketPortCloseStart ( pPort,
                                         bCloseNow,
                                         DEBUG_CLOSE | DEBUG_LISTEN | DEBUG_CONNECTION );
      if (( EFI_SUCCESS != Status )
        && ( EFI_NOT_READY != Status )) {
        errno = EIO;
        break;
      }

      //
      //  Set the next port
      //
      pPort = pNextPort;
    }

    //
    //  Attempt to finish closing the socket
    //
    if ( NULL == pPort ) {
      Status = EslSocketClosePoll ( pSocketProtocol, &errno );
    }
  }
  else {
    Status = EFI_NOT_READY;
    errno = EAGAIN;
  }

  //
  //  Release the socket layer synchronization
  //
  RESTORE_TPL ( TplPrevious );

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Connect to a remote system via the network.

  This routine calls the network specific layer to establish
  the remote system address and establish the connection to
  the remote system.

  The ::connect routine calls this routine to establish a
  connection with the specified remote system.  This routine
  is designed to be polled by the connect routine for completion
  of the network connection.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]  pSockAddr       Network address of the remote system.
  @param[in]  SockAddrLength  Length in bytes of the network address.
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     The connection was successfully established.
  @retval EFI_NOT_READY   The connection is in progress, call this routine again.
  @retval Others          The connection attempt failed.
 **/
EFI_STATUS
EslSocketConnect (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength,
  IN int * pErrno
  )
{
  struct sockaddr_in6 LocalAddress;
  ESL_PORT * pPort;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DEBUG (( DEBUG_CONNECT, "Entering SocketConnect\r\n" ));

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Validate the name length
    //
    if ( SockAddrLength < ( sizeof ( struct sockaddr ) - sizeof ( pSockAddr->sa_data ))) {
      DEBUG (( DEBUG_CONNECT,
                "ERROR - Invalid bind name length: %d\r\n",
                SockAddrLength ));
      Status = EFI_INVALID_PARAMETER;
      pSocket->errno = EINVAL;
    }
    else {
      //
      //  Assume success
      //
      pSocket->errno = 0;

      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Validate the socket state
      //
      switch ( pSocket->State ) {
      default:
        //
        //  Wrong socket state
        //
        pSocket->errno = EIO;
        Status = EFI_DEVICE_ERROR;
        break;

      case SOCKET_STATE_NOT_CONFIGURED:
      case SOCKET_STATE_BOUND:
        //
        //  Validate the address length
        //
        if ( SockAddrLength >= pSocket->pApi->MinimumAddressLength ) {
          //
          //  Verify the API
          //
          if ( NULL == pSocket->pApi->pfnRemoteAddrSet ) {
            //
            //  Already connected
            //
            pSocket->errno = ENOTSUP;
            Status = EFI_UNSUPPORTED;
          }
          else {
            //
            //  Determine if BIND was already called
            //
            if ( NULL == pSocket->pPortList ) {
              //
              //  Allow any local port
              //
              ZeroMem ( &LocalAddress, sizeof ( LocalAddress ));
              LocalAddress.sin6_len = (uint8_t)pSocket->pApi->MinimumAddressLength;
              LocalAddress.sin6_family = pSocket->pApi->AddressFamily;
              Status = EslSocketBind ( &pSocket->SocketProtocol,
                                       (struct sockaddr *)&LocalAddress,
                                       LocalAddress.sin6_len,
                                       &pSocket->errno );
            }
            if ( NULL != pSocket->pPortList ) {
              //
              //  Walk the list of ports
              //
              pPort = pSocket->pPortList;
              while ( NULL != pPort ) {
                //
                //  Set the remote address
                //
                Status = pSocket->pApi->pfnRemoteAddrSet ( pPort,
                                                           pSockAddr,
                                                           SockAddrLength );
                if ( EFI_ERROR ( Status )) {
                  break;
                }

                //
                //  Set the next port
                //
                pPort = pPort->pLinkSocket;
              }

              //
              //  Verify the API
              //
              if (( !EFI_ERROR ( Status ))
                && ( NULL != pSocket->pApi->pfnConnectStart )) {
                //
                //  Initiate the connection with the remote system
                //
                Status = pSocket->pApi->pfnConnectStart ( pSocket );

                //
                //  Set the next state if connecting
                //
                if ( EFI_NOT_READY == Status ) {
                  pSocket->State = SOCKET_STATE_CONNECTING;
                }
              }
            }
          }
        }
        else {
          DEBUG (( DEBUG_CONNECT,
                    "ERROR - Invalid address length: %d\r\n",
                    SockAddrLength ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EINVAL;
        }
        break;

      case SOCKET_STATE_CONNECTING:
        //
        //  Poll the network adapter
        //
        EslSocketRxPoll ( pSocket );

        //
        //  Poll for connection completion
        //
        if ( NULL == pSocket->pApi->pfnConnectPoll ) {
          //
          //  Already connected
          //
          pSocket->errno = EISCONN;
          Status = EFI_ALREADY_STARTED;
        }
        else {
          Status = pSocket->pApi->pfnConnectPoll ( pSocket );

          //
          //  Set the next state if connected
          //
          if ( EFI_NOT_READY != Status ) {
            if ( EFI_ERROR ( Status )) {
              pSocket->State = SOCKET_STATE_BOUND;
            }
          }
        }
        break;

      case SOCKET_STATE_CONNECTED:
        //
        //  Connected
        //
        Status = EFI_SUCCESS;
        break;
      }

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      //
      //  Bad socket protocol
      //
      DEBUG (( DEBUG_ERROR | DEBUG_CONNECT,
                "ERROR - pSocketProtocol invalid!\r\n" ));
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }

  //
  //  Return the operation status
  //
  DEBUG (( DEBUG_CONNECT, "Exiting SocketConnect, Status: %r\r\n", Status ));
  return Status;
}


/** Copy a fragmented buffer into a destination buffer.

  This support routine copies a fragmented buffer to the caller specified buffer.

  This routine is called by ::EslIp4Receive and ::EslUdp4Receive.

  @param[in]  FragmentCount   Number of fragments in the table
  @param[in]  pFragmentTable  Address of an EFI_IP4_FRAGMENT_DATA structure
  @param[in]  BufferLength    Length of the the buffer
  @param[in]  pBuffer         Address of a buffer to receive the data.
  @param[in]  pDataLength     Number of received data bytes in the buffer.

  @return   Returns the address of the next free byte in the buffer.
**/
UINT8 *
EslSocketCopyFragmentedBuffer (
  IN UINT32 FragmentCount,
  IN EFI_IP4_FRAGMENT_DATA * pFragmentTable,
  IN size_t BufferLength,
  IN UINT8 * pBuffer,
  OUT size_t * pDataLength
  )
{
  size_t BytesToCopy;
  UINT32 Fragment;
  UINT8 * pBufferEnd;
  UINT8 * pData;

  DBG_ENTER ( );

  //
  //  Validate the IP and UDP structures are identical
  //
  ASSERT ( OFFSET_OF ( EFI_IP4_FRAGMENT_DATA, FragmentLength )
           == OFFSET_OF ( EFI_UDP4_FRAGMENT_DATA, FragmentLength ));
  ASSERT ( OFFSET_OF ( EFI_IP4_FRAGMENT_DATA, FragmentBuffer )
           == OFFSET_OF ( EFI_UDP4_FRAGMENT_DATA, FragmentBuffer ));

  //
  //  Copy the received data
  //
  Fragment = 0;
  pBufferEnd = &pBuffer [ BufferLength ];
  while (( pBufferEnd > pBuffer ) && ( FragmentCount > Fragment )) {
    //
    //  Determine the amount of received data
    //
    pData = pFragmentTable[Fragment].FragmentBuffer;
    BytesToCopy = pFragmentTable[Fragment].FragmentLength;
    if (((size_t)( pBufferEnd - pBuffer )) < BytesToCopy ) {
      BytesToCopy = pBufferEnd - pBuffer;
    }

    //
    //  Move the data into the buffer
    //
    DEBUG (( DEBUG_RX,
              "0x%08x --> 0x%08x: Copy data 0x%08x bytes\r\n",
              pData,
              pBuffer,
              BytesToCopy ));
    CopyMem ( pBuffer, pData, BytesToCopy );
    pBuffer += BytesToCopy;
    Fragment += 1;
  }

  //
  //  Return the data length and the buffer address
  //
  *pDataLength = BufferLength - ( pBufferEnd - pBuffer );
  DBG_EXIT_HEX ( pBuffer );
  return pBuffer;
}


/** Free the socket.

  This routine frees the socket structure and handle resources.

  The ::close routine calls EslServiceFreeProtocol which then calls
  this routine to free the socket context structure and close the
  handle.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS   The socket resources were returned successfully.
**/
EFI_STATUS
EslSocketFree (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  )
{
  EFI_HANDLE ChildHandle;
  int errno;
  ESL_LAYER * pLayer;
  ESL_SOCKET * pSocket;
  ESL_SOCKET * pSocketPrevious;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume failure
  //
  errno = EIO;
  pSocket = NULL;
  Status = EFI_INVALID_PARAMETER;

  //
  //  Validate the socket
  //
  pLayer = &mEslLayer;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Synchronize with the socket layer
    //
    RAISE_TPL ( TplPrevious, TPL_SOCKETS );

    //
    //  Walk the socket list
    //
    pSocketPrevious = pLayer->pSocketList;
    if ( NULL != pSocketPrevious ) {
      if ( pSocket == pSocketPrevious ) {
        //
        //  Remove the socket from the head of the list
        //
        pLayer->pSocketList = pSocket->pNext;
      }
      else {
        //
        //  Find the socket in the middle of the list
        //
        while (( NULL != pSocketPrevious )
          && ( pSocket != pSocketPrevious->pNext )) {
          //
          //  Set the next socket
          //
          pSocketPrevious = pSocketPrevious->pNext;
        }
        if ( NULL != pSocketPrevious ) {
          //
          //  Remove the socket from the middle of the list
          //
          pSocketPrevious = pSocket->pNext;
        }
      }
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                "ERROR - Socket list is empty!\r\n" ));
    }

    //
    //  Release the socket layer synchronization
    //
    RESTORE_TPL ( TplPrevious );

    //
    //  Determine if the socket was found
    //
    if ( NULL != pSocketPrevious ) {
      pSocket->pNext = NULL;

      //
      //  Remove the socket protocol
      //
      ChildHandle = pSocket->SocketProtocol.SocketHandle;
      Status = gBS->UninstallMultipleProtocolInterfaces (
                ChildHandle,
                &gEfiSocketProtocolGuid,
                &pSocket->SocketProtocol,
                NULL );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_POOL | DEBUG_INFO,
                    "Removed:   gEfiSocketProtocolGuid from 0x%08x\r\n",
                    ChildHandle ));

        //
        //  Free the socket structure
        //
        Status = gBS->FreePool ( pSocket );
        if ( !EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_POOL,
                    "0x%08x: Free pSocket, %d bytes\r\n",
                    pSocket,
                    sizeof ( *pSocket )));
          errno = 0;
        }
        else {
          DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                    "ERROR - Failed to free pSocket 0x%08x, Status: %r\r\n",
                    pSocket,
                    Status ));
        }
      }
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INFO,
                    "ERROR - Failed to remove gEfiSocketProtocolGuid from 0x%08x, Status: %r\r\n",
                    ChildHandle,
                    Status ));
      }
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_INFO,
                "ERROR - The socket was not in the socket list!\r\n" ));
      Status = EFI_NOT_FOUND;
    }
  }
  else {
    DEBUG (( DEBUG_ERROR,
              "ERROR - Invalid parameter pSocketProtocol is NULL\r\n" ));
  }

  //
  //  Return the errno value if possible
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Get the local address.

  This routine calls the network specific layer to get the network
  address of the local host connection point.

  The ::getsockname routine calls this routine to obtain the network
  address associated with the local host connection point.

  @param[in]      pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[out]     pAddress        Network address to receive the local system address
  @param[in,out]  pAddressLength  Length of the local network address structure
  @param[out]     pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Local address successfully returned
 **/
EFI_STATUS
EslSocketGetLocalAddress (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  )
{
  socklen_t LengthInBytes;
  ESL_PORT * pPort;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Verify the socket state
    //
    EslSocketIsConfigured ( pSocket );
    if ( pSocket->bAddressSet ) {
      //
      //  Verify the address buffer and length address
      //
      if (( NULL != pAddress ) && ( NULL != pAddressLength )) {
        //
        //  Verify the API
        //
        if ( NULL == pSocket->pApi->pfnLocalAddrGet ) {
          Status = EFI_UNSUPPORTED;
          pSocket->errno = ENOTSUP;
        }
        else {
          //
          //  Synchronize with the socket layer
          //
          RAISE_TPL ( TplPrevious, TPL_SOCKETS );

          //
          //  Verify that there is just a single connection
          //
          pPort = pSocket->pPortList;
          if ( NULL != pPort ) {
            //
            //  Verify the address length
            //
            LengthInBytes = pSocket->pApi->AddressLength;
            if (( LengthInBytes <= *pAddressLength )
              && ( 255 >= LengthInBytes )) {
              //
              //  Return the local address and address length
              //
              ZeroMem ( pAddress, LengthInBytes );
              pAddress->sa_len = (uint8_t)LengthInBytes;
              *pAddressLength = pAddress->sa_len;
              pSocket->pApi->pfnLocalAddrGet ( pPort, pAddress );
              pSocket->errno = 0;
              Status = EFI_SUCCESS;
            }
            else {
              pSocket->errno = EINVAL;
              Status = EFI_INVALID_PARAMETER;
            }
          }
          else {
            pSocket->errno = ENOTCONN;
            Status = EFI_NOT_STARTED;
          }

          //
          //  Release the socket layer synchronization
          //
          RESTORE_TPL ( TplPrevious );
        }
      }
      else {
        pSocket->errno = EINVAL;
        Status = EFI_INVALID_PARAMETER;
      }
    }
    else {
      //
      //  Address not set
      //
      Status = EFI_NOT_STARTED;
      pSocket->errno = EADDRNOTAVAIL;
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Get the peer address.

  This routine calls the network specific layer to get the remote
  system connection point.

  The ::getpeername routine calls this routine to obtain the network
  address of the remote connection point.

  @param[in]      pSocketProtocol   Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[out]     pAddress          Network address to receive the remote system address
  @param[in,out]  pAddressLength    Length of the remote network address structure
  @param[out]     pErrno            Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Remote address successfully returned
 **/
EFI_STATUS
EslSocketGetPeerAddress (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  )
{
  socklen_t LengthInBytes;
  ESL_PORT * pPort;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Verify the socket state
    //
    Status = EslSocketIsConfigured ( pSocket );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Verify the API
      //
      if ( NULL == pSocket->pApi->pfnRemoteAddrGet ) {
        Status = EFI_UNSUPPORTED;
        pSocket->errno = ENOTSUP;
      }
      else {
        //
        //  Verify the address buffer and length address
        //
        if (( NULL != pAddress ) && ( NULL != pAddressLength )) {
          //
          //  Verify the socket state
          //
          if ( SOCKET_STATE_CONNECTED == pSocket->State ) {
            //
            //  Synchronize with the socket layer
            //
            RAISE_TPL ( TplPrevious, TPL_SOCKETS );

            //
            //  Verify that there is just a single connection
            //
            pPort = pSocket->pPortList;
            if (( NULL != pPort ) && ( NULL == pPort->pLinkSocket )) {
              //
              //  Verify the address length
              //
              LengthInBytes = pSocket->pApi->AddressLength;
              if ( LengthInBytes <= *pAddressLength ) {
                //
                //  Return the local address
                //
                ZeroMem ( pAddress, LengthInBytes );
                pAddress->sa_len = (uint8_t)LengthInBytes;
                *pAddressLength = pAddress->sa_len;
                pSocket->pApi->pfnRemoteAddrGet ( pPort, pAddress );
                pSocket->errno = 0;
                Status = EFI_SUCCESS;
              }
              else {
                pSocket->errno = EINVAL;
                Status = EFI_INVALID_PARAMETER;
              }
            }
            else {
              pSocket->errno = ENOTCONN;
              Status = EFI_NOT_STARTED;
            }

            //
            //  Release the socket layer synchronization
            //
            RESTORE_TPL ( TplPrevious );
          }
          else {
            pSocket->errno = ENOTCONN;
            Status = EFI_NOT_STARTED;
          }
        }
        else {
          pSocket->errno = EINVAL;
          Status = EFI_INVALID_PARAMETER;
        }
      }
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Free the ESL_IO_MGMT event and structure.

  This support routine walks the free list to close the event in
  the ESL_IO_MGMT structure and remove the structure from the free
  list.

  See the \ref TransmitEngine section.

  @param[in]  pPort         Address of an ::ESL_PORT structure
  @param[in]  ppFreeQueue   Address of the free queue head
  @param[in]  DebugFlags    Flags for debug messages
  @param[in]  pEventName    Zero terminated string containing the event name

  @retval EFI_SUCCESS - The structures were properly initialized
**/
EFI_STATUS
EslSocketIoFree (
  IN ESL_PORT * pPort,
  IN ESL_IO_MGMT ** ppFreeQueue,
  IN UINTN DebugFlags,
  IN CHAR8 * pEventName
  )
{
  UINT8 * pBuffer;
  EFI_EVENT * pEvent;
  ESL_IO_MGMT * pIo;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Walk the list of IO structures
  //
  pSocket = pPort->pSocket;
  while ( *ppFreeQueue ) {
    //
    //  Free the event for this structure
    //
    pIo = *ppFreeQueue;
    pBuffer = (UINT8 *)pIo;
    pBuffer = &pBuffer[ pSocket->TxTokenEventOffset ];
    pEvent = (EFI_EVENT *)pBuffer;
    Status = gBS->CloseEvent ( *pEvent );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close the %a event, Status: %r\r\n",
                pEventName,
                Status ));
      pSocket->errno = ENOMEM;
      break;
    }
    DEBUG (( DebugFlags,
              "0x%08x: Closed %a event 0x%08x\r\n",
              pIo,
              pEventName,
              *pEvent ));

    //
    //  Remove this structure from the queue
    //
    *ppFreeQueue = pIo->pNext;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Initialize the ESL_IO_MGMT structures.

  This support routine initializes the ESL_IO_MGMT structure and
  places them on to a free list.

  This routine is called by ::EslSocketPortAllocate routines to prepare
  the transmit engines.  See the \ref TransmitEngine section.

  @param[in]        pPort         Address of an ::ESL_PORT structure
  @param[in, out]   ppIo          Address containing the first structure address.  Upon
                                  return this buffer contains the next structure address.
  @param[in]        TokenCount    Number of structures to initialize
  @param[in]        ppFreeQueue   Address of the free queue head
  @param[in]        DebugFlags    Flags for debug messages
  @param[in]        pEventName    Zero terminated string containing the event name
  @param[in]        pfnCompletion Completion routine address

  @retval EFI_SUCCESS - The structures were properly initialized
**/
EFI_STATUS
EslSocketIoInit (
  IN ESL_PORT * pPort,
  IN ESL_IO_MGMT ** ppIo,
  IN UINTN TokenCount,
  IN ESL_IO_MGMT ** ppFreeQueue,
  IN UINTN DebugFlags,
  IN CHAR8 * pEventName,
  IN PFN_API_IO_COMPLETE pfnCompletion
  )
{
  ESL_IO_MGMT * pEnd;
  EFI_EVENT * pEvent;
  ESL_IO_MGMT * pIo;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Walk the list of IO structures
  //
  pSocket = pPort->pSocket;
  pIo = *ppIo;
  pEnd = &pIo [ TokenCount ];
  while ( pEnd > pIo ) {
    //
    //  Initialize the IO structure
    //
    pIo->pPort = pPort;
    pIo->pPacket = NULL;

    //
    //  Allocate the event for this structure
    //
    pEvent = (EFI_EVENT *)&(((UINT8 *)pIo)[ pSocket->TxTokenEventOffset ]);
    Status = gBS->CreateEvent ( EVT_NOTIFY_SIGNAL,
                                TPL_SOCKETS,
                                (EFI_EVENT_NOTIFY)pfnCompletion,
                                pIo,
                                pEvent );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to create the %a event, Status: %r\r\n",
                pEventName,
                Status ));
      pSocket->errno = ENOMEM;
      break;
    }
    DEBUG (( DebugFlags,
              "0x%08x: Created %a event 0x%08x\r\n",
              pIo,
              pEventName,
              *pEvent ));

    //
    //  Add this structure to the queue
    //
    pIo->pNext = *ppFreeQueue;
    *ppFreeQueue = pIo;

    //
    //  Set the next structure
    //
    pIo += 1;
  }

  //
  //  Save the next structure
  //
  *ppIo = pIo;

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Determine if the socket is configured.

  This support routine is called to determine if the socket if the
  configuration call was made to the network layer.  The following
  routines call this routine to verify that they may be successful
  in their operations:
  <ul>
    <li>::EslSocketGetLocalAddress</li>
    <li>::EslSocketGetPeerAddress</li>
    <li>::EslSocketPoll</li>
    <li>::EslSocketReceive</li>
    <li>::EslSocketTransmit</li>
  </ul>

  @param[in]  pSocket       Address of an ::ESL_SOCKET structure

  @retval EFI_SUCCESS - The socket is configured
**/
EFI_STATUS
EslSocketIsConfigured (
  IN ESL_SOCKET * pSocket
  )
{
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Verify the socket state
  //
  if ( !pSocket->bConfigured ) {
    DBG_ENTER ( );

    //
    //  Verify the API
    //
    if ( NULL == pSocket->pApi->pfnIsConfigured ) {
      Status = EFI_UNSUPPORTED;
      pSocket->errno = ENOTSUP;
    }
    else {
      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Determine if the socket is configured
      //
      Status = pSocket->pApi->pfnIsConfigured ( pSocket );

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );

      //
      //  Set errno if a failure occurs
      //
      if ( EFI_ERROR ( Status )) {
        pSocket->errno = EADDRNOTAVAIL;
      }
    }

    DBG_EXIT_STATUS ( Status );
  }

  //
  //  Return the configuration status
  //
  return Status;
}


/** Establish the known port to listen for network connections.

  This routine calls into the network protocol layer to establish
  a handler that is called upon connection completion.  The handler
  is responsible for inserting the connection into the FIFO.

  The ::listen routine indirectly calls this routine to place the
  socket into a state that enables connection attempts.  Connections
  are placed in a FIFO that is serviced by the application.  The
  application calls the ::accept (::EslSocketAccept) routine to
  remove the next connection from the FIFO and get the associated
  socket and address.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]  Backlog         Backlog specifies the maximum FIFO depth for
                              the connections waiting for the application
                              to call accept.  Connection attempts received
                              while the queue is full are refused.
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created
  @retval Other - Failed to enable the socket for listen
**/
EFI_STATUS
EslSocketListen (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN INT32 Backlog,
  OUT int * pErrno
  )
{
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_STATUS TempStatus;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Verify the API
    //
    if ( NULL == pSocket->pApi->pfnListen ) {
      Status = EFI_UNSUPPORTED;
      pSocket->errno = ENOTSUP;
    }
    else {
      //
      //  Assume success
      //
      pSocket->Status = EFI_SUCCESS;
      pSocket->errno = 0;

      //
      //  Verify that the bind operation was successful
      //
      if ( SOCKET_STATE_BOUND == pSocket->State ) {
        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Create the event for SocketAccept completion
        //
        Status = gBS->CreateEvent ( 0,
                                    TPL_SOCKETS,
                                    NULL,
                                    NULL,
                                    &pSocket->WaitAccept );
        if ( !EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_POOL,
                    "0x%08x: Created WaitAccept event\r\n",
                    pSocket->WaitAccept ));
          //
          //  Set the maximum FIFO depth
          //
          if ( 0 >= Backlog ) {
            Backlog = MAX_PENDING_CONNECTIONS;
          }
          else {
            if ( SOMAXCONN < Backlog ) {
              Backlog = SOMAXCONN;
            }
            else {
              pSocket->MaxFifoDepth = Backlog;
            }
          }

          //
          //  Initiate the connection attempt listen
          //
          Status = pSocket->pApi->pfnListen ( pSocket );

          //
          //  Place the socket in the listen state if successful
          //
          if ( !EFI_ERROR ( Status )) {
            pSocket->State = SOCKET_STATE_LISTENING;
            pSocket->bListenCalled = TRUE;
          }
          else {
            //
            //  Not waiting for SocketAccept to complete
            //
            TempStatus = gBS->CloseEvent ( pSocket->WaitAccept );
            if ( !EFI_ERROR ( TempStatus )) {
              DEBUG (( DEBUG_POOL,
                        "0x%08x: Closed WaitAccept event\r\n",
                        pSocket->WaitAccept ));
              pSocket->WaitAccept = NULL;
            }
            else {
              DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                        "ERROR - Failed to close WaitAccept event, Status: %r\r\n",
                        TempStatus ));
              ASSERT ( EFI_SUCCESS == TempStatus );
            }
          }
        }
        else {
          DEBUG (( DEBUG_ERROR | DEBUG_LISTEN,
                    "ERROR - Failed to create the WaitAccept event, Status: %r\r\n",
                    Status ));
          pSocket->errno = ENOMEM;
        }

        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );
      }
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_LISTEN,
                  "ERROR - Bind operation must be performed first!\r\n" ));
        pSocket->errno = ( SOCKET_STATE_NOT_CONFIGURED == pSocket->State ) ? EDESTADDRREQ
                                                                           : EINVAL;
        Status = EFI_NO_MAPPING;
      }
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Get the socket options.

  This routine handles the socket level options and passes the
  others to the network specific layer.

  The ::getsockopt routine calls this routine to retrieve the
  socket options one at a time by name.

  @param[in]      pSocketProtocol   Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]      level             Option protocol level
  @param[in]      OptionName        Name of the option
  @param[out]     pOptionValue      Buffer to receive the option value
  @param[in,out]  pOptionLength     Length of the buffer in bytes,
                                    upon return length of the option value in bytes
  @param[out]     pErrno            Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received
 **/
EFI_STATUS
EslSocketOptionGet (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int level,
  IN int OptionName,
  OUT void * __restrict pOptionValue,
  IN OUT socklen_t * __restrict pOptionLength,
  IN int * pErrno
  )
{
  int errno;
  socklen_t LengthInBytes;
  socklen_t MaxBytes;
  CONST UINT8 * pOptionData;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume failure
  //
  errno = EINVAL;
  Status = EFI_INVALID_PARAMETER;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL == pSocketProtocol ) {
    DEBUG (( DEBUG_OPTION, "ERROR - pSocketProtocol is NULL!\r\n" ));
  }
  else if ( NULL == pOptionValue ) {
    DEBUG (( DEBUG_OPTION, "ERROR - No option buffer specified\r\n" ));
  }
  else if ( NULL == pOptionLength ) {
    DEBUG (( DEBUG_OPTION, "ERROR - Option length not specified!\r\n" ));
  }
  else {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
    LengthInBytes = 0;
    MaxBytes = *pOptionLength;
    pOptionData = NULL;
    switch ( level ) {
    default:
      //
      //  See if the protocol will handle the option
      //
      if ( NULL != pSocket->pApi->pfnOptionGet ) {
        if ( pSocket->pApi->DefaultProtocol == level ) {
          Status = pSocket->pApi->pfnOptionGet ( pSocket,
                                                 OptionName,
                                                 (CONST void ** __restrict)&pOptionData,
                                                 &LengthInBytes );
          errno = pSocket->errno;
          break;
        }
        else {
          //
          //  Protocol not supported
          //
          DEBUG (( DEBUG_OPTION,
                    "ERROR - The socket does not support this protocol!\r\n" ));
        }
      }
      else {
        //
        //  Protocol level not supported
        //
        DEBUG (( DEBUG_OPTION,
                  "ERROR - %a does not support any options!\r\n",
                  pSocket->pApi->pName ));
      }
      errno = ENOPROTOOPT;
      Status = EFI_INVALID_PARAMETER;
      break;

    case SOL_SOCKET:
      switch ( OptionName ) {
      default:
        //
        //  Socket option not supported
        //
        DEBUG (( DEBUG_INFO | DEBUG_OPTION, "ERROR - Invalid socket option!\r\n" ));
        errno = EINVAL;
        Status = EFI_INVALID_PARAMETER;
        break;

      case SO_ACCEPTCONN:
        //
        //  Return the listen flag
        //
        pOptionData = (CONST UINT8 *)&pSocket->bListenCalled;
        LengthInBytes = sizeof ( pSocket->bListenCalled );
        break;

      case SO_DEBUG:
        //
        //  Return the debug flags
        //
        pOptionData = (CONST UINT8 *)&pSocket->bOobInLine;
        LengthInBytes = sizeof ( pSocket->bOobInLine );
        break;

      case SO_OOBINLINE:
        //
        //  Return the out-of-band inline flag
        //
        pOptionData = (CONST UINT8 *)&pSocket->bOobInLine;
        LengthInBytes = sizeof ( pSocket->bOobInLine );
        break;

      case SO_RCVTIMEO:
        //
        //  Return the receive timeout
        //
        pOptionData = (CONST UINT8 *)&pSocket->RxTimeout;
        LengthInBytes = sizeof ( pSocket->RxTimeout );
        break;

      case SO_RCVBUF:
        //
        //  Return the maximum receive buffer size
        //
        pOptionData = (CONST UINT8 *)&pSocket->MaxRxBuf;
        LengthInBytes = sizeof ( pSocket->MaxRxBuf );
        break;

      case SO_REUSEADDR:
        //
        //  Return the address reuse flag
        //
        pOptionData = (UINT8 *)&pSocket->bReUseAddr;
        LengthInBytes = sizeof ( pSocket->bReUseAddr );
        break;

      case SO_SNDBUF:
        //
        //  Return the maximum transmit buffer size
        //
        pOptionData = (CONST UINT8 *)&pSocket->MaxTxBuf;
        LengthInBytes = sizeof ( pSocket->MaxTxBuf );
        break;

      case SO_TYPE:
        //
        //  Return the socket type
        //
        pOptionData = (CONST UINT8 *)&pSocket->Type;
        LengthInBytes = sizeof ( pSocket->Type );
        break;
      }
      break;
    }

    //
    //  Return the option length
    //
    *pOptionLength = LengthInBytes;

    //
    //  Determine if the option is present
    //
    if ( 0 != LengthInBytes ) {
      //
      //  Silently truncate the value length
      //
      if ( LengthInBytes > MaxBytes ) {
        DEBUG (( DEBUG_OPTION,
                  "INFO - Truncating option from %d to %d bytes\r\n",
                  LengthInBytes,
                  MaxBytes ));
        LengthInBytes = MaxBytes;
      }

      //
      //  Return the value
      //
      CopyMem ( pOptionValue, pOptionData, LengthInBytes );

      //
      //  Zero fill any remaining space
      //
      if ( LengthInBytes < MaxBytes ) {
        ZeroMem ( &((UINT8 *)pOptionValue)[LengthInBytes], MaxBytes - LengthInBytes );
      }
      errno = 0;
      Status = EFI_SUCCESS;
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Set the socket options.

  This routine handles the socket level options and passes the
  others to the network specific layer.

  The ::setsockopt routine calls this routine to adjust the socket
  options one at a time by name.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]  level           Option protocol level
  @param[in]  OptionName      Name of the option
  @param[in]  pOptionValue    Buffer containing the option value
  @param[in]  OptionLength    Length of the buffer in bytes
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Option successfully set
**/
EFI_STATUS
EslSocketOptionSet (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int level,
  IN int OptionName,
  IN CONST void * pOptionValue,
  IN socklen_t OptionLength,
  IN int * pErrno
  )
{
  BOOLEAN bTrueFalse;
  int errno;
  socklen_t LengthInBytes;
  UINT8 * pOptionData;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume failure
  //
  errno = EINVAL;
  Status = EFI_INVALID_PARAMETER;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL == pSocketProtocol ) {
    DEBUG (( DEBUG_OPTION, "ERROR - pSocketProtocol is NULL!\r\n" ));
  }
  else if ( NULL == pOptionValue ) {
    DEBUG (( DEBUG_OPTION, "ERROR - No option buffer specified\r\n" ));
  }
  else
  {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
    if ( pSocket->bRxDisable || pSocket->bTxDisable ) {
      DEBUG (( DEBUG_OPTION, "ERROR - Socket has been shutdown!\r\n" ));
    }
    else {
      LengthInBytes = 0;
      pOptionData = NULL;
      switch ( level ) {
      default:
        //
        //  See if the protocol will handle the option
        //
        if ( NULL != pSocket->pApi->pfnOptionSet ) {
          if ( pSocket->pApi->DefaultProtocol == level ) {
            Status = pSocket->pApi->pfnOptionSet ( pSocket,
                                                   OptionName,
                                                   pOptionValue,
                                                   OptionLength );
            errno = pSocket->errno;
            break;
          }
          else {
            //
            //  Protocol not supported
            //
            DEBUG (( DEBUG_OPTION,
                      "ERROR - The socket does not support this protocol!\r\n" ));
          }
        }
        else {
          //
          //  Protocol level not supported
          //
          DEBUG (( DEBUG_OPTION,
                    "ERROR - %a does not support any options!\r\n",
                    pSocket->pApi->pName ));
        }
        errno = ENOPROTOOPT;
        Status = EFI_INVALID_PARAMETER;
        break;

      case SOL_SOCKET:
        switch ( OptionName ) {
        default:
          //
          //  Option not supported
          //
          DEBUG (( DEBUG_OPTION,
                    "ERROR - Sockets does not support this option!\r\n" ));
          errno = EINVAL;
          Status = EFI_INVALID_PARAMETER;
          break;

        case SO_DEBUG:
          //
          //  Set the debug flags
          //
          pOptionData = (UINT8 *)&pSocket->bOobInLine;
          LengthInBytes = sizeof ( pSocket->bOobInLine );
          break;

        case SO_OOBINLINE:
          pOptionData = (UINT8 *)&pSocket->bOobInLine;
          LengthInBytes = sizeof ( pSocket->bOobInLine );

          //
          //  Validate the option length
          //
          if ( sizeof ( UINT32 ) == OptionLength ) {
            //
            //  Restrict the input to TRUE or FALSE
            //
            bTrueFalse = TRUE;
            if ( 0 == *(UINT32 *)pOptionValue ) {
              bTrueFalse = FALSE;
            }
            pOptionValue = &bTrueFalse;
          }
          else {
            //
            //  Force an invalid option length error
            //
            OptionLength = LengthInBytes - 1;
          }
          break;

        case SO_RCVTIMEO:
          //
          //  Return the receive timeout
          //
          pOptionData = (UINT8 *)&pSocket->RxTimeout;
          LengthInBytes = sizeof ( pSocket->RxTimeout );
          break;

        case SO_RCVBUF:
          //
          //  Return the maximum receive buffer size
          //
          pOptionData = (UINT8 *)&pSocket->MaxRxBuf;
          LengthInBytes = sizeof ( pSocket->MaxRxBuf );
          break;

        case SO_REUSEADDR:
          //
          //  Return the address reuse flag
          //
          pOptionData = (UINT8 *)&pSocket->bReUseAddr;
          LengthInBytes = sizeof ( pSocket->bReUseAddr );
          break;

        case SO_SNDBUF:
          //
          //  Send buffer size
          //
          //
          //  Return the maximum transmit buffer size
          //
          pOptionData = (UINT8 *)&pSocket->MaxTxBuf;
          LengthInBytes = sizeof ( pSocket->MaxTxBuf );
          break;
        }
        break;
      }

      //
      //  Determine if an option was found
      //
      if ( 0 != LengthInBytes ) {
        //
        //  Validate the option length
        //
        if ( LengthInBytes <= OptionLength ) {
          //
          //  Set the option value
          //
          CopyMem ( pOptionData, pOptionValue, LengthInBytes );
          errno = 0;
          Status = EFI_SUCCESS;
        }
        else {
          DEBUG (( DEBUG_OPTION,
                    "ERROR - Buffer to small, %d bytes < %d bytes!\r\n",
                    OptionLength,
                    LengthInBytes ));
        }
      }
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**  Allocate a packet for a receive or transmit operation.

  This support routine is called by ::EslSocketRxStart and the
  network specific TxBuffer routines to get buffer space for the
  next operation.

  @param[in]  ppPacket      Address to receive the ::ESL_PACKET structure
  @param[in]  LengthInBytes Length of the packet structure
  @param[in]  ZeroBytes     Length of packet to zero
  @param[in]  DebugFlags    Flags for debug messages

  @retval EFI_SUCCESS - The packet was allocated successfully
**/
EFI_STATUS
EslSocketPacketAllocate (
  IN ESL_PACKET ** ppPacket,
  IN size_t LengthInBytes,
  IN size_t ZeroBytes,
  IN UINTN DebugFlags
  )
{
  ESL_PACKET * pPacket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Allocate a packet structure
  //
  LengthInBytes += sizeof ( *pPacket )
                - sizeof ( pPacket->Op );
  Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                               LengthInBytes,
                               (VOID **)&pPacket );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DebugFlags | DEBUG_POOL,
              "0x%08x: Allocate pPacket, %d bytes\r\n",
              pPacket,
              LengthInBytes ));
    if ( 0 != ZeroBytes ) {
      ZeroMem ( &pPacket->Op, ZeroBytes );
    }
    pPacket->PacketSize = LengthInBytes;
  }
  else {
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INFO,
              "ERROR - Packet allocation failed for %d bytes, Status: %r\r\n",
              LengthInBytes,
              Status ));
    pPacket = NULL;
  }

  //
  //  Return the packet
  //
  *ppPacket = pPacket;

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Free a packet used for receive or transmit operation.

  This support routine is called by the network specific Close
  and TxComplete routines and during error cases in RxComplete
  and TxBuffer.  Note that the network layers typically place
  receive packets on the ESL_SOCKET::pRxFree list for reuse.

  @param[in]  pPacket     Address of an ::ESL_PACKET structure
  @param[in]  DebugFlags  Flags for debug messages

  @retval EFI_SUCCESS - The packet was allocated successfully
**/
EFI_STATUS
EslSocketPacketFree (
  IN ESL_PACKET * pPacket,
  IN UINTN DebugFlags
  )
{
  UINTN LengthInBytes;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Free a packet structure
  //
  LengthInBytes = pPacket->PacketSize;
  Status = gBS->FreePool ( pPacket );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DebugFlags | DEBUG_POOL,
              "0x%08x: Free pPacket, %d bytes\r\n",
              pPacket,
              LengthInBytes ));
  }
  else {
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INFO,
              "ERROR - Failed to free packet 0x%08x, Status: %r\r\n",
              pPacket,
              Status ));
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Poll a socket for pending activity.

  This routine builds a detected event mask which is returned to
  the caller in the buffer provided.

  The ::poll routine calls this routine to determine if the socket
  needs to be serviced as a result of connection, error, receive or
  transmit activity.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]  Events          Events of interest for this socket
  @param[in]  pEvents         Address to receive the detected events
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully polled
  @retval EFI_INVALID_PARAMETER - When pEvents is NULL
**/
EFI_STATUS
EslSocketPoll (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN short Events,
  IN short * pEvents,
  IN int * pErrno
  )
{
  short DetectedEvents;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  short ValidEvents;
  int   _errno = EINVAL;

  DEBUG (( DEBUG_POLL, "Entering SocketPoll\r\n" ));

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;
  DetectedEvents = 0;
  pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
  pSocket->errno = 0;

  //
  //  Verify the socket state
  //
  Status = EslSocketIsConfigured ( pSocket );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Check for invalid events
    //
    ValidEvents = POLLIN
                | POLLPRI
                | POLLOUT | POLLWRNORM
                | POLLERR
                | POLLHUP
                | POLLNVAL
                | POLLRDNORM
                | POLLRDBAND
                | POLLWRBAND ;
    if ( 0 != ( Events & ( ~ValidEvents ))) {
      DetectedEvents |= POLLNVAL;
      DEBUG (( DEBUG_INFO | DEBUG_POLL,
                "ERROR - Invalid event mask, Valid Events: 0x%04x, Invalid Events: 0x%04x\r\n",
                Events & ValidEvents,
                Events & ( ~ValidEvents )));
    }
    else {
      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Increase the network performance by extending the
      //  polling (idle) loop down into the LAN driver
      //
      EslSocketRxPoll ( pSocket );

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );

      //
      //  Check for pending connections
      //
      if ( 0 != pSocket->FifoDepth ) {
        //
        //  A connection is waiting for an accept call
        //  See posix connect documentation at
        //  http://pubs.opengroup.org/onlinepubs/9699919799/functions/accept.htm
        //
        DetectedEvents |= POLLIN | POLLRDNORM;
      }
      if ( pSocket->bConnected ) {
        //
        //  A connection is present
        //  See posix connect documentation at
        //  http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.htm
        //
        DetectedEvents |= POLLOUT | POLLWRNORM;
      }

      //
      //  The following bits are set based upon the POSIX poll documentation at
      //  http://pubs.opengroup.org/onlinepubs/9699919799/functions/poll.html
      //

      //
      //  Check for urgent receive data
      //
      if ( 0 < pSocket->RxOobBytes ) {
        DetectedEvents |= POLLRDBAND | POLLPRI | POLLIN;
      }

      //
      //  Check for normal receive data
      //
      if (( 0 < pSocket->RxBytes )
        || ( EFI_SUCCESS != pSocket->RxError )) {
        DetectedEvents |= POLLRDNORM | POLLIN;
      }

      //
      //  Handle the receive errors
      //
      if (( EFI_SUCCESS != pSocket->RxError )
        && ( 0 == ( DetectedEvents & POLLIN ))) {
        DetectedEvents |= POLLERR | POLLIN | POLLRDNORM | POLLRDBAND;
      }

      //
      //  Check for urgent transmit data buffer space
      //
      if (( MAX_TX_DATA > pSocket->TxOobBytes )
        || ( EFI_SUCCESS != pSocket->TxError )) {
        DetectedEvents |= POLLWRBAND;
      }

      //
      //  Check for normal transmit data buffer space
      //
      if (( MAX_TX_DATA > pSocket->TxBytes )
        || ( EFI_SUCCESS != pSocket->TxError )) {
        DetectedEvents |= POLLWRNORM;
      }

      //
      //  Handle the transmit error
      //
      if ( EFI_ERROR ( pSocket->TxError )) {
        DetectedEvents |= POLLERR;
      }
      _errno = pSocket->errno;
    }
  }

  //
  //  Return the detected events
  //
  *pEvents = DetectedEvents & ( Events
                              | POLLERR
                              | POLLHUP
                              | POLLNVAL );
  if ( NULL != pErrno ) {
    *pErrno = _errno;
  }
  //
  //  Return the operation status
  //
  DEBUG (( DEBUG_POLL, "Exiting SocketPoll, Status: %r\r\n", Status ));
  return Status;
}


/** Allocate and initialize a ESL_PORT structure.

  This routine initializes an ::ESL_PORT structure for use by
  the socket.  This routine calls a routine via
  ESL_PROTOCOL_API::pfnPortAllocate to initialize the network
  specific resources.  The resources are released later by the
  \ref PortCloseStateMachine.

  This support routine is called by:
  <ul>
    <li>::EslSocketBind</li>
    <li>::EslTcp4ListenComplete</li>
  </ul>
  to connect the socket with the underlying network adapter
  to the socket.

  @param[in]  pSocket     Address of an ::ESL_SOCKET structure.
  @param[in]  pService    Address of an ::ESL_SERVICE structure.
  @param[in]  ChildHandle Network protocol child handle
  @param[in]  pSockAddr   Address of a sockaddr structure that contains the
                          connection point on the local machine.  An IPv4 address
                          of INADDR_ANY specifies that the connection is made to
                          all of the network stacks on the platform.  Specifying a
                          specific IPv4 address restricts the connection to the
                          network stack supporting that address.  Specifying zero
                          for the port causes the network layer to assign a port
                          number from the dynamic range.  Specifying a specific
                          port number causes the network layer to use that port.
  @param[in]  bBindTest   TRUE if EslSocketBindTest should be called
  @param[in]  DebugFlags  Flags for debug messages
  @param[out] ppPort      Buffer to receive new ::ESL_PORT structure address

  @retval EFI_SUCCESS - Socket successfully created
**/
EFI_STATUS
EslSocketPortAllocate (
  IN ESL_SOCKET * pSocket,
  IN ESL_SERVICE * pService,
  IN EFI_HANDLE ChildHandle,
  IN CONST struct sockaddr * pSockAddr,
  IN BOOLEAN bBindTest,
  IN UINTN DebugFlags,
  OUT ESL_PORT ** ppPort
  )
{
  UINTN LengthInBytes;
  UINT8 * pBuffer;
  ESL_IO_MGMT * pIo;
  ESL_LAYER * pLayer;
  ESL_PORT * pPort;
  EFI_SERVICE_BINDING_PROTOCOL * pServiceBinding;
  CONST ESL_SOCKET_BINDING * pSocketBinding;
  EFI_STATUS Status;
  EFI_STATUS TempStatus;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Use for/break instead of goto
  pSocketBinding = pService->pSocketBinding;
  for ( ; ; ) {
    //
    //  Allocate a port structure
    //
    pLayer = &mEslLayer;
    LengthInBytes = sizeof ( *pPort )
                  + ESL_STRUCTURE_ALIGNMENT_BYTES
                  + (( pSocketBinding->RxIo
                       + pSocketBinding->TxIoNormal
                       + pSocketBinding->TxIoUrgent )
                     * sizeof ( ESL_IO_MGMT ));
    pPort = (ESL_PORT *) AllocateZeroPool ( LengthInBytes );
    if ( NULL == pPort ) {
      Status = EFI_OUT_OF_RESOURCES;
      pSocket->errno = ENOMEM;
      break;
    }
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT,
              "0x%08x: Allocate pPort, %d bytes\r\n",
              pPort,
              LengthInBytes ));

    //
    //  Initialize the port
    //
    pPort->DebugFlags = DebugFlags;
    pPort->Handle = ChildHandle;
    pPort->pService = pService;
    pPort->pServiceBinding = pService->pServiceBinding;
    pPort->pSocket = pSocket;
    pPort->pSocketBinding = pService->pSocketBinding;
    pPort->Signature = PORT_SIGNATURE;

    //
    //  Open the port protocol
    //
    Status = gBS->OpenProtocol ( pPort->Handle,
                                 pSocketBinding->pNetworkProtocolGuid,
                                 &pPort->pProtocol.v,
                                 pLayer->ImageHandle,
                                 NULL,
                                 EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to open network protocol GUID on controller 0x%08x\r\n",
                pPort->Handle ));
      pSocket->errno = EEXIST;
      break;
    }
    DEBUG (( DebugFlags,
              "0x%08x: Network protocol GUID opened on controller 0x%08x\r\n",
              pPort->pProtocol.v,
              pPort->Handle ));

    //
    //  Initialize the port specific resources
    //
    Status = pSocket->pApi->pfnPortAllocate ( pPort,
                                              DebugFlags );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Set the local address
    //
    Status = pSocket->pApi->pfnLocalAddrSet ( pPort, pSockAddr, bBindTest );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Test the address/port configuration
    //
    if ( bBindTest ) {
      Status = EslSocketBindTest ( pPort, pSocket->pApi->BindTestErrno );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Initialize the receive structures
    //
    pBuffer = (UINT8 *)&pPort[ 1 ];
    pBuffer = &pBuffer[ ESL_STRUCTURE_ALIGNMENT_BYTES ];
    pBuffer = (UINT8 *)( ESL_STRUCTURE_ALIGNMENT_MASK & (UINTN)pBuffer );
    pIo = (ESL_IO_MGMT *)pBuffer;
    if (( 0 != pSocketBinding->RxIo )
      && ( NULL != pSocket->pApi->pfnRxComplete )) {
      Status = EslSocketIoInit ( pPort,
                                 &pIo,
                                 pSocketBinding->RxIo,
                                 &pPort->pRxFree,
                                 DebugFlags | DEBUG_POOL,
                                 "receive",
                                 pSocket->pApi->pfnRxComplete );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Initialize the urgent transmit structures
    //
    if (( 0 != pSocketBinding->TxIoUrgent )
      && ( NULL != pSocket->pApi->pfnTxOobComplete )) {
      Status = EslSocketIoInit ( pPort,
                                 &pIo,
                                 pSocketBinding->TxIoUrgent,
                                 &pPort->pTxOobFree,
                                 DebugFlags | DEBUG_POOL,
                                 "urgent transmit",
                                 pSocket->pApi->pfnTxOobComplete );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Initialize the normal transmit structures
    //
    if (( 0 != pSocketBinding->TxIoNormal )
      && ( NULL != pSocket->pApi->pfnTxComplete )) {
      Status = EslSocketIoInit ( pPort,
                                 &pIo,
                                 pSocketBinding->TxIoNormal,
                                 &pPort->pTxFree,
                                 DebugFlags | DEBUG_POOL,
                                 "normal transmit",
                                 pSocket->pApi->pfnTxComplete );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Add this port to the socket
    //
    pPort->pLinkSocket = pSocket->pPortList;
    pSocket->pPortList = pPort;
    DEBUG (( DebugFlags,
              "0x%08x: Socket adding port: 0x%08x\r\n",
              pSocket,
              pPort ));

    //
    //  Add this port to the service
    //
    pPort->pLinkService = pService->pPortList;
    pService->pPortList = pPort;

    //
    //  Return the port
    //
    *ppPort = pPort;
    break;
  }

  //
  //  Clean up after the error if necessary
  //
  if ( EFI_ERROR ( Status )) {
    if ( NULL != pPort ) {
      //
      //  Close the port
      //
      EslSocketPortClose ( pPort );
    }
    else {
      //
      //  Close the port if necessary
      //
      pServiceBinding = pService->pServiceBinding;
      TempStatus = pServiceBinding->DestroyChild ( pServiceBinding,
                                                   ChildHandle );
      if ( !EFI_ERROR ( TempStatus )) {
        DEBUG (( DEBUG_BIND | DEBUG_POOL,
                  "0x%08x: %s port handle destroyed\r\n",
                  ChildHandle,
                  pSocketBinding->pName ));
      }
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_BIND | DEBUG_POOL,
                  "ERROR - Failed to destroy the %s port handle 0x%08x, Status: %r\r\n",
                  pSocketBinding->pName,
                  ChildHandle,
                  TempStatus ));
        ASSERT ( EFI_SUCCESS == TempStatus );
      }
    }
  }
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Close a port.

  This routine releases the resources allocated by ::EslSocketPortAllocate.
  This routine calls ESL_PROTOCOL_API::pfnPortClose to release the network
  specific resources.

  This routine is called by:
  <ul>
    <li>::EslSocketPortAllocate - Port initialization failure</li>
    <li>::EslSocketPortCloseRxDone - Last step of close processing</li>
    <li>::EslTcp4ConnectComplete - Connection failure and reducing the port list to a single port</li>
  </ul>
  See the \ref PortCloseStateMachine section.

  @param[in]  pPort       Address of an ::ESL_PORT structure.

  @retval EFI_SUCCESS     The port is closed
  @retval other           Port close error
**/
EFI_STATUS
EslSocketPortClose (
  IN ESL_PORT * pPort
  )
{
  UINTN DebugFlags;
  ESL_LAYER * pLayer;
  ESL_PACKET * pPacket;
  ESL_PORT * pPreviousPort;
  ESL_SERVICE * pService;
  EFI_SERVICE_BINDING_PROTOCOL * pServiceBinding;
  CONST ESL_SOCKET_BINDING * pSocketBinding;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Locate the port in the socket list
  //
  Status = EFI_SUCCESS;
  pLayer = &mEslLayer;
  DebugFlags = pPort->DebugFlags;
  pSocket = pPort->pSocket;
  pPreviousPort = pSocket->pPortList;
  if ( pPreviousPort == pPort ) {
    //
    //  Remove this port from the head of the socket list
    //
    pSocket->pPortList = pPort->pLinkSocket;
  }
  else {
    //
    //  Locate the port in the middle of the socket list
    //
    while (( NULL != pPreviousPort )
      && ( pPreviousPort->pLinkSocket != pPort )) {
      pPreviousPort = pPreviousPort->pLinkSocket;
    }
    if ( NULL != pPreviousPort ) {
      //
      //  Remove the port from the middle of the socket list
      //
      pPreviousPort->pLinkSocket = pPort->pLinkSocket;
    }
  }

  //
  //  Locate the port in the service list
  //  Note that the port may not be in the service list
  //  if the service has been shutdown.
  //
  pService = pPort->pService;
  if ( NULL != pService ) {
    pPreviousPort = pService->pPortList;
    if ( pPreviousPort == pPort ) {
      //
      //  Remove this port from the head of the service list
      //
      pService->pPortList = pPort->pLinkService;
    }
    else {
      //
      //  Locate the port in the middle of the service list
      //
      while (( NULL != pPreviousPort )
        && ( pPreviousPort->pLinkService != pPort )) {
        pPreviousPort = pPreviousPort->pLinkService;
      }
      if ( NULL != pPreviousPort ) {
        //
        //  Remove the port from the middle of the service list
        //
        pPreviousPort->pLinkService = pPort->pLinkService;
      }
    }
  }

  //
  //  Empty the urgent receive queue
  //
  while ( NULL != pSocket->pRxOobPacketListHead ) {
    pPacket = pSocket->pRxOobPacketListHead;
    pSocket->pRxOobPacketListHead = pPacket->pNext;
    pSocket->pApi->pfnPacketFree ( pPacket, &pSocket->RxOobBytes );
    EslSocketPacketFree ( pPacket, DEBUG_RX );
  }
  pSocket->pRxOobPacketListTail = NULL;
  ASSERT ( 0 == pSocket->RxOobBytes );

  //
  //  Empty the receive queue
  //
  while ( NULL != pSocket->pRxPacketListHead ) {
    pPacket = pSocket->pRxPacketListHead;
    pSocket->pRxPacketListHead = pPacket->pNext;
    pSocket->pApi->pfnPacketFree ( pPacket, &pSocket->RxBytes );
    EslSocketPacketFree ( pPacket, DEBUG_RX );
  }
  pSocket->pRxPacketListTail = NULL;
  ASSERT ( 0 == pSocket->RxBytes );

  //
  //  Empty the receive free queue
  //
  while ( NULL != pSocket->pRxFree ) {
    pPacket = pSocket->pRxFree;
    pSocket->pRxFree = pPacket->pNext;
    EslSocketPacketFree ( pPacket, DEBUG_RX );
  }

  //
  //  Release the network specific resources
  //
  if ( NULL != pSocket->pApi->pfnPortClose ) {
    Status = pSocket->pApi->pfnPortClose ( pPort );
  }

  //
  //  Done with the normal transmit events
  //
  Status = EslSocketIoFree ( pPort,
                             &pPort->pTxFree,
                             DebugFlags | DEBUG_POOL,
                             "normal transmit" );

  //
  //  Done with the urgent transmit events
  //
  Status = EslSocketIoFree ( pPort,
                             &pPort->pTxOobFree,
                             DebugFlags | DEBUG_POOL,
                             "urgent transmit" );

  //
  //  Done with the receive events
  //
  Status = EslSocketIoFree ( pPort,
                             &pPort->pRxFree,
                             DebugFlags | DEBUG_POOL,
                             "receive" );

  //
  //  Done with the lower layer network protocol
  //
  pSocketBinding = pPort->pSocketBinding;
  if ( NULL != pPort->pProtocol.v ) {
    Status = gBS->CloseProtocol ( pPort->Handle,
                                  pSocketBinding->pNetworkProtocolGuid,
                                  pLayer->ImageHandle,
                                  NULL );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags,
                "0x%08x: Network protocol GUID closed on controller 0x%08x\r\n",
                pPort->pProtocol.v,
                pPort->Handle ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close network protocol GUID on controller 0x%08x, Status: %r\r\n",
                pPort->Handle,
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Done with the network port
  //
  pServiceBinding = pPort->pServiceBinding;
  if ( NULL != pPort->Handle ) {
    Status = pServiceBinding->DestroyChild ( pServiceBinding,
                                             pPort->Handle );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL,
                "0x%08x: %s port handle destroyed\r\n",
                pPort->Handle,
                pSocketBinding->pName ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags | DEBUG_POOL,
                "ERROR - Failed to destroy the %s port handle, Status: %r\r\n",
                pSocketBinding->pName,
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Release the port structure
  //
  Status = gBS->FreePool ( pPort );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DebugFlags | DEBUG_POOL,
              "0x%08x: Free pPort, %d bytes\r\n",
              pPort,
              sizeof ( *pPort )));
  }
  else {
    DEBUG (( DEBUG_ERROR | DebugFlags | DEBUG_POOL,
              "ERROR - Failed to free pPort: 0x%08x, Status: %r\r\n",
              pPort,
              Status ));
    ASSERT ( EFI_SUCCESS == Status );
  }

  //
  //  Mark the socket as closed if necessary
  //
  if ( NULL == pSocket->pPortList ) {
    pSocket->State = SOCKET_STATE_CLOSED;
    DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
              "0x%08x: Socket State: SOCKET_STATE_CLOSED\r\n",
              pSocket ));
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Port close state 3.

  This routine attempts to complete the port close operation.

  This routine is called by the TCP layer upon completion of
  the close operation and by ::EslSocketPortCloseTxDone.
  See the \ref PortCloseStateMachine section.

  @param[in]  Event     The close completion event
  @param[in]  pPort     Address of an ::ESL_PORT structure.
**/
VOID
EslSocketPortCloseComplete (
  IN EFI_EVENT Event,
  IN ESL_PORT * pPort
  )
{
  ESL_IO_MGMT * pIo;
  EFI_STATUS Status;

  DBG_ENTER ( );
  VERIFY_AT_TPL ( TPL_SOCKETS );

  //  Update the port state
  pPort->State = PORT_STATE_CLOSE_DONE;
  DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
            "0x%08x: Port Close State: PORT_STATE_CLOSE_DONE\r\n",
            pPort ));

  //  Shutdown the receive operation on the port
  if ( NULL != pPort->pfnRxCancel ) {
    pIo = pPort->pRxActive;
    while ( NULL != pIo ) {
      EslSocketRxCancel ( pPort, pIo );
      pIo = pIo->pNext;
    }
  }

  //  Determine if the receive operation is pending
  Status = EslSocketPortCloseRxDone ( pPort );
  DBG_EXIT_STATUS ( Status );
  --Status;
}


/** Port close state 4.

  This routine determines the state of the receive operations and
  continues the close operation after the pending receive operations
  are cancelled.

  This routine is called by
  <ul>
    <li>::EslSocketPortCloseComplete</li>
    <li>::EslSocketPortCloseTxDone</li>
    <li>::EslSocketRxComplete</li>
  </ul>
  to determine the state of the receive operations.
  See the \ref PortCloseStateMachine section.

  @param[in]  pPort       Address of an ::ESL_PORT structure.

  @retval EFI_SUCCESS         The port is closed
  @retval EFI_NOT_READY       The port is still closing
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.
**/
EFI_STATUS
EslSocketPortCloseRxDone (
  IN ESL_PORT * pPort
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Verify that the port is closing
  //
  Status = EFI_ALREADY_STARTED;
  if ( PORT_STATE_CLOSE_DONE == pPort->State ) {
    //
    //  Determine if the receive operation is pending
    //
    Status = EFI_NOT_READY;
    if ( NULL == pPort->pRxActive ) {
      //
      //  The receive operation is complete
      //  Update the port state
      //
      pPort->State = PORT_STATE_CLOSE_RX_DONE;
      DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                "0x%08x: Port Close State: PORT_STATE_CLOSE_RX_DONE\r\n",
                pPort ));

      //
      //  Complete the port close operation
      //
      Status = EslSocketPortClose ( pPort );
    }
    else {
      DEBUG_CODE_BEGIN ();
      {
        ESL_IO_MGMT * pIo;
        //
        //  Display the outstanding receive operations
        //
        DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                  "0x%08x: Port Close: Receive still pending!\r\n",
                  pPort ));
        pIo = pPort->pRxActive;
        while ( NULL != pIo ) {
          DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                    "0x%08x: Packet pending on network adapter\r\n",
                    pIo->pPacket ));
          pIo = pIo->pNext;
        }
      }
      DEBUG_CODE_END ( );
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Start the close operation on a port, state 1.

  This routine marks the port as closed and initiates the \ref
  PortCloseStateMachine. The first step is to allow the \ref
  TransmitEngine to run down.

  This routine is called by ::EslSocketCloseStart to initiate the socket
  network specific close operation on the socket.

  @param[in]  pPort       Address of an ::ESL_PORT structure.
  @param[in]  bCloseNow   Set TRUE to abort active transfers
  @param[in]  DebugFlags  Flags for debug messages

  @retval EFI_SUCCESS         The port is closed, not normally returned
  @retval EFI_NOT_READY       The port has started the closing process
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.
**/
EFI_STATUS
EslSocketPortCloseStart (
  IN ESL_PORT * pPort,
  IN BOOLEAN bCloseNow,
  IN UINTN DebugFlags
  )
{
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Mark the port as closing
  //
  Status = EFI_ALREADY_STARTED;
  pSocket = pPort->pSocket;
  pSocket->errno = EALREADY;
  if ( PORT_STATE_CLOSE_STARTED > pPort->State ) {

    //
    //  Update the port state
    //
    pPort->State = PORT_STATE_CLOSE_STARTED;
    DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
              "0x%08x: Port Close State: PORT_STATE_CLOSE_STARTED\r\n",
              pPort ));
    pPort->bCloseNow = bCloseNow;
    pPort->DebugFlags = DebugFlags;

    //
    //  Determine if transmits are complete
    //
    Status = EslSocketPortCloseTxDone ( pPort );
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Port close state 2.

  This routine determines the state of the transmit engine and
  continue the close operation after the transmission is complete.
  The next step is to stop the \ref ReceiveEngine.
  See the \ref PortCloseStateMachine section.

  This routine is called by ::EslSocketPortCloseStart to determine
  if the transmission is complete.

  @param[in]  pPort           Address of an ::ESL_PORT structure.

  @retval EFI_SUCCESS         The port is closed, not normally returned
  @retval EFI_NOT_READY       The port is still closing
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.

**/
EFI_STATUS
EslSocketPortCloseTxDone (
  IN ESL_PORT * pPort
  )
{
  ESL_IO_MGMT * pIo;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  All transmissions are complete or must be stopped
  //  Mark the port as TX complete
  //
  Status = EFI_ALREADY_STARTED;
  if ( PORT_STATE_CLOSE_STARTED == pPort->State ) {
    //
    //  Verify that the transmissions are complete
    //
    pSocket = pPort->pSocket;
    if ( pPort->bCloseNow
         || ( EFI_SUCCESS != pSocket->TxError )
         || (( NULL == pPort->pTxActive )
                && ( NULL == pPort->pTxOobActive ))) {
      //
      //  Update the port state
      //
      pPort->State = PORT_STATE_CLOSE_TX_DONE;
      DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                "0x%08x: Port Close State: PORT_STATE_CLOSE_TX_DONE\r\n",
                pPort ));

      //
      //  Close the port
      //  Skip the close operation if the port is not configured
      //
      Status = EFI_SUCCESS;
      pSocket = pPort->pSocket;
      if (( pPort->bConfigured )
        && ( NULL != pSocket->pApi->pfnPortCloseOp )) {
          //
          //  Start the close operation
          //
          Status = pSocket->pApi->pfnPortCloseOp ( pPort );
          DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                    "0x%08x: Port Close: Close operation still pending!\r\n",
                    pPort ));
          ASSERT ( EFI_SUCCESS == Status );
      }
      else {
        //
        //  The receive operation is complete
        //  Update the port state
        //
        EslSocketPortCloseComplete ( NULL, pPort );
      }
    }
    else {
      //
      //  Transmissions are still active, exit
      //
      Status = EFI_NOT_READY;
      pSocket->errno = EAGAIN;
      DEBUG_CODE_BEGIN ( );
      {
        ESL_PACKET * pPacket;

        DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                  "0x%08x: Port Close: Transmits are still pending!\r\n",
                  pPort ));

        //
        //  Display the pending urgent transmit packets
        //
        pPacket = pSocket->pTxOobPacketListHead;
        while ( NULL != pPacket ) {
          DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                    "0x%08x: Packet pending on urgent TX list, %d bytes\r\n",
                    pPacket,
                    pPacket->PacketSize ));
          pPacket = pPacket->pNext;
        }

        pIo = pPort->pTxOobActive;
        while ( NULL != pIo ) {
          pPacket = pIo->pPacket;
          DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                    "0x%08x: Packet active %d bytes, pIo: 0x%08x\r\n",
                    pPacket,
                    pPacket->PacketSize,
                    pIo ));
          pIo = pIo->pNext;
        }

        //
        //  Display the pending normal transmit packets
        //
        pPacket = pSocket->pTxPacketListHead;
        while ( NULL != pPacket ) {
          DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                    "0x%08x: Packet pending on normal TX list, %d bytes\r\n",
                    pPacket,
                    pPacket->PacketSize ));
          pPacket = pPacket->pNext;
        }

        pIo = pPort->pTxActive;
        while ( NULL != pIo ) {
          pPacket = pIo->pPacket;
          DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                    "0x%08x: Packet active %d bytes, pIo: 0x%08x\r\n",
                    pPacket,
                    pPacket->PacketSize,
                    pIo ));
          pIo = pIo->pNext;
        }
      }
      DEBUG_CODE_END ();
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Receive data from a network connection.

  This routine calls the network specific routine to remove the
  next portion of data from the receive queue and return it to the
  caller.

  The ::recvfrom routine calls this routine to determine if any data
  is received from the remote system.  Note that the other routines
  ::recv and ::read are layered on top of ::recvfrom.

  @param[in]      pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]      Flags           Message control flags
  @param[in]      BufferLength    Length of the the buffer
  @param[in]      pBuffer         Address of a buffer to receive the data.
  @param[in]      pDataLength     Number of received data bytes in the buffer.
  @param[out]     pAddress        Network address to receive the remote system address
  @param[in,out]  pAddressLength  Length of the remote network address structure
  @param[out]     pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received
**/
EFI_STATUS
EslSocketReceive (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN INT32 Flags,
  IN size_t BufferLength,
  IN UINT8 * pBuffer,
  OUT size_t * pDataLength,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  )
{
  union {
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
  } Addr;
  socklen_t AddressLength;
  BOOLEAN bConsumePacket;
  BOOLEAN bUrgentQueue;
  size_t DataLength;
  ESL_PACKET * pNextPacket;
  ESL_PACKET * pPacket;
  ESL_PORT * pPort;
  ESL_PACKET ** ppQueueHead;
  ESL_PACKET ** ppQueueTail;
  struct sockaddr * pRemoteAddress;
  size_t * pRxDataBytes;
  ESL_SOCKET * pSocket;
  size_t SkipBytes;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Validate the return address parameters
    //
    if (( NULL == pAddress ) || ( NULL != pAddressLength )) {
      //
      //  Return the transmit error if necessary
      //
      if ( EFI_SUCCESS != pSocket->TxError ) {
        pSocket->errno = EIO;
        Status = pSocket->TxError;
        pSocket->TxError = EFI_SUCCESS;
      }
      else {
        //
        //  Verify the socket state
        //
        Status = EslSocketIsConfigured ( pSocket );
        if ( !EFI_ERROR ( Status )) {
          //
          //  Validate the buffer length
          //
          if (( NULL == pDataLength )
            || ( NULL == pBuffer )) {
            if ( NULL == pDataLength ) {
              DEBUG (( DEBUG_RX,
                        "ERROR - pDataLength is NULL!\r\n" ));
            }
            else {
              DEBUG (( DEBUG_RX,
                        "ERROR - pBuffer is NULL!\r\n" ));
            }
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EFAULT;
          }
          else {
            //
            //  Verify the API
            //
            if ( NULL == pSocket->pApi->pfnReceive ) {
              Status = EFI_UNSUPPORTED;
              pSocket->errno = ENOTSUP;
            }
            else {
              //
              //  Zero the receive address if being returned
              //
              pRemoteAddress = NULL;
              if ( NULL != pAddress ) {
                pRemoteAddress = (struct sockaddr *)&Addr;
                ZeroMem ( pRemoteAddress, sizeof ( Addr ));
                pRemoteAddress->sa_family = pSocket->pApi->AddressFamily;
                pRemoteAddress->sa_len = (UINT8)pSocket->pApi->AddressLength;
              }

              //
              //  Synchronize with the socket layer
              //
              RAISE_TPL ( TplPrevious, TPL_SOCKETS );

              //
              //  Assume failure
              //
              Status = EFI_UNSUPPORTED;
              pSocket->errno = ENOTCONN;

              //
              //  Verify that the socket is connected
              //
              if ( SOCKET_STATE_CONNECTED == pSocket->State ) {
                //
                //  Poll the network to increase performance
                //
                EslSocketRxPoll ( pSocket );

                //
                //  Locate the port
                //
                pPort = pSocket->pPortList;
                if ( NULL != pPort ) {
                  //
                  //  Determine the queue head
                  //
                  bUrgentQueue = (BOOLEAN)( 0 != ( Flags & MSG_OOB ));
                  if ( bUrgentQueue ) {
                    ppQueueHead = &pSocket->pRxOobPacketListHead;
                    ppQueueTail = &pSocket->pRxOobPacketListTail;
                    pRxDataBytes = &pSocket->RxOobBytes;
                  }
                  else {
                    ppQueueHead = &pSocket->pRxPacketListHead;
                    ppQueueTail = &pSocket->pRxPacketListTail;
                    pRxDataBytes = &pSocket->RxBytes;
                  }

                  //
                  //  Determine if there is any data on the queue
                  //
                  *pDataLength = 0;
                  pPacket = *ppQueueHead;
                  if ( NULL != pPacket ) {
                    //
                    //  Copy the received data
                    //
                    do {
                      //
                      //  Attempt to receive a packet
                      //
                      SkipBytes = 0;
                      bConsumePacket = (BOOLEAN)( 0 == ( Flags & MSG_PEEK ));
                      pBuffer = pSocket->pApi->pfnReceive ( pPort,
                                                            pPacket,
                                                            &bConsumePacket,
                                                            BufferLength,
                                                            pBuffer,
                                                            &DataLength,
                                                            (struct sockaddr *)&Addr,
                                                            &SkipBytes );
                      *pDataLength += DataLength;
                      BufferLength -= DataLength;

                      //
                      //  Determine if the data is being read
                      //
                      pNextPacket = pPacket->pNext;
                      if ( bConsumePacket ) {
                        //
                        //  All done with this packet
                        //  Account for any discarded data
                        //
                        pSocket->pApi->pfnPacketFree ( pPacket, pRxDataBytes );
                        if ( 0 != SkipBytes ) {
                          DEBUG (( DEBUG_RX,
                                    "0x%08x: Port, packet read, skipping over 0x%08x bytes\r\n",
                                    pPort,
                                    SkipBytes ));
                        }

                        //
                        //  Remove this packet from the queue
                        //
                        *ppQueueHead = pPacket->pNext;
                        if ( NULL == *ppQueueHead ) {
                          *ppQueueTail = NULL;
                        }

                        //
                        //  Move the packet to the free queue
                        //
                        pPacket->pNext = pSocket->pRxFree;
                        pSocket->pRxFree = pPacket;
                        DEBUG (( DEBUG_RX,
                                  "0x%08x: Port freeing packet 0x%08x\r\n",
                                  pPort,
                                  pPacket ));

                        //
                        //  Restart the receive operation if necessary
                        //
                        if (( NULL != pPort->pRxFree )
                          && ( MAX_RX_DATA > pSocket->RxBytes )) {
                            EslSocketRxStart ( pPort );
                        }
                      }

                      //
                      //  Get the next packet
                      //
                      pPacket = pNextPacket;
                    } while (( SOCK_STREAM == pSocket->Type )
                          && ( NULL != pPacket )
                          && ( 0 < BufferLength ));

                    //
                    //  Successful operation
                    //
                    Status = EFI_SUCCESS;
                    pSocket->errno = 0;
                  }
                  else {
                    //
                    //  The queue is empty
                    //  Determine if it is time to return the receive error
                    //
                    if ( EFI_ERROR ( pSocket->RxError )
                      && ( NULL == pSocket->pRxPacketListHead )
                      && ( NULL == pSocket->pRxOobPacketListHead )) {
                      Status = pSocket->RxError;
                      pSocket->RxError = EFI_SUCCESS;
                      switch ( Status ) {
                      default:
                        pSocket->errno = EIO;
                        break;

                      case EFI_CONNECTION_FIN:
                        //
                        //  Continue to return zero bytes received when the
                        //  peer has successfully closed the connection
                        //
                        pSocket->RxError = EFI_CONNECTION_FIN;
                        *pDataLength = 0;
                        pSocket->errno = 0;
                        Status = EFI_SUCCESS;
                        break;

                      case EFI_CONNECTION_REFUSED:
                        pSocket->errno = ECONNREFUSED;
                        break;

                      case EFI_CONNECTION_RESET:
                        pSocket->errno = ECONNRESET;
                        break;

                      case EFI_HOST_UNREACHABLE:
                        pSocket->errno = EHOSTUNREACH;
                        break;

                      case EFI_NETWORK_UNREACHABLE:
                        pSocket->errno = ENETUNREACH;
                        break;

                      case EFI_PORT_UNREACHABLE:
                        pSocket->errno = EPROTONOSUPPORT;
                        break;

                      case EFI_PROTOCOL_UNREACHABLE:
                        pSocket->errno = ENOPROTOOPT;
                        break;
                      }
                    }
                    else {
                      Status = EFI_NOT_READY;
                      pSocket->errno = EAGAIN;
                    }
                  }
                }
              }

              //
              //  Release the socket layer synchronization
              //
              RESTORE_TPL ( TplPrevious );

              if (( !EFI_ERROR ( Status )) && ( NULL != pAddress )) {
                //
                //  Return the remote address if requested, truncate if necessary
                //
                AddressLength = pRemoteAddress->sa_len;
                if ( AddressLength > *pAddressLength ) {
                  AddressLength = *pAddressLength;
                }
                DEBUG (( DEBUG_RX,
                          "Returning the remote address, 0x%016x bytes --> 0x%16x\r\n", *pAddressLength, pAddress ));
                ZeroMem ( pAddress, *pAddressLength );
                CopyMem ( pAddress, &Addr, AddressLength );

                //
                //  Update the address length
                //
                *pAddressLength = pRemoteAddress->sa_len;
              }
            }
          }
        }
      }


    }
    else {
      //
      //  Bad return address pointer and length
      //
      Status = EFI_INVALID_PARAMETER;
      pSocket->errno = EINVAL;
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Cancel the receive operations.

  This routine cancels a pending receive operation.
  See the \ref ReceiveEngine section.

  This routine is called by ::EslSocketShutdown when the socket
  layer is being shutdown.

  @param[in]  pPort     Address of an ::ESL_PORT structure
  @param[in]  pIo       Address of an ::ESL_IO_MGMT structure
**/
VOID
EslSocketRxCancel (
  IN ESL_PORT * pPort,
  IN ESL_IO_MGMT * pIo
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Cancel the outstanding receive
  //
  Status = pPort->pfnRxCancel ( pPort->pProtocol.v,
                                &pIo->Token );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( pPort->DebugFlags | DEBUG_CLOSE | DEBUG_INFO,
              "0x%08x: Packet receive aborted on port: 0x%08x\r\n",
              pIo->pPacket,
              pPort ));
  }
  else {
    DEBUG (( pPort->DebugFlags | DEBUG_CLOSE | DEBUG_INFO,
              "0x%08x: Packet receive pending on Port 0x%08x, Status: %r\r\n",
              pIo->pPacket,
              pPort,
              Status ));
  }
  DBG_EXIT ( );
}


/** Process the receive completion.

  This routine queues the data in FIFO order in either the urgent
  or normal data queues depending upon the type of data received.
  See the \ref ReceiveEngine section.

  This routine is called when some data is received by:
  <ul>
    <li>::EslIp4RxComplete</li>
    <li>::EslTcp4RxComplete</li>
    <li>::EslUdp4RxComplete</li>
  </ul>

  @param[in]  pIo           Address of an ::ESL_IO_MGMT structure
  @param[in]  Status        Receive status
  @param[in]  LengthInBytes Length of the receive data
  @param[in]  bUrgent       TRUE if urgent data is received and FALSE
                            for normal data.
**/
VOID
EslSocketRxComplete (
  IN ESL_IO_MGMT * pIo,
  IN EFI_STATUS Status,
  IN UINTN LengthInBytes,
  IN BOOLEAN bUrgent
  )
{
  BOOLEAN bUrgentQueue;
  ESL_IO_MGMT * pIoNext;
  ESL_PACKET * pPacket;
  ESL_PORT * pPort;
  ESL_PACKET * pPrevious;
  ESL_PACKET ** ppQueueHead;
  ESL_PACKET ** ppQueueTail;
  size_t * pRxBytes;
  ESL_SOCKET * pSocket;

  DBG_ENTER ( );
  VERIFY_AT_TPL ( TPL_SOCKETS );

  //
  //  Locate the active receive packet
  //
  pPacket = pIo->pPacket;
  pPort = pIo->pPort;
  pSocket = pPort->pSocket;

  //
  //         pPort->pRxActive
  //                |
  //                V
  //          +-------------+   +-------------+   +-------------+
  //  Active  | ESL_IO_MGMT |-->| ESL_IO_MGMT |-->| ESL_IO_MGMT |--> NULL
  //          +-------------+   +-------------+   +-------------+
  //
  //          +-------------+   +-------------+   +-------------+
  //  Free    | ESL_IO_MGMT |-->| ESL_IO_MGMT |-->| ESL_IO_MGMT |--> NULL
  //          +-------------+   +-------------+   +-------------+
  //                ^
  //                |
  //          pPort->pRxFree
  //
  //
  //  Remove the IO structure from the active list
  //  The following code searches for the entry in the list and does not
  //  assume that the receive operations complete in the order they were
  //  issued to the UEFI network layer.
  //
  pIoNext = pPort->pRxActive;
  while (( NULL != pIoNext ) && ( pIoNext != pIo ) && ( pIoNext->pNext != pIo ))
  {
    pIoNext = pIoNext->pNext;
  }
  ASSERT ( NULL != pIoNext );
  if ( pIoNext == pIo ) {
    pPort->pRxActive = pIo->pNext;  //  Beginning of list
  }
  else {
    pIoNext->pNext = pIo->pNext;    //  Middle of list
  }

  //
  //  Free the IO structure
  //
  pIo->pNext = pPort->pRxFree;
  pPort->pRxFree = pIo;

  //
  //            pRxOobPacketListHead              pRxOobPacketListTail
  //                      |                                 |
  //                      V                                 V
  //               +------------+   +------------+   +------------+
  //  Urgent Data  | ESL_PACKET |-->| ESL_PACKET |-->| ESL_PACKET |--> NULL
  //               +------------+   +------------+   +------------+
  //
  //               +------------+   +------------+   +------------+
  //  Normal Data  | ESL_PACKET |-->| ESL_PACKET |-->| ESL_PACKET |--> NULL
  //               +------------+   +------------+   +------------+
  //                      ^                                 ^
  //                      |                                 |
  //              pRxPacketListHead                pRxPacketListTail
  //
  //
  //  Determine the queue to use
  //
  bUrgentQueue = (BOOLEAN)( bUrgent
               && pSocket->pApi->bOobSupported
               && ( !pSocket->bOobInLine ));
  if ( bUrgentQueue ) {
    ppQueueHead = &pSocket->pRxOobPacketListHead;
    ppQueueTail = &pSocket->pRxOobPacketListTail;
    pRxBytes = &pSocket->RxOobBytes;
  }
  else {
    ppQueueHead = &pSocket->pRxPacketListHead;
    ppQueueTail = &pSocket->pRxPacketListTail;
    pRxBytes = &pSocket->RxBytes;
  }

  //
  //  Determine if this receive was successful
  //
  if (( !EFI_ERROR ( Status ))
    && ( PORT_STATE_CLOSE_STARTED > pPort->State )
    && ( !pSocket->bRxDisable )) {
    //
    //  Account for the received data
    //
    *pRxBytes += LengthInBytes;

    //
    //  Log the received data
    //
    DEBUG (( DEBUG_RX | DEBUG_INFO,
              "0x%08x: Packet queued on %s queue of port 0x%08x with 0x%08x bytes of %s data\r\n",
              pPacket,
              bUrgentQueue ? L"urgent" : L"normal",
              pPort,
              LengthInBytes,
              bUrgent ? L"urgent" : L"normal" ));

    //
    //  Add the packet to the list tail.
    //
    pPacket->pNext = NULL;
    pPrevious = *ppQueueTail;
    if ( NULL == pPrevious ) {
      *ppQueueHead = pPacket;
    }
    else {
      pPrevious->pNext = pPacket;
    }
    *ppQueueTail = pPacket;

    //
    //  Attempt to restart this receive operation
    //
    if ( pSocket->MaxRxBuf > pSocket->RxBytes ) {
      EslSocketRxStart ( pPort );
    }
    else {
      DEBUG (( DEBUG_RX,
                "0x%08x: Port RX suspended, 0x%08x bytes queued\r\n",
                pPort,
                pSocket->RxBytes ));
    }
  }
  else {
    if ( EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_RX | DEBUG_INFO,
                  "ERROR - Receive error on port 0x%08x, packet 0x%08x, Status:%r\r\n",
                  pPort,
                  pPacket,
                  Status ));
    }

    //
    //  Account for the receive bytes and release the driver's buffer
    //
    if ( !EFI_ERROR ( Status )) {
      *pRxBytes += LengthInBytes;
      pSocket->pApi->pfnPacketFree ( pPacket, pRxBytes );
    }

    //
    //  Receive error, free the packet save the error
    //
    EslSocketPacketFree ( pPacket, DEBUG_RX );
    if ( !EFI_ERROR ( pSocket->RxError )) {
      pSocket->RxError = Status;
    }

    //
    //  Update the port state
    //
    if ( PORT_STATE_CLOSE_STARTED <= pPort->State ) {
      if ( PORT_STATE_CLOSE_DONE == pPort->State ) {
        EslSocketPortCloseRxDone ( pPort );
      }
    }
    else {
      if ( EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_RX | DEBUG_INFO,
                  "0x%08x: Port state: PORT_STATE_RX_ERROR, Status: %r\r\n",
                  pPort,
                  Status ));
        pPort->State = PORT_STATE_RX_ERROR;
      }
    }
  }

  DBG_EXIT ( );
}


/** Poll a socket for pending receive activity.

  This routine is called at elivated TPL and extends the idle
  loop which polls a socket down into the LAN driver layer to
  determine if there is any receive activity.

  The ::EslSocketPoll, ::EslSocketReceive and ::EslSocketTransmit
  routines call this routine when there is nothing to do.

  @param[in]  pSocket   Address of an ::EFI_SOCKET structure.
 **/
VOID
EslSocketRxPoll (
  IN ESL_SOCKET * pSocket
  )
{
  ESL_PORT * pPort;

  DEBUG (( DEBUG_POLL, "Entering EslSocketRxPoll\r\n" ));

  //
  //  Increase the network performance by extending the
  //  polling (idle) loop down into the LAN driver
  //
  pPort = pSocket->pPortList;
  while ( NULL != pPort ) {
    //
    //  Poll the LAN adapter
    //
    pPort->pfnRxPoll ( pPort->pProtocol.v );

    //
    //  Locate the next LAN adapter
    //
    pPort = pPort->pLinkSocket;
  }

  DEBUG (( DEBUG_POLL, "Exiting EslSocketRxPoll\r\n" ));
}


/** Start a receive operation.

  This routine posts a receive buffer to the network adapter.
  See the \ref ReceiveEngine section.

  This support routine is called by:
  <ul>
    <li>::EslIp4Receive to restart the receive engine to release flow control.</li>
    <li>::EslIp4RxComplete to continue the operation of the receive engine if flow control is not being applied.</li>
    <li>::EslIp4SocketIsConfigured to start the receive engine for the new socket.</li>
    <li>::EslTcp4ListenComplete to start the recevie engine for the new socket.</li>
    <li>::EslTcp4Receive to restart the receive engine to release flow control.</li>
    <li>::EslTcp4RxComplete to continue the operation of the receive engine if flow control is not being applied.</li>
    <li>::EslUdp4Receive to restart the receive engine to release flow control.</li>
    <li>::EslUdp4RxComplete to continue the operation of the receive engine if flow control is not being applied.</li>
    <li>::EslUdp4SocketIsConfigured to start the recevie engine for the new socket.</li>
  </ul>

  @param[in]  pPort       Address of an ::ESL_PORT structure.
**/
VOID
EslSocketRxStart (
  IN ESL_PORT * pPort
  )
{
  UINT8 * pBuffer;
  ESL_IO_MGMT * pIo;
  ESL_PACKET * pPacket;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Determine if a receive is already pending
  //
  Status = EFI_SUCCESS;
  pPacket = NULL;
  pSocket = pPort->pSocket;
  if ( !EFI_ERROR ( pPort->pSocket->RxError )) {
    if (( NULL != pPort->pRxFree )
      && ( !pSocket->bRxDisable )
      && ( PORT_STATE_CLOSE_STARTED > pPort->State )) {
      //
      //  Start all of the pending receive operations
      //
      while ( NULL != pPort->pRxFree ) {
        //
        //  Determine if there are any free packets
        //
        pPacket = pSocket->pRxFree;
        if ( NULL != pPacket ) {
          //
          //  Remove this packet from the free list
          //
          pSocket->pRxFree = pPacket->pNext;
          DEBUG (( DEBUG_RX,
                    "0x%08x: Port removed packet 0x%08x from free list\r\n",
                    pPort,
                    pPacket ));
        }
        else {
          //
          //  Allocate a packet structure
          //
          Status = EslSocketPacketAllocate ( &pPacket,
                                             pSocket->pApi->RxPacketBytes,
                                             pSocket->pApi->RxZeroBytes,
                                             DEBUG_RX );
          if ( EFI_ERROR ( Status )) {
            pPacket = NULL;
            DEBUG (( DEBUG_ERROR | DEBUG_RX,
                      "0x%08x: Port failed to allocate RX packet, Status: %r\r\n",
                      pPort,
                      Status ));
            break;
          }
        }

        //
        //  Connect the IO and packet structures
        //
        pIo = pPort->pRxFree;
        pIo->pPacket = pPacket;

        //
        //  Eliminate the need for IP4 and UDP4 specific routines by
        //  clearing the RX data pointer here.
        //
        //  No driver buffer for this packet
        //
        //    +--------------------+
        //    | ESL_IO_MGMT        |
        //    |                    |
        //    |    +---------------+
        //    |    | Token         |
        //    |    |        RxData --> NULL
        //    +----+---------------+
        //
        pBuffer = (UINT8 *)pIo;
        pBuffer = &pBuffer[ pSocket->pApi->RxBufferOffset ];
        *(VOID **)pBuffer = NULL;

        //
        //  Network specific receive packet initialization
        //
        if ( NULL != pSocket->pApi->pfnRxStart ) {
          pSocket->pApi->pfnRxStart ( pPort, pIo );
        }

        //
        //  Start the receive on the packet
        //
        Status = pPort->pfnRxStart ( pPort->pProtocol.v, &pIo->Token );
        if ( !EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_RX | DEBUG_INFO,
                    "0x%08x: Packet receive pending on port 0x%08x\r\n",
                    pPacket,
                    pPort ));
          //
          //  Allocate the receive control structure
          //
          pPort->pRxFree = pIo->pNext;

          //
          //  Mark this receive as pending
          //
          pIo->pNext = pPort->pRxActive;
          pPort->pRxActive = pIo;

        }
        else {
          DEBUG (( DEBUG_RX | DEBUG_INFO,
                    "ERROR - Failed to post a receive on port 0x%08x, Status: %r\r\n",
                    pPort,
                    Status ));
          if ( !EFI_ERROR ( pSocket->RxError )) {
            //
            //  Save the error status
            //
            pSocket->RxError = Status;
          }

          //
          //  Free the packet
          //
          pIo->pPacket = NULL;
          pPacket->pNext = pSocket->pRxFree;
          pSocket->pRxFree = pPacket;
          break;
        }
      }
    }
    else {
      if ( NULL == pPort->pRxFree ) {
        DEBUG (( DEBUG_RX | DEBUG_INFO,
                  "0x%08x: Port, no available ESL_IO_MGMT structures\r\n",
                  pPort));
      }
      if ( pSocket->bRxDisable ) {
        DEBUG (( DEBUG_RX | DEBUG_INFO,
                  "0x%08x: Port, receive disabled!\r\n",
                  pPort ));
      }
      if ( PORT_STATE_CLOSE_STARTED <= pPort->State ) {
        DEBUG (( DEBUG_RX | DEBUG_INFO,
                  "0x%08x: Port, is closing!\r\n",
                  pPort ));
      }
    }
  }
  else {
    DEBUG (( DEBUG_ERROR | DEBUG_RX,
              "ERROR - Previous receive error, Status: %r\r\n",
               pPort->pSocket->RxError ));
  }

  DBG_EXIT ( );
}


/** Shutdown the socket receive and transmit operations.

  This routine sets a flag to stop future transmissions and calls
  the network specific layer to cancel the pending receive operation.

  The ::shutdown routine calls this routine to stop receive and transmit
  operations on the socket.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]  How             Which operations to stop
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket operations successfully shutdown
**/
EFI_STATUS
EslSocketShutdown (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int How,
  IN int * pErrno
  )
{
  ESL_IO_MGMT * pIo;
  ESL_PORT * pPort;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Verify that the socket is connected
    //
    if ( pSocket->bConnected ) {
      //
      //  Validate the How value
      //
      if (( SHUT_RD <= How ) && ( SHUT_RDWR >= How )) {
        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Disable the receiver if requested
        //
        if (( SHUT_RD == How ) || ( SHUT_RDWR == How )) {
          pSocket->bRxDisable = TRUE;
        }

        //
        //  Disable the transmitter if requested
        //
        if (( SHUT_WR == How ) || ( SHUT_RDWR == How )) {
          pSocket->bTxDisable = TRUE;
        }

        //
        //  Cancel the pending receive operations
        //
        if ( pSocket->bRxDisable ) {
          //
          //  Walk the list of ports
          //
          pPort = pSocket->pPortList;
          while ( NULL != pPort ) {
            //
            //  Walk the list of active receive operations
            //
            pIo = pPort->pRxActive;
            while ( NULL != pIo ) {
              EslSocketRxCancel ( pPort, pIo );
            }

            //
            //  Set the next port
            //
            pPort = pPort->pLinkSocket;
          }
        }

        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );
      }
      else {
        //
        //  Invalid How value
        //
        pSocket->errno = EINVAL;
        Status = EFI_INVALID_PARAMETER;
      }
    }
    else {
      //
      //  The socket is not connected
      //
      pSocket->errno = ENOTCONN;
      Status = EFI_NOT_STARTED;
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Send data using a network connection.

  This routine calls the network specific layer to queue the data
  for transmission.  Eventually the buffer will reach the head of
  the queue and will get transmitted over the network by the
  \ref TransmitEngine.  For datagram
  sockets (SOCK_DGRAM and SOCK_RAW) there is no guarantee that
  the data reaches the application running on the remote system.

  The ::sendto routine calls this routine to send data to the remote
  system.  Note that ::send and ::write are layered on top of ::sendto.

  @param[in]  pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param[in]  Flags           Message control flags
  @param[in]  BufferLength    Length of the the buffer
  @param[in]  pBuffer         Address of a buffer containing the data to send
  @param[in]  pDataLength     Address to receive the number of data bytes sent
  @param[in]  pAddress        Network address of the remote system address
  @param[in]  AddressLength   Length of the remote network address structure
  @param[out] pErrno          Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully queued for transmit
**/
EFI_STATUS
EslSocketTransmit (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int Flags,
  IN size_t BufferLength,
  IN CONST UINT8 * pBuffer,
  OUT size_t * pDataLength,
  IN const struct sockaddr * pAddress,
  IN socklen_t AddressLength,
  IN int * pErrno
  )
{
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Return the transmit error if necessary
    //
    if ( EFI_SUCCESS != pSocket->TxError ) {
      pSocket->errno = EIO;
      Status = pSocket->TxError;
      pSocket->TxError = EFI_SUCCESS;
    }
    else {
      //
      //  Verify the socket state
      //
      Status = EslSocketIsConfigured ( pSocket );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Verify that transmit is still allowed
        //
        if ( !pSocket->bTxDisable ) {
          //
          //  Validate the buffer length
          //
          if (( NULL == pDataLength )
            && ( 0 > pDataLength )
            && ( NULL == pBuffer )) {
            if ( NULL == pDataLength ) {
              DEBUG (( DEBUG_RX,
                        "ERROR - pDataLength is NULL!\r\n" ));
            }
            else if ( NULL == pBuffer ) {
              DEBUG (( DEBUG_RX,
                        "ERROR - pBuffer is NULL!\r\n" ));
            }
            else {
              DEBUG (( DEBUG_RX,
                        "ERROR - Data length < 0!\r\n" ));
            }
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EFAULT;
          }
          else {
            //
            //  Validate the remote network address
            //
            if (( NULL != pAddress )
              && ( AddressLength < pAddress->sa_len )) {
              DEBUG (( DEBUG_TX,
                        "ERROR - Invalid sin_len field in address\r\n" ));
              Status = EFI_INVALID_PARAMETER;
              pSocket->errno = EFAULT;
            }
            else {
              //
              //  Verify the API
              //
              if ( NULL == pSocket->pApi->pfnTransmit ) {
                Status = EFI_UNSUPPORTED;
                pSocket->errno = ENOTSUP;
              }
              else {
                //
                //  Synchronize with the socket layer
                //
                RAISE_TPL ( TplPrevious, TPL_SOCKETS );

                //
                //  Poll the network to increase performance
                //
                EslSocketRxPoll ( pSocket );

                //
                //  Attempt to buffer the packet for transmission
                //
                Status = pSocket->pApi->pfnTransmit ( pSocket,
                                                      Flags,
                                                      BufferLength,
                                                      pBuffer,
                                                      pDataLength,
                                                      pAddress,
                                                      AddressLength );

                //
                //  Release the socket layer synchronization
                //
                RESTORE_TPL ( TplPrevious );
              }
            }
          }
        }
        else {
          //
          //  The transmitter was shutdown
          //
          pSocket->errno = EPIPE;
          Status = EFI_NOT_STARTED;
        }
      }
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = ENOTSOCK;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Complete the transmit operation.

  This support routine handles the transmit completion processing for
  the various network layers.  It frees the ::ESL_IO_MGMT structure
  and and frees packet resources by calling ::EslSocketPacketFree.
  Transmit errors are logged in ESL_SOCKET::TxError.
  See the \ref TransmitEngine section.

  This routine is called by:
  <ul>
    <li>::EslIp4TxComplete</li>
    <li>::EslTcp4TxComplete</li>
    <li>::EslTcp4TxOobComplete</li>
    <li>::EslUdp4TxComplete</li>
  </ul>

  @param[in]  pIo             Address of an ::ESL_IO_MGMT structure
  @param[in]  LengthInBytes   Length of the data in bytes
  @param[in]  Status          Transmit operation status
  @param[in]  pQueueType      Zero terminated string describing queue type
  @param[in]  ppQueueHead     Transmit queue head address
  @param[in]  ppQueueTail     Transmit queue tail address
  @param[in]  ppActive        Active transmit queue address
  @param[in]  ppFree          Free transmit queue address
**/
VOID
EslSocketTxComplete (
  IN ESL_IO_MGMT * pIo,
  IN UINT32 LengthInBytes,
  IN EFI_STATUS Status,
  IN CONST CHAR8 * pQueueType,
  IN ESL_PACKET ** ppQueueHead,
  IN ESL_PACKET ** ppQueueTail,
  IN ESL_IO_MGMT ** ppActive,
  IN ESL_IO_MGMT ** ppFree
  )
{
  ESL_PACKET * pCurrentPacket;
  ESL_IO_MGMT * pIoNext;
  ESL_PACKET * pNextPacket;
  ESL_PACKET * pPacket;
  ESL_PORT * pPort;
  ESL_SOCKET * pSocket;

  DBG_ENTER ( );
  VERIFY_AT_TPL ( TPL_SOCKETS );

  //
  //  Locate the active transmit packet
  //
  pPacket = pIo->pPacket;
  pPort = pIo->pPort;
  pSocket = pPort->pSocket;

  //
  //  No more packet
  //
  pIo->pPacket = NULL;

  //
  //  Remove the IO structure from the active list
  //
  pIoNext = *ppActive;
  while (( NULL != pIoNext ) && ( pIoNext != pIo ) && ( pIoNext->pNext != pIo ))
  {
    pIoNext = pIoNext->pNext;
  }
  ASSERT ( NULL != pIoNext );
  if ( pIoNext == pIo ) {
    *ppActive = pIo->pNext;       //  Beginning of list
  }
  else {
    pIoNext->pNext = pIo->pNext;  //  Middle of list
  }

  //
  //  Free the IO structure
  //
  pIo->pNext = *ppFree;
  *ppFree = pIo;

  //
  //  Display the results
  //
  DEBUG (( DEBUG_TX | DEBUG_INFO,
            "0x%08x: pIo Released\r\n",
            pIo ));

  //
  //  Save any transmit error
  //
  if ( EFI_ERROR ( Status )) {
    if ( !EFI_ERROR ( pSocket->TxError )) {
      pSocket->TxError = Status;
    }
    DEBUG (( DEBUG_TX | DEBUG_INFO,
              "ERROR - Transmit failure for %apacket 0x%08x, Status: %r\r\n",
              pQueueType,
              pPacket,
              Status ));

    //
    //  Empty the normal transmit list
    //
    pCurrentPacket = pPacket;
    pNextPacket = *ppQueueHead;
    while ( NULL != pNextPacket ) {
      pPacket = pNextPacket;
      pNextPacket = pPacket->pNext;
      EslSocketPacketFree ( pPacket, DEBUG_TX );
    }
    *ppQueueHead = NULL;
    *ppQueueTail = NULL;
    pPacket = pCurrentPacket;
  }
  else {
    DEBUG (( DEBUG_TX | DEBUG_INFO,
              "0x%08x: %apacket transmitted %d bytes successfully\r\n",
              pPacket,
              pQueueType,
              LengthInBytes ));

    //
    //  Verify the transmit engine is still running
    //
    if ( !pPort->bCloseNow ) {
      //
      //  Start the next packet transmission
      //
      EslSocketTxStart ( pPort,
                         ppQueueHead,
                         ppQueueTail,
                         ppActive,
                         ppFree );
    }
  }

  //
  //  Release this packet
  //
  EslSocketPacketFree ( pPacket, DEBUG_TX );

  //
  //  Finish the close operation if necessary
  //
  if ( PORT_STATE_CLOSE_STARTED <= pPort->State ) {
    //
    //  Indicate that the transmit is complete
    //
    EslSocketPortCloseTxDone ( pPort );
  }

  DBG_EXIT ( );
}


/** Transmit data using a network connection.

  This support routine starts a transmit operation on the
  underlying network layer.

  The network specific code calls this routine to start a
  transmit operation.  See the \ref TransmitEngine section.

  @param[in]  pPort           Address of an ::ESL_PORT structure
  @param[in]  ppQueueHead     Transmit queue head address
  @param[in]  ppQueueTail     Transmit queue tail address
  @param[in]  ppActive        Active transmit queue address
  @param[in]  ppFree          Free transmit queue address
**/
VOID
EslSocketTxStart (
  IN ESL_PORT * pPort,
  IN ESL_PACKET ** ppQueueHead,
  IN ESL_PACKET ** ppQueueTail,
  IN ESL_IO_MGMT ** ppActive,
  IN ESL_IO_MGMT ** ppFree
  )
{
  UINT8 * pBuffer;
  ESL_IO_MGMT * pIo;
  ESL_PACKET * pNextPacket;
  ESL_PACKET * pPacket;
  VOID ** ppTokenData;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Get the packet from the queue head
  //
  pPacket = *ppQueueHead;
  pIo = *ppFree;
  if (( NULL != pPacket ) && ( NULL != pIo )) {
    pSocket = pPort->pSocket;
    //
    //     *ppQueueHead: pSocket->pRxPacketListHead or pSocket->pRxOobPacketListHead
    //          |
    //          V
    //        +------------+   +------------+   +------------+
    //  Data  | ESL_PACKET |-->| ESL_PACKET |-->| ESL_PACKET |--> NULL
    //        +------------+   +------------+   +------------+
    //                                                     ^
    //                                                     |
    //     *ppQueueTail: pSocket->pRxPacketListTail or pSocket->pRxOobPacketListTail
    //
    //
    //  Remove the packet from the queue
    //
    pNextPacket = pPacket->pNext;
    *ppQueueHead = pNextPacket;
    if ( NULL == pNextPacket ) {
      *ppQueueTail = NULL;
    }
    pPacket->pNext = NULL;

    //
    //  Eliminate the need for IP4 and UDP4 specific routines by
    //  connecting the token with the TX data control structure here.
    //
    //    +--------------------+   +--------------------+
    //    | ESL_IO_MGMT        |   | ESL_PACKET         |
    //    |                    |   |                    |
    //    |    +---------------+   +----------------+   |
    //    |    | Token         |   | Buffer Length  |   |
    //    |    |        TxData --> | Buffer Address |   |
    //    |    |               |   +----------------+---+
    //    |    |        Event  |   | Data Buffer        |
    //    +----+---------------+   |                    |
    //                             +--------------------+
    //
    //  Compute the address of the TxData pointer in the token
    //
    pBuffer = (UINT8 *)&pIo->Token;
    pBuffer = &pBuffer[ pSocket->TxTokenOffset ];
    ppTokenData = (VOID **)pBuffer;

    //
    //  Compute the address of the TX data control structure in the packet
    //
    //      * EFI_IP4_TRANSMIT_DATA
    //      * EFI_TCP4_TRANSMIT_DATA
    //      * EFI_UDP4_TRANSMIT_DATA
    //
    pBuffer = (UINT8 *)pPacket;
    pBuffer = &pBuffer[ pSocket->TxPacketOffset ];

    //
    //  Connect the token to the transmit data control structure
    //
    *ppTokenData = (VOID **)pBuffer;

    //
    //  Display the results
    //
    DEBUG (( DEBUG_TX | DEBUG_INFO,
              "0x%08x: pIo allocated for pPacket: 0x%08x\r\n",
              pIo,
              pPacket ));

    //
    //  Start the transmit operation
    //
    Status = pPort->pfnTxStart ( pPort->pProtocol.v,
                                 &pIo->Token );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Connect the structures
      //
      pIo->pPacket = pPacket;

      //
      //          +-------------+   +-------------+   +-------------+
      //  Free    | ESL_IO_MGMT |-->| ESL_IO_MGMT |-->| ESL_IO_MGMT |--> NULL
      //          +-------------+   +-------------+   +-------------+
      //              ^
      //              |
      //          *ppFree:  pPort->pTxFree or pTxOobFree
      //
      //
      //  Remove the IO structure from the queue
      //
      *ppFree = pIo->pNext;

      //
      //         *ppActive:  pPort->pTxActive or pTxOobActive
      //             |
      //             V
      //          +-------------+   +-------------+   +-------------+
      //  Active  | ESL_IO_MGMT |-->| ESL_IO_MGMT |-->| ESL_IO_MGMT |--> NULL
      //          +-------------+   +-------------+   +-------------+
      //
      //
      //  Mark this packet as active
      //
      pIo->pPacket = pPacket;
      pIo->pNext = *ppActive;
      *ppActive = pIo;
    }
    else {
      //
      //  Display the transmit error
      //
      DEBUG (( DEBUG_TX | DEBUG_INFO,
                "0x%08x, 0x%08x: pIo, pPacket transmit failure: %r\r\n",
                pIo,
                pPacket,
                Status ));
      if ( EFI_SUCCESS == pSocket->TxError ) {
        pSocket->TxError = Status;
      }

      //
      //  Free the IO structure
      //
      pIo->pNext = *ppFree;
      *ppFree = pIo;

      //
      //  Discard the transmit buffer
      //
      EslSocketPacketFree ( pPacket, DEBUG_TX );
    }
  }

  DBG_EXIT ( );
}
