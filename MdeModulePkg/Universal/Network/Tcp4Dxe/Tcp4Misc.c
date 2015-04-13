/** @file
  Misc support routines for tcp.

Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Tcp4Main.h"

#include <Library/DevicePathLib.h>

LIST_ENTRY      mTcpRunQue = {
  &mTcpRunQue,
  &mTcpRunQue
};

LIST_ENTRY      mTcpListenQue = {
  &mTcpListenQue,
  &mTcpListenQue
};

TCP_SEQNO       mTcpGlobalIss = 0x4d7e980b;

CHAR16   *mTcpStateName[] = {
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

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpInitTcbLocal (
  IN OUT TCP_CB *Tcb
  )
{
  //
  // Compute the checksum of the fixed parts of pseudo header
  //
  Tcb->HeadSum = NetPseudoHeadChecksum (
                  Tcb->LocalEnd.Ip,
                  Tcb->RemoteEnd.Ip,
                  0x06,
                  0
                  );

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

    RcvMss = TcpGetRcvMss (Tcb->Sk);
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

  if (TCP_FLG_ON (Opt->Flag, TCP_OPTION_RCVD_WS) &&
      !TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_WS)) {

    Tcb->SndWndScale  = Opt->WndScale;

    Tcb->RcvWndScale  = TcpComputeScale (Tcb);
    TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_RCVD_WS);

  } else {
    //
    // One end doesn't support window scale option. use zero.
    //
    Tcb->RcvWndScale = 0;
  }

  if (TCP_FLG_ON (Opt->Flag, TCP_OPTION_RCVD_TS) &&
      !TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_TS)) {

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
  Locate a listen TCB that matchs the Local and Remote.

  @param  Local                 Pointer to the local (IP, Port).
  @param  Remote                Pointer to the remote (IP, Port).

  @return  Pointer to the TCP_CB with the least number of wildcard,
           if NULL no match is found.

**/
TCP_CB *
TcpLocateListenTcb (
  IN TCP_PEER *Local,
  IN TCP_PEER *Remote
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

    if ((Local->Port != Node->LocalEnd.Port) ||
        !TCP_PEER_MATCH (Remote, &Node->RemoteEnd) ||
        !TCP_PEER_MATCH (Local, &Node->LocalEnd)) {

      continue;
    }

    //
    // Compute the number of wildcard
    //
    Cur = 0;
    if (Node->RemoteEnd.Ip == 0) {
      Cur++;
    }

    if (Node->RemoteEnd.Port == 0) {
      Cur++;
    }

    if (Node->LocalEnd.Ip == 0) {
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

  @param  Addr                  Pointer to the IP address needs to match.
  @param  Port                  The port number needs to match.

  @return  The Tcb which matches the <Addr Port> paire exists or not.

**/
BOOLEAN
TcpFindTcbByPeer (
  IN EFI_IPv4_ADDRESS  *Addr,
  IN TCP_PORTNO        Port
  )
{
  TCP_PORTNO      LocalPort;
  LIST_ENTRY      *Entry;
  TCP_CB          *Tcb;

  ASSERT ((Addr != NULL) && (Port != 0));

  LocalPort = HTONS (Port);

  NET_LIST_FOR_EACH (Entry, &mTcpListenQue) {
    Tcb = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if (EFI_IP4_EQUAL (Addr, &Tcb->LocalEnd.Ip) &&
      (LocalPort == Tcb->LocalEnd.Port)) {

      return TRUE;
    }
  }

  NET_LIST_FOR_EACH (Entry, &mTcpRunQue) {
    Tcb = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if (EFI_IP4_EQUAL (Addr, &Tcb->LocalEnd.Ip) &&
      (LocalPort == Tcb->LocalEnd.Port)) {

      return TRUE;
    }
  }

  return FALSE;
}


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
  )
{
  TCP_PEER        Local;
  TCP_PEER        Remote;
  LIST_ENTRY      *Entry;
  TCP_CB          *Tcb;

  Local.Port  = LocalPort;
  Local.Ip    = LocalIp;

  Remote.Port = RemotePort;
  Remote.Ip   = RemoteIp;

  //
  // First check for exact match.
  //
  NET_LIST_FOR_EACH (Entry, &mTcpRunQue) {
    Tcb = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if (TCP_PEER_EQUAL (&Remote, &Tcb->RemoteEnd) &&
        TCP_PEER_EQUAL (&Local, &Tcb->LocalEnd)) {

      RemoveEntryList (&Tcb->List);
      InsertHeadList (&mTcpRunQue, &Tcb->List);

      return Tcb;
    }
  }

  //
  // Only check listen queue when SYN flag is on
  //
  if (Syn) {
    return TcpLocateListenTcb (&Local, &Remote);
  }

  return NULL;
}


/**
  Insert a Tcb into the proper queue.

  @param  Tcb                   Pointer to the TCP_CB to be inserted.

  @retval 0                     The Tcb is inserted successfully.
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
    ((Tcb->State == TCP_LISTEN) ||
     (Tcb->State == TCP_SYN_SENT) ||
     (Tcb->State == TCP_SYN_RCVD) ||
     (Tcb->State == TCP_CLOSED))
    );

  if (Tcb->LocalEnd.Port == 0) {
    return -1;
  }

  Head = &mTcpRunQue;

  if (Tcb->State == TCP_LISTEN) {
    Head = &mTcpListenQue;
  }

  //
  // Check that Tcb isn't already on the list.
  //
  NET_LIST_FOR_EACH (Entry, Head) {
    Node = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if (TCP_PEER_EQUAL (&Tcb->LocalEnd, &Node->LocalEnd) &&
        TCP_PEER_EQUAL (&Tcb->RemoteEnd, &Node->RemoteEnd)) {

      return -1;
    }
  }

  InsertHeadList (Head, &Tcb->List);

  return 0;
}


/**
  Clone a TCB_CB from Tcb.

  @param  Tcb                   Pointer to the TCP_CB to be cloned.

  @return  Pointer to the new cloned TCP_CB, if NULL error condition occurred.

**/
TCP_CB *
TcpCloneTcb (
  IN TCP_CB *Tcb
  )
{
  TCP_CB               *Clone;

  Clone = AllocatePool (sizeof (TCP_CB));

  if (Clone == NULL) {
    return NULL;

  }

  CopyMem (Clone, Tcb, sizeof (TCP_CB));

  //
  // Increate the reference count of the shared IpInfo.
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

  ((TCP4_PROTO_DATA *) (Clone->Sk->ProtoReserved))->TcpPcb = Clone;

  return Clone;
}


/**
  Compute an ISS to be used by a new connection.

  @return  The result ISS.

**/
TCP_SEQNO
TcpGetIss (
  VOID
  )
{
  mTcpGlobalIss += 2048;
  return mTcpGlobalIss;
}


/**
  Get the local mss.

  @param  Sock        Pointer to the socket to get mss

  @return  The mss size.

**/
UINT16
TcpGetRcvMss (
  IN SOCKET  *Sock
  )
{
  EFI_IP4_MODE_DATA       Ip4Mode;
  TCP4_PROTO_DATA         *TcpProto;
  EFI_IP4_PROTOCOL        *Ip;

  ASSERT (Sock != NULL);

  TcpProto = (TCP4_PROTO_DATA *) Sock->ProtoReserved;
  Ip       = TcpProto->TcpService->IpIo->Ip.Ip4;
  ASSERT (Ip != NULL);

  Ip->GetModeData (Ip, &Ip4Mode, NULL, NULL);

  return (UINT16) (Ip4Mode.MaxPacketSize - sizeof (TCP_HEAD));
}


/**
  Set the Tcb's state.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.
  @param  State                 The state to be set.

**/
VOID
TcpSetState (
  IN OUT TCP_CB  *Tcb,
  IN     UINT8   State
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
      // A new connection is accepted by a listening socket, install
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
  )
{
  UINT16  Checksum;

  Checksum  = NetbufChecksum (Nbuf);
  Checksum  = NetAddChecksum (Checksum, HeadSum);

  Checksum  = NetAddChecksum (
                Checksum,
                HTONS ((UINT16) Nbuf->TotalSize)
                );

  return (UINT16) ~Checksum;
}

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
    // RFC requires that initial window not be scaled
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
  Reset the connection related with Tcb.

  @param  Tcb                   Pointer to the TCP_CB of the connection to be
                                reset.

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

  TcpSendIpPacket (Tcb, Nbuf, Tcb->LocalEnd.Ip, Tcb->RemoteEnd.Ip);

  NetbufFree (Nbuf);
}


