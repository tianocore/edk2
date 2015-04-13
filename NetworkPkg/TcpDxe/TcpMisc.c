/** @file
  Misc support routines for TCP driver.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "TcpMain.h"

LIST_ENTRY      mTcpRunQue = {
  &mTcpRunQue,
  &mTcpRunQue
};

LIST_ENTRY      mTcpListenQue = {
  &mTcpListenQue,
  &mTcpListenQue
};

TCP_SEQNO       mTcpGlobalIss = TCP_BASE_ISS;

CHAR16          *mTcpStateName[] = {
  L"TCP_CLOSED",
  L"TCP_LISTEN",
  L"TCP_SYN_SENT",
  L"TCP_SYN_RCVD",
  L"TCP_ESTABLISHED",
  L"TCP_FIN_WAIT_1",
  L"TCP_FIN_WAIT_2",
  L"TCP_CLOSING",
  L"TCP_TIME_WAIT",
  L"TCP_CLOSE_WAIT",
  L"TCP_LAST_ACK"
};


/**
  Initialize the Tcb local related members.

  @param[in, out]  Tcb               Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpInitTcbLocal (
  IN OUT TCP_CB *Tcb
  )
{
  //
  // Compute the checksum of the fixed parts of pseudo header
  //
  if (Tcb->Sk->IpVersion == IP_VERSION_4) {
    Tcb->HeadSum = NetPseudoHeadChecksum (
                    Tcb->LocalEnd.Ip.Addr[0],
                    Tcb->RemoteEnd.Ip.Addr[0],
                    0x06,
                    0
                    );
  } else {
    Tcb->HeadSum = NetIp6PseudoHeadChecksum (
                    &Tcb->LocalEnd.Ip.v6,
                    &Tcb->RemoteEnd.Ip.v6,
                    0x06,
                    0
                    );
  }

  Tcb->Iss    = TcpGetIss ();
  Tcb->SndUna = Tcb->Iss;
  Tcb->SndNxt = Tcb->Iss;

  Tcb->SndWl2 = Tcb->Iss;
  Tcb->SndWnd = 536;

  Tcb->RcvWnd = GET_RCV_BUFFSIZE (Tcb->Sk);

  //
  // First window size is never scaled
  //
  Tcb->RcvWndScale  = 0;

  Tcb->ProbeTimerOn = FALSE;
}

/**
  Initialize the peer related members.

  @param[in, out]  Tcb    Pointer to the TCP_CB of this TCP instance.
  @param[in]       Seg    Pointer to the segment that contains the peer's intial info.
  @param[in]       Opt    Pointer to the options announced by the peer.

**/
VOID
TcpInitTcbPeer (
  IN OUT TCP_CB     *Tcb,
  IN     TCP_SEG    *Seg,
  IN     TCP_OPTION *Opt
  )
{
  UINT16  RcvMss;

  ASSERT ((Tcb != NULL) && (Seg != NULL) && (Opt != NULL));
  ASSERT (TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN));

  Tcb->SndWnd     = Seg->Wnd;
  Tcb->SndWndMax  = Tcb->SndWnd;
  Tcb->SndWl1     = Seg->Seq;

  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_ACK)) {
    Tcb->SndWl2 = Seg->Ack;
  } else {
    Tcb->SndWl2 = Tcb->Iss + 1;
  }

  if (TCP_FLG_ON (Opt->Flag, TCP_OPTION_RCVD_MSS)) {
    Tcb->SndMss = (UINT16) MAX (64, Opt->Mss);

    RcvMss      = TcpGetRcvMss (Tcb->Sk);
    if (Tcb->SndMss > RcvMss) {
      Tcb->SndMss = RcvMss;
    }

  } else {
    //
    // One end doesn't support MSS option, use default.
    //
    Tcb->RcvMss = 536;
  }

  Tcb->CWnd   = Tcb->SndMss;

  Tcb->Irs    = Seg->Seq;
  Tcb->RcvNxt = Tcb->Irs + 1;

  Tcb->RcvWl2 = Tcb->RcvNxt;

  if (TCP_FLG_ON (Opt->Flag, TCP_OPTION_RCVD_WS) && !TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_WS)) {

    Tcb->SndWndScale  = Opt->WndScale;

    Tcb->RcvWndScale  = TcpComputeScale (Tcb);
    TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_RCVD_WS);

  } else {
    //
    // One end doesn't support window scale option. use zero.
    //
    Tcb->RcvWndScale = 0;
  }

  if (TCP_FLG_ON (Opt->Flag, TCP_OPTION_RCVD_TS) && !TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_TS)) {

    TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_SND_TS);
    TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_RCVD_TS);

    Tcb->TsRecent = Opt->TSVal;

    //
    // Compute the effective SndMss per RFC1122
    // section 4.2.2.6. If timestamp option is
    // enabled, it will always occupy 12 bytes.
    //
    Tcb->SndMss -= TCP_OPTION_TS_ALIGNED_LEN;
  }
}

