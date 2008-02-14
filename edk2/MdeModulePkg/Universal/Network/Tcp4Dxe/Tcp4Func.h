/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Tcp4Func.h

Abstract:


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
BOOLEAN
TcpFindTcbByPeer (
  IN EFI_IPv4_ADDRESS  *Addr,
  IN TCP_PORTNO        Port
  );

TCP_CB  *
TcpLocateTcb (
  IN TCP_PORTNO  LocalPort,
  IN UINT32      LocalIp,
  IN TCP_PORTNO  RemotePort,
  IN UINT32      RemoteIp,
  IN BOOLEAN     Syn
  );

INTN
TcpInsertTcb (
  IN TCP_CB *Tcb
  );

TCP_CB  *
TcpCloneTcb (
  IN TCP_CB *Tcb
  );

TCP_SEQNO
TcpGetIss (
  VOID
  );

VOID
TcpInitTcbLocal (
  IN TCP_CB *Tcb
  );

VOID
TcpInitTcbPeer (
  IN TCP_CB     *Tcb,
  IN TCP_SEG    *Seg,
  IN TCP_OPTION *Opt
  );

UINT16
TcpGetRcvMss (
  IN SOCKET *Sock
  );

VOID
TcpSetState (
  IN TCP_CB *Tcb,
  IN UINT8  State
  );

//
// Functions in Tcp4Output.c
//
INTN
TcpSendIpPacket (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf,
  IN UINT32  Src,
  IN UINT32  Dst
  );

INTN
TcpToSendData (
  IN TCP_CB *Tcb,
  IN INTN    Force
  );

VOID
TcpToSendAck (
  IN TCP_CB *Tcb
  );

VOID
TcpSendAck (
  IN TCP_CB *Tcb
  );

INTN
TcpSendZeroProbe (
  IN TCP_CB *Tcb
  );

INTN
TcpDeliverData (
  IN TCP_CB *Tcb
  );

INTN
TcpSendReset (
  IN TCP_CB   *Tcb,
  IN TCP_HEAD *Head,
  IN INT32    Len,
  IN UINT32   Local,
  IN UINT32   Remote
  );

UINT32
TcpRcvWinOld (
  IN TCP_CB *Tcb
  );

UINT32
TcpRcvWinNow (
  IN TCP_CB *Tcb
  );

INTN
TcpRetransmit (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq
  );

UINT32
TcpDataToSend (
  IN TCP_CB *Tcb,
  IN INTN   Force
  );

INTN
TcpVerifySegment (
  IN NET_BUF *Nbuf
  );

INTN
TcpCheckSndQue (
  IN LIST_ENTRY     *Head
  );

NET_BUF *
TcpGetSegmentSndQue (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  );

NET_BUF *
TcpGetSegmentSock (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  );

NET_BUF *
TcpGetSegment (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  );

TCP_SEQNO
TcpGetMaxSndNxt (
  IN TCP_CB *Tcb
  );

//
// Functions from Tcp4Input.c
//
VOID
TcpIcmpInput (
  IN NET_BUF     *Nbuf,
  IN ICMP_ERROR  IcmpErr,
  IN UINT32      Src,
  IN UINT32      Dst
  );

INTN
TcpInput (
  IN NET_BUF *Nbuf,
  IN UINT32  Src,
  IN UINT32  Dst
  );

INTN
TcpSeqAcceptable (
  IN TCP_CB  *Tcb,
  IN TCP_SEG *Seg
  );

VOID
TcpFastRecover (
  IN TCP_CB  *Tcb,
  IN TCP_SEG *Seg
  );

VOID
TcpFastLossRecover (
  IN TCP_CB  *Tcb,
  IN TCP_SEG *Seg
  );

VOID
TcpComputeRtt (
  IN TCP_CB *Tcb,
  IN UINT32 Measure
  );

INTN
TcpTrimInWnd (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Buf
  );

VOID
TcpQueueData (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  );

VOID
TcpAdjustSndQue (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Ack
  );

//
// Functions from Tcp4Misc.c
//
UINT16
TcpChecksum (
  IN NET_BUF *Buf,
  IN UINT16  HeadChecksum
  );

TCP_SEG *
TcpFormatNetbuf (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  );

VOID
TcpOnAppConnect (
  IN TCP_CB  *Tcb
  );

INTN
TcpOnAppConsume (
  IN TCP_CB *Tcb
  );

VOID
TcpOnAppClose (
  IN TCP_CB *Tcb
  );

INTN
TcpOnAppSend (
  IN TCP_CB *Tcb
  );

VOID
TcpOnAppAbort (
  IN TCP_CB *Tcb
  );

VOID
TcpResetConnection (
  IN TCP_CB *Tcb
  );

//
// Functions in Tcp4Timer.c
//
VOID
TcpClose (
  IN TCP_CB *Tcb
  );

VOID
EFIAPI
TcpTicking (
  IN EFI_EVENT Event,
  IN VOID      *Context
  );

VOID
TcpSetTimer (
  IN TCP_CB *Tcb,
  IN UINT16 Timer,
  IN UINT32 TimeOut
  );

VOID
TcpClearTimer (
  IN TCP_CB *Tcb,
  IN UINT16 Timer
  );

VOID
TcpClearAllTimer (
  IN TCP_CB *Tcb
  );

VOID
TcpSetProbeTimer (
  IN TCP_CB *Tcb
  );

VOID
TcpSetKeepaliveTimer (
  IN TCP_CB *Tcb
  );

VOID
TcpBackoffRto (
  IN TCP_CB *Tcb
  );

EFI_STATUS
TcpSetVariableData (
  IN TCP4_SERVICE_DATA  *Tcp4Service
  );

VOID
TcpClearVariableData (
  IN TCP4_SERVICE_DATA  *Tcp4Service
  );

EFI_STATUS
TcpInstallDevicePath (
  IN SOCKET  *Sock
  );

#endif
