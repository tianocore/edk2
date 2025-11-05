/** @file
  Declaration of external functions shared in TCP driver.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCP_FUNC_H_
#define _TCP_FUNC_H_

#include "TcpOption.h"

#define TCP_COMP_VAL(Min, Max, Default, Val) \
  ((((Val) <= (Max)) && ((Val) >= (Min))) ? (Val) : (Default))

/**
  Timeout handler prototype.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
typedef
VOID
(*TCP_TIMER_HANDLER) (
  IN OUT TCP_CB  *Tcb
  );

//
// Functions in TcpMisc.c
//

/**
  Initialize the Tcb locally related members.

  @param[in, out]  Tcb               Pointer to the TCP_CB of this TCP instance.

  @retval EFI_SUCCESS             The operation completed successfully
  @retval others                  The underlying functions failed and could not complete the operation

**/
EFI_STATUS
TcpInitTcbLocal (
  IN OUT TCP_CB  *Tcb
  );

/**
  Initialize the peer related members.

  @param[in, out]  Tcb    Pointer to the TCP_CB of this TCP instance.
  @param[in]       Seg    Pointer to the segment that contains the peer's initial information.
  @param[in]       Opt    Pointer to the options announced by the peer.

**/
VOID
TcpInitTcbPeer (
  IN OUT TCP_CB      *Tcb,
  IN     TCP_SEG     *Seg,
  IN     TCP_OPTION  *Opt
  );

/**
  Try to find one Tcb whose <Ip, Port> equals to <IN Addr, IN Port>.

  @param[in]  Addr     Pointer to the IP address needs to match.
  @param[in]  Port     The port number needs to match.
  @param[in]  Version  IP_VERSION_4 indicates TCP is running on IP4 stack.
                       IP_VERSION_6 indicates TCP is running on IP6 stack.


  @retval     TRUE     The Tcb which matches the <Addr Port> pairs exists.
  @retval     FALSE    Otherwise

**/
BOOLEAN
TcpFindTcbByPeer (
  IN EFI_IP_ADDRESS  *Addr,
  IN TCP_PORTNO      Port,
  IN UINT8           Version
  );

/**
  Locate the TCP_CB related to the socket pair.

  @param[in]  LocalPort      The local port number.
  @param[in]  LocalIp        The local IP address.
  @param[in]  RemotePort     The remote port number.
  @param[in]  RemoteIp       The remote IP address.
  @param[in]  Version        IP_VERSION_4 indicates TCP is running on IP4 stack,
                             IP_VERSION_6 indicates TCP is running on IP6 stack.
  @param[in]  Syn            If TRUE, the listen sockets are searched.

  @return Pointer to the related TCP_CB. If NULL, no match is found.

**/
TCP_CB *
TcpLocateTcb (
  IN TCP_PORTNO      LocalPort,
  IN EFI_IP_ADDRESS  *LocalIp,
  IN TCP_PORTNO      RemotePort,
  IN EFI_IP_ADDRESS  *RemoteIp,
  IN UINT8           Version,
  IN BOOLEAN         Syn
  );

/**
  Insert a Tcb into the proper queue.

  @param[in]  Tcb               Pointer to the TCP_CB to be inserted.

  @retval 0                     The Tcb was inserted successfully.
  @retval -1                    An error condition occurred.

**/
INTN
TcpInsertTcb (
  IN TCP_CB  *Tcb
  );

/**
  Clone a TCP_CB from Tcb.

  @param[in]  Tcb                   Pointer to the TCP_CB to be cloned.

  @return Pointer to the new cloned TCP_CB. If NULL, an error condition occurred.

**/
TCP_CB *
TcpCloneTcb (
  IN TCP_CB  *Tcb
  );

/**
  Get the local mss.

  @param[in]  Sock        Pointer to the socket to get mss.

  @return The mss size.

**/
UINT16
TcpGetRcvMss (
  IN SOCKET  *Sock
  );

/**
  Set the Tcb's state.

  @param[in]  Tcb                   Pointer to the TCP_CB of this TCP instance.
  @param[in]  State                 The state to be set.

**/
VOID
TcpSetState (
  IN TCP_CB  *Tcb,
  IN UINT8   State
  );

