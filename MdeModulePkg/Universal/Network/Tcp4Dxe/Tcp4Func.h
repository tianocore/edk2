/** @file
  Tcp function header file.

Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCP4_FUNC_H_
#define _TCP4_FUNC_H_

//
// Declaration of all the functions in TCP
// protocol. It is intended to keep tcp.h
// clear.
//

//
// Functions in tcp.c
//

/**
  Try to find one Tcb whose <Ip, Port> equals to <IN Addr, IN Port>.

  @param  Addr                  Pointer to the IP address needs to match.
  @param  Port                  The port number needs to match.

  @return  The Tcb which matches the <Addr Port> paire exists or not.

**/
BOOLEAN
TcpFindTcbByPeer (
  IN EFI_IPv4_ADDRESS  *Addr,
  IN TCP_PORTNO        Port
  );

/**
  Locate the TCP_CB related to the socket pair.

  @param  LocalPort             The local port number.
  @param  LocalIp               The local IP address.
  @param  RemotePort            The remote port number.
  @param  RemoteIp              The remote IP address.
  @param  Syn                   Whether to search the listen sockets, if TRUE, the
                                listen sockets are searched.

  @return  Pointer to the related TCP_CB, if NULL no match is found.

**/
TCP_CB *
TcpLocateTcb (
  IN TCP_PORTNO  LocalPort,
  IN UINT32      LocalIp,
  IN TCP_PORTNO  RemotePort,
  IN UINT32      RemoteIp,
  IN BOOLEAN     Syn
  );

/**
  Insert a Tcb into the proper queue.

  @param  Tcb                   Pointer to the TCP_CB to be inserted.

  @retval 0                     The Tcb is inserted successfully.
  @retval -1                    Error condition occurred.

**/
INTN
TcpInsertTcb (
  IN TCP_CB *Tcb
  );

/**
  Clone a TCP_CB from Tcb.

  @param  Tcb                   Pointer to the TCP_CB to be cloned.

  @return  Pointer to the new cloned TCP_CB, if NULL error condition occurred.

**/
TCP_CB *
TcpCloneTcb (
  IN TCP_CB *Tcb
  );

/**
  Compute an ISS to be used by a new connection.

  @return  The result ISS.

**/
TCP_SEQNO
TcpGetIss (
  VOID
  );

/**
  Initialize the Tcb local related members.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpInitTcbLocal (
  IN OUT TCP_CB *Tcb
  );

/**
  Initialize the peer related members.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.
  @param  Seg                   Pointer to the segment that contains the peer's
                                intial info.
  @param  Opt                   Pointer to the options announced by the peer.

**/
VOID
TcpInitTcbPeer (
  IN OUT TCP_CB     *Tcb,
  IN     TCP_SEG    *Seg,
  IN     TCP_OPTION *Opt
  );

/**
  Get the local mss.

  @param  Sock        Pointer to the socket to get mss

  @return  The mss size.

**/
UINT16
TcpGetRcvMss (
  IN SOCKET  *Sock
  );

/**
  Set the Tcb's state.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.
  @param  State                 The state to be set.

**/
VOID
TcpSetState (
  IN OUT TCP_CB    *Tcb,
  IN     UINT8     State
  );

//
// Functions in Tcp4Output.c
//
/**
  Send the segment to IP via IpIo function.

  @param  Tcb         Pointer to the TCP_CB of this TCP instance.
  @param  Nbuf        Pointer to the TCP segment to be sent.
  @param  Src         Source address of the TCP segment.
  @param  Dest        Destination address of the TCP segment.

  @retval 0           The segment was sent out successfully.
  @retval -1          The segment was failed to send.

**/
INTN
TcpSendIpPacket (
  IN TCP_CB    *Tcb,
  IN NET_BUF   *Nbuf,
  IN UINT32    Src,
  IN UINT32    Dest
  );

/**
  Check whether to send data/SYN/FIN and piggy back an ACK.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  Force   Whether to ignore the sender's SWS avoidance algorithm and send
                  out data by force.

  @return The number of bytes sent.

**/
INTN
TcpToSendData (
  IN OUT TCP_CB *Tcb,
  IN     INTN Force
  );

/**
  Check whether to send an ACK or delayed ACK.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpToSendAck (
  IN OUT TCP_CB *Tcb
  );

/**
  Send an ACK immediately.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSendAck (
  IN OUT TCP_CB *Tcb
  );

/**
  Send a zero probe segment. It can be used by keepalive and zero window probe.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.

  @retval 0       The zero probe segment was sent out successfully.
  @retval other   Error condition occurred.

**/
INTN
TcpSendZeroProbe (
  IN OUT TCP_CB *Tcb
  );

/**
  Process the data and FIN flag, check whether to deliver
  data to the socket layer.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @retval 0        No error occurred to deliver data.
  @retval -1       Error condition occurred. Proper response is to reset the
                   connection.

**/
INTN
TcpDeliverData (
  IN OUT TCP_CB *Tcb
  );