/**
  Check whether one IP address equals the other.

  @param[in]   Ip1     Pointer to IP address to be checked.
  @param[in]   Ip2     Pointer to IP address to be checked.
  @param[in]   Version IP_VERSION_4 indicates the IP address is an IPv4 address,
                       IP_VERSION_6 indicates the IP address is an IPv6 address.

  @retval      TRUE    Ip1 equals Ip2.
  @retval      FALSE   Ip1 does not equal Ip2.

**/
BOOLEAN
TcpIsIpEqual (
  IN EFI_IP_ADDRESS  *Ip1,
  IN EFI_IP_ADDRESS  *Ip2,
  IN UINT8           Version
  )
{
  ASSERT ((Version == IP_VERSION_4) || (Version == IP_VERSION_6));

  if (Version == IP_VERSION_4) {
    return (BOOLEAN) (Ip1->Addr[0] == Ip2->Addr[0]);
  } else {
    return (BOOLEAN) EFI_IP6_EQUAL (&Ip1->v6, &Ip2->v6);
  }
}

/**
  Check whether one IP address is filled with ZERO.

  @param[in]   Ip      Pointer to the IP address to be checked.
  @param[in]   Version IP_VERSION_4 indicates the IP address is an IPv4 address,
                       IP_VERSION_6 indicates the IP address is an IPv6 address.

  @retval      TRUE    Ip is all zero address.
  @retval      FALSE   Ip is not all zero address.

**/
BOOLEAN
TcpIsIpZero (
  IN EFI_IP_ADDRESS *Ip,
  IN UINT8          Version
  )
{
  ASSERT ((Version == IP_VERSION_4) || (Version == IP_VERSION_6));

  if (Version == IP_VERSION_4) {
    return (BOOLEAN) (Ip->Addr[0] == 0);
  } else {
    return (BOOLEAN) ((Ip->Addr[0] == 0) && (Ip->Addr[1] == 0) &&
      (Ip->Addr[2] == 0) && (Ip->Addr[3] == 0));
  }
}

/**
  Locate a listen TCB that matchs the Local and Remote.

  @param[in]  Local    Pointer to the local (IP, Port).
  @param[in]  Remote   Pointer to the remote (IP, Port).
  @param[in]  Version  IP_VERSION_4 indicates TCP is running on IP4 stack,
                       IP_VERSION_6 indicates TCP is running on IP6 stack.

  @return  Pointer to the TCP_CB with the least number of wildcards,
           if NULL no match is found.

**/
TCP_CB *
TcpLocateListenTcb (
  IN TCP_PEER    *Local,
  IN TCP_PEER    *Remote,
  IN UINT8       Version
  )
{
  LIST_ENTRY      *Entry;
  TCP_CB          *Node;
  TCP_CB          *Match;
  INTN            Last;
  INTN            Cur;

  Last  = 4;
  Match = NULL;

  NET_LIST_FOR_EACH (Entry, &mTcpListenQue) {
    Node = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if ((Version != Node->Sk->IpVersion) ||
        (Local->Port != Node->LocalEnd.Port) ||
        !TCP_PEER_MATCH (Remote, &Node->RemoteEnd, Version) ||
        !TCP_PEER_MATCH (Local, &Node->LocalEnd, Version)
          ) {

      continue;
    }

    //
    // Compute the number of wildcard
    //
    Cur = 0;
    if (TcpIsIpZero (&Node->RemoteEnd.Ip, Version)) {
      Cur++;
    }

    if (Node->RemoteEnd.Port == 0) {
      Cur++;
    }

    if (TcpIsIpZero (&Node->LocalEnd.Ip, Version)) {
      Cur++;
    }

    if (Cur < Last) {
      if (Cur == 0) {
        return Node;
      }

      Last  = Cur;
      Match = Node;
    }
  }

  return Match;
}

