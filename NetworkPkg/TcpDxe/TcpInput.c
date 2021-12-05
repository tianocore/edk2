/** @file
  TCP input process routines.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TcpMain.h"

/**
  Check whether the sequence number of the incoming segment is acceptable.

  @param[in]  Tcb Pointer to the TCP_CB of this TCP instance.
  @param[in]  Seg Pointer to the incoming segment.

  @retval 1       The sequence number is acceptable.
  @retval 0       The sequence number is not acceptable.

**/
INTN
TcpSeqAcceptable (
  IN TCP_CB   *Tcb,
  IN TCP_SEG  *Seg
  )
{
  return (TCP_SEQ_LEQ (Tcb->RcvNxt, Seg->End) &&
          TCP_SEQ_LT (Seg->Seq, Tcb->RcvWl2 + Tcb->RcvWnd));
}

/**
  NewReno fast recovery defined in RFC3782.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]       Seg      Segment that triggers the fast recovery.

**/
VOID
TcpFastRecover (
  IN OUT TCP_CB   *Tcb,
  IN     TCP_SEG  *Seg
  )
{
  UINT32  FlightSize;
  UINT32  Acked;

  //
  // Step 1: Three duplicate ACKs and not in fast recovery
  //
  if (Tcb->CongestState != TCP_CONGEST_RECOVER) {
    //
    // Step 1A: Invoking fast retransmission.
    //
    FlightSize = TCP_SUB_SEQ (Tcb->SndNxt, Tcb->SndUna);

    Tcb->Ssthresh = MAX (FlightSize >> 1, (UINT32)(2 * Tcb->SndMss));
    Tcb->Recover  = Tcb->SndNxt;

    Tcb->CongestState = TCP_CONGEST_RECOVER;
    TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_RTT_ON);

    //
    // Step 2: Entering fast retransmission
    //
    TcpRetransmit (Tcb, Tcb->SndUna);
    Tcb->CWnd = Tcb->Ssthresh + 3 * Tcb->SndMss;

    DEBUG (
      (DEBUG_NET,
       "TcpFastRecover: enter fast retransmission for TCB %p, recover point is %d\n",
       Tcb,
       Tcb->Recover)
      );
    return;
  }

  //
  // During fast recovery, execute Step 3, 4, 5 of RFC3782
  //
  if (Seg->Ack == Tcb->SndUna) {
    //
    // Step 3: Fast Recovery,
    // If this is a duplicated ACK, increse Cwnd by SMSS.
    //

    // Step 4 is skipped here only to be executed later
    // by TcpToSendData
    //
    Tcb->CWnd += Tcb->SndMss;
    DEBUG (
      (DEBUG_NET,
       "TcpFastRecover: received another duplicated ACK (%d) for TCB %p\n",
       Seg->Ack,
       Tcb)
      );
  } else {
    //
    // New data is ACKed, check whether it is a
    // full ACK or partial ACK
    //
    if (TCP_SEQ_GEQ (Seg->Ack, Tcb->Recover)) {
      //
      // Step 5 - Full ACK:
      // deflate the congestion window, and exit fast recovery
      //
      FlightSize = TCP_SUB_SEQ (Tcb->SndNxt, Tcb->SndUna);

      Tcb->CWnd = MIN (Tcb->Ssthresh, FlightSize + Tcb->SndMss);

      Tcb->CongestState = TCP_CONGEST_OPEN;
      DEBUG (
        (DEBUG_NET,
         "TcpFastRecover: received a full ACK(%d) for TCB %p, exit fast recovery\n",
         Seg->Ack,
         Tcb)
        );
    } else {
      //
      // Step 5 - Partial ACK:
      // fast retransmit the first unacknowledge field
      // , then deflate the CWnd
      //
      TcpRetransmit (Tcb, Seg->Ack);
      Acked = TCP_SUB_SEQ (Seg->Ack, Tcb->SndUna);

      //
      // Deflate the CWnd by the amount of new data
      // ACKed by SEG.ACK. If more than one SMSS data
      // is ACKed, add back SMSS byte to CWnd after
      //
      if (Acked >= Tcb->SndMss) {
        Acked -= Tcb->SndMss;
      }

      Tcb->CWnd -= Acked;

      DEBUG (
        (DEBUG_NET,
         "TcpFastRecover: received a partial ACK(%d) for TCB %p\n",
         Seg->Ack,
         Tcb)
        );
    }
  }
}

/**
  NewReno fast loss recovery defined in RFC3792.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]       Seg      Segment that triggers the fast loss recovery.

**/
VOID
TcpFastLossRecover (
  IN OUT TCP_CB   *Tcb,
  IN     TCP_SEG  *Seg
  )
{
  if (TCP_SEQ_GT (Seg->Ack, Tcb->SndUna)) {
    //
    // New data is ACKed, check whether it is a
    // full ACK or partial ACK
    //
    if (TCP_SEQ_GEQ (Seg->Ack, Tcb->LossRecover)) {
      //
      // Full ACK: exit the loss recovery.
      //
      Tcb->LossTimes    = 0;
      Tcb->CongestState = TCP_CONGEST_OPEN;

      DEBUG (
        (DEBUG_NET,
         "TcpFastLossRecover: received a full ACK(%d) for TCB %p\n",
         Seg->Ack,
         Tcb)
        );
    } else {
      //
      // Partial ACK:
      // fast retransmit the first unacknowledge field.
      //
      TcpRetransmit (Tcb, Seg->Ack);
      DEBUG (
        (DEBUG_NET,
         "TcpFastLossRecover: received a partial ACK(%d) for TCB %p\n",
         Seg->Ack,
         Tcb)
        );
    }
  }
}

