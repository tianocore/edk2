/** @file
  The implementation of a dispatch routine for processing TCP requests.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TcpMain.h"

/**
  Add or remove a route entry in the IP route table associated with this TCP instance.

  @param[in]  Tcb               Pointer to the TCP_CB of this TCP instance.
  @param[in]  RouteInfo         Pointer to the route information to be processed.

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
  IP_IO_IP_PROTOCOL  Ip;

  Ip = Tcb->IpInfo->Ip;

  ASSERT (Ip.Ip4 != NULL);

  return Ip.Ip4->Routes (
                   Ip.Ip4,
                   RouteInfo->DeleteRoute,
                   RouteInfo->SubnetAddress,
                   RouteInfo->SubnetMask,
                   RouteInfo->GatewayAddress
                   );
}

/**
  Get the operational settings of this TCPv4 instance.

  @param[in]       Tcb           Pointer to the TCP_CB of this TCP instance.
  @param[in, out]  Mode          Pointer to the buffer to store the operational
                                 settings.

  @retval EFI_SUCCESS            The mode data was read.
  @retval EFI_NOT_STARTED        No configuration data is available because this
                                 instance hasn't been started.

**/
EFI_STATUS
Tcp4GetMode (
  IN     TCP_CB          *Tcb,
  IN OUT TCP4_MODE_DATA  *Mode
  )
{
  SOCKET                 *Sock;
  EFI_TCP4_CONFIG_DATA   *ConfigData;
  EFI_TCP4_ACCESS_POINT  *AccessPoint;
  EFI_TCP4_OPTION        *Option;
  EFI_IP4_PROTOCOL       *Ip;

  Sock = Tcb->Sk;

  if (!SOCK_IS_CONFIGURED (Sock) && (Mode->Tcp4ConfigData != NULL)) {
    return EFI_NOT_STARTED;
  }

  if (Mode->Tcp4State != NULL) {
    *(Mode->Tcp4State) = (EFI_TCP4_CONNECTION_STATE)Tcb->State;
  }

  if (Mode->Tcp4ConfigData != NULL) {
    ConfigData  = Mode->Tcp4ConfigData;
    AccessPoint = &(ConfigData->AccessPoint);
    Option      = ConfigData->ControlOption;

    ConfigData->TypeOfService = Tcb->Tos;
    ConfigData->TimeToLive    = Tcb->Ttl;

    AccessPoint->UseDefaultAddress = Tcb->UseDefaultAddr;

    IP4_COPY_ADDRESS (&AccessPoint->StationAddress, &Tcb->LocalEnd.Ip);

    IP4_COPY_ADDRESS (&AccessPoint->SubnetMask, &Tcb->SubnetMask);
    AccessPoint->StationPort = NTOHS (Tcb->LocalEnd.Port);

    IP4_COPY_ADDRESS (&AccessPoint->RemoteAddress, &Tcb->RemoteEnd.Ip);

    AccessPoint->RemotePort = NTOHS (Tcb->RemoteEnd.Port);
    AccessPoint->ActiveFlag = (BOOLEAN)(Tcb->State != TCP_LISTEN);

    if (Option != NULL) {
      Option->ReceiveBufferSize = GET_RCV_BUFFSIZE (Tcb->Sk);
      Option->SendBufferSize    = GET_SND_BUFFSIZE (Tcb->Sk);
      Option->MaxSynBackLog     = GET_BACKLOG (Tcb->Sk);

      Option->ConnectionTimeout = Tcb->ConnectTimeout / TCP_TICK_HZ;
      Option->DataRetries       = Tcb->MaxRexmit;
      Option->FinTimeout        = Tcb->FinWait2Timeout / TCP_TICK_HZ;
      Option->TimeWaitTimeout   = Tcb->TimeWaitTimeout / TCP_TICK_HZ;
      Option->KeepAliveProbes   = Tcb->MaxKeepAlive;
      Option->KeepAliveTime     = Tcb->KeepAliveIdle / TCP_TICK_HZ;
      Option->KeepAliveInterval = Tcb->KeepAlivePeriod / TCP_TICK_HZ;

      Option->EnableNagle         = (BOOLEAN)(!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_NAGLE));
      Option->EnableTimeStamp     = (BOOLEAN)(!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_TS));
      Option->EnableWindowScaling = (BOOLEAN)(!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_WS));

      Option->EnableSelectiveAck     = FALSE;
      Option->EnablePathMtuDiscovery = FALSE;
    }
  }

  Ip = Tcb->IpInfo->Ip.Ip4;
  ASSERT (Ip != NULL);

  return Ip->GetModeData (Ip, Mode->Ip4ModeData, Mode->MnpConfigData, Mode->SnpModeData);
}

