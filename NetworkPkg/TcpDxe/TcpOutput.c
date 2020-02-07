/** @file
  TCP output process routines.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TcpMain.h"

UINT8  mTcpOutFlag[] = {
  0,                          // TCP_CLOSED
  0,                          // TCP_LISTEN
  TCP_FLG_SYN,                // TCP_SYN_SENT
  TCP_FLG_SYN | TCP_FLG_ACK,  // TCP_SYN_RCVD
  TCP_FLG_ACK,                // TCP_ESTABLISHED
  TCP_FLG_FIN | TCP_FLG_ACK,  // TCP_FIN_WAIT_1
  TCP_FLG_ACK,                // TCP_FIN_WAIT_2
  TCP_FLG_ACK | TCP_FLG_FIN,  // TCP_CLOSING
  TCP_FLG_ACK,                // TCP_TIME_WAIT
  TCP_FLG_ACK,                // TCP_CLOSE_WAIT
  TCP_FLG_FIN | TCP_FLG_ACK   // TCP_LAST_ACK
};

/**
  Compute the sequence space left in the old receive window.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The sequence space left in the old receive window.

**/
UINT32
TcpRcvWinOld (
  IN TCP_CB *Tcb
  )
{
  UINT32  OldWin;

  OldWin = 0;

  if (TCP_SEQ_GT (Tcb->RcvWl2 + Tcb->RcvWnd, Tcb->RcvNxt)) {

    OldWin = TCP_SUB_SEQ (
              Tcb->RcvWl2 + Tcb->RcvWnd,
              Tcb->RcvNxt
              );
  }

  return OldWin;
}

/**
  Compute the current receive window.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The size of the current receive window, in bytes.

**/
UINT32
TcpRcvWinNow (
  IN TCP_CB *Tcb
  )
{
  SOCKET  *Sk;
  UINT32  Win;
  UINT32  Increase;
  UINT32  OldWin;

  Sk = Tcb->Sk;
  ASSERT (Sk != NULL);

  OldWin    = TcpRcvWinOld (Tcb);

  Win       = SockGetFreeSpace (Sk, SOCK_RCV_BUF);

  Increase  = 0;
  if (Win > OldWin) {
    Increase = Win - OldWin;
  }

  //
  // Receiver's SWS: don't advertise a bigger window
  // unless it can be increased by at least one Mss or
  // half of the receive buffer.
  //
  if ((Increase > Tcb->SndMss) || (2 * Increase >= GET_RCV_BUFFSIZE (Sk))) {

    return Win;
  }

  return OldWin;
}

/**
  Compute the value to fill in the window size field of the outgoing segment.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]       Syn     The flag to indicate whether the outgoing segment
                           is a SYN segment.

  @return The value of the local receive window size used to fill the outgoing segment.

**/
UINT16
TcpComputeWnd (
  IN OUT TCP_CB  *Tcb,
  IN     BOOLEAN Syn
  )
{
  UINT32  Wnd;

  //
  // RFC requires that initial window not be scaled
  //
  if (Syn) {

    Wnd = GET_RCV_BUFFSIZE (Tcb->Sk);
  } else {

    Wnd         = TcpRcvWinNow (Tcb);

    Tcb->RcvWnd = Wnd;
  }

  Wnd = MIN (Wnd >> Tcb->RcvWndScale, 0xffff);
  return NTOHS ((UINT16) Wnd);
}

/**
  Get the maximum SndNxt.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return The sequence number of the maximum SndNxt.

**/
TCP_SEQNO
TcpGetMaxSndNxt (
  IN TCP_CB *Tcb
  )
{
  LIST_ENTRY      *Entry;
  NET_BUF         *Nbuf;

  if (IsListEmpty (&Tcb->SndQue)) {
    return Tcb->SndNxt;
  }

  Entry = Tcb->SndQue.BackLink;
  Nbuf  = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);

  ASSERT (TCP_SEQ_GEQ (TCPSEG_NETBUF (Nbuf)->End, Tcb->SndNxt));
  return TCPSEG_NETBUF (Nbuf)->End;
}