/**
  Try to find one Tcb whose <Ip, Port> equals to <IN Addr, IN Port>.

  @param[in]  Addr     Pointer to the IP address needs to match.
  @param[in]  Port     The port number needs to match.
  @param[in]  Version  IP_VERSION_4 indicates TCP is running on IP4 stack,
                       IP_VERSION_6 indicates TCP is running on IP6 stack.


  @retval     TRUE     The Tcb which matches the <Addr Port> pair exists.
  @retval     FALSE    Otherwise

**/
BOOLEAN
TcpFindTcbByPeer (
  IN EFI_IP_ADDRESS  *Addr,
  IN TCP_PORTNO      Port,
  IN UINT8           Version
  )
{
  TCP_PORTNO      LocalPort;
  LIST_ENTRY      *Entry;
  TCP_CB          *Tcb;

  ASSERT ((Addr != NULL) && (Port != 0));

  LocalPort = HTONS (Port);

  NET_LIST_FOR_EACH (Entry, &mTcpListenQue) {
    Tcb = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if ((Version == Tcb->Sk->IpVersion) &&
      TcpIsIpEqual (Addr, &Tcb->LocalEnd.Ip, Version) &&
        (LocalPort == Tcb->LocalEnd.Port)
        ) {

      return TRUE;
    }
  }

  NET_LIST_FOR_EACH (Entry, &mTcpRunQue) {
    Tcb = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if ((Version == Tcb->Sk->IpVersion) &&
      TcpIsIpEqual (Addr, &Tcb->LocalEnd.Ip, Version) &&
        (LocalPort == Tcb->LocalEnd.Port)
        ) {

      return TRUE;
    }
  }

  return FALSE;
}

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
  )
{
  TCP_PEER        Local;
  TCP_PEER        Remote;
  LIST_ENTRY      *Entry;
  TCP_CB          *Tcb;

  Local.Port  = LocalPort;
  Remote.Port = RemotePort;

  CopyMem (&Local.Ip, LocalIp, sizeof (EFI_IP_ADDRESS));
  CopyMem (&Remote.Ip, RemoteIp, sizeof (EFI_IP_ADDRESS));

  //
  // First check for exact match.
  //
  NET_LIST_FOR_EACH (Entry, &mTcpRunQue) {
    Tcb = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if ((Version == Tcb->Sk->IpVersion) &&
        TCP_PEER_EQUAL (&Remote, &Tcb->RemoteEnd, Version) &&
        TCP_PEER_EQUAL (&Local, &Tcb->LocalEnd, Version)
          ) {

      RemoveEntryList (&Tcb->List);
      InsertHeadList (&mTcpRunQue, &Tcb->List);

      return Tcb;
    }
  }

  //
  // Only check the listen queue when the SYN flag is on.
  //
  if (Syn) {
    return TcpLocateListenTcb (&Local, &Remote, Version);
  }

  return NULL;
}

