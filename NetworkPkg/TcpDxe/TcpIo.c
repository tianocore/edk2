/** @file
  Implementation of I/O interfaces between TCP and IpIoLib.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TcpMain.h"

/**
  Packet receive callback function provided to IP_IO, used to call
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
  IN EFI_STATUS                       Status,
  IN UINT8                            IcmpErr,
  IN EFI_NET_SESSION_DATA             *NetSession,
  IN NET_BUF                          *Pkt,
  IN VOID                             *Context    OPTIONAL
  )
{
  if (EFI_SUCCESS == Status) {
    TcpInput (Pkt, &NetSession->Source, &NetSession->Dest, NetSession->IpVersion);
  } else {
    TcpIcmpInput (
      Pkt,
      IcmpErr,
      &NetSession->Source,
      &NetSession->Dest,
      NetSession->IpVersion
      );
  }
}

/**
  Send the segment to IP via IpIo function.

  @param[in]  Tcb                Pointer to the TCP_CB of this TCP instance.
  @param[in]  Nbuf               Pointer to the TCP segment to be sent.
  @param[in]  Src                Source address of the TCP segment.
  @param[in]  Dest               Destination address of the TCP segment.
  @param[in]  Version            IP_VERSION_4 or IP_VERSION_6

  @retval 0                      The segment was sent out successfully.
  @retval -1                     The segment failed to send.

**/
INTN
TcpSendIpPacket (
  IN TCP_CB          *Tcb,
  IN NET_BUF         *Nbuf,
  IN EFI_IP_ADDRESS  *Src,
  IN EFI_IP_ADDRESS  *Dest,
  IN UINT8           Version
  )
{
  EFI_STATUS       Status;
  IP_IO            *IpIo;
  IP_IO_OVERRIDE   Override;
  SOCKET           *Sock;
  VOID             *IpSender;
  TCP_PROTO_DATA  *TcpProto;

  if (NULL == Tcb) {

    IpIo     = NULL;
    IpSender = IpIoFindSender (&IpIo, Version, Src);

    if (IpSender == NULL) {
      DEBUG ((EFI_D_WARN, "TcpSendIpPacket: No appropriate IpSender.\n"));
      return -1;
    }

    if (Version == IP_VERSION_6) {
      //
      // It's tricky here. EFI IPv6 Spec don't allow an instance overriding the
      // destination address if the dest is already specified through the
      // configuration data. Here we get the IpIo we need and use the default IP
      // instance in this IpIo to send the packet. The dest address is configured
      // to be the unspecified address for the default IP instance.
      //
      IpSender = NULL;
    }
  } else {

    Sock     = Tcb->Sk;
    TcpProto = (TCP_PROTO_DATA *) Sock->ProtoReserved;
    IpIo     = TcpProto->TcpService->IpIo;
    IpSender = Tcb->IpInfo;

    if (Version == IP_VERSION_6) {
      //
      // It's IPv6 and this TCP segment belongs to a solid TCB, in such case
      // the destination address can't be overridden, so reset the Dest to NULL.
      //
      if (!Tcb->RemoteIpZero) {
        Dest = NULL;
      }
    }
  }

  ASSERT (Version == IpIo->IpVersion);

  if (Version == IP_VERSION_4) {
    Override.Ip4OverrideData.TypeOfService = 0;
    Override.Ip4OverrideData.TimeToLive    = 255;
    Override.Ip4OverrideData.DoNotFragment = FALSE;
    Override.Ip4OverrideData.Protocol      = EFI_IP_PROTO_TCP;
    ZeroMem (&Override.Ip4OverrideData.GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Override.Ip4OverrideData.SourceAddress, Src, sizeof (EFI_IPv4_ADDRESS));
  } else {
    Override.Ip6OverrideData.Protocol  = EFI_IP_PROTO_TCP;
    Override.Ip6OverrideData.HopLimit  = 255;
    Override.Ip6OverrideData.FlowLabel = 0;
  }

  Status = IpIoSend (IpIo, Nbuf, IpSender, NULL, NULL, Dest, &Override);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TcpSendIpPacket: return %r error\n", Status));
    return -1;
  }

  return 0;
}

/**
  Refresh the remote peer's Neighbor Cache State if already exists.

  @param[in]  Tcb                Pointer to the TCP_CB of this TCP instance.
  @param[in]  Neighbor           Source address of the TCP segment.
  @param[in]  Timeout            Time in 100-ns units that this entry will remain
                                 in the neighbor cache. A value of zero means that
                                 the entry  is permanent. A value of non-zero means
                                 that the entry is dynamic and will be deleted
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
  )
{
  IP_IO            *IpIo;
  SOCKET           *Sock;
  TCP_PROTO_DATA  *TcpProto;

  if (NULL == Tcb) {
    IpIo = NULL;
    IpIoFindSender (&IpIo, IP_VERSION_6, Neighbor);

    if (IpIo == NULL) {
      DEBUG ((EFI_D_WARN, "Tcp6AddNeighbor: No appropriate IpIo.\n"));
      return EFI_NOT_STARTED;
    }

  } else {
    Sock     = Tcb->Sk;
    TcpProto = (TCP_PROTO_DATA *) Sock->ProtoReserved;
    IpIo     = TcpProto->TcpService->IpIo;
  }

  return IpIoRefreshNeighbor (IpIo, Neighbor, Timeout);
}