/**
  Compute the RTT as specified in RFC2988.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]       Measure  Currently measured RTT in heartbeats.

**/
VOID
TcpComputeRtt (
  IN OUT TCP_CB  *Tcb,
  IN     UINT32  Measure
  )
{
  INT32  Var;

  //
  // Step 2.3: Compute the RTO for subsequent RTT measurement.
  //
  if (Tcb->SRtt != 0) {
    Var = Tcb->SRtt - (Measure << TCP_RTT_SHIFT);

    if (Var < 0) {
      Var = -Var;
    }

    Tcb->RttVar = (3 * Tcb->RttVar + Var) >> 2;
    Tcb->SRtt   = 7 * (Tcb->SRtt >> 3) + Measure;
  } else {
    //
    // Step 2.2: compute the first RTT measure
    //
    Tcb->SRtt   = Measure << TCP_RTT_SHIFT;
    Tcb->RttVar = Measure << (TCP_RTT_SHIFT - 1);
  }

  Tcb->Rto = (Tcb->SRtt + MAX (8, 4 * Tcb->RttVar)) >> TCP_RTT_SHIFT;

  //
  // Step 2.4: Limit the RTO to at least 1 second
  // Step 2.5: Limit the RTO to a maximum value that
  // is at least 60 second
  //
  if (Tcb->Rto < TCP_RTO_MIN) {
    Tcb->Rto = TCP_RTO_MIN;
  } else if (Tcb->Rto > TCP_RTO_MAX) {
    Tcb->Rto = TCP_RTO_MAX;
  }

  DEBUG (
    (DEBUG_NET,
     "TcpComputeRtt: new RTT for TCB %p computed SRTT: %d RTTVAR: %d RTO: %d\n",
     Tcb,
     Tcb->SRtt,
     Tcb->RttVar,
     Tcb->Rto)
    );
}

/**
  Trim the data; SYN and FIN to fit into the window defined by Left and Right.

  @param[in]  Nbuf     The buffer that contains a received TCP segment without an IP header.
  @param[in]  Left     The sequence number of the window's left edge.
  @param[in]  Right    The sequence number of the window's right edge.

  @retval     0        The segment is broken.
  @retval     1        The segment is in good shape.

**/
INTN
TcpTrimSegment (
  IN NET_BUF    *Nbuf,
  IN TCP_SEQNO  Left,
  IN TCP_SEQNO  Right
  )
{
  TCP_SEG    *Seg;
  TCP_SEQNO  Urg;
  UINT32     Drop;

  Seg = TCPSEG_NETBUF (Nbuf);

  //
  // If the segment is completely out of window,
  // truncate every thing, include SYN and FIN.
  //
  if (TCP_SEQ_LEQ (Seg->End, Left) || TCP_SEQ_LEQ (Right, Seg->Seq)) {
    TCP_CLEAR_FLG (Seg->Flag, TCP_FLG_SYN);
    TCP_CLEAR_FLG (Seg->Flag, TCP_FLG_FIN);

    Seg->Seq = Seg->End;
    NetbufTrim (Nbuf, Nbuf->TotalSize, NET_BUF_HEAD);
    return 1;
  }

  //
  // Adjust the buffer header
  //
  if (TCP_SEQ_LT (Seg->Seq, Left)) {
    Drop     = TCP_SUB_SEQ (Left, Seg->Seq);
    Urg      = Seg->Seq + Seg->Urg;
    Seg->Seq = Left;

    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN)) {
      TCP_CLEAR_FLG (Seg->Flag, TCP_FLG_SYN);
      Drop--;
    }

    //
    // Adjust the urgent point
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_URG)) {
      if (TCP_SEQ_LT (Urg, Seg->Seq)) {
        TCP_CLEAR_FLG (Seg->Flag, TCP_FLG_URG);
      } else {
        Seg->Urg = (UINT16)TCP_SUB_SEQ (Urg, Seg->Seq);
      }
    }

    if (Drop != 0) {
      NetbufTrim (Nbuf, Drop, NET_BUF_HEAD);
    }
  }

  //
  // Adjust the buffer tail
  //
  if (TCP_SEQ_GT (Seg->End, Right)) {
    Drop     = TCP_SUB_SEQ (Seg->End, Right);
    Seg->End = Right;

    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_FIN)) {
      TCP_CLEAR_FLG (Seg->Flag, TCP_FLG_FIN);
      Drop--;
    }

    if (Drop != 0) {
      NetbufTrim (Nbuf, Drop, NET_BUF_TAIL);
    }
  }

  return TcpVerifySegment (Nbuf);
}

/**
  Trim off the data outside the tcb's receive window.

  @param[in]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]  Nbuf     Pointer to the NET_BUF containing the received tcp segment.

  @retval     0        The segment is broken.
  @retval     1        The segment is in good shape.

**/
INTN
TcpTrimInWnd (
  IN TCP_CB   *Tcb,
  IN NET_BUF  *Nbuf
  )
{
  return TcpTrimSegment (Nbuf, Tcb->RcvNxt, Tcb->RcvWl2 + Tcb->RcvWnd);
}

