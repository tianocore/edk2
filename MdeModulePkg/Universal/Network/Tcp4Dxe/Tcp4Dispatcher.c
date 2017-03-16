/** @file
  Tcp request dispatcher implementation.

(C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2005 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Tcp4Main.h"

#define TCP_COMP_VAL(Min, Max, Default, Val) \
  ((((Val) <= (Max)) && ((Val) >= (Min))) ? (Val) : (Default))

/**
  Add or remove a route entry in the IP route table associated with this TCP instance.

  @param  Tcb                   Pointer to the TCP_CB of this TCP instance.
  @param  RouteInfo             Pointer to the route info to be processed.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_NOT_STARTED       The driver instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration(DHCP,
                                BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_OUT_OF_RESOURCES  Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND         This route is not in the routing table
                                (when RouteInfo->DeleteRoute is TRUE).
  @retval EFI_ACCESS_DENIED     The route is already defined in the routing table
                                (when RouteInfo->DeleteRoute is FALSE).
**/
EFI_STATUS
Tcp4Route (
  IN TCP_CB           *Tcb,
  IN TCP4_ROUTE_INFO  *RouteInfo
  )
{
  EFI_IP4_PROTOCOL  *Ip4;

  Ip4 = Tcb->IpInfo->Ip.Ip4;

  ASSERT (Ip4 != NULL);

  return Ip4->Routes (
              Ip4,
              RouteInfo->DeleteRoute,
              RouteInfo->SubnetAddress,
              RouteInfo->SubnetMask,
              RouteInfo->GatewayAddress
              );

}


/**
  Get the operational settings of this TCP instance.

  @param  Tcb                    Pointer to the TCP_CB of this TCP instance.
  @param  Mode                   Pointer to the buffer to store the operational
                                 settings.

  @retval EFI_SUCCESS            The mode data is read.
  @retval EFI_NOT_STARTED        No configuration data is available because this
                                 instance hasn't been started.

**/
EFI_STATUS
Tcp4GetMode (
  IN     TCP_CB         *Tcb,
  IN OUT TCP4_MODE_DATA *Mode
  )
{
  SOCKET                *Sock;
  EFI_TCP4_CONFIG_DATA  *ConfigData;
  EFI_TCP4_ACCESS_POINT *AccessPoint;
  EFI_TCP4_OPTION       *Option;
  EFI_IP4_PROTOCOL      *Ip;

  Sock = Tcb->Sk;

  if (!SOCK_IS_CONFIGURED (Sock) && (Mode->Tcp4ConfigData != NULL)) {
    return EFI_NOT_STARTED;
  }

  if (Mode->Tcp4State != NULL) {
    *(Mode->Tcp4State) = (EFI_TCP4_CONNECTION_STATE) Tcb->State;
  }

  if (Mode->Tcp4ConfigData != NULL) {

    ConfigData                      = Mode->Tcp4ConfigData;
    AccessPoint                     = &(ConfigData->AccessPoint);
    Option                          = ConfigData->ControlOption;

    ConfigData->TypeOfService       = Tcb->Tos;
    ConfigData->TimeToLive          = Tcb->Ttl;

    AccessPoint->UseDefaultAddress  = Tcb->UseDefaultAddr;

    IP4_COPY_ADDRESS (&AccessPoint->StationAddress, &Tcb->LocalEnd.Ip);
    IP4_COPY_ADDRESS (&AccessPoint->SubnetMask, &Tcb->SubnetMask);
    AccessPoint->StationPort        = NTOHS (Tcb->LocalEnd.Port);

    IP4_COPY_ADDRESS (&AccessPoint->RemoteAddress, &Tcb->RemoteEnd.Ip);
    AccessPoint->RemotePort         = NTOHS (Tcb->RemoteEnd.Port);
    AccessPoint->ActiveFlag         = (BOOLEAN) (Tcb->State != TCP_LISTEN);

    if (Option != NULL) {
      Option->ReceiveBufferSize       = GET_RCV_BUFFSIZE (Tcb->Sk);
      Option->SendBufferSize          = GET_SND_BUFFSIZE (Tcb->Sk);
      Option->MaxSynBackLog           = GET_BACKLOG (Tcb->Sk);

      Option->ConnectionTimeout       = Tcb->ConnectTimeout / TCP_TICK_HZ;
      Option->DataRetries             = Tcb->MaxRexmit;
      Option->FinTimeout              = Tcb->FinWait2Timeout / TCP_TICK_HZ;
      Option->TimeWaitTimeout         = Tcb->TimeWaitTimeout / TCP_TICK_HZ;
      Option->KeepAliveProbes         = Tcb->MaxKeepAlive;
      Option->KeepAliveTime           = Tcb->KeepAliveIdle / TCP_TICK_HZ;
      Option->KeepAliveInterval       = Tcb->KeepAlivePeriod / TCP_TICK_HZ;

      Option->EnableNagle         = (BOOLEAN) (!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_NAGLE));
      Option->EnableTimeStamp     = (BOOLEAN) (!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_TS));
      Option->EnableWindowScaling = (BOOLEAN) (!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_WS));

      Option->EnableSelectiveAck      = FALSE;
      Option->EnablePathMtuDiscovery  = FALSE;
    }
  }

  Ip = Tcb->IpInfo->Ip.Ip4;
  ASSERT (Ip != NULL);

  return Ip->GetModeData (Ip, Mode->Ip4ModeData, Mode->MnpConfigData, Mode->SnpModeData);
}