/**
  Compute how much data to send.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Force   If TRUE, to ignore the sender's SWS avoidance algorithm and send
                      out data by force.

  @return The length of the data can be sent. If 0, no data can be sent.

**/
UINT32
TcpDataToSend (
  IN TCP_CB *Tcb,
  IN INTN   Force
  )
{
  SOCKET  *Sk;
  UINT32  Win;
  UINT32  Len;
  UINT32  Left;
  UINT32  Limit;

  Sk = Tcb->Sk;
  ASSERT (Sk != NULL);

  //
  // TCP should NOT send data beyond the send window
  // and congestion window. The right edge of send
  // window is defined as SND.WL2 + SND.WND. The right
  // edge of congestion window is defined as SND.UNA +
  // CWND.
  //
  Win   = 0;
  Limit = Tcb->SndWl2 + Tcb->SndWnd;

  if (TCP_SEQ_GT (Limit, Tcb->SndUna + Tcb->CWnd)) {

    Limit = Tcb->SndUna + Tcb->CWnd;
  }

  if (TCP_SEQ_GT (Limit, Tcb->SndNxt)) {
    Win = TCP_SUB_SEQ (Limit, Tcb->SndNxt);
  }

  //
  // The data to send contains two parts: the data on the
  // socket send queue, and the data on the TCB's send
  // buffer. The later can be non-zero if the peer shrinks
  // its advertised window.
  //
  Left  = GET_SND_DATASIZE (Sk) + TCP_SUB_SEQ (TcpGetMaxSndNxt (Tcb), Tcb->SndNxt);

  Len   = MIN (Win, Left);

  if (Len > Tcb->SndMss) {
    Len = Tcb->SndMss;
  }

  if ((Force != 0)|| (Len == 0 && Left == 0)) {
    return Len;
  }

  if (Len == 0 && Left != 0) {
    goto SetPersistTimer;
  }

  //
  // Sender's SWS avoidance: Don't send a small segment unless
  // a)A full-sized segment can be sent,
  // b)At least one-half of the maximum sized windows that
  // the other end has ever advertised.
  // c)It can send everything it has, and either it isn't
  // expecting an ACK, or the Nagle algorithm is disabled.
  //
  if ((Len == Tcb->SndMss) || (2 * Len >= Tcb->SndWndMax)) {

    return Len;
  }

  if ((Len == Left) &&
      ((Tcb->SndNxt == Tcb->SndUna) || TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_NAGLE))
      ) {

    return Len;
  }

  //
  // RFC1122 suggests to set a timer when SWSA forbids TCP
  // sending more data, and combines it with a probe timer.
  //
SetPersistTimer:
  if (!TCP_TIMER_ON (Tcb->EnabledTimer, TCP_TIMER_REXMIT)) {

    DEBUG (
      (EFI_D_WARN,
      "TcpDataToSend: enter persistent state for TCB %p\n",
      Tcb)
      );

    if (!Tcb->ProbeTimerOn) {
      TcpSetProbeTimer (Tcb);
    }
  }

  return 0;
}