/**
  Process the data and FIN flag, and check whether to deliver
  data to the socket layer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

  @retval 0        No error occurred to deliver data.
  @retval -1       An error condition occurred. The proper response is to reset the
                   connection.

**/
INTN
TcpDeliverData (
  IN OUT TCP_CB  *Tcb
  )
{
  LIST_ENTRY  *Entry;
  NET_BUF     *Nbuf;
  TCP_SEQNO   Seq;
  TCP_SEG     *Seg;
  UINT32      Urgent;

  ASSERT ((Tcb != NULL) && (Tcb->Sk != NULL));

  //
  // make sure there is some data queued,
  // and TCP is in a proper state
  //
  if (IsListEmpty (&Tcb->RcvQue) || !TCP_CONNECTED (Tcb->State)) {
    return 0;
  }

  //
  // Deliver data to the socket layer
  //
  Entry = Tcb->RcvQue.ForwardLink;
  Seq   = Tcb->RcvNxt;

  while (Entry != &Tcb->RcvQue) {
    Nbuf = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);
    Seg  = TCPSEG_NETBUF (Nbuf);

    if (TcpVerifySegment (Nbuf) == 0) {
      DEBUG (
        (DEBUG_ERROR,
         "TcpToSendData: discard a broken segment for TCB %p\n",
         Tcb)
        );
      NetbufFree (Nbuf);
      return -1;
    }

    ASSERT (Nbuf->Tcp == NULL);

    if (TCP_SEQ_GT (Seg->Seq, Seq)) {
      break;
    }

    Entry       = Entry->ForwardLink;
    Seq         = Seg->End;
    Tcb->RcvNxt = Seq;

    RemoveEntryList (&Nbuf->List);

    //
    // RFC793 Eighth step: process FIN in sequence
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_FIN)) {
      //
      // The peer sends to us junky data after FIN,
      // reset the connection.
      //
      if (!IsListEmpty (&Tcb->RcvQue)) {
        DEBUG (
          (DEBUG_ERROR,
           "TcpDeliverData: data received after FIN from peer of TCB %p, reset connection\n",
           Tcb)
          );

        NetbufFree (Nbuf);
        return -1;
      }

      DEBUG (
        (DEBUG_NET,
         "TcpDeliverData: processing FIN from peer of TCB %p\n",
         Tcb)
        );

      switch (Tcb->State) {
        case TCP_SYN_RCVD:
        case TCP_ESTABLISHED:

          TcpSetState (Tcb, TCP_CLOSE_WAIT);
          break;

        case TCP_FIN_WAIT_1:

          if (!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_FIN_ACKED)) {
            TcpSetState (Tcb, TCP_CLOSING);
            break;
          }

        //
        // fall through
        //
        case TCP_FIN_WAIT_2:

          TcpSetState (Tcb, TCP_TIME_WAIT);
          TcpClearAllTimer (Tcb);

          if (Tcb->TimeWaitTimeout != 0) {
            TcpSetTimer (Tcb, TCP_TIMER_2MSL, Tcb->TimeWaitTimeout);
          } else {
            DEBUG (
              (DEBUG_WARN,
               "Connection closed immediately because app disables TIME_WAIT timer for %p\n",
               Tcb)
              );

            TcpSendAck (Tcb);
            TcpClose (Tcb);
          }

          break;

        case TCP_CLOSE_WAIT:
        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
          //
          // The peer sends to us junk FIN byte. Discard
          // the buffer then reset the connection
          //
          NetbufFree (Nbuf);
          return -1;
          break;
        default:
          break;
      }

      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW);

      Seg->End--;
    }

    //
    // Don't delay the ack if PUSH flag is on.
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_PSH)) {
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW);
    }

    if (Nbuf->TotalSize != 0) {
      Urgent = 0;

      if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_RCVD_URG) &&
          TCP_SEQ_LEQ (Seg->Seq, Tcb->RcvUp))
      {
        if (TCP_SEQ_LEQ (Seg->End, Tcb->RcvUp)) {
          Urgent = Nbuf->TotalSize;
        } else {
          Urgent = TCP_SUB_SEQ (Tcb->RcvUp, Seg->Seq) + 1;
        }
      }

      SockDataRcvd (Tcb->Sk, Nbuf, Urgent);
    }

    if (TCP_FIN_RCVD (Tcb->State)) {
      SockNoMoreData (Tcb->Sk);
    }

    NetbufFree (Nbuf);
  }

  return 0;
}

/**
  Store the data into the reassemble queue.

  @param[in, out]  Tcb   Pointer to the TCP_CB of this TCP instance.
  @param[in]       Nbuf  Pointer to the buffer containing the data to be queued.

  @retval          0     An error condition occurred.
  @retval          1     No error occurred to queue data.

**/
INTN
TcpQueueData (
  IN OUT TCP_CB   *Tcb,
  IN     NET_BUF  *Nbuf
  )
{
  TCP_SEG     *Seg;
  LIST_ENTRY  *Head;
  LIST_ENTRY  *Prev;
  LIST_ENTRY  *Cur;
  NET_BUF     *Node;

  ASSERT ((Tcb != NULL) && (Nbuf != NULL) && (Nbuf->Tcp == NULL));

  NET_GET_REF (Nbuf);

  Seg  = TCPSEG_NETBUF (Nbuf);
  Head = &Tcb->RcvQue;

  //
  // Fast path to process normal case. That is,
  // no out-of-order segments are received.
  //
  if (IsListEmpty (Head)) {
    InsertTailList (Head, &Nbuf->List);
    return 1;
  }

  //
  // Find the point to insert the buffer
  //
  for (Prev = Head, Cur = Head->ForwardLink;
       Cur != Head;
       Prev = Cur, Cur = Cur->ForwardLink)
  {
    Node = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);

    if (TCP_SEQ_LT (Seg->Seq, TCPSEG_NETBUF (Node)->Seq)) {
      break;
    }
  }

  //
  // Check whether the current segment overlaps with the
  // previous segment.
  //
  if (Prev != Head) {
    Node = NET_LIST_USER_STRUCT (Prev, NET_BUF, List);

    if (TCP_SEQ_LT (Seg->Seq, TCPSEG_NETBUF (Node)->End)) {
      if (TCP_SEQ_LEQ (Seg->End, TCPSEG_NETBUF (Node)->End)) {
        return 1;
      }

      if (TcpTrimSegment (Nbuf, TCPSEG_NETBUF (Node)->End, Seg->End) == 0) {
        return 0;
      }
    }
  }

  InsertHeadList (Prev, &Nbuf->List);

  TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW);

  //
  // Check the segments after the insert point.
  //
  while (Cur != Head) {
    Node = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);

    if (TCP_SEQ_LEQ (TCPSEG_NETBUF (Node)->End, Seg->End)) {
      Cur = Cur->ForwardLink;

      RemoveEntryList (&Node->List);
      NetbufFree (Node);
      continue;
    }

    if (TCP_SEQ_LT (TCPSEG_NETBUF (Node)->Seq, Seg->End)) {
      if (TCP_SEQ_LEQ (TCPSEG_NETBUF (Node)->Seq, Seg->Seq)) {
        RemoveEntryList (&Nbuf->List);
        return 1;
      }

      if (TcpTrimSegment (Nbuf, Seg->Seq, TCPSEG_NETBUF (Node)->Seq) == 0) {
        RemoveEntryList (&Nbuf->List);
        return 0;
      }

      break;
    }

    Cur = Cur->ForwardLink;
  }

  return 1;
}