/**
  Get the operational settings of this TCPv6 instance.

  @param[in]       Tcb           Pointer to the TCP_CB of this TCP instance.
  @param[in, out]  Mode          Pointer to the buffer to store the operational
                                 settings.

  @retval EFI_SUCCESS            The mode data was read.
  @retval EFI_NOT_STARTED        No configuration data is available because this
                                 instance hasn't been started.

**/
EFI_STATUS
Tcp6GetMode (
  IN     TCP_CB          *Tcb,
  IN OUT TCP6_MODE_DATA  *Mode
  )
{
  SOCKET                 *Sock;
  EFI_TCP6_CONFIG_DATA   *ConfigData;
  EFI_TCP6_ACCESS_POINT  *AccessPoint;
  EFI_TCP6_OPTION        *Option;
  EFI_IP6_PROTOCOL       *Ip;

  Sock = Tcb->Sk;

  if (!SOCK_IS_CONFIGURED (Sock) && (Mode->Tcp6ConfigData != NULL)) {
    return EFI_NOT_STARTED;
  }

  if (Mode->Tcp6State != NULL) {
    *(Mode->Tcp6State) = (EFI_TCP6_CONNECTION_STATE)(Tcb->State);
  }

  if (Mode->Tcp6ConfigData != NULL) {
    ConfigData  = Mode->Tcp6ConfigData;
    AccessPoint = &(ConfigData->AccessPoint);
    Option      = ConfigData->ControlOption;

    ConfigData->TrafficClass = Tcb->Tos;
    ConfigData->HopLimit     = Tcb->Ttl;

    AccessPoint->StationPort = NTOHS (Tcb->LocalEnd.Port);
    AccessPoint->RemotePort  = NTOHS (Tcb->RemoteEnd.Port);
    AccessPoint->ActiveFlag  = (BOOLEAN)(Tcb->State != TCP_LISTEN);

    IP6_COPY_ADDRESS (&AccessPoint->StationAddress, &Tcb->LocalEnd.Ip);
    IP6_COPY_ADDRESS (&AccessPoint->RemoteAddress, &Tcb->RemoteEnd.Ip);

    if (Option != NULL) {
      Option->ReceiveBufferSize = GET_RCV_BUFFSIZE (Tcb->Sk);
      Option->SendBufferSize    = GET_SND_BUFFSIZE (Tcb->Sk);
      Option->MaxSynBackLog     = GET_BACKLOG (Tcb->Sk);

      Option->ConnectionTimeout = Tcb->ConnectTimeout / TCP_TICK_HZ;
      Option->DataRetries       = Tcb->MaxRexmit;
      Option->FinTimeout        = Tcb->FinWait2Timeout / TCP_TICK_HZ;
      Option->TimeWaitTimeout   = Tcb->TimeWaitTimeout / TCP_TICK_HZ;
      Option->KeepAliveProbes   = Tcb->MaxKeepAlive;
      Option->KeepAliveTime     = Tcb->KeepAliveIdle / TCP_TICK_HZ;
      Option->KeepAliveInterval = Tcb->KeepAlivePeriod / TCP_TICK_HZ;

      Option->EnableNagle         = (BOOLEAN)(!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_NAGLE));
      Option->EnableTimeStamp     = (BOOLEAN)(!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_TS));
      Option->EnableWindowScaling = (BOOLEAN)(!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_WS));

      Option->EnableSelectiveAck     = FALSE;
      Option->EnablePathMtuDiscovery = FALSE;
    }
  }

  Ip = Tcb->IpInfo->Ip.Ip6;
  ASSERT (Ip != NULL);

  return Ip->GetModeData (Ip, Mode->Ip6ModeData, Mode->MnpConfigData, Mode->SnpModeData);
}