/**
  Build the TCP header of the TCP segment and transmit the segment by IP.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]       Nbuf    Pointer to the buffer containing the segment to be
                           sent out.

  @retval 0       The segment was sent out successfully.
  @retval -1      An error condition occurred.

**/
INTN
TcpTransmitSegment (
  IN OUT TCP_CB  *Tcb,
  IN     NET_BUF *Nbuf
  )
{
  UINT16    Len;
  TCP_HEAD  *Head;
  TCP_SEG   *Seg;
  BOOLEAN   Syn;
  UINT32    DataLen;

  ASSERT ((Nbuf != NULL) && (Nbuf->Tcp == NULL));

  if (TcpVerifySegment (Nbuf) == 0) {
    return -1;
  }

  DataLen = Nbuf->TotalSize;

  Seg     = TCPSEG_NETBUF (Nbuf);
  Syn     = TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN);

  if (Syn) {

    Len = TcpSynBuildOption (Tcb, Nbuf);
  } else {

    Len = TcpBuildOption (Tcb, Nbuf);
  }

  ASSERT ((Len % 4 == 0) && (Len <= 40));

  Len += sizeof (TCP_HEAD);

  Head = (TCP_HEAD *) NetbufAllocSpace (
                        Nbuf,
                        sizeof (TCP_HEAD),
                        NET_BUF_HEAD
                        );

  ASSERT (Head != NULL);

  Nbuf->Tcp       = Head;

  Head->SrcPort   = Tcb->LocalEnd.Port;
  Head->DstPort   = Tcb->RemoteEnd.Port;
  Head->Seq       = NTOHL (Seg->Seq);
  Head->Ack       = NTOHL (Tcb->RcvNxt);
  Head->HeadLen   = (UINT8) (Len >> 2);
  Head->Res       = 0;
  Head->Wnd       = TcpComputeWnd (Tcb, Syn);
  Head->Checksum  = 0;

  //
  // Check whether to set the PSH flag.
  //
  TCP_CLEAR_FLG (Seg->Flag, TCP_FLG_PSH);

  if (DataLen != 0) {
    if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_SND_PSH) &&
        TCP_SEQ_BETWEEN (Seg->Seq, Tcb->SndPsh, Seg->End)
        ) {

      TCP_SET_FLG (Seg->Flag, TCP_FLG_PSH);
      TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_SND_PSH);

    } else if ((Seg->End == Tcb->SndNxt) && (GET_SND_DATASIZE (Tcb->Sk) == 0)) {

      TCP_SET_FLG (Seg->Flag, TCP_FLG_PSH);
    }
  }

  //
  // Check whether to set the URG flag and the urgent pointer.
  //
  TCP_CLEAR_FLG (Seg->Flag, TCP_FLG_URG);

  if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_SND_URG) && TCP_SEQ_LEQ (Seg->Seq, Tcb->SndUp)) {

    TCP_SET_FLG (Seg->Flag, TCP_FLG_URG);

    if (TCP_SEQ_LT (Tcb->SndUp, Seg->End)) {

      Seg->Urg = (UINT16) TCP_SUB_SEQ (Tcb->SndUp, Seg->Seq);
    } else {

      Seg->Urg = (UINT16) MIN (
                            TCP_SUB_SEQ (Tcb->SndUp,
                            Seg->Seq),
                            0xffff
                            );
    }
  }

  Head->Flag      = Seg->Flag;
  Head->Urg       = NTOHS (Seg->Urg);
  Head->Checksum  = TcpChecksum (Nbuf, Tcb->HeadSum);

  //
  // Update the TCP session's control information.
  //
  Tcb->RcvWl2 = Tcb->RcvNxt;
  if (Syn) {
    Tcb->RcvWnd = NTOHS (Head->Wnd);
  }

  //
  // Clear the delayedack flag.
  //
  Tcb->DelayedAck = 0;

  return TcpSendIpPacket (Tcb, Nbuf, &Tcb->LocalEnd.Ip, &Tcb->RemoteEnd.Ip, Tcb->Sk->IpVersion);
}