/**
  Send a RESET segment in response to the segment received.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance, may be NULL.
  @param  Head    TCP header of the segment that triggers the reset.
  @param  Len     Length of the segment that triggers the reset.
  @param  Local   Local IP address.
  @param  Remote  Remote peer's IP address.

  @retval 0       A reset is sent or no need to send it.
  @retval -1      No reset is sent.

**/
INTN
TcpSendReset (
  IN TCP_CB    *Tcb,
  IN TCP_HEAD  *Head,
  IN INT32     Len,
  IN UINT32    Local,
  IN UINT32    Remote
  );

/**
  Compute the sequence space left in the old receive window.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The sequence space left in the old receive window.

**/
UINT32
TcpRcvWinOld (
  IN TCP_CB *Tcb
  );

/**
  Compute the current receive window.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The size of the current receive window, in bytes.

**/
UINT32
TcpRcvWinNow (
  IN TCP_CB *Tcb
  );

/**
  Retransmit the segment from sequence Seq.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  Seq     The sequence number of the segment to be retransmitted.

  @retval 0       Retransmission succeeded.
  @retval -1      Error condition occurred.

**/
INTN
TcpRetransmit (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq
  );

/**
  Compute how much data to send.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  Force   Whether to ignore the sender's SWS avoidance algorithm and send
                  out data by force.

  @return The length of the data can be sent, if 0, no data can be sent.

**/
UINT32
TcpDataToSend (
  IN TCP_CB *Tcb,
  IN INTN   Force
  );

/**
  Verify that the segment is in good shape.

  @param  Nbuf    Buffer that contains the segment to be checked.

  @retval 0       The segment is broken.
  @retval 1       The segment is in good shape.

**/
INTN
TcpVerifySegment (
  IN NET_BUF *Nbuf
  );

/**
  Verify that all the segments in SndQue are in good shape.

  @param  Head    Pointer to the head node of the SndQue.

  @retval 0       At least one segment is broken.
  @retval 1       All segments in the specific queue are in good shape.

**/
INTN
TcpCheckSndQue (
  IN LIST_ENTRY     *Head
  );

/**
  Get a segment from the Tcb's SndQue.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  Seq     The sequence number of the segment.
  @param  Len     The maximum length of the segment.

  @return Pointer to the segment, if NULL some error occurred.

**/
NET_BUF *
TcpGetSegmentSndQue (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  );

/**
  Get a segment from the Tcb's socket buffer.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  Seq     The sequence number of the segment.
  @param  Len     The maximum length of the segment.

  @return Pointer to the segment, if NULL some error occurred.

**/
NET_BUF *
TcpGetSegmentSock (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  );

/**
  Get a segment starting from sequence Seq of a maximum
  length of Len.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  Seq     The sequence number of the segment.
  @param  Len     The maximum length of the segment.

  @return Pointer to the segment, if NULL some error occurred.

**/
NET_BUF *
TcpGetSegment (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  );

/**
  Get the maximum SndNxt.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The sequence number of the maximum SndNxt.

**/
TCP_SEQNO
TcpGetMaxSndNxt (
  IN TCP_CB *Tcb
  );

//
// Functions from Tcp4Input.c
//
/**
  Process the received ICMP error messages for TCP.

  @param  Nbuf     Buffer that contains part of the TCP segment without IP header
                   truncated from the ICMP error packet.
  @param  IcmpErr  The ICMP error code interpreted from ICMP error packet.
  @param  Src      Source address of the ICMP error message.
  @param  Dst      Destination address of the ICMP error message.

**/
VOID
TcpIcmpInput (
  IN NET_BUF     *Nbuf,
  IN UINT8       IcmpErr,
  IN UINT32      Src,
  IN UINT32      Dst
  );

/**
  Process the received TCP segments.

  @param  Nbuf     Buffer that contains received TCP segment without IP header.
  @param  Src      Source address of the segment, or the peer's IP address.
  @param  Dst      Destination address of the segment, or the local end's IP
                   address.

  @retval 0        Segment is processed successfully. It is either accepted or
                   discarded. But no connection is reset by the segment.
  @retval -1       A connection is reset by the segment.

**/
INTN
TcpInput (
  IN NET_BUF *Nbuf,
  IN UINT32  Src,
  IN UINT32  Dst
  );

/**
  Check whether the sequence number of the incoming segment is acceptable.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Seg      Pointer to the incoming segment.

  @retval 1       The sequence number is acceptable.
  @retval 0       The sequence number is not acceptable.

**/
INTN
TcpSeqAcceptable (
  IN TCP_CB  *Tcb,
  IN TCP_SEG *Seg
  );

/**
  NewReno fast recovery, RFC3782.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Seg      Segment that triggers the fast recovery.

**/
VOID
TcpFastRecover (
  IN OUT TCP_CB  *Tcb,
  IN     TCP_SEG *Seg
  );

/**
  NewReno fast loss recovery, RFC3792.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Seg      Segment that triggers the fast loss recovery.

**/
VOID
TcpFastLossRecover (
  IN OUT TCP_CB  *Tcb,
  IN     TCP_SEG *Seg
  );