/**
  Adjust the send queue or the retransmit queue.

  @param[in]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]  Ack      The acknowledge sequence number of the received segment.

  @retval          0     An error condition occurred.
  @retval          1     No error occurred.

**/
INTN
TcpAdjustSndQue (
  IN TCP_CB     *Tcb,
  IN TCP_SEQNO  Ack
  )
{
  LIST_ENTRY  *Head;
  LIST_ENTRY  *Cur;
  NET_BUF     *Node;
  TCP_SEG     *Seg;

  Head = &Tcb->SndQue;
  Cur  = Head->ForwardLink;

  while (Cur != Head) {
    Node = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);
    Seg  = TCPSEG_NETBUF (Node);

    if (TCP_SEQ_GEQ (Seg->Seq, Ack)) {
      break;
    }

    //
    // Remove completely ACKed segments
    //
    if (TCP_SEQ_LEQ (Seg->End, Ack)) {
      Cur = Cur->ForwardLink;

      RemoveEntryList (&Node->List);
      NetbufFree (Node);
      continue;
    }

    return TcpTrimSegment (Node, Ack, Seg->End);
  }

  return 1;
}

/**
  Process the received TCP segments.

  @param[in]  Nbuf     Buffer that contains received a TCP segment without an IP header.
  @param[in]  Src      Source address of the segment, or the peer's IP address.
  @param[in]  Dst      Destination address of the segment, or the local end's IP
                       address.
  @param[in]  Version  IP_VERSION_4 indicates IP4 stack. IP_VERSION_6 indicates
                       IP6 stack.

  @retval 0        Segment  processed successfully. It is either accepted or
                   discarded. However, no connection is reset by the segment.
  @retval -1       A connection is reset by the segment.

**/
INTN
TcpInput (
  IN NET_BUF         *Nbuf,
  IN EFI_IP_ADDRESS  *Src,
  IN EFI_IP_ADDRESS  *Dst,
  IN UINT8           Version
  )
{
  TCP_CB      *Tcb;
  TCP_CB      *Parent;
  TCP_OPTION  Option;
  TCP_HEAD    *Head;
  INT32       Len;
  TCP_SEG     *Seg;
  TCP_SEQNO   Right;
  TCP_SEQNO   Urg;
  UINT16      Checksum;
  INT32       Usable;

  ASSERT ((Version == IP_VERSION_4) || (Version == IP_VERSION_6));

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  Parent = NULL;
  Tcb    = NULL;

  Head = (TCP_HEAD *)NetbufGetByte (Nbuf, 0, NULL);
  ASSERT (Head != NULL);

  if (Nbuf->TotalSize < sizeof (TCP_HEAD)) {
    DEBUG ((DEBUG_NET, "TcpInput: received a malformed packet\n"));
    goto DISCARD;
  }

  Len = Nbuf->TotalSize - (Head->HeadLen << 2);

  if ((Head->HeadLen < 5) || (Len < 0)) {
    DEBUG ((DEBUG_NET, "TcpInput: received a malformed packet\n"));

    goto DISCARD;
  }

  if (Version == IP_VERSION_4) {
    Checksum = NetPseudoHeadChecksum (Src->Addr[0], Dst->Addr[0], 6, 0);
  } else {
    Checksum = NetIp6PseudoHeadChecksum (&Src->v6, &Dst->v6, 6, 0);
  }

  Checksum = TcpChecksum (Nbuf, Checksum);

  if (Checksum != 0) {
    DEBUG ((DEBUG_ERROR, "TcpInput: received a checksum error packet\n"));
    goto DISCARD;
  }

  if (TCP_FLG_ON (Head->Flag, TCP_FLG_SYN)) {
    Len++;
  }

  if (TCP_FLG_ON (Head->Flag, TCP_FLG_FIN)) {
    Len++;
  }

  Tcb = TcpLocateTcb (
          Head->DstPort,
          Dst,
          Head->SrcPort,
          Src,
          Version,
          (BOOLEAN)TCP_FLG_ON (Head->Flag, TCP_FLG_SYN)
          );

  if ((Tcb == NULL) || (Tcb->State == TCP_CLOSED)) {
    DEBUG ((DEBUG_NET, "TcpInput: send reset because no TCB found\n"));

    Tcb = NULL;
    goto SEND_RESET;
  }

  Seg = TcpFormatNetbuf (Tcb, Nbuf);

  //
  // RFC1122 recommended reaction to illegal option
  // (in fact, an illegal option length) is reset.
  //
  if (TcpParseOption (Nbuf->Tcp, &Option) == -1) {
    DEBUG (
      (DEBUG_ERROR,
       "TcpInput: reset the peer because of malformed option for TCB %p\n",
       Tcb)
      );

    goto SEND_RESET;
  }

  //
  // From now on, the segment is headless
  //
  NetbufTrim (Nbuf, (Head->HeadLen << 2), NET_BUF_HEAD);
  Nbuf->Tcp = NULL;

  //
  // Process the segment in LISTEN state.
  //
  if (Tcb->State == TCP_LISTEN) {
    //
    // First step: Check RST
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_RST)) {
      DEBUG (
        (DEBUG_WARN,
         "TcpInput: discard a reset segment for TCB %p in listening\n",
         Tcb)
        );

      goto DISCARD;
    }

    //
    // Second step: Check ACK.
    // Any ACK sent to TCP in LISTEN is reseted.
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_ACK)) {
      DEBUG (
        (DEBUG_WARN,
         "TcpInput: send reset because of segment with ACK for TCB %p in listening\n",
         Tcb)
        );

      goto SEND_RESET;
    }

    //
    // Third step: Check SYN
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN)) {
      //
      // create a child TCB to handle the data
      //
      Parent = Tcb;

      Tcb = TcpCloneTcb (Parent);
      if (Tcb == NULL) {
        DEBUG (
          (DEBUG_ERROR,
           "TcpInput: discard a segment because failed to clone a child for TCB %p\n",
           Tcb)
          );

        goto DISCARD;
      }

      DEBUG (
        (DEBUG_NET,
         "TcpInput: create a child for TCB %p in listening\n",
         Tcb)
        );

      //
      // init the TCB structure
      //
      IP6_COPY_ADDRESS (&Tcb->LocalEnd.Ip, Dst);
      IP6_COPY_ADDRESS (&Tcb->RemoteEnd.Ip, Src);
      Tcb->LocalEnd.Port  = Head->DstPort;
      Tcb->RemoteEnd.Port = Head->SrcPort;

      TcpInitTcbLocal (Tcb);
      TcpInitTcbPeer (Tcb, Seg, &Option);

      TcpSetState (Tcb, TCP_SYN_RCVD);
      TcpSetTimer (Tcb, TCP_TIMER_CONNECT, Tcb->ConnectTimeout);
      if (TcpTrimInWnd (Tcb, Nbuf) == 0) {
        DEBUG (
          (DEBUG_ERROR,
           "TcpInput: discard a broken segment for TCB %p\n",
           Tcb)
          );

        goto DISCARD;
      }

      goto StepSix;
    }

    goto DISCARD;
  } else if (Tcb->State == TCP_SYN_SENT) {
    //
    // First step: Check ACK bit
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_ACK) && (Seg->Ack != Tcb->Iss + 1)) {
      DEBUG (
        (DEBUG_WARN,
         "TcpInput: send reset because of wrong ACK received for TCB %p in SYN_SENT\n",
         Tcb)
        );

      goto SEND_RESET;
    }

    //
    // Second step: Check RST bit
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_RST)) {
      if (TCP_FLG_ON (Seg->Flag, TCP_FLG_ACK)) {
        DEBUG (
          (DEBUG_WARN,
           "TcpInput: connection reset by peer for TCB %p in SYN_SENT\n",
           Tcb)
          );

        SOCK_ERROR (Tcb->Sk, EFI_CONNECTION_RESET);
        goto DROP_CONNECTION;
      } else {
        DEBUG (
          (DEBUG_WARN,
           "TcpInput: discard a reset segment because of no ACK for TCB %p in SYN_SENT\n",
           Tcb)
          );

        goto DISCARD;
      }
    }

    //
    // Third step: Check security and precedence. Skipped
    //

    //
    // Fourth step: Check SYN. Pay attention to simultaneous open
    //
    if (TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN)) {
      TcpInitTcbPeer (Tcb, Seg, &Option);

      if (TCP_FLG_ON (Seg->Flag, TCP_FLG_ACK)) {
        Tcb->SndUna = Seg->Ack;
      }

      TcpClearTimer (Tcb, TCP_TIMER_REXMIT);

      if (TCP_SEQ_GT (Tcb->SndUna, Tcb->Iss)) {
        TcpSetState (Tcb, TCP_ESTABLISHED);

        TcpClearTimer (Tcb, TCP_TIMER_CONNECT);
        TcpDeliverData (Tcb);

        if ((Tcb->CongestState == TCP_CONGEST_OPEN) &&
            TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_RTT_ON))
        {
          TcpComputeRtt (Tcb, Tcb->RttMeasure);
          TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_RTT_ON);
        }

        if (TcpTrimInWnd (Tcb, Nbuf) == 0) {
          DEBUG (
            (DEBUG_ERROR,
             "TcpInput: discard a broken segment for TCB %p\n",
             Tcb)
            );

          goto DISCARD;
        }

        TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW);

        DEBUG (
          (DEBUG_NET,
           "TcpInput: connection established for TCB %p in SYN_SENT\n",
           Tcb)
          );

        goto StepSix;
      } else {
        //
        // Received a SYN segment without ACK, simultaneous open.
        //
        TcpSetState (Tcb, TCP_SYN_RCVD);

        ASSERT (Tcb->SndNxt == Tcb->Iss + 1);

        if ((TcpAdjustSndQue (Tcb, Tcb->SndNxt) == 0) || (TcpTrimInWnd (Tcb, Nbuf) == 0)) {
          DEBUG (
            (DEBUG_ERROR,
             "TcpInput: discard a broken segment for TCB %p\n",
             Tcb)
            );

          goto DISCARD;
        }

        DEBUG (
          (DEBUG_WARN,
           "TcpInput: simultaneous open for TCB %p in SYN_SENT\n",
           Tcb)
          );

        goto StepSix;
      }
    }

    goto DISCARD;
  }

  //
  // Process segment in SYN_RCVD or TCP_CONNECTED states
  //

  //
  // Clear probe timer since the RecvWindow is opened.
  //
  if (Tcb->ProbeTimerOn && (Seg->Wnd != 0)) {
    TcpClearTimer (Tcb, TCP_TIMER_PROBE);
    Tcb->ProbeTimerOn = FALSE;
  }

  //
  // First step: Check whether SEG.SEQ is acceptable
  //
  if (TcpSeqAcceptable (Tcb, Seg) == 0) {
    DEBUG (
      (DEBUG_WARN,
       "TcpInput: sequence acceptance test failed for segment of TCB %p\n",
       Tcb)
      );

    if (!TCP_FLG_ON (Seg->Flag, TCP_FLG_RST)) {
      TcpSendAck (Tcb);
    }

    goto DISCARD;
  }

  if ((TCP_SEQ_LT (Seg->Seq, Tcb->RcvWl2)) &&
      (Tcb->RcvWl2 == Seg->End) &&
      !TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN | TCP_FLG_FIN))
  {
    TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW);
  }

  //
  // Second step: Check the RST
  //
  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_RST)) {
    DEBUG ((DEBUG_WARN, "TcpInput: connection reset for TCB %p\n", Tcb));

    if (Tcb->State == TCP_SYN_RCVD) {
      SOCK_ERROR (Tcb->Sk, EFI_CONNECTION_REFUSED);

      //
      // This TCB comes from either a LISTEN TCB,
      // or active open TCB with simultaneous open.
      // Do NOT signal user CONNECTION refused
      // if it comes from a LISTEN TCB.
      //
    } else if ((Tcb->State == TCP_ESTABLISHED) ||
               (Tcb->State == TCP_FIN_WAIT_1) ||
               (Tcb->State == TCP_FIN_WAIT_2) ||
               (Tcb->State == TCP_CLOSE_WAIT))
    {
      SOCK_ERROR (Tcb->Sk, EFI_CONNECTION_RESET);
    } else {
    }

    goto DROP_CONNECTION;
  }

  //
  // Trim the data and flags.
  //
  if (TcpTrimInWnd (Tcb, Nbuf) == 0) {
    DEBUG (
      (DEBUG_ERROR,
       "TcpInput: discard a broken segment for TCB %p\n",
       Tcb)
      );

    goto DISCARD;
  }

  //
  // Third step: Check security and precedence, Ignored
  //

  //
  // Fourth step: Check the SYN bit.
  //
  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN)) {
    DEBUG (
      (DEBUG_WARN,
       "TcpInput: connection reset because received extra SYN for TCB %p\n",
       Tcb)
      );

    SOCK_ERROR (Tcb->Sk, EFI_CONNECTION_RESET);
    goto RESET_THEN_DROP;
  }

  //
  // Fifth step: Check the ACK
  //
  if (!TCP_FLG_ON (Seg->Flag, TCP_FLG_ACK)) {
    DEBUG (
      (DEBUG_WARN,
       "TcpInput: segment discard because of no ACK for connected TCB %p\n",
       Tcb)
      );

    goto DISCARD;
  } else {
    if ((Tcb->IpInfo->IpVersion == IP_VERSION_6) && (Tcb->Tick == 0)) {
      Tcp6RefreshNeighbor (Tcb, Src, TCP6_KEEP_NEIGHBOR_TIME * TICKS_PER_SECOND);
      Tcb->Tick = TCP6_REFRESH_NEIGHBOR_TICK;
    }
  }

  if (Tcb->State == TCP_SYN_RCVD) {
    if (TCP_SEQ_LT (Tcb->SndUna, Seg->Ack) &&
        TCP_SEQ_LEQ (Seg->Ack, Tcb->SndNxt))
    {
      Tcb->SndWnd    = Seg->Wnd;
      Tcb->SndWndMax = MAX (Tcb->SndWnd, Tcb->SndWndMax);
      Tcb->SndWl1    = Seg->Seq;
      Tcb->SndWl2    = Seg->Ack;
      TcpSetState (Tcb, TCP_ESTABLISHED);

      TcpClearTimer (Tcb, TCP_TIMER_CONNECT);
      TcpDeliverData (Tcb);

      DEBUG (
        (DEBUG_NET,
         "TcpInput: connection established for TCB %p in SYN_RCVD\n",
         Tcb)
        );

      //
      // Continue the process as ESTABLISHED state
      //
    } else {
      DEBUG (
        (DEBUG_WARN,
         "TcpInput: send reset because of wrong ACK for TCB %p in SYN_RCVD\n",
         Tcb)
        );

      goto SEND_RESET;
    }
  }

  if (TCP_SEQ_LT (Seg->Ack, Tcb->SndUna)) {
    DEBUG (
      (DEBUG_WARN,
       "TcpInput: ignore the out-of-data ACK for connected TCB %p\n",
       Tcb)
      );

    goto StepSix;
  } else if (TCP_SEQ_GT (Seg->Ack, Tcb->SndNxt)) {
    DEBUG (
      (DEBUG_WARN,
       "TcpInput: discard segment for future ACK for connected TCB %p\n",
       Tcb)
      );

    TcpSendAck (Tcb);
    goto DISCARD;
  }

  //
  // From now on: SND.UNA <= SEG.ACK <= SND.NXT.
  //
  if (TCP_FLG_ON (Option.Flag, TCP_OPTION_RCVD_TS)) {
    //
    // update TsRecent as specified in page 16 RFC1323.
    // RcvWl2 equals to the variable "LastAckSent"
    // defined there.
    //
    if (TCP_SEQ_LEQ (Seg->Seq, Tcb->RcvWl2) &&
        TCP_SEQ_LT (Tcb->RcvWl2, Seg->End))
    {
      Tcb->TsRecent    = Option.TSVal;
      Tcb->TsRecentAge = mTcpTick;
    }

    TcpComputeRtt (Tcb, TCP_SUB_TIME (mTcpTick, Option.TSEcr));
  } else if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_RTT_ON)) {
    ASSERT (Tcb->CongestState == TCP_CONGEST_OPEN);

    TcpComputeRtt (Tcb, Tcb->RttMeasure);
    TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_RTT_ON);
  }

  if (Seg->Ack == Tcb->SndNxt) {
    TcpClearTimer (Tcb, TCP_TIMER_REXMIT);
  } else {
    TcpSetTimer (Tcb, TCP_TIMER_REXMIT, Tcb->Rto);
  }

  //
  // Count duplicate acks.
  //
  if ((Seg->Ack == Tcb->SndUna) &&
      (Tcb->SndUna != Tcb->SndNxt) &&
      (Seg->Wnd == Tcb->SndWnd) &&
      (0 == Len))
  {
    Tcb->DupAck++;
  } else {
    Tcb->DupAck = 0;
  }

  //
  // Congestion avoidance, fast recovery and fast retransmission.
  //
  if (((Tcb->CongestState == TCP_CONGEST_OPEN) && (Tcb->DupAck < 3)) ||
      (Tcb->CongestState == TCP_CONGEST_LOSS))
  {
    if (TCP_SEQ_GT (Seg->Ack, Tcb->SndUna)) {
      if (Tcb->CWnd < Tcb->Ssthresh) {
        Tcb->CWnd += Tcb->SndMss;
      } else {
        Tcb->CWnd += MAX (Tcb->SndMss * Tcb->SndMss / Tcb->CWnd, 1);
      }

      Tcb->CWnd = MIN (Tcb->CWnd, TCP_MAX_WIN << Tcb->SndWndScale);
    }

    if (Tcb->CongestState == TCP_CONGEST_LOSS) {
      TcpFastLossRecover (Tcb, Seg);
    }
  } else {
    TcpFastRecover (Tcb, Seg);
  }

  if (TCP_SEQ_GT (Seg->Ack, Tcb->SndUna)) {
    if (TcpAdjustSndQue (Tcb, Seg->Ack) == 0) {
      DEBUG (
        (DEBUG_ERROR,
         "TcpInput: discard a broken segment for TCB %p\n",
         Tcb)
        );

      goto DISCARD;
    }

    Tcb->SndUna = Seg->Ack;

    if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_SND_URG) &&
        TCP_SEQ_LT (Tcb->SndUp, Seg->Ack))
    {
      TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_SND_URG);
    }
  }

  //
  // Update window info
  //
  if (TCP_SEQ_LT (Tcb->SndWl1, Seg->Seq) ||
      ((Tcb->SndWl1 == Seg->Seq) && TCP_SEQ_LEQ (Tcb->SndWl2, Seg->Ack)))
  {
    Right = Seg->Ack + Seg->Wnd;

    if (TCP_SEQ_LT (Right, Tcb->SndWl2 + Tcb->SndWnd)) {
      if ((Tcb->SndWl1 == Seg->Seq) &&
          (Tcb->SndWl2 == Seg->Ack) &&
          (Len == 0))
      {
        goto NO_UPDATE;
      }

      DEBUG (
        (DEBUG_WARN,
         "TcpInput: peer shrinks the window for connected TCB %p\n",
         Tcb)
        );

      if ((Tcb->CongestState == TCP_CONGEST_RECOVER) &&
          (TCP_SEQ_LT (Right, Tcb->Recover)))
      {
        Tcb->Recover = Right;
      }

      if ((Tcb->CongestState == TCP_CONGEST_LOSS) &&
          (TCP_SEQ_LT (Right, Tcb->LossRecover)))
      {
        Tcb->LossRecover = Right;
      }

      if (TCP_SEQ_LT (Right, Tcb->SndNxt)) {
        //
        // Check for Window Retraction in RFC7923 section 2.4.
        // The lower n bits of the peer's actual receive window is wiped out if TCP
        // window scale is enabled, it will look like the peer is shrinking the window.
        // Check whether the SndNxt is out of the advertised receive window by more than
        // 2^Rcv.Wind.Shift before moving the SndNxt to the left.
        //
        DEBUG (
          (DEBUG_WARN,
           "TcpInput: peer advise negative useable window for connected TCB %p\n",
           Tcb)
          );
        Usable = TCP_SUB_SEQ (Tcb->SndNxt, Right);
        if ((Usable >> Tcb->SndWndScale) > 0) {
          DEBUG (
            (DEBUG_WARN,
             "TcpInput: SndNxt is out of window by more than window scale for TCB %p\n",
             Tcb)
            );
          Tcb->SndNxt = Right;
        }

        if (Right == Tcb->SndUna) {
          TcpClearTimer (Tcb, TCP_TIMER_REXMIT);
          TcpSetProbeTimer (Tcb);
        }
      }
    }

    Tcb->SndWnd    = Seg->Wnd;
    Tcb->SndWndMax = MAX (Tcb->SndWnd, Tcb->SndWndMax);
    Tcb->SndWl1    = Seg->Seq;
    Tcb->SndWl2    = Seg->Ack;
  }