/**
  Get a segment from the Tcb's SndQue.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Seq     The sequence number of the segment.
  @param[in]  Len     The maximum length of the segment.

  @return Pointer to the segment. If NULL, some error occurred.

**/
NET_BUF *
TcpGetSegmentSndQue (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  )
{
  LIST_ENTRY      *Head;
  LIST_ENTRY      *Cur;
  NET_BUF         *Node;
  TCP_SEG         *Seg;
  NET_BUF         *Nbuf;
  TCP_SEQNO       End;
  UINT8           *Data;
  UINT8           Flag;
  INT32           Offset;
  INT32           CopyLen;

  ASSERT ((Tcb != NULL) && TCP_SEQ_LEQ (Seq, Tcb->SndNxt) && (Len > 0));

  //
  // Find the segment that contains the Seq.
  //
  Head  = &Tcb->SndQue;

  Node  = NULL;
  Seg   = NULL;

  NET_LIST_FOR_EACH (Cur, Head) {
    Node  = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);
    Seg   = TCPSEG_NETBUF (Node);

    if (TCP_SEQ_LT (Seq, Seg->End) && TCP_SEQ_LEQ (Seg->Seq, Seq)) {

      break;
    }
  }

  if ((Cur == Head) || (Seg == NULL) || (Node == NULL)) {
    return NULL;
  }

  //
  // Return the buffer if it can be returned without
  // adjustment:
  //
  if ((Seg->Seq == Seq) &&
      TCP_SEQ_LEQ (Seg->End, Seg->Seq + Len) &&
      !NET_BUF_SHARED (Node)
      ) {

    NET_GET_REF (Node);
    return Node;
  }

  //
  // Create a new buffer and copy data there.
  //
  Nbuf = NetbufAlloc (Len + TCP_MAX_HEAD);

  if (Nbuf == NULL) {
    return NULL;
  }

  NetbufReserve (Nbuf, TCP_MAX_HEAD);

  Flag  = Seg->Flag;
  End   = Seg->End;

  if (TCP_SEQ_LT (Seq + Len, Seg->End)) {
    End = Seq + Len;
  }

  CopyLen = TCP_SUB_SEQ (End, Seq);
  Offset  = TCP_SUB_SEQ (Seq, Seg->Seq);

  //
  // If SYN is set and out of the range, clear the flag.
  // Because the sequence of the first byte is SEG.SEQ+1,
  // adjust Offset by -1. If SYN is in the range, copy
  // one byte less.
  //
  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN)) {

    if (TCP_SEQ_LT (Seg->Seq, Seq)) {

      TCP_CLEAR_FLG (Flag, TCP_FLG_SYN);
      Offset--;
    } else {

      CopyLen--;
    }
  }

  //
  // If FIN is set and in the range, copy one byte less,
  // and if it is out of the range, clear the flag.
  //
  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_FIN)) {

    if (Seg->End == End) {

      CopyLen--;
    } else {

      TCP_CLEAR_FLG (Flag, TCP_FLG_FIN);
    }
  }

  ASSERT (CopyLen >= 0);

  //
  // Copy data to the segment
  //
  if (CopyLen != 0) {
    Data = NetbufAllocSpace (Nbuf, CopyLen, NET_BUF_TAIL);
    ASSERT (Data != NULL);

    if ((INT32) NetbufCopy (Node, Offset, CopyLen, Data) != CopyLen) {
      goto OnError;
    }
  }

  CopyMem (TCPSEG_NETBUF (Nbuf), Seg, sizeof (TCP_SEG));

  TCPSEG_NETBUF (Nbuf)->Seq   = Seq;
  TCPSEG_NETBUF (Nbuf)->End   = End;
  TCPSEG_NETBUF (Nbuf)->Flag  = Flag;

  return Nbuf;

OnError:
  NetbufFree (Nbuf);
  return NULL;
}

/**
  Get a segment from the Tcb's socket buffer.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Seq     The sequence number of the segment.
  @param[in]  Len     The maximum length of the segment.

  @return Pointer to the segment. If NULL, some error occurred.

**/
NET_BUF *
TcpGetSegmentSock (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  )
{
  NET_BUF *Nbuf;
  UINT8   *Data;
  UINT32  DataGet;

  ASSERT ((Tcb != NULL) && (Tcb->Sk != NULL));

  Nbuf = NetbufAlloc (Len + TCP_MAX_HEAD);

  if (Nbuf == NULL) {
    DEBUG (
      (EFI_D_ERROR,
      "TcpGetSegmentSock: failed to allocate a netbuf for TCB %p\n",
      Tcb)
      );

    return NULL;
  }

  NetbufReserve (Nbuf, TCP_MAX_HEAD);

  DataGet = 0;

  if (Len != 0) {
    //
    // copy data to the segment.
    //
    Data = NetbufAllocSpace (Nbuf, Len, NET_BUF_TAIL);
    ASSERT (Data != NULL);

    DataGet = SockGetDataToSend (Tcb->Sk, 0, Len, Data);
  }

  NET_GET_REF (Nbuf);

  TCPSEG_NETBUF (Nbuf)->Seq = Seq;
  TCPSEG_NETBUF (Nbuf)->End = Seq + Len;

  InsertTailList (&(Tcb->SndQue), &(Nbuf->List));

  if (DataGet != 0) {

    SockDataSent (Tcb->Sk, DataGet);
  }

  return Nbuf;
}

/**
  Get a segment starting from sequence Seq of a maximum
  length of Len.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Seq     The sequence number of the segment.
  @param[in]  Len     The maximum length of the segment.

  @return Pointer to the segment. If NULL, some error occurred.

**/
NET_BUF *
TcpGetSegment (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq,
  IN UINT32    Len
  )
{
  NET_BUF *Nbuf;

  ASSERT (Tcb != NULL);

  //
  // Compare the SndNxt with the max sequence number sent.
  //
  if ((Len != 0) && TCP_SEQ_LT (Seq, TcpGetMaxSndNxt (Tcb))) {

    Nbuf = TcpGetSegmentSndQue (Tcb, Seq, Len);
  } else {

    Nbuf = TcpGetSegmentSock (Tcb, Seq, Len);
  }

  if (TcpVerifySegment (Nbuf) == 0) {
    NetbufFree (Nbuf);
    return NULL;
  }

  return Nbuf;
}