/**
  If TcpAp->StationPort isn't zero, check whether the access point
  is registered, else generate a random station port for this
  access point.

  @param[in]  TcpAp              Pointer to the access point.
  @param[in]  IpVersion          IP_VERSION_4 or IP_VERSION_6

  @retval EFI_SUCCESS            The check passed or the port is assigned.
  @retval EFI_INVALID_PARAMETER  The non-zero station port is already used.
  @retval EFI_OUT_OF_RESOURCES   No port can be allocated.

**/
EFI_STATUS
TcpBind (
  IN TCP_ACCESS_POINT  *TcpAp,
  IN UINT8             IpVersion
  )
{
  BOOLEAN         Cycle;
  EFI_IP_ADDRESS  Local;
  UINT16          *Port;
  UINT16          *RandomPort;

  if (IpVersion == IP_VERSION_4) {
    IP4_COPY_ADDRESS (&Local, &TcpAp->Tcp4Ap.StationAddress);
    Port       = &TcpAp->Tcp4Ap.StationPort;
    RandomPort = &mTcp4RandomPort;
  } else {
    IP6_COPY_ADDRESS (&Local, &TcpAp->Tcp6Ap.StationAddress);
    Port       = &TcpAp->Tcp6Ap.StationPort;
    RandomPort = &mTcp6RandomPort;
  }

  if (0 != *Port) {
    //
    // Check if a same endpoing is bound.
    //
    if (TcpFindTcbByPeer (&Local, *Port, IpVersion)) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // generate a random port
    //
    Cycle = FALSE;

    if (TCP_PORT_USER_RESERVED == *RandomPort) {
      *RandomPort = TCP_PORT_KNOWN;
    }

    (*RandomPort)++;

    while (TcpFindTcbByPeer (&Local, *RandomPort, IpVersion)) {
      (*RandomPort)++;

      if (*RandomPort <= TCP_PORT_KNOWN) {
        if (Cycle) {
          DEBUG (
            (DEBUG_ERROR,
             "TcpBind: no port can be allocated for this pcb\n")
            );
          return EFI_OUT_OF_RESOURCES;
        }

        *RandomPort = TCP_PORT_KNOWN + 1;

        Cycle = TRUE;
      }
    }

    *Port = *RandomPort;
  }

  return EFI_SUCCESS;
}

/**
  Flush the Tcb add its associated protocols.

  @param[in, out]  Tcb      Pointer to the TCP_CB to be flushed.

**/
VOID
TcpFlushPcb (
  IN OUT TCP_CB  *Tcb
  )
{
  SOCKET  *Sock;

  IpIoConfigIp (Tcb->IpInfo, NULL);

  Sock = Tcb->Sk;

  if (SOCK_IS_CONFIGURED (Sock)) {
    RemoveEntryList (&Tcb->List);

    if (Sock->DevicePath != NULL) {
      //
      // Uninstall the device path protocol.
      //
      gBS->UninstallProtocolInterface (
             Sock->SockHandle,
             &gEfiDevicePathProtocolGuid,
             Sock->DevicePath
             );

      FreePool (Sock->DevicePath);
      Sock->DevicePath = NULL;
    }
  }

  NetbufFreeList (&Tcb->SndQue);
  NetbufFreeList (&Tcb->RcvQue);
  Tcb->State        = TCP_CLOSED;
  Tcb->RemoteIpZero = FALSE;
}