NO_UPDATE:

  if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_FIN_SENT) &&
      (Tcb->SndUna == Tcb->SndNxt))
  {
    DEBUG (
      (DEBUG_NET,
       "TcpInput: local FIN is ACKed by peer for connected TCB %p\n",
       Tcb)
      );

    TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_FIN_ACKED);
  }

  //
  // Transit the state if proper.
  //
  switch (Tcb->State) {
    case TCP_FIN_WAIT_1:

      if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_FIN_ACKED)) {
        TcpSetState (Tcb, TCP_FIN_WAIT_2);

        TcpClearAllTimer (Tcb);
        TcpSetTimer (Tcb, TCP_TIMER_FINWAIT2, Tcb->FinWait2Timeout);
      }

    case TCP_FIN_WAIT_2:

      break;

    case TCP_CLOSE_WAIT:
      break;

    case TCP_CLOSING:

      if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_FIN_ACKED)) {
        TcpSetState (Tcb, TCP_TIME_WAIT);

        TcpClearAllTimer (Tcb);

        if (Tcb->TimeWaitTimeout != 0) {
          TcpSetTimer (Tcb, TCP_TIMER_2MSL, Tcb->TimeWaitTimeout);
        } else {
          DEBUG (
            (DEBUG_WARN,
             "Connection closed immediately because app disables TIME_WAIT timer for %p\n",
             Tcb)
            );

          TcpClose (Tcb);
        }
      }

      break;

    case TCP_LAST_ACK:

      if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_FIN_ACKED)) {
        TcpSetState (Tcb, TCP_CLOSED);
      }

      break;

    case TCP_TIME_WAIT:

      TcpSendAck (Tcb);

      if (Tcb->TimeWaitTimeout != 0) {
        TcpSetTimer (Tcb, TCP_TIMER_2MSL, Tcb->TimeWaitTimeout);
      } else {
        DEBUG (
          (DEBUG_WARN,
           "Connection closed immediately because app disables TIME_WAIT timer for %p\n",
           Tcb)
          );

        TcpClose (Tcb);
      }

      break;

    default:
      break;
  }

  //
  // Sixth step: Check the URG bit.update the Urg point
  // if in TCP_CAN_RECV, otherwise, leave the RcvUp intact.
  //