/**
  Retransmit the segment from sequence Seq.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Seq     The sequence number of the segment to be retransmitted.

  @retval 0       Retransmission succeeded.
  @retval -1      Error condition occurred.

**/
INTN
TcpRetransmit (
  IN TCP_CB    *Tcb,
  IN TCP_SEQNO Seq
  )
{
  NET_BUF *Nbuf;
  UINT32  Len;

  //
  // Compute the maximum length of retransmission. It is
  // limited by three factors:
  // 1. Less than SndMss
  // 2. Must in the current send window
  // 3. Will not change the boundaries of queued segments.
  //

  //
  // Handle the Window Retraction if TCP window scale is enabled according to RFC7323:
  //   On first retransmission, or if the sequence number is out of
  //   window by less than 2^Rcv.Wind.Shift, then do normal
  //   retransmission(s) without regard to the receiver window as long
  //   as the original segment was in window when it was sent.
  //
  if ((Tcb->SndWndScale != 0) &&
      (TCP_SEQ_GT (Seq, Tcb->RetxmitSeqMax) || TCP_SEQ_BETWEEN (Tcb->SndWl2 + Tcb->SndWnd, Seq, Tcb->SndWl2 + Tcb->SndWnd + (1 << Tcb->SndWndScale)))) {
    Len = TCP_SUB_SEQ (Tcb->SndNxt, Seq);
    DEBUG (
      (EFI_D_WARN,
      "TcpRetransmit: retransmission without regard to the receiver window for TCB %p\n",
      Tcb)
      );

  } else if (TCP_SEQ_GEQ (Tcb->SndWl2 + Tcb->SndWnd, Seq)) {
    Len = TCP_SUB_SEQ (Tcb->SndWl2 + Tcb->SndWnd, Seq);

  } else {
    DEBUG (
      (EFI_D_WARN,
      "TcpRetransmit: retransmission cancelled because send window too small for TCB %p\n",
      Tcb)
      );

    return 0;
  }

  Len = MIN (Len, Tcb->SndMss);

  Nbuf = TcpGetSegmentSndQue (Tcb, Seq, Len);
  if (Nbuf == NULL) {
    return -1;
  }

  if (TcpVerifySegment (Nbuf) == 0) {
    goto OnError;
  }

  if (TcpTransmitSegment (Tcb, Nbuf) != 0) {
    goto OnError;
  }

  if (TCP_SEQ_GT (Seq, Tcb->RetxmitSeqMax)) {
    Tcb->RetxmitSeqMax = Seq;
  }

  //
  // The retransmitted buffer may be on the SndQue,
  // trim TCP head because all the buffers on SndQue
  // are headless.
  //
  ASSERT (Nbuf->Tcp != NULL);
  NetbufTrim (Nbuf, (Nbuf->Tcp->HeadLen << 2), NET_BUF_HEAD);
  Nbuf->Tcp = NULL;

  NetbufFree (Nbuf);
  return 0;

OnError:
  if (Nbuf != NULL) {
    NetbufFree (Nbuf);
  }

  return -1;
}

/**
  Verify that all the segments in SndQue are in good shape.

  @param[in]  Head    Pointer to the head node of the SndQue.

  @retval     0       At least one segment is broken.
  @retval     1       All segments in the specific queue are in good shape.

**/
INTN
TcpCheckSndQue (
  IN LIST_ENTRY     *Head
  )
{
  LIST_ENTRY      *Entry;
  NET_BUF         *Nbuf;
  TCP_SEQNO       Seq;

  if (IsListEmpty (Head)) {
    return 1;
  }
  //
  // Initialize the Seq.
  //
  Entry = Head->ForwardLink;
  Nbuf  = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);
  Seq   = TCPSEG_NETBUF (Nbuf)->Seq;

  NET_LIST_FOR_EACH (Entry, Head) {
    Nbuf = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);

    if (TcpVerifySegment (Nbuf) == 0) {
      return 0;
    }

    //
    // All the node in the SndQue should has:
    // SEG.SEQ = LAST_SEG.END
    //
    if (Seq != TCPSEG_NETBUF (Nbuf)->Seq) {
      return 0;
    }

    Seq = TCPSEG_NETBUF (Nbuf)->End;
  }

  return 1;
}

