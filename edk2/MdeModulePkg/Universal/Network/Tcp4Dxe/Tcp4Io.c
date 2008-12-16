/** @file
  I/O interfaces between TCP and IpIo.

Copyright (c) 2005 - 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Tcp4Main.h"


/**
  Packet receive callback function provided to IP_IO, used to call
  the proper function to handle the packet received by IP.

  @param  Status      Status of the received packet.
  @param  IcmpErr     ICMP error number.
  @param  NetSession  Pointer to the net session of this packet.
  @param  Pkt         Pointer to the recieved packet.
  @param  Context     Pointer to the context configured in IpIoOpen(), not used
                      now.

  @return None

**/
VOID
Tcp4RxCallback (
  IN EFI_STATUS                       Status,
  IN ICMP_ERROR                       IcmpErr,
  IN EFI_NET_SESSION_DATA             *NetSession,
  IN NET_BUF                          *Pkt,
  IN VOID                             *Context    OPTIONAL
  )
{
  if (EFI_SUCCESS == Status) {
    TcpInput (Pkt, NetSession->Source, NetSession->Dest);
  } else {
    TcpIcmpInput (Pkt, IcmpErr, NetSession->Source, NetSession->Dest);
  }
}


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
  )
{
  EFI_STATUS       Status;
  IP_IO            *IpIo;
  IP_IO_OVERRIDE   Override;
  SOCKET           *Sock;
  VOID             *IpSender;
  TCP4_PROTO_DATA  *TcpProto;

  if (NULL == Tcb) {

    IpIo     = NULL;
    IpSender = IpIoFindSender (&IpIo, Src);

    if (IpSender == NULL) {
      DEBUG ((EFI_D_WARN, "TcpSendIpPacket: No appropriate IpSender.\n"));
      return -1;
    }
  } else {

    Sock     = Tcb->Sk;
    TcpProto = (TCP4_PROTO_DATA *) Sock->ProtoReserved;
    IpIo     = TcpProto->TcpService->IpIo;
    IpSender = Tcb->IpInfo;
  }

  Override.TypeOfService            = 0;
  Override.TimeToLive               = 255;
  Override.DoNotFragment            = FALSE;
  Override.Protocol                 = EFI_IP_PROTO_TCP;
  ZeroMem (&Override.GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Override.SourceAddress, &Src, sizeof (EFI_IPv4_ADDRESS));

  Status = IpIoSend (IpIo, Nbuf, IpSender, NULL, NULL, Dest, &Override);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TcpSendIpPacket: return %r error\n", Status));
    return -1;
  }

  return 0;
}