/**
  Initialize an active connection.

  @param  Tcb                   Pointer to the TCP_CB that wants to initiate a
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

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpOnAppClose (
  IN OUT TCP_CB *Tcb
  )
{
  ASSERT (Tcb != NULL);

  if (!IsListEmpty (&Tcb->RcvQue) || GET_RCV_DATASIZE (Tcb->Sk) != 0) {

    DEBUG ((EFI_D_WARN, "TcpOnAppClose: connection reset "
      "because data is lost for TCB %p\n", Tcb));

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

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.

  @retval 0                     Whether the data is sent out or is buffered for
                                further sending.
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
  Application has consumed some data, check whether
  to send a window updata ack or a delayed ack.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpOnAppConsume (
  IN TCP_CB *Tcb
  )
{
  UINT32 TcpOld;

  switch (Tcb->State) {
  case TCP_CLOSED:
    return;

  case TCP_LISTEN:
    return;

  case TCP_SYN_SENT:
  case TCP_SYN_RCVD:
    return;

  case TCP_ESTABLISHED:
    TcpOld = TcpRcvWinOld (Tcb);
    if (TcpRcvWinNow (Tcb) > TcpOld) {

      if (TcpOld < Tcb->RcvMss) {

        DEBUG ((EFI_D_INFO, "TcpOnAppConsume: send a window"
          " update for a window closed Tcb %p\n", Tcb));

        TcpSendAck (Tcb);
      } else if (Tcb->DelayedAck == 0) {

        DEBUG ((EFI_D_INFO, "TcpOnAppConsume: scheduled a delayed"
          " ACK to update window for Tcb %p\n", Tcb));

        Tcb->DelayedAck = 1;
      }
    }

    break;

  case TCP_CLOSE_WAIT:
    return;

  case TCP_FIN_WAIT_1:
  case TCP_FIN_WAIT_2:
  case TCP_CLOSING:
  case TCP_LAST_ACK:
  case TCP_TIME_WAIT:
    return;

  default:
    break;
  }
}


/**
  Abort the connection by sending a reset segment, called
  when the application wants to abort the connection.

  @param  Tcb                   Pointer to the TCP_CB of the TCP instance.

**/
VOID
TcpOnAppAbort (
  IN TCP_CB *Tcb
  )
{
  DEBUG ((EFI_D_WARN, "TcpOnAppAbort: connection reset "
    "issued by application for TCB %p\n", Tcb));

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
  Install the device path protocol on the TCP instance.

  @param  Sock             Pointer to the socket representing the TCP instance.

  @retval  EFI_SUCCESS     The device path protocol is installed.
  @retval  other           Failed to install the device path protocol.

**/
EFI_STATUS
TcpInstallDevicePath (
  IN SOCKET *Sock
  )
{
  TCP4_PROTO_DATA    *TcpProto;
  TCP4_SERVICE_DATA  *TcpService;
  TCP_CB             *Tcb;
  IPv4_DEVICE_PATH   Ip4DPathNode;
  EFI_STATUS         Status;
  TCP_PORTNO         LocalPort;
  TCP_PORTNO         RemotePort;

  TcpProto   = (TCP4_PROTO_DATA *) Sock->ProtoReserved;
  TcpService = TcpProto->TcpService;
  Tcb        = TcpProto->TcpPcb;

  LocalPort = NTOHS (Tcb->LocalEnd.Port);
  RemotePort = NTOHS (Tcb->RemoteEnd.Port);
  NetLibCreateIPv4DPathNode (
    &Ip4DPathNode,
    TcpService->ControllerHandle,
    Tcb->LocalEnd.Ip,
    LocalPort,
    Tcb->RemoteEnd.Ip,
    RemotePort,
    EFI_IP_PROTO_TCP,
    Tcb->UseDefaultAddr
    );

  IP4_COPY_ADDRESS (&Ip4DPathNode.SubnetMask, &Tcb->SubnetMask);

  Sock->DevicePath = AppendDevicePathNode (
                       Sock->ParentDevicePath,
                       (EFI_DEVICE_PATH_PROTOCOL *) &Ip4DPathNode
                       );
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
  }

  return Status;
}