/**
  Check whether to send data/SYN/FIN and piggyback an ACK.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]       Force   If TRUE, ignore the sender's SWS avoidance algorithm
                           and send out data by force.

  @return The number of bytes sent.

**/
INTN
TcpToSendData (
  IN OUT TCP_CB *Tcb,
  IN     INTN   Force
  )
{
  UINT32    Len;
  INTN      Sent;
  UINT8     Flag;
  NET_BUF   *Nbuf;
  TCP_SEG   *Seg;
  TCP_SEQNO Seq;
  TCP_SEQNO End;

  ASSERT ((Tcb != NULL) && (Tcb->Sk != NULL) && (Tcb->State != TCP_LISTEN));

  Sent = 0;

  if ((Tcb->State == TCP_CLOSED) || TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_FIN_SENT)) {

    return 0;
  }

  do {
    //
    // Compute how much data can be sent
    //
    Len   = TcpDataToSend (Tcb, Force);
    Seq   = Tcb->SndNxt;

    ASSERT ((Tcb->State) < (ARRAY_SIZE (mTcpOutFlag)));
    Flag  = mTcpOutFlag[Tcb->State];

    if ((Flag & TCP_FLG_SYN) != 0) {

      Seq = Tcb->Iss;
      Len = 0;
    }

    //
    // Only send a segment without data if SYN or
    // FIN is set.
    //
    if ((Len == 0) && ((Flag & (TCP_FLG_SYN | TCP_FLG_FIN)) == 0)) {
      return Sent;
    }

    Nbuf = TcpGetSegment (Tcb, Seq, Len);

    if (Nbuf == NULL) {
      DEBUG (
        (EFI_D_ERROR,
        "TcpToSendData: failed to get a segment for TCB %p\n",
        Tcb)
        );

      goto OnError;
    }

    Seg = TCPSEG_NETBUF (Nbuf);

    //
    // Set the TcpSeg in Nbuf.
    //
    Len = Nbuf->TotalSize;
    End = Seq + Len;
    if (TCP_FLG_ON (Flag, TCP_FLG_SYN)) {
      End++;
    }

    if ((Flag & TCP_FLG_FIN) != 0) {
      //
      // Send FIN if all data is sent, and FIN is
      // in the window
      //
      if ((TcpGetMaxSndNxt (Tcb) == Tcb->SndNxt) &&
          (GET_SND_DATASIZE (Tcb->Sk) == 0) &&
          TCP_SEQ_LT (End + 1, Tcb->SndWnd + Tcb->SndWl2)
            ) {
        DEBUG (
          (EFI_D_NET,
          "TcpToSendData: send FIN to peer for TCB %p in state %s\n",
          Tcb,
          mTcpStateName[Tcb->State])
          );

        End++;
      } else {
        TCP_CLEAR_FLG (Flag, TCP_FLG_FIN);
      }
    }

    Seg->Seq  = Seq;
    Seg->End  = End;
    Seg->Flag = Flag;

    if (TcpVerifySegment (Nbuf) == 0 || TcpCheckSndQue (&Tcb->SndQue) == 0) {
      DEBUG (
        (EFI_D_ERROR,
        "TcpToSendData: discard a broken segment for TCB %p\n",
        Tcb)
        );
      goto OnError;
    }

    //
    // Don't send an empty segment here.
    //
    if (Seg->End == Seg->Seq) {
      DEBUG (
        (EFI_D_WARN,
        "TcpToSendData: created a empty segment for TCB %p, free it now\n",
        Tcb)
        );

      goto OnError;
    }

    if (TcpTransmitSegment (Tcb, Nbuf) != 0) {
      NetbufTrim (Nbuf, (Nbuf->Tcp->HeadLen << 2), NET_BUF_HEAD);
      Nbuf->Tcp = NULL;

      if ((Flag & TCP_FLG_FIN) != 0)  {
        TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_FIN_SENT);
      }

      goto OnError;
    }

    Sent += TCP_SUB_SEQ (End, Seq);

    //
    // All the buffers in the SndQue are headless.
    //
    ASSERT (Nbuf->Tcp != NULL);

    NetbufTrim (Nbuf, (Nbuf->Tcp->HeadLen << 2), NET_BUF_HEAD);
    Nbuf->Tcp = NULL;

    NetbufFree (Nbuf);

    //
    // Update the status in TCB.
    //
    Tcb->DelayedAck = 0;

    if ((Flag & TCP_FLG_FIN) != 0) {
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_FIN_SENT);
    }

    if (TCP_SEQ_GT (End, Tcb->SndNxt)) {
      Tcb->SndNxt = End;
    }

    if (!TCP_TIMER_ON (Tcb->EnabledTimer, TCP_TIMER_REXMIT)) {
      TcpSetTimer (Tcb, TCP_TIMER_REXMIT, Tcb->Rto);
    }

    //
    // Enable RTT measurement only if not in retransmit.
    // Karn's algorithm requires not to update RTT when in loss.
    //
    if ((Tcb->CongestState == TCP_CONGEST_OPEN) && !TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_RTT_ON)) {

      DEBUG (
        (EFI_D_NET,
        "TcpToSendData: set RTT measure sequence %d for TCB %p\n",
        Seq,
        Tcb)
        );

      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_RTT_ON);
      Tcb->RttSeq     = Seq;
      Tcb->RttMeasure = 0;
    }

  } while (Len == Tcb->SndMss);

  return Sent;