/**
  Compute the TCP segment's checksum.

  @param[in]  Nbuf       Pointer to the buffer that contains the TCP segment.
  @param[in]  HeadSum    The checksum value of the fixed part of pseudo header.

  @return The checksum value.

**/
UINT16
TcpChecksum (
  IN NET_BUF  *Nbuf,
  IN UINT16   HeadSum
  );

/**
  Translate the information from the head of the received TCP
  segment Nbuf contains, and fill it into a TCP_SEG structure.

  @param[in]       Tcb           Pointer to the TCP_CB of this TCP instance.
  @param[in, out]  Nbuf          Pointer to the buffer contains the TCP segment.

  @return Pointer to the TCP_SEG that contains the translated TCP head information.

**/
TCP_SEG *
TcpFormatNetbuf (
  IN     TCP_CB   *Tcb,
  IN OUT NET_BUF  *Nbuf
  );

/**
  Initialize an active connection,

  @param[in, out]  Tcb          Pointer to the TCP_CB that wants to initiate a
                                connection.

  @retval EFI_SUCCESS             The operation completed successfully
  @retval others                  The underlying functions failed and could not complete the operation

**/
EFI_STATUS
TcpOnAppConnect (
  IN OUT TCP_CB  *Tcb
  );

/**
  Application has consumed some data, check whether
  to send a window update ack or a delayed ack.

  @param[in]  Tcb        Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpOnAppConsume (
  IN TCP_CB  *Tcb
  );

/**
  Initiate the connection close procedure, called when
  applications want to close the connection.

  @param[in, out]  Tcb          Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpOnAppClose (
  IN OUT TCP_CB  *Tcb
  );

/**
  Check whether the application's newly delivered data can be sent out.

  @param[in, out]  Tcb          Pointer to the TCP_CB of this TCP instance.

  @retval 0                     The data has been sent out successfully.
  @retval -1                    The Tcb is not in a state that data is permitted to
                                be sent out.

**/
INTN
TcpOnAppSend (
  IN OUT TCP_CB  *Tcb
  );

/**
  Abort the connection by sending a reset segment: called
  when the application wants to abort the connection.

  @param[in]  Tcb                   Pointer to the TCP_CB of the TCP instance.

**/
VOID
TcpOnAppAbort (
  IN TCP_CB  *Tcb
  );

/**
  Reset the connection related with Tcb.

  @param[in]  Tcb         Pointer to the TCP_CB of the connection to be reset.

**/
VOID
TcpResetConnection (
  IN TCP_CB  *Tcb
  );

/**
  Install the device path protocol on the TCP instance.

  @param[in]  Sock          Pointer to the socket representing the TCP instance.

  @retval EFI_SUCCESS           The device path protocol installed.
  @retval other                 Failed to install the device path protocol.

**/
EFI_STATUS
TcpInstallDevicePath (
  IN SOCKET  *Sock
  );

//
// Functions in TcpOutput.c
//

/**
  Compute the sequence space left in the old receive window.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The sequence space left in the old receive window.

**/
UINT32
TcpRcvWinOld (
  IN TCP_CB  *Tcb
  );

/**
  Compute the current receive window.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The size of the current receive window, in bytes.

**/
UINT32
TcpRcvWinNow (
  IN TCP_CB  *Tcb
  );

/**
  Get the maximum SndNxt.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The sequence number of the maximum SndNxt.

**/
TCP_SEQNO
TcpGetMaxSndNxt (
  IN TCP_CB  *Tcb
  );

/**
  Compute how much data to send.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Force   If TRUE, ignore the sender's SWS avoidance algorithm
                      and send out data by force.

  @return The length of the data that can be sent. If 0, no data can be sent.

**/
UINT32
TcpDataToSend (
  IN TCP_CB  *Tcb,
  IN INTN    Force
  );

/**
  Retransmit the segment from sequence Seq.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Seq     The sequence number of the segment to be retransmitted.

  @retval 0       The retransmission succeeded.
  @retval -1      An error condition occurred.

**/
INTN
TcpRetransmit (
  IN TCP_CB     *Tcb,
  IN TCP_SEQNO  Seq
  );