/**
  If AP->StationPort isn't zero, check whether the access point
  is registered, else generate a random station port for this
  access point.

  @param  AP                     Pointer to the access point.

  @retval EFI_SUCCESS            The check is passed or the port is assigned.
  @retval EFI_INVALID_PARAMETER  The non-zero station port is already used.
  @retval EFI_OUT_OF_RESOURCES   No port can be allocated.

**/
EFI_STATUS
Tcp4Bind (
  IN EFI_TCP4_ACCESS_POINT *AP
  )
{
  BOOLEAN Cycle;

  if (0 != AP->StationPort) {
    //
    // check if a same endpoint is bound
    //
    if (TcpFindTcbByPeer (&AP->StationAddress, AP->StationPort)) {

      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // generate a random port
    //
    Cycle = FALSE;

    if (TCP4_PORT_USER_RESERVED == mTcp4RandomPort) {
      mTcp4RandomPort = TCP4_PORT_KNOWN;
    }

    mTcp4RandomPort++;

    while (TcpFindTcbByPeer (&AP->StationAddress, mTcp4RandomPort)) {

      mTcp4RandomPort++;

      if (mTcp4RandomPort <= TCP4_PORT_KNOWN) {

        if (Cycle) {
          DEBUG ((EFI_D_ERROR, "Tcp4Bind: no port can be allocated "
            "for this pcb\n"));

          return EFI_OUT_OF_RESOURCES;
        }

        mTcp4RandomPort = TCP4_PORT_KNOWN + 1;

        Cycle = TRUE;
      }

    }

    AP->StationPort = mTcp4RandomPort;
  }

  return EFI_SUCCESS;
}


/**
  Flush the Tcb add its associated protocols.

  @param  Tcb                    Pointer to the TCP_CB to be flushed.

**/
VOID
Tcp4FlushPcb (
  IN TCP_CB *Tcb
  )
{
  SOCKET           *Sock;

  IpIoConfigIp (Tcb->IpInfo, NULL);

  Sock     = Tcb->Sk;

  if (SOCK_IS_CONFIGURED (Sock)) {
    RemoveEntryList (&Tcb->List);

    //
    // Uninstall the device path protocol.
    //
    if (Sock->DevicePath != NULL) {
      gBS->UninstallProtocolInterface (
             Sock->SockHandle,
             &gEfiDevicePathProtocolGuid,
             Sock->DevicePath
             );
      FreePool (Sock->DevicePath);
    }
  }

  NetbufFreeList (&Tcb->SndQue);
  NetbufFreeList (&Tcb->RcvQue);
  Tcb->State = TCP_CLOSED;
}

/**
  Attach a Pcb to the socket.

  @param  Sk                     Pointer to the socket of this TCP instance.

  @retval EFI_SUCCESS            The operation is completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Failed due to resource limit.

**/
EFI_STATUS
Tcp4AttachPcb (
  IN SOCKET  *Sk
  )
{
  TCP_CB            *Tcb;
  TCP4_PROTO_DATA   *ProtoData;
  IP_IO             *IpIo;
  EFI_STATUS        Status;
  VOID              *Ip;

  Tcb = AllocateZeroPool (sizeof (TCP_CB));

  if (Tcb == NULL) {

    DEBUG ((EFI_D_ERROR, "Tcp4ConfigurePcb: failed to allocate a TCB\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  ProtoData  = (TCP4_PROTO_DATA *) Sk->ProtoReserved;
  IpIo       = ProtoData->TcpService->IpIo;

  //
  // Create an IpInfo for this Tcb.
  //
  Tcb->IpInfo = IpIoAddIp (IpIo);
  if (Tcb->IpInfo == NULL) {

    FreePool (Tcb);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Open the new created IP instance BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  Tcb->IpInfo->ChildHandle,
                  &gEfiIp4ProtocolGuid,
                  &Ip,
                  IpIo->Image,
                  Sk->SockHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    IpIoRemoveIp (IpIo, Tcb->IpInfo);
    return Status;
  }

  InitializeListHead (&Tcb->List);
  InitializeListHead (&Tcb->SndQue);
  InitializeListHead (&Tcb->RcvQue);

  Tcb->State        = TCP_CLOSED;
  Tcb->Sk           = Sk;
  ProtoData->TcpPcb = Tcb;

  return EFI_SUCCESS;
}

/**
  Detach the Pcb of the socket.

  @param  Sk                     Pointer to the socket of this TCP instance.

**/
VOID
Tcp4DetachPcb (
  IN SOCKET  *Sk
  )
{
  TCP4_PROTO_DATA  *ProtoData;
  TCP_CB           *Tcb;

  ProtoData = (TCP4_PROTO_DATA *) Sk->ProtoReserved;
  Tcb       = ProtoData->TcpPcb;

  ASSERT (Tcb != NULL);

  Tcp4FlushPcb (Tcb);
  
  IpIoRemoveIp (ProtoData->TcpService->IpIo, Tcb->IpInfo);

  FreePool (Tcb);

  ProtoData->TcpPcb = NULL;
}


/**
  Configure the Pcb using CfgData.

  @param  Sk                     Pointer to the socket of this TCP instance.
  @param  CfgData                Pointer to the TCP configuration data.

  @retval EFI_SUCCESS            The operation is completed successfully.
  @retval EFI_INVALID_PARAMETER  A same access point has been configured in
                                 another TCP instance.
  @retval EFI_OUT_OF_RESOURCES   Failed due to resource limit.

**/
EFI_STATUS
Tcp4ConfigurePcb (
  IN SOCKET               *Sk,
  IN EFI_TCP4_CONFIG_DATA *CfgData
  )
{
  EFI_IP4_CONFIG_DATA IpCfgData;
  EFI_STATUS          Status;
  EFI_TCP4_OPTION     *Option;
  TCP4_PROTO_DATA     *TcpProto;
  TCP_CB              *Tcb;

  ASSERT ((CfgData != NULL) && (Sk != NULL) && (Sk->SockHandle != NULL));

  TcpProto = (TCP4_PROTO_DATA *) Sk->ProtoReserved;
  Tcb      = TcpProto->TcpPcb;

  ASSERT (Tcb != NULL);

  //
  // Add Ip for send pkt to the peer
  //
  CopyMem (&IpCfgData, &mIp4IoDefaultIpConfigData, sizeof (IpCfgData));
  IpCfgData.DefaultProtocol   = EFI_IP_PROTO_TCP;
  IpCfgData.UseDefaultAddress = CfgData->AccessPoint.UseDefaultAddress;
  IpCfgData.StationAddress    = CfgData->AccessPoint.StationAddress;
  IpCfgData.SubnetMask        = CfgData->AccessPoint.SubnetMask;
  IpCfgData.ReceiveTimeout    = (UINT32) (-1);

  //
  // Configure the IP instance this Tcb consumes.
  //
  Status = IpIoConfigIp (Tcb->IpInfo, &IpCfgData);
  if (EFI_ERROR (Status)) {
    goto OnExit;
  }

  //
  // Get the default address info if the instance is configured to use default address.
  //
  if (CfgData->AccessPoint.UseDefaultAddress) {
    CfgData->AccessPoint.StationAddress = IpCfgData.StationAddress;
    CfgData->AccessPoint.SubnetMask     = IpCfgData.SubnetMask;
  }

  //
  // check if we can bind this endpoint in CfgData
  //
  Status = Tcp4Bind (&(CfgData->AccessPoint));

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Tcp4ConfigurePcb: Bind endpoint failed "
      "with %r\n", Status));

    goto OnExit;
  }

  //
  // Initalize the operating information in this Tcb
  //
  ASSERT (Tcb->State == TCP_CLOSED &&
    IsListEmpty (&Tcb->SndQue) &&
    IsListEmpty (&Tcb->RcvQue));

  TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_NO_KEEPALIVE);
  Tcb->State            = TCP_CLOSED;

  Tcb->SndMss           = 536;
  Tcb->RcvMss           = TcpGetRcvMss (Sk);

  Tcb->SRtt             = 0;
  Tcb->Rto              = 3 * TCP_TICK_HZ;

  Tcb->CWnd             = Tcb->SndMss;
  Tcb->Ssthresh         = 0xffffffff;

  Tcb->CongestState     = TCP_CONGEST_OPEN;

  Tcb->KeepAliveIdle    = TCP_KEEPALIVE_IDLE_MIN;
  Tcb->KeepAlivePeriod  = TCP_KEEPALIVE_PERIOD;
  Tcb->MaxKeepAlive     = TCP_MAX_KEEPALIVE;
  Tcb->MaxRexmit        = TCP_MAX_LOSS;
  Tcb->FinWait2Timeout  = TCP_FIN_WAIT2_TIME;
  Tcb->TimeWaitTimeout  = TCP_TIME_WAIT_TIME;
  Tcb->ConnectTimeout   = TCP_CONNECT_TIME;

  //
  // initialize Tcb in the light of CfgData
  //
  Tcb->Ttl            = CfgData->TimeToLive;
  Tcb->Tos            = CfgData->TypeOfService;

  Tcb->UseDefaultAddr = CfgData->AccessPoint.UseDefaultAddress;

  CopyMem (&Tcb->LocalEnd.Ip, &CfgData->AccessPoint.StationAddress, sizeof (IP4_ADDR));
  Tcb->LocalEnd.Port  = HTONS (CfgData->AccessPoint.StationPort);
  IP4_COPY_ADDRESS (&Tcb->SubnetMask, &CfgData->AccessPoint.SubnetMask);

  if (CfgData->AccessPoint.ActiveFlag) {
    CopyMem (&Tcb->RemoteEnd.Ip, &CfgData->AccessPoint.RemoteAddress, sizeof (IP4_ADDR));
    Tcb->RemoteEnd.Port = HTONS (CfgData->AccessPoint.RemotePort);
  } else {
    Tcb->RemoteEnd.Ip   = 0;
    Tcb->RemoteEnd.Port = 0;
  }

  Option              = CfgData->ControlOption;

  if (Option != NULL) {
    SET_RCV_BUFFSIZE (
      Sk,
      (UINT32) (TCP_COMP_VAL (
                  TCP_RCV_BUF_SIZE_MIN,
                  TCP_RCV_BUF_SIZE,
                  TCP_RCV_BUF_SIZE,
                  Option->ReceiveBufferSize
                  )
               )
      );
    SET_SND_BUFFSIZE (
      Sk,
      (UINT32) (TCP_COMP_VAL (
                  TCP_SND_BUF_SIZE_MIN,
                  TCP_SND_BUF_SIZE,
                  TCP_SND_BUF_SIZE,
                  Option->SendBufferSize
                  )
               )
      );

    SET_BACKLOG (
      Sk,
      (UINT32) (TCP_COMP_VAL (
                  TCP_BACKLOG_MIN,
                  TCP_BACKLOG,
                  TCP_BACKLOG,
                  Option->MaxSynBackLog
                  )
               )
      );

    Tcb->MaxRexmit = (UINT16) TCP_COMP_VAL (
                                TCP_MAX_LOSS_MIN,
                                TCP_MAX_LOSS,
                                TCP_MAX_LOSS,
                                Option->DataRetries
                                );
    Tcb->FinWait2Timeout = TCP_COMP_VAL (
                              TCP_FIN_WAIT2_TIME,
                              TCP_FIN_WAIT2_TIME_MAX,
                              TCP_FIN_WAIT2_TIME,
                              (UINT32) (Option->FinTimeout * TCP_TICK_HZ)
                              );

    if (Option->TimeWaitTimeout != 0) {
      Tcb->TimeWaitTimeout = TCP_COMP_VAL (
                               TCP_TIME_WAIT_TIME,
                               TCP_TIME_WAIT_TIME_MAX,
                               TCP_TIME_WAIT_TIME,
                               (UINT32) (Option->TimeWaitTimeout * TCP_TICK_HZ)
                               );
    } else {
      Tcb->TimeWaitTimeout = 0;
    }

    if (Option->KeepAliveProbes != 0) {
      TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_NO_KEEPALIVE);

      Tcb->MaxKeepAlive = (UINT8) TCP_COMP_VAL (
                                    TCP_MAX_KEEPALIVE_MIN,
                                    TCP_MAX_KEEPALIVE,
                                    TCP_MAX_KEEPALIVE,
                                    Option->KeepAliveProbes
                                    );
      Tcb->KeepAliveIdle = TCP_COMP_VAL (
                             TCP_KEEPALIVE_IDLE_MIN,
                             TCP_KEEPALIVE_IDLE_MAX,
                             TCP_KEEPALIVE_IDLE_MIN,
                             (UINT32) (Option->KeepAliveTime * TCP_TICK_HZ)
                             );
      Tcb->KeepAlivePeriod = TCP_COMP_VAL (
                               TCP_KEEPALIVE_PERIOD_MIN,
                               TCP_KEEPALIVE_PERIOD,
                               TCP_KEEPALIVE_PERIOD,
                               (UINT32) (Option->KeepAliveInterval * TCP_TICK_HZ)
                               );
    }

    Tcb->ConnectTimeout = TCP_COMP_VAL (
                            TCP_CONNECT_TIME_MIN,
                            TCP_CONNECT_TIME,
                            TCP_CONNECT_TIME,
                            (UINT32) (Option->ConnectionTimeout * TCP_TICK_HZ)
                            );

    if (!Option->EnableNagle) {
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_NO_NAGLE);
    }

    if (!Option->EnableTimeStamp) {
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_NO_TS);
    }

    if (!Option->EnableWindowScaling) {
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_NO_WS);
    }
  }

  //
  // The socket is bound, the <SrcIp, SrcPort, DstIp, DstPort> is
  // determined, construct the IP device path and install it.
  //
  Status = TcpInstallDevicePath (Sk);
  if (EFI_ERROR (Status)) {
    goto OnExit;
  }

  //
  // update state of Tcb and socket
  //
  if (!CfgData->AccessPoint.ActiveFlag) {

    TcpSetState (Tcb, TCP_LISTEN);
    SockSetState (Sk, SO_LISTENING);

    Sk->ConfigureState = SO_CONFIGURED_PASSIVE;
  } else {

    Sk->ConfigureState = SO_CONFIGURED_ACTIVE;
  }

  TcpInsertTcb (Tcb);