OnError:
  if (Nbuf != NULL) {
    NetbufFree (Nbuf);
  }

  return Sent;
}

/**
  Send an ACK immediately.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSendAck (
  IN OUT TCP_CB *Tcb
  )
{
  NET_BUF *Nbuf;
  TCP_SEG *Seg;

  Nbuf = NetbufAlloc (TCP_MAX_HEAD);

  if (Nbuf == NULL) {
    return;
  }

  NetbufReserve (Nbuf, TCP_MAX_HEAD);

  Seg       = TCPSEG_NETBUF (Nbuf);
  Seg->Seq  = Tcb->SndNxt;
  Seg->End  = Tcb->SndNxt;
  Seg->Flag = TCP_FLG_ACK;

  if (TcpTransmitSegment (Tcb, Nbuf) == 0) {
    TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW);
    Tcb->DelayedAck = 0;
  }

  NetbufFree (Nbuf);
}

/**
  Send a zero probe segment. It can be used by keepalive and zero window probe.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.

  @retval 0       The zero probe segment was sent out successfully.
  @retval other   An error condition occurred.

**/
INTN
TcpSendZeroProbe (
  IN OUT TCP_CB *Tcb
  )
{
  NET_BUF *Nbuf;
  TCP_SEG *Seg;
  INTN     Result;

  Nbuf = NetbufAlloc (TCP_MAX_HEAD);

  if (Nbuf == NULL) {
    return -1;
  }

  NetbufReserve (Nbuf, TCP_MAX_HEAD);

  //
  // SndNxt-1 is out of window. The peer should respond
  // with an ACK.
  //
  Seg       = TCPSEG_NETBUF (Nbuf);
  Seg->Seq  = Tcb->SndNxt - 1;
  Seg->End  = Tcb->SndNxt - 1;
  Seg->Flag = TCP_FLG_ACK;

  Result    = TcpTransmitSegment (Tcb, Nbuf);
  NetbufFree (Nbuf);

  return Result;
}