/**
  Check whether to send data/SYN/FIN and piggyback an ACK.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]       Force   If TRUE, ignore the sender's SWS avoidance algorithm
                           and send out data by force.

  @return The number of bytes sent.

**/
INTN
TcpToSendData (
  IN OUT TCP_CB  *Tcb,
  IN     INTN    Force
  );

/**
  Check whether to send an ACK or delayed ACK.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpToSendAck (
  IN OUT TCP_CB  *Tcb
  );

/**
  Send an ACK immediately.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSendAck (
  IN OUT TCP_CB  *Tcb
  );

/**
  Send a zero probe segment. It can be used by keepalive and zero window probe.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.

  @retval 0       The zero probe segment was sent out successfully.
  @retval other   An error condition occurred.

**/
INTN
TcpSendZeroProbe (
  IN OUT TCP_CB  *Tcb
  );

/**
  Send a RESET segment in response to the segment received.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance, may be NULL.
  @param[in]  Head    TCP header of the segment that triggers the reset.
  @param[in]  Len     Length of the segment that triggers the reset.
  @param[in]  Local   Local IP address.
  @param[in]  Remote  Remote peer's IP address.
  @param[in]  Version IP_VERSION_4 indicates TCP is running on IP4 stack,
                      IP_VERSION_6 indicates TCP is running on IP6 stack.

  @retval     0       A reset is sent or no need to send it.
  @retval     -1      No reset is sent.

**/
INTN
TcpSendReset (
  IN TCP_CB          *Tcb,
  IN TCP_HEAD        *Head,
  IN INT32           Len,
  IN EFI_IP_ADDRESS  *Local,
  IN EFI_IP_ADDRESS  *Remote,
  IN UINT8           Version
  );

/**
  Verify that the segment is in good shape.

  @param[in]  Nbuf    Buffer that contains the segment to be checked.

  @retval     0       The segment is broken.
  @retval     1       The segment is in good shape.

**/
INTN
TcpVerifySegment (
  IN NET_BUF  *Nbuf
  );

//
// Functions from TcpInput.c
//

/**
  Process the received ICMP error messages for TCP.

  @param[in]  Nbuf     Buffer that contains part of the TCP segment without IP header
                       truncated from the ICMP error packet.
  @param[in]  IcmpErr  The ICMP error code interpreted from an ICMP error packet.
  @param[in]  Src      Source address of the ICMP error message.
  @param[in]  Dst      Destination address of the ICMP error message.
  @param[in]  Version  IP_VERSION_4 indicates IP4 stack, IP_VERSION_6 indicates
                       IP6 stack.

**/
VOID
TcpIcmpInput (
  IN NET_BUF         *Nbuf,
  IN UINT8           IcmpErr,
  IN EFI_IP_ADDRESS  *Src,
  IN EFI_IP_ADDRESS  *Dst,
  IN UINT8           Version
  );

/**
  Process the received TCP segments.

  @param[in]  Nbuf     Buffer that contains received TCP segment without an IP header.
  @param[in]  Src      Source address of the segment, or the peer's IP address.
  @param[in]  Dst      Destination address of the segment, or the local end's IP
                       address.
  @param[in]  Version  IP_VERSION_4 indicates IP4 stack, IP_VERSION_6 indicates
                       IP6 stack.

  @retval 0        The segment processed successfully. It is either accepted or
                   discarded. But no connection is reset by the segment.
  @retval -1       A connection is reset by the segment.

**/
INTN
TcpInput (
  IN NET_BUF         *Nbuf,
  IN EFI_IP_ADDRESS  *Src,
  IN EFI_IP_ADDRESS  *Dst,
  IN UINT8           Version
  );

//
// Functions in TcpTimer.c
//

/**
  Close the TCP connection.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpClose (
  IN OUT TCP_CB  *Tcb
  );

/**
  Heart beat timer handler, queues the DPC at TPL_CALLBACK.

  @param[in]  Event    Timer event signaled, ignored.
  @param[in]  Context  Context of the timer event, ignored.

**/
VOID
EFIAPI
TcpTicking (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Enable a TCP timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]       Timer    The index of the timer to be enabled.
  @param[in]       TimeOut  The timeout value of this timer.