OnExit:

  return Status;
}


/**
  The procotol handler provided to the socket layer, used to
  dispatch the socket level requests by calling the corresponding
  TCP layer functions.

  @param  Sock                   Pointer to the socket of this TCP instance.
  @param  Request                The code of this operation request.
  @param  Data                   Pointer to the operation specific data passed in
                                 together with the operation request.

  @retval EFI_SUCCESS            The socket request is completed successfully.
  @retval other                  The error status returned by the corresponding TCP
                                 layer function.

**/
EFI_STATUS
Tcp4Dispatcher (
  IN SOCKET                  *Sock,
  IN UINT8                   Request,
  IN VOID                    *Data    OPTIONAL
  )
{
  TCP_CB            *Tcb;
  TCP4_PROTO_DATA   *ProtoData;
  EFI_IP4_PROTOCOL  *Ip;

  ProtoData = (TCP4_PROTO_DATA *) Sock->ProtoReserved;
  Tcb       = ProtoData->TcpPcb;

  switch (Request) {
  case SOCK_POLL:
    Ip = ProtoData->TcpService->IpIo->Ip.Ip4;
    Ip->Poll (Ip);
    break;

  case SOCK_CONSUMED:
    //
    // After user received data from socket buffer, socket will
    // notify TCP using this message to give it a chance to send out
    // window update information
    //
    ASSERT (Tcb != NULL);
    TcpOnAppConsume (Tcb);
    break;

  case SOCK_SND:

    ASSERT (Tcb != NULL);
    TcpOnAppSend (Tcb);
    break;

  case SOCK_CLOSE:

    TcpOnAppClose (Tcb);

    break;

  case SOCK_ABORT:

    TcpOnAppAbort (Tcb);

    break;

  case SOCK_SNDPUSH:
    Tcb->SndPsh = TcpGetMaxSndNxt (Tcb) + GET_SND_DATASIZE (Tcb->Sk);
    TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_SND_PSH);

    break;

  case SOCK_SNDURG:
    Tcb->SndUp = TcpGetMaxSndNxt (Tcb) + GET_SND_DATASIZE (Tcb->Sk) - 1;
    TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_SND_URG);

    break;

  case SOCK_CONNECT:

    TcpOnAppConnect (Tcb);

    break;

  case SOCK_ATTACH:

    return Tcp4AttachPcb (Sock);

  case SOCK_FLUSH:

    Tcp4FlushPcb (Tcb);

    break;

  case SOCK_DETACH:

    Tcp4DetachPcb (Sock);

    break;

  case SOCK_CONFIGURE:

    return Tcp4ConfigurePcb (
            Sock,
            (EFI_TCP4_CONFIG_DATA *) Data
            );

  case SOCK_MODE:

    ASSERT ((Data != NULL) && (Tcb != NULL));

    return Tcp4GetMode (Tcb, (TCP4_MODE_DATA *) Data);

  case SOCK_ROUTE:

    ASSERT ((Data != NULL) && (Tcb != NULL));

    return Tcp4Route (Tcb, (TCP4_ROUTE_INFO *) Data);

  default:
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;

}