StepSix:

  Tcb->Idle = 0;
  TcpSetKeepaliveTimer (Tcb);

  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_URG) && !TCP_FIN_RCVD (Tcb->State)) {
    DEBUG (
      (DEBUG_NET,
       "TcpInput: received urgent data from peer for connected TCB %p\n",
       Tcb)
      );

    Urg = Seg->Seq + Seg->Urg;

    if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_RCVD_URG) &&
        TCP_SEQ_GT (Urg, Tcb->RcvUp))
    {
      Tcb->RcvUp = Urg;
    } else {
      Tcb->RcvUp = Urg;
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_RCVD_URG);
    }
  }

  //
  // Seventh step: Process the segment data
  //
  if (Seg->End != Seg->Seq) {
    if (TCP_FIN_RCVD (Tcb->State)) {
      DEBUG (
        (DEBUG_WARN,
         "TcpInput: connection reset because data is lost for connected TCB %p\n",
         Tcb)
        );

      goto RESET_THEN_DROP;
    }

    if (TCP_LOCAL_CLOSED (Tcb->State) && (Nbuf->TotalSize != 0)) {
      DEBUG (
        (DEBUG_WARN,
         "TcpInput: connection reset because data is lost for connected TCB %p\n",
         Tcb)
        );

      goto RESET_THEN_DROP;
    }

    if (TcpQueueData (Tcb, Nbuf) == 0) {
      DEBUG (
        (DEBUG_ERROR,
         "TcpInput: discard a broken segment for TCB %p\n",
         Tcb)
        );

      goto DISCARD;
    }

    if (TcpDeliverData (Tcb) == -1) {
      goto RESET_THEN_DROP;
    }

    if (!IsListEmpty (&Tcb->RcvQue)) {
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW);
    }
  }

  //
  // Eighth step: check the FIN.
  // This step is moved to TcpDeliverData. FIN will be
  // processed in sequence there. Check the comments in
  // the beginning of the file header for information.
  //

  //
  // Tcb is a new child of the listening Parent,
  // commit it.
  //
  if (Parent != NULL) {
    Tcb->Parent = Parent;
    TcpInsertTcb (Tcb);
  }

  if ((Tcb->State != TCP_CLOSED) &&
      (TcpToSendData (Tcb, 0) == 0) &&
      (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_ACK_NOW) || (Nbuf->TotalSize != 0)))
  {
    TcpToSendAck (Tcb);
  }

  NetbufFree (Nbuf);
  return 0;