**/
VOID
TcpSetTimer (
  IN OUT TCP_CB  *Tcb,
  IN     UINT16  Timer,
  IN     UINT32  TimeOut
  );

/**
  Clear one TCP timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]       Timer    The index of the timer to be cleared.

**/
VOID
TcpClearTimer (
  IN OUT TCP_CB  *Tcb,
  IN     UINT16  Timer
  );

/**
  Clear all TCP timers.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpClearAllTimer (
  IN OUT TCP_CB  *Tcb
  );

/**
  Enable the window prober timer and set the timeout value.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSetProbeTimer (
  IN OUT TCP_CB  *Tcb
  );

/**
  Enable the keepalive timer and set the timeout value.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSetKeepaliveTimer (
  IN OUT TCP_CB  *Tcb
  );

//
// Functions in TcpIo.c
//

/**
  Packet receive callback function provided to IP_IO. Used to call
  the proper function to handle the packet received by IP.

  @param[in] Status        Result of the receive request.
  @param[in] IcmpErr       Valid when Status is EFI_ICMP_ERROR.
  @param[in] NetSession    The IP session for the received packet.
  @param[in] Pkt           Packet received.
  @param[in] Context       The data provided by the user for the received packet when
                           the callback is registered in IP_IO_OPEN_DATA::RcvdContext.
                           This is an optional parameter that may be NULL.

**/
VOID
EFIAPI
TcpRxCallback (
  IN EFI_STATUS            Status,
  IN UINT8                 IcmpErr,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Pkt,
  IN VOID                  *Context    OPTIONAL
  );

/**
  Send the segment to IP via IpIo function.

  @param[in]  Tcb                Pointer to the TCP_CB of this TCP instance.
  @param[in]  Nbuf               Pointer to the TCP segment to be sent.
  @param[in]  Src                Source address of the TCP segment.
  @param[in]  Dest               Destination address of the TCP segment.
  @param[in]  Version            IP_VERSION_4 or IP_VERSION_6

  @retval 0                      The segment was sent out successfully.
  @retval -1                     The segment failed to be sent.

**/
INTN
TcpSendIpPacket (
  IN TCP_CB          *Tcb,
  IN NET_BUF         *Nbuf,
  IN EFI_IP_ADDRESS  *Src,
  IN EFI_IP_ADDRESS  *Dest,
  IN UINT8           Version
  );

/**
  Refresh the remote peer's Neighbor Cache State if already exists.

  @param[in]  Tcb                Pointer to the TCP_CB of this TCP instance.
  @param[in]  Neighbor           Source address of the TCP segment.
  @param[in]  Timeout            Time in 100-ns units that this entry will remain
                                 in the neighbor cache. A value of zero means that
                                 the entry  is permanent. A value of non-zero means
                                 that the entry is  dynamic and will be deleted
                                 after Timeout.

  @retval EFI_SUCCESS            Successfully updated the neighbor relationship.
  @retval EFI_NOT_STARTED        The IpIo is not configured.
  @retval EFI_INVALID_PARAMETER  Any input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resources.
  @retval EFI_NOT_FOUND          This entry is not in the neighbor table.

**/
EFI_STATUS
Tcp6RefreshNeighbor (
  IN TCP_CB          *Tcb,
  IN EFI_IP_ADDRESS  *Neighbor,
  IN UINT32          Timeout
  );

//
// Functions in TcpDispatcher.c
//

/**
  The protocol handler provided to the socket layer, used to
  dispatch the socket level requests by calling the corresponding
  TCP layer functions.

  @param[in]  Sock               Pointer to the socket of this TCP instance.
  @param[in]  Request            The code of this operation request.
  @param[in]  Data               Pointer to the operation specific data passed in
                                 together with the operation request. This is an
                                 optional parameter that may be NULL.

  @retval EFI_SUCCESS            The socket request completed successfully.
  @retval other                  The error status returned by the corresponding TCP
                                 layer function.

**/
EFI_STATUS
TcpDispatcher (
  IN SOCKET  *Sock,
  IN UINT8   Request,
  IN VOID    *Data    OPTIONAL
  );

#endif