/**
  Insert a Tcb into the proper queue.

  @param[in]  Tcb               Pointer to the TCP_CB to be inserted.

  @retval 0                     The Tcb was inserted successfully.
  @retval -1                    Error condition occurred.

**/
INTN
TcpInsertTcb (
  IN TCP_CB *Tcb
  )
{
  LIST_ENTRY       *Entry;
  LIST_ENTRY       *Head;
  TCP_CB           *Node;

  ASSERT (
    (Tcb != NULL) &&
    (
    (Tcb->State == TCP_LISTEN) ||
    (Tcb->State == TCP_SYN_SENT) ||
    (Tcb->State == TCP_SYN_RCVD) ||
    (Tcb->State == TCP_CLOSED)
    )
    );

  if (Tcb->LocalEnd.Port == 0) {
    return -1;
  }

  Head = &mTcpRunQue;

  if (Tcb->State == TCP_LISTEN) {
    Head = &mTcpListenQue;
  }

  //
  // Check that the Tcb isn't already on the list.
  //
  NET_LIST_FOR_EACH (Entry, Head) {
    Node = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if (TCP_PEER_EQUAL (&Tcb->LocalEnd, &Node->LocalEnd, Tcb->Sk->IpVersion) &&
        TCP_PEER_EQUAL (&Tcb->RemoteEnd, &Node->RemoteEnd, Tcb->Sk->IpVersion)
          ) {

      return -1;
    }
  }

  InsertHeadList (Head, &Tcb->List);


  return 0;
}

/**
  Clone a TCP_CB from Tcb.

  @param[in]  Tcb                   Pointer to the TCP_CB to be cloned.

  @return Pointer to the new cloned TCP_CB; if NULL, error condition occurred.

**/
TCP_CB *
TcpCloneTcb (
  IN TCP_CB *Tcb
  )
{
  TCP_CB             *Clone;

  Clone = AllocateZeroPool (sizeof (TCP_CB));

  if (Clone == NULL) {
    return NULL;
  }

  CopyMem (Clone, Tcb, sizeof (TCP_CB));

  //
  // Increase the reference count of the shared IpInfo.
  //
  NET_GET_REF (Tcb->IpInfo);

  InitializeListHead (&Clone->List);
  InitializeListHead (&Clone->SndQue);
  InitializeListHead (&Clone->RcvQue);

  Clone->Sk = SockClone (Tcb->Sk);
  if (Clone->Sk == NULL) {
    DEBUG ((EFI_D_ERROR, "TcpCloneTcb: failed to clone a sock\n"));
    FreePool (Clone);
    return NULL;
  }

  ((TCP_PROTO_DATA *) (Clone->Sk->ProtoReserved))->TcpPcb = Clone;

  return Clone;
}

/**
  Compute an ISS to be used by a new connection.

  @return The resulting ISS.

**/
TCP_SEQNO
TcpGetIss (
  VOID
  )
{
  mTcpGlobalIss += TCP_ISS_INCREMENT_1;
  return mTcpGlobalIss;
}

/**
  Get the local mss.

  @param[in]  Sock        Pointer to the socket to get mss.

  @return The mss size.

**/
UINT16
TcpGetRcvMss (
  IN SOCKET  *Sock
  )
{
  EFI_IP4_MODE_DATA      Ip4Mode;
  EFI_IP6_MODE_DATA      Ip6Mode;
  EFI_IP4_PROTOCOL       *Ip4;
  EFI_IP6_PROTOCOL       *Ip6;
  TCP_PROTO_DATA         *TcpProto;

  ASSERT (Sock != NULL);

  ZeroMem (&Ip4Mode, sizeof (EFI_IP4_MODE_DATA));
  ZeroMem (&Ip6Mode, sizeof (EFI_IP6_MODE_DATA));

  TcpProto = (TCP_PROTO_DATA *) Sock->ProtoReserved;

  if (Sock->IpVersion == IP_VERSION_4) {
    Ip4 = TcpProto->TcpService->IpIo->Ip.Ip4;
    ASSERT (Ip4 != NULL);
    Ip4->GetModeData (Ip4, &Ip4Mode, NULL, NULL);

    return (UINT16) (Ip4Mode.MaxPacketSize - sizeof (TCP_HEAD));
  } else {
    Ip6 = TcpProto->TcpService->IpIo->Ip.Ip6;
    ASSERT (Ip6 != NULL);
    Ip6->GetModeData (Ip6, &Ip6Mode, NULL, NULL);

    return (UINT16) (Ip6Mode.MaxPacketSize - sizeof (TCP_HEAD));
  }
}