RESET_THEN_DROP:
  TcpSendReset (Tcb, Head, Len, Dst, Src, Version);

DROP_CONNECTION:
  ASSERT ((Tcb != NULL) && (Tcb->Sk != NULL));

  NetbufFree (Nbuf);
  TcpClose (Tcb);

  return -1;

SEND_RESET:

  TcpSendReset (Tcb, Head, Len, Dst, Src, Version);

DISCARD:

  //
  // Tcb is a child of Parent, and it doesn't survive
  //
  DEBUG ((DEBUG_WARN, "TcpInput: Discard a packet\n"));
  NetbufFree (Nbuf);

  if ((Parent != NULL) && (Tcb != NULL)) {
    ASSERT (Tcb->Sk != NULL);
    TcpClose (Tcb);
  }

  return 0;
}

/**
  Process the received ICMP error messages for TCP.

  @param[in]  Nbuf     The buffer that contains part of the TCP segment without an IP header
                       truncated from the ICMP error packet.
  @param[in]  IcmpErr  The ICMP error code interpreted from an ICMP error packet.
  @param[in]  Src      Source address of the ICMP error message.
  @param[in]  Dst      Destination address of the ICMP error message.
  @param[in]  Version  IP_VERSION_4 indicates IP4 stack. IP_VERSION_6 indicates
                       IP6 stack.

**/
VOID
TcpIcmpInput (
  IN NET_BUF         *Nbuf,
  IN UINT8           IcmpErr,
  IN EFI_IP_ADDRESS  *Src,
  IN EFI_IP_ADDRESS  *Dst,
  IN UINT8           Version
  )
{
  TCP_HEAD    *Head;
  TCP_CB      *Tcb;
  TCP_SEQNO   Seq;
  EFI_STATUS  IcmpErrStatus;
  BOOLEAN     IcmpErrIsHard;
  BOOLEAN     IcmpErrNotify;

  if (Nbuf->TotalSize < sizeof (TCP_HEAD)) {
    goto CLEAN_EXIT;
  }

  Head = (TCP_HEAD *)NetbufGetByte (Nbuf, 0, NULL);
  ASSERT (Head != NULL);

  Tcb = TcpLocateTcb (
          Head->DstPort,
          Dst,
          Head->SrcPort,
          Src,
          Version,
          FALSE
          );
  if ((Tcb == NULL) || (Tcb->State == TCP_CLOSED)) {
    goto CLEAN_EXIT;
  }

  //
  // Validate the sequence number.
  //
  Seq = NTOHL (Head->Seq);
  if (!(TCP_SEQ_LEQ (Tcb->SndUna, Seq) && TCP_SEQ_LT (Seq, Tcb->SndNxt))) {
    goto CLEAN_EXIT;
  }

  IcmpErrStatus = IpIoGetIcmpErrStatus (
                    IcmpErr,
                    Tcb->Sk->IpVersion,
                    &IcmpErrIsHard,
                    &IcmpErrNotify
                    );

  if (IcmpErrNotify) {
    SOCK_ERROR (Tcb->Sk, IcmpErrStatus);
  }

  if (IcmpErrIsHard) {
    TcpClose (Tcb);
  }

CLEAN_EXIT:

  NetbufFree (Nbuf);
}