/**
  Attach a Pcb to the socket.

  @param[in]  Sk                 Pointer to the socket of this TCP instance.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Failed due to resource limits.

**/
EFI_STATUS
TcpAttachPcb (
  IN SOCKET  *Sk
  )
{
  TCP_CB          *Tcb;
  TCP_PROTO_DATA  *ProtoData;
  IP_IO           *IpIo;
  EFI_STATUS      Status;
  VOID            *Ip;
  EFI_GUID        *IpProtocolGuid;

  if (Sk->IpVersion == IP_VERSION_4) {
    IpProtocolGuid = &gEfiIp4ProtocolGuid;
  } else {
    IpProtocolGuid = &gEfiIp6ProtocolGuid;
  }

  Tcb = AllocateZeroPool (sizeof (TCP_CB));

  if (Tcb == NULL) {
    DEBUG ((DEBUG_ERROR, "TcpConfigurePcb: failed to allocate a TCB\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  ProtoData = (TCP_PROTO_DATA *)Sk->ProtoReserved;
  IpIo      = ProtoData->TcpService->IpIo;

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
                  IpProtocolGuid,
                  &Ip,
                  IpIo->Image,
                  Sk->SockHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    IpIoRemoveIp (IpIo, Tcb->IpInfo);
    FreePool (Tcb);
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

  @param[in, out]  Sk           Pointer to the socket of this TCP instance.

**/
VOID
TcpDetachPcb (
  IN OUT SOCKET  *Sk
  )
{
  TCP_PROTO_DATA  *ProtoData;
  TCP_CB          *Tcb;

  ProtoData = (TCP_PROTO_DATA *)Sk->ProtoReserved;
  Tcb       = ProtoData->TcpPcb;

  ASSERT (Tcb != NULL);

  TcpFlushPcb (Tcb);

  IpIoRemoveIp (ProtoData->TcpService->IpIo, Tcb->IpInfo);

  FreePool (Tcb);

  ProtoData->TcpPcb = NULL;
}

/**
  Configure the Pcb using CfgData.

  @param[in]  Sk                 Pointer to the socket of this TCP instance.
  @param[in]  CfgData            Pointer to the TCP configuration data.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  A same access point has been configured in
                                 another TCP instance.
  @retval EFI_OUT_OF_RESOURCES   Failed due to resource limits.

**/
EFI_STATUS
TcpConfigurePcb (
  IN SOCKET           *Sk,
  IN TCP_CONFIG_DATA  *CfgData
  )
{
  IP_IO_IP_CONFIG_DATA  IpCfgData;
  EFI_STATUS            Status;
  EFI_TCP4_OPTION       *Option;
  TCP_PROTO_DATA        *TcpProto;
  TCP_CB                *Tcb;
  TCP_ACCESS_POINT      *TcpAp;

  ASSERT ((CfgData != NULL) && (Sk != NULL) && (Sk->SockHandle != NULL));

  TcpProto = (TCP_PROTO_DATA *)Sk->ProtoReserved;
  Tcb      = TcpProto->TcpPcb;

  ASSERT (Tcb != NULL);

  if (Sk->IpVersion == IP_VERSION_4) {
    //
    // Add Ip for send pkt to the peer
    //
    CopyMem (&IpCfgData.Ip4CfgData, &mIp4IoDefaultIpConfigData, sizeof (EFI_IP4_CONFIG_DATA));
    IpCfgData.Ip4CfgData.DefaultProtocol   = EFI_IP_PROTO_TCP;
    IpCfgData.Ip4CfgData.TypeOfService     = CfgData->Tcp4CfgData.TypeOfService;
    IpCfgData.Ip4CfgData.TimeToLive        = CfgData->Tcp4CfgData.TimeToLive;
    IpCfgData.Ip4CfgData.UseDefaultAddress = CfgData->Tcp4CfgData.AccessPoint.UseDefaultAddress;
    IP4_COPY_ADDRESS (
      &IpCfgData.Ip4CfgData.SubnetMask,
      &CfgData->Tcp4CfgData.AccessPoint.SubnetMask
      );
    IpCfgData.Ip4CfgData.ReceiveTimeout = (UINT32)(-1);
    IP4_COPY_ADDRESS (
      &IpCfgData.Ip4CfgData.StationAddress,
      &CfgData->Tcp4CfgData.AccessPoint.StationAddress
      );
  } else {
    ASSERT (Sk->IpVersion == IP_VERSION_6);

    CopyMem (&IpCfgData.Ip6CfgData, &mIp6IoDefaultIpConfigData, sizeof (EFI_IP6_CONFIG_DATA));
    IpCfgData.Ip6CfgData.DefaultProtocol = EFI_IP_PROTO_TCP;
    IpCfgData.Ip6CfgData.TrafficClass    = CfgData->Tcp6CfgData.TrafficClass;
    IpCfgData.Ip6CfgData.HopLimit        = CfgData->Tcp6CfgData.HopLimit;
    IpCfgData.Ip6CfgData.ReceiveTimeout  = (UINT32)(-1);
    IP6_COPY_ADDRESS (
      &IpCfgData.Ip6CfgData.StationAddress,
      &CfgData->Tcp6CfgData.AccessPoint.StationAddress
      );
    IP6_COPY_ADDRESS (
      &IpCfgData.Ip6CfgData.DestinationAddress,
      &CfgData->Tcp6CfgData.AccessPoint.RemoteAddress
      );
  }

  //
  // Configure the IP instance this Tcb consumes.
  //
  Status = IpIoConfigIp (Tcb->IpInfo, &IpCfgData);
  if (EFI_ERROR (Status)) {
    goto OnExit;
  }

  if (Sk->IpVersion == IP_VERSION_4) {
    //
    // Get the default address information if the instance is configured to use default address.
    //
    IP4_COPY_ADDRESS (
      &CfgData->Tcp4CfgData.AccessPoint.StationAddress,
      &IpCfgData.Ip4CfgData.StationAddress
      );
    IP4_COPY_ADDRESS (
      &CfgData->Tcp4CfgData.AccessPoint.SubnetMask,
      &IpCfgData.Ip4CfgData.SubnetMask
      );

    TcpAp = (TCP_ACCESS_POINT *)&CfgData->Tcp4CfgData.AccessPoint;
  } else {
    IP6_COPY_ADDRESS (
      &CfgData->Tcp6CfgData.AccessPoint.StationAddress,
      &IpCfgData.Ip6CfgData.StationAddress
      );

    TcpAp = (TCP_ACCESS_POINT *)&CfgData->Tcp6CfgData.AccessPoint;
  }

  //
  // check if we can bind this endpoint in CfgData
  //
  Status = TcpBind (TcpAp, Sk->IpVersion);

  if (EFI_ERROR (Status)) {
    DEBUG (
      (DEBUG_ERROR,
       "TcpConfigurePcb: Bind endpoint failed with %r\n",
       Status)
      );

    goto OnExit;
  }

  //
  // Initialize the operating information in this Tcb
  //
  ASSERT (
    Tcb->State == TCP_CLOSED &&
    IsListEmpty (&Tcb->SndQue) &&
    IsListEmpty (&Tcb->RcvQue)
    );

  TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_NO_KEEPALIVE);
  Tcb->State = TCP_CLOSED;

  Tcb->SndMss = 536;
  Tcb->RcvMss = TcpGetRcvMss (Sk);

  Tcb->SRtt = 0;
  Tcb->Rto  = 3 * TCP_TICK_HZ;

  Tcb->CWnd     = Tcb->SndMss;
  Tcb->Ssthresh = 0xffffffff;

  Tcb->CongestState = TCP_CONGEST_OPEN;

  Tcb->KeepAliveIdle   = TCP_KEEPALIVE_IDLE_MIN;
  Tcb->KeepAlivePeriod = TCP_KEEPALIVE_PERIOD;
  Tcb->MaxKeepAlive    = TCP_MAX_KEEPALIVE;
  Tcb->MaxRexmit       = TCP_MAX_LOSS;
  Tcb->FinWait2Timeout = TCP_FIN_WAIT2_TIME;
  Tcb->TimeWaitTimeout = TCP_TIME_WAIT_TIME;
  Tcb->ConnectTimeout  = TCP_CONNECT_TIME;

  if (Sk->IpVersion == IP_VERSION_4) {
    //
    // initialize Tcb in the light of CfgData
    //
    Tcb->Ttl = CfgData->Tcp4CfgData.TimeToLive;
    Tcb->Tos = CfgData->Tcp4CfgData.TypeOfService;

    Tcb->UseDefaultAddr = CfgData->Tcp4CfgData.AccessPoint.UseDefaultAddress;

    CopyMem (&Tcb->LocalEnd.Ip, &CfgData->Tcp4CfgData.AccessPoint.StationAddress, sizeof (IP4_ADDR));
    Tcb->LocalEnd.Port = HTONS (CfgData->Tcp4CfgData.AccessPoint.StationPort);
    IP4_COPY_ADDRESS (&Tcb->SubnetMask, &CfgData->Tcp4CfgData.AccessPoint.SubnetMask);

    CopyMem (&Tcb->RemoteEnd.Ip, &CfgData->Tcp4CfgData.AccessPoint.RemoteAddress, sizeof (IP4_ADDR));
    Tcb->RemoteEnd.Port = HTONS (CfgData->Tcp4CfgData.AccessPoint.RemotePort);

    Option = CfgData->Tcp4CfgData.ControlOption;
  } else {
    Tcb->Ttl = CfgData->Tcp6CfgData.HopLimit;
    Tcb->Tos = CfgData->Tcp6CfgData.TrafficClass;

    IP6_COPY_ADDRESS (&Tcb->LocalEnd.Ip, &CfgData->Tcp6CfgData.AccessPoint.StationAddress);
    Tcb->LocalEnd.Port = HTONS (CfgData->Tcp6CfgData.AccessPoint.StationPort);

    IP6_COPY_ADDRESS (&Tcb->RemoteEnd.Ip, &CfgData->Tcp6CfgData.AccessPoint.RemoteAddress);
    Tcb->RemoteEnd.Port = HTONS (CfgData->Tcp6CfgData.AccessPoint.RemotePort);

    //
    // Type EFI_TCP4_OPTION and EFI_TCP6_OPTION are the same.
    //
    Option = (EFI_TCP4_OPTION *)CfgData->Tcp6CfgData.ControlOption;
  }

  if (Option != NULL) {
    SET_RCV_BUFFSIZE (
      Sk,
      (UINT32)(TCP_COMP_VAL (
                 TCP_RCV_BUF_SIZE_MIN,
                 TCP_RCV_BUF_SIZE,
                 TCP_RCV_BUF_SIZE,
                 Option->ReceiveBufferSize
                 )
               )
      );
    SET_SND_BUFFSIZE (
      Sk,
      (UINT32)(TCP_COMP_VAL (
                 TCP_SND_BUF_SIZE_MIN,
                 TCP_SND_BUF_SIZE,
                 TCP_SND_BUF_SIZE,
                 Option->SendBufferSize
                 )
               )
      );

    SET_BACKLOG (
      Sk,
      (UINT32)(TCP_COMP_VAL (
                 TCP_BACKLOG_MIN,
                 TCP_BACKLOG,
                 TCP_BACKLOG,
                 Option->MaxSynBackLog
                 )
               )
      );

    Tcb->MaxRexmit = (UINT16)TCP_COMP_VAL (
                               TCP_MAX_LOSS_MIN,
                               TCP_MAX_LOSS,
                               TCP_MAX_LOSS,
                               Option->DataRetries
                               );
    Tcb->FinWait2Timeout = TCP_COMP_VAL (
                             TCP_FIN_WAIT2_TIME,
                             TCP_FIN_WAIT2_TIME_MAX,
                             TCP_FIN_WAIT2_TIME,
                             (UINT32)(Option->FinTimeout * TCP_TICK_HZ)
                             );

    if (Option->TimeWaitTimeout != 0) {
      Tcb->TimeWaitTimeout = TCP_COMP_VAL (
                               TCP_TIME_WAIT_TIME,
                               TCP_TIME_WAIT_TIME_MAX,
                               TCP_TIME_WAIT_TIME,
                               (UINT32)(Option->TimeWaitTimeout * TCP_TICK_HZ)
                               );
    } else {
      Tcb->TimeWaitTimeout = 0;
    }

    if (Option->KeepAliveProbes != 0) {
      TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_NO_KEEPALIVE);

      Tcb->MaxKeepAlive = (UINT8)TCP_COMP_VAL (
                                   TCP_MAX_KEEPALIVE_MIN,
                                   TCP_MAX_KEEPALIVE,
                                   TCP_MAX_KEEPALIVE,
                                   Option->KeepAliveProbes
                                   );
      Tcb->KeepAliveIdle = TCP_COMP_VAL (
                             TCP_KEEPALIVE_IDLE_MIN,
                             TCP_KEEPALIVE_IDLE_MAX,
                             TCP_KEEPALIVE_IDLE_MIN,
                             (UINT32)(Option->KeepAliveTime * TCP_TICK_HZ)
                             );
      Tcb->KeepAlivePeriod = TCP_COMP_VAL (
                               TCP_KEEPALIVE_PERIOD_MIN,
                               TCP_KEEPALIVE_PERIOD,
                               TCP_KEEPALIVE_PERIOD,
                               (UINT32)(Option->KeepAliveInterval * TCP_TICK_HZ)
                               );
    }

    Tcb->ConnectTimeout = TCP_COMP_VAL (
                            TCP_CONNECT_TIME_MIN,
                            TCP_CONNECT_TIME,
                            TCP_CONNECT_TIME,
                            (UINT32)(Option->ConnectionTimeout * TCP_TICK_HZ)
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
  if (((Sk->IpVersion == IP_VERSION_4) && !CfgData->Tcp4CfgData.AccessPoint.ActiveFlag) ||
      ((Sk->IpVersion == IP_VERSION_6) && !CfgData->Tcp6CfgData.AccessPoint.ActiveFlag)
      )
  {
    TcpSetState (Tcb, TCP_LISTEN);
    SockSetState (Sk, SO_LISTENING);

    Sk->ConfigureState = SO_CONFIGURED_PASSIVE;
  } else {
    Sk->ConfigureState = SO_CONFIGURED_ACTIVE;
  }

  if (Sk->IpVersion == IP_VERSION_6) {
    Tcb->Tick = TCP6_REFRESH_NEIGHBOR_TICK;

    if (NetIp6IsUnspecifiedAddr (&Tcb->RemoteEnd.Ip.v6)) {
      Tcb->RemoteIpZero = TRUE;
    }
  }

  TcpInsertTcb (Tcb);

OnExit:

  return Status;
}

/**
  The protocol handler provided to the socket layer, which is used to
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
  )
{
  TCP_CB          *Tcb;
  TCP_PROTO_DATA  *ProtoData;

  ProtoData = (TCP_PROTO_DATA *)Sock->ProtoReserved;
  Tcb       = ProtoData->TcpPcb;

  switch (Request) {
    case SOCK_POLL:
      if (Tcb->Sk->IpVersion == IP_VERSION_4) {
        ProtoData->TcpService->IpIo->Ip.Ip4->Poll (ProtoData->TcpService->IpIo->Ip.Ip4);
      } else {
        ProtoData->TcpService->IpIo->Ip.Ip6->Poll (ProtoData->TcpService->IpIo->Ip.Ip6);
      }

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

      return TcpAttachPcb (Sock);

      break;

    case SOCK_FLUSH:

      TcpFlushPcb (Tcb);

      break;

    case SOCK_DETACH:

      TcpDetachPcb (Sock);

      break;

    case SOCK_CONFIGURE:

      return TcpConfigurePcb (
               Sock,
               (TCP_CONFIG_DATA *)Data
               );

      break;

    case SOCK_MODE:

      ASSERT ((Data != NULL) && (Tcb != NULL));

      if (Tcb->Sk->IpVersion == IP_VERSION_4) {
        return Tcp4GetMode (Tcb, (TCP4_MODE_DATA *)Data);
      } else {
        return Tcp6GetMode (Tcb, (TCP6_MODE_DATA *)Data);
      }

      break;

    case SOCK_ROUTE:

      ASSERT ((Data != NULL) && (Tcb != NULL) && (Tcb->Sk->IpVersion == IP_VERSION_4));

      return Tcp4Route (Tcb, (TCP4_ROUTE_INFO *)Data);

    default:

      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