/**
  Check whether to send an ACK or delayed ACK.

  @param[in, out]  Tcb     Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpToSendAck (
  IN OUT TCP_CB *Tcb
  )
{
  UINT32 TcpNow;

  //
  // Generally, TCP should send a delayed ACK unless:
  //   1. ACK at least every other FULL sized segment received.
  //   2. Packets received out of order.
  //   3. Receiving window is open.
  //
  if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW) || (Tcb->DelayedAck >= 1)) {
    TcpSendAck (Tcb);
    return;
  }

  TcpNow = TcpRcvWinNow (Tcb);

  if (TcpNow > TcpRcvWinOld (Tcb)) {
    TcpSendAck (Tcb);
    return;
  }

  DEBUG (
    (EFI_D_NET,
    "TcpToSendAck: scheduled a delayed ACK for TCB %p\n",
    Tcb)
    );

  //
  // Schedule a delayed ACK.
  //
  Tcb->DelayedAck++;
}

/**
  Send a RESET segment in response to the segment received.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance. May be NULL.
  @param[in]  Head    TCP header of the segment that triggers the reset.
  @param[in]  Len     Length of the segment that triggers the reset.
  @param[in]  Local   Local IP address.
  @param[in]  Remote  Remote peer's IP address.
  @param[in]  Version IP_VERSION_4 indicates TCP is running on IP4 stack,
                      IP_VERSION_6 indicates TCP is running on IP6 stack.

  @retval     0       A reset was sent or there is no need to send it.
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
  )
{
  NET_BUF   *Nbuf;
  TCP_HEAD  *Nhead;
  UINT16    HeadSum;

  //
  // Don't respond to a Reset with reset.
  //
  if ((Head->Flag & TCP_FLG_RST) != 0) {
    return 0;
  }

  Nbuf = NetbufAlloc (TCP_MAX_HEAD);

  if (Nbuf == NULL) {
    return -1;
  }

  Nhead = (TCP_HEAD *) NetbufAllocSpace (
                        Nbuf,
                        sizeof (TCP_HEAD),
                        NET_BUF_TAIL
                        );

  ASSERT (Nhead != NULL);

  Nbuf->Tcp   = Nhead;
  Nhead->Flag = TCP_FLG_RST;

  //
  // Derive Seq/ACK from the segment if no TCB
  // is associated with it, otherwise derive from the Tcb.
  //
  if (Tcb == NULL) {

    if (TCP_FLG_ON (Head->Flag, TCP_FLG_ACK)) {
      Nhead->Seq  = Head->Ack;
      Nhead->Ack  = 0;
    } else {
      Nhead->Seq = 0;
      TCP_SET_FLG (Nhead->Flag, TCP_FLG_ACK);
      Nhead->Ack = HTONL (NTOHL (Head->Seq) + Len);
    }
  } else {

    Nhead->Seq  = HTONL (Tcb->SndNxt);
    Nhead->Ack  = HTONL (Tcb->RcvNxt);
    TCP_SET_FLG (Nhead->Flag, TCP_FLG_ACK);
  }

  Nhead->SrcPort  = Head->DstPort;
  Nhead->DstPort  = Head->SrcPort;
  Nhead->HeadLen  = (UINT8) (sizeof (TCP_HEAD) >> 2);
  Nhead->Res      = 0;
  Nhead->Wnd      = HTONS (0xFFFF);
  Nhead->Checksum = 0;
  Nhead->Urg      = 0;

  if (Version == IP_VERSION_4) {
    HeadSum = NetPseudoHeadChecksum (Local->Addr[0], Remote->Addr[0], 6, 0);
  } else {
    HeadSum = NetIp6PseudoHeadChecksum (&Local->v6, &Remote->v6, 6, 0);
  }

  Nhead->Checksum = TcpChecksum (Nbuf, HeadSum);

  TcpSendIpPacket (Tcb, Nbuf, Local, Remote, Version);

  NetbufFree (Nbuf);

  return 0;
}

/**
  Verify that the segment is in good shape.

  @param[in]  Nbuf    The buffer that contains the segment to be checked.

  @retval     0       The segment is broken.
  @retval     1       The segment is in good shape.

**/
INTN
TcpVerifySegment (
  IN NET_BUF *Nbuf
  )
{
  TCP_HEAD  *Head;
  TCP_SEG   *Seg;
  UINT32    Len;

  if (Nbuf == NULL) {
    return 1;
  }

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  Seg   = TCPSEG_NETBUF (Nbuf);
  Len   = Nbuf->TotalSize;
  Head  = Nbuf->Tcp;

  if (Head != NULL) {
    if (Head->Flag != Seg->Flag) {
      return 0;
    }

    Len -= (Head->HeadLen << 2);
  }

  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN)) {
    Len++;
  }

  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_FIN)) {
    Len++;
  }

  if (Seg->Seq + Len != Seg->End) {
    return 0;
  }

  return 1;
}