/**
  Set the Tcb's state.

  @param[in]  Tcb                   Pointer to the TCP_CB of this TCP instance.
  @param[in]  State                 The state to be set.

**/
VOID
TcpSetState (
  IN TCP_CB *Tcb,
  IN UINT8  State
  )
{
  ASSERT (Tcb->State < (sizeof (mTcpStateName) / sizeof (CHAR16 *)));
  ASSERT (State < (sizeof (mTcpStateName) / sizeof (CHAR16 *)));

  DEBUG (
    (EFI_D_INFO,
    "Tcb (%p) state %s --> %s\n",
    Tcb,
    mTcpStateName[Tcb->State],
    mTcpStateName[State])
    );

  Tcb->State = State;

  switch (State) {
  case TCP_ESTABLISHED:

    SockConnEstablished (Tcb->Sk);

    if (Tcb->Parent != NULL) {
      //
      // A new connection is accepted by a listening socket. Install
      // the device path.
      //
      TcpInstallDevicePath (Tcb->Sk);
    }

    break;

  case TCP_CLOSED:

    SockConnClosed (Tcb->Sk);

    break;
  default:
    break;
  }
}

/**
  Compute the TCP segment's checksum.

  @param[in]  Nbuf       Pointer to the buffer that contains the TCP segment.
  @param[in]  HeadSum    The checksum value of the fixed part of pseudo header.

  @return The checksum value.

**/
UINT16
TcpChecksum (
  IN NET_BUF *Nbuf,
  IN UINT16  HeadSum
  )
{
  UINT16  Checksum;

  Checksum  = NetbufChecksum (Nbuf);
  Checksum  = NetAddChecksum (Checksum, HeadSum);

  Checksum = NetAddChecksum (
              Checksum,
              HTONS ((UINT16) Nbuf->TotalSize)
              );

  return (UINT16) (~Checksum);
}

/**
  Translate the information from the head of the received TCP
  segment Nbuf contents and fill it into a TCP_SEG structure.

  @param[in]       Tcb           Pointer to the TCP_CB of this TCP instance.
  @param[in, out]  Nbuf          Pointer to the buffer contains the TCP segment.

  @return Pointer to the TCP_SEG that contains the translated TCP head information.

**/
TCP_SEG *
TcpFormatNetbuf (
  IN     TCP_CB  *Tcb,
  IN OUT NET_BUF *Nbuf
  )
{
  TCP_SEG   *Seg;
  TCP_HEAD  *Head;

  Seg       = TCPSEG_NETBUF (Nbuf);
  Head      = (TCP_HEAD *) NetbufGetByte (Nbuf, 0, NULL);
  ASSERT (Head != NULL);

  Nbuf->Tcp = Head;

  Seg->Seq  = NTOHL (Head->Seq);
  Seg->Ack  = NTOHL (Head->Ack);
  Seg->End  = Seg->Seq + (Nbuf->TotalSize - (Head->HeadLen << 2));

  Seg->Urg  = NTOHS (Head->Urg);
  Seg->Wnd  = (NTOHS (Head->Wnd) << Tcb->SndWndScale);
  Seg->Flag = Head->Flag;

  //
  // SYN and FIN flag occupy one sequence space each.
  //
  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_SYN)) {
    //
    // RFC requires that the initial window not be scaled.
    //
    Seg->Wnd = NTOHS (Head->Wnd);
    Seg->End++;
  }

  if (TCP_FLG_ON (Seg->Flag, TCP_FLG_FIN)) {
    Seg->End++;
  }

  return Seg;
}

/**
  Initialize an active connection.

  @param[in, out]  Tcb          Pointer to the TCP_CB that wants to initiate a
                                connection.

**/
VOID
TcpOnAppConnect (
  IN OUT TCP_CB  *Tcb
  )
{
  TcpInitTcbLocal (Tcb);
  TcpSetState (Tcb, TCP_SYN_SENT);

  TcpSetTimer (Tcb, TCP_TIMER_CONNECT, Tcb->ConnectTimeout);
  TcpToSendData (Tcb, 1);
}