/**
  Compute the RTT as specified in RFC2988.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Measure  Currently measured RTT in heart beats.

**/
VOID
TcpComputeRtt (
  IN OUT TCP_CB *Tcb,
  IN     UINT32 Measure
  );

/**
  Trim off the data outside the tcb's receive window.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Nbuf     Pointer to the NET_BUF containing the received tcp segment.

**/
VOID
TcpTrimInWnd (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  );

/**
  Store the data into the reassemble queue.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Nbuf     Pointer to the buffer containing the data to be queued.

**/
VOID
TcpQueueData (
  IN OUT TCP_CB  *Tcb,
  IN     NET_BUF *Nbuf
  );

/**
  Ajust the send queue or the retransmit queue.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Ack      The acknowledge seuqence number of the received segment.

**/
VOID
TcpAdjustSndQue (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Ack
  );

//
// Functions from Tcp4Misc.c
//
/**
  Compute the TCP segment's checksum.

  @param  Nbuf                  Pointer to the buffer that contains the TCP
                                segment.
  @param  HeadSum               The checksum value of the fixed part of pseudo
                                header.

  @return  The checksum value.

**/
UINT16
TcpChecksum (
  IN NET_BUF *Nbuf,
  IN UINT16  HeadSum
  );

/**
  Translate the information from the head of the received TCP
  segment Nbuf contains and fill it into a TCP_SEG structure.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.
  @param  Nbuf                  Pointer to the buffer contains the TCP segment.

  @return  Pointer to the TCP_SEG that contains the translated TCP head information.

**/
TCP_SEG *
TcpFormatNetbuf (
  IN     TCP_CB  *Tcb,
  IN OUT NET_BUF *Nbuf
  );

/**
  Initialize an active connection.

  @param  Tcb                   Pointer to the TCP_CB that wants to initiate a
                                connection.

**/
VOID
TcpOnAppConnect (
  IN OUT TCP_CB  *Tcb
  );

/**
  Application has consumed some data, check whether
  to send a window updata ack or a delayed ack.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpOnAppConsume (
  IN TCP_CB *Tcb
  );

/**
  Initiate the connection close procedure, called when
  applications want to close the connection.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpOnAppClose (
  IN OUT TCP_CB *Tcb
  );

/**
  Check whether the application's newly delivered data can be sent out.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.

  @retval 0                     Whether the data is sent out or is buffered for
                                further sending.
  @retval -1                    The Tcb is not in a state that data is permitted to
                                be sent out.

**/
INTN
TcpOnAppSend (
  IN OUT TCP_CB *Tcb
  );

/**
  Abort the connection by sending a reset segment, called
  when the application wants to abort the connection.

  @param  Tcb                   Pointer to the TCP_CB of the TCP instance.

**/
VOID
TcpOnAppAbort (
  IN TCP_CB *Tcb
  );

/**
  Reset the connection related with Tcb.

  @param  Tcb                   Pointer to the TCP_CB of the connection to be
                                reset.

**/
VOID
TcpResetConnection (
  IN TCP_CB *Tcb
  );

//
// Functions in Tcp4Timer.c
//
/**
  Close the TCP connection.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpClose (
  IN OUT TCP_CB *Tcb
  );

/**
  Heart beat timer handler, queues the DPC at TPL_CALLBACK.

  @param  Event    Timer event signaled, ignored.
  @param  Context  Context of the timer event, ignored.

**/
VOID
EFIAPI
TcpTicking (
  IN EFI_EVENT Event,
  IN VOID      *Context
  );

/**
  Enable a TCP timer.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Timer    The index of the timer to be enabled.
  @param  TimeOut  The timeout value of this timer.

**/
VOID
TcpSetTimer (
  IN OUT TCP_CB *Tcb,
  IN     UINT16 Timer,
  IN     UINT32 TimeOut
  );

/**
  Clear one TCP timer.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Timer    The index of the timer to be cleared.

**/
VOID
TcpClearTimer (
  IN OUT TCP_CB *Tcb,
  IN     UINT16 Timer
  );

/**
  Clear all TCP timers.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpClearAllTimer (
  IN OUT TCP_CB *Tcb
  );

/**
  Enable the window prober timer and set the timeout value.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSetProbeTimer (
  IN OUT TCP_CB *Tcb
  );

/**
  Enable the keepalive timer and set the timeout value.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSetKeepaliveTimer (
  IN OUT TCP_CB *Tcb
  );

/**
  Backoff the RTO.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpBackoffRto (
  IN OUT TCP_CB *Tcb
  );

/**
  Install the device path protocol on the TCP instance.

  @param  Sock             Pointer to the socket representing the TCP instance.

  @retval  EFI_SUCCESS     The device path protocol is installed.
  @retval  other           Failed to install the device path protocol.

**/
EFI_STATUS
TcpInstallDevicePath (
  IN SOCKET *Sock
  );

#endif