/**
  Initiate the connection close procedure, called when
  applications want to close the connection.

  @param[in, out]  Tcb          Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpOnAppClose (
  IN OUT TCP_CB *Tcb
  )
{
  ASSERT (Tcb != NULL);

  if (!IsListEmpty (&Tcb->RcvQue) || GET_RCV_DATASIZE (Tcb->Sk) != 0) {

    DEBUG (
      (EFI_D_WARN,
      "TcpOnAppClose: connection reset because data is lost for TCB %p\n",
      Tcb)
      );

    TcpResetConnection (Tcb);
    TcpClose (Tcb);
    return;
  }

  switch (Tcb->State) {
  case TCP_CLOSED:
  case TCP_LISTEN:
  case TCP_SYN_SENT:
    TcpSetState (Tcb, TCP_CLOSED);
    break;

  case TCP_SYN_RCVD:
  case TCP_ESTABLISHED:
    TcpSetState (Tcb, TCP_FIN_WAIT_1);
    break;

  case TCP_CLOSE_WAIT:
    TcpSetState (Tcb, TCP_LAST_ACK);
    break;
  default:
    break;
  }

  TcpToSendData (Tcb, 1);
}

/**
  Check whether the application's newly delivered data can be sent out.

  @param[in, out]  Tcb          Pointer to the TCP_CB of this TCP instance.

  @retval 0                     The data has been sent out successfully.
  @retval -1                    The Tcb is not in a state that data is permitted to
                                be sent out.

**/
INTN
TcpOnAppSend (
  IN OUT TCP_CB *Tcb
  )
{

  switch (Tcb->State) {
  case TCP_CLOSED:
    return -1;

  case TCP_LISTEN:
    return -1;

  case TCP_SYN_SENT:
  case TCP_SYN_RCVD:
    return 0;

  case TCP_ESTABLISHED:
  case TCP_CLOSE_WAIT:
    TcpToSendData (Tcb, 0);
    return 0;

  case TCP_FIN_WAIT_1:
  case TCP_FIN_WAIT_2:
  case TCP_CLOSING:
  case TCP_LAST_ACK:
  case TCP_TIME_WAIT:
    return -1;

  default:
    break;
  }

  return 0;
}

/**
  Application has consumed some data. Check whether
  to send a window update ack or a delayed ack.

  @param[in]  Tcb        Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpOnAppConsume (
  IN TCP_CB *Tcb
  )
{
  UINT32 TcpOld;

  switch (Tcb->State) {
  case TCP_ESTABLISHED:
    TcpOld = TcpRcvWinOld (Tcb);
    if (TcpRcvWinNow (Tcb) > TcpOld) {

      if (TcpOld < Tcb->RcvMss) {

        DEBUG (
          (EFI_D_INFO,
          "TcpOnAppConsume: send a window update for a window closed Tcb %p\n",
          Tcb)
          );

        TcpSendAck (Tcb);
      } else if (Tcb->DelayedAck == 0) {

        DEBUG (
          (EFI_D_INFO,
          "TcpOnAppConsume: scheduled a delayed ACK to update window for Tcb %p\n",
          Tcb)
          );

        Tcb->DelayedAck = 1;
      }
    }

    break;

  default:
    break;
  }
}

/**
  Abort the connection by sending a reset segment. Called
  when the application wants to abort the connection.

  @param[in]  Tcb                   Pointer to the TCP_CB of the TCP instance.

**/
VOID
TcpOnAppAbort (
  IN TCP_CB *Tcb
  )
{
  DEBUG (
    (EFI_D_WARN,
    "TcpOnAppAbort: connection reset issued by application for TCB %p\n",
    Tcb)
    );

  switch (Tcb->State) {
  case TCP_SYN_RCVD:
  case TCP_ESTABLISHED:
  case TCP_FIN_WAIT_1:
  case TCP_FIN_WAIT_2:
  case TCP_CLOSE_WAIT:
    TcpResetConnection (Tcb);
    break;
  default:
    break;
  }

  TcpSetState (Tcb, TCP_CLOSED);
}

/**
  Reset the connection related with Tcb.

  @param[in]  Tcb         Pointer to the TCP_CB of the connection to be reset.

**/
VOID
TcpResetConnection (
  IN TCP_CB *Tcb
  )
{
  NET_BUF   *Nbuf;
  TCP_HEAD  *Nhead;

  Nbuf = NetbufAlloc (TCP_MAX_HEAD);

  if (Nbuf == NULL) {
    return ;
  }

  Nhead = (TCP_HEAD *) NetbufAllocSpace (
                        Nbuf,
                        sizeof (TCP_HEAD),
                        NET_BUF_TAIL
                        );

  ASSERT (Nhead != NULL);

  Nbuf->Tcp       = Nhead;

  Nhead->Flag     = TCP_FLG_RST;
  Nhead->Seq      = HTONL (Tcb->SndNxt);
  Nhead->Ack      = HTONL (Tcb->RcvNxt);
  Nhead->SrcPort  = Tcb->LocalEnd.Port;
  Nhead->DstPort  = Tcb->RemoteEnd.Port;
  Nhead->HeadLen  = (UINT8) (sizeof (TCP_HEAD) >> 2);
  Nhead->Res      = 0;
  Nhead->Wnd      = HTONS (0xFFFF);
  Nhead->Checksum = 0;
  Nhead->Urg      = 0;
  Nhead->Checksum = TcpChecksum (Nbuf, Tcb->HeadSum);

  TcpSendIpPacket (Tcb, Nbuf, &Tcb->LocalEnd.Ip, &Tcb->RemoteEnd.Ip, Tcb->Sk->IpVersion);

  NetbufFree (Nbuf);
}

/**
  Install the device path protocol on the TCP instance.

  @param[in]  Sock          Pointer to the socket representing the TCP instance.

  @retval EFI_SUCCESS           The device path protocol was installed.
  @retval other                 Failed to install the device path protocol.

**/
EFI_STATUS
TcpInstallDevicePath (
  IN SOCKET *Sock
  )
{
  TCP_PROTO_DATA           *TcpProto;
  TCP_SERVICE_DATA         *TcpService;
  TCP_CB                   *Tcb;
  IPv4_DEVICE_PATH         Ip4DPathNode;
  IPv6_DEVICE_PATH         Ip6DPathNode;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_STATUS               Status;
  TCP_PORTNO               LocalPort;
  TCP_PORTNO               RemotePort;

  TcpProto   = (TCP_PROTO_DATA *) Sock->ProtoReserved;
  TcpService = TcpProto->TcpService;
  Tcb        = TcpProto->TcpPcb;

  LocalPort = NTOHS (Tcb->LocalEnd.Port);
  RemotePort = NTOHS (Tcb->RemoteEnd.Port);
  if (Sock->IpVersion == IP_VERSION_4) {
    NetLibCreateIPv4DPathNode (
      &Ip4DPathNode,
      TcpService->ControllerHandle,
      Tcb->LocalEnd.Ip.Addr[0],
      LocalPort,
      Tcb->RemoteEnd.Ip.Addr[0],
      RemotePort,
      EFI_IP_PROTO_TCP,
      Tcb->UseDefaultAddr
      );

    IP4_COPY_ADDRESS (&Ip4DPathNode.SubnetMask, &Tcb->SubnetMask);

    DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) &Ip4DPathNode;
  } else {
    NetLibCreateIPv6DPathNode (
      &Ip6DPathNode,
      TcpService->ControllerHandle,
      &Tcb->LocalEnd.Ip.v6,
      LocalPort,
      &Tcb->RemoteEnd.Ip.v6,
      RemotePort,
      EFI_IP_PROTO_TCP
      );

    DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) &Ip6DPathNode;
  }

  Sock->DevicePath = AppendDevicePathNode (Sock->ParentDevicePath, DevicePath);
  if (Sock->DevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->InstallProtocolInterface (
                  &Sock->SockHandle,
                  &gEfiDevicePathProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  Sock->DevicePath
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Sock->DevicePath);
    Sock->DevicePath = NULL;
  }

  return Status;
}

