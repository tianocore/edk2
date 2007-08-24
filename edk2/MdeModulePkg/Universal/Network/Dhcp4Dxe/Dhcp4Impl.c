/** @file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Dhcp4Impl.c

Abstract:

  This file implement the EFI_DHCP4_PROTOCOL interface.


**/


#include "Dhcp4Impl.h"


/**
  Get the current operation parameter and lease for the network interface.

  @param  This                   The DHCP protocol instance
  @param  Dhcp4ModeData          The variable to save the DHCP mode data.

  @retval EFI_INVALID_PARAMETER  The parameter is invalid
  @retval EFI_SUCCESS            The Dhcp4ModeData is updated with the current
                                 operation parameter.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4GetModeData (
  IN  EFI_DHCP4_PROTOCOL    *This,
  OUT EFI_DHCP4_MODE_DATA   *Dhcp4ModeData
  )
{
  DHCP_PROTOCOL             *Instance;
  DHCP_SERVICE              *DhcpSb;
  DHCP_PARAMETER            *Para;
  EFI_TPL                   OldTpl;
  IP4_ADDR                  Ip;

  //
  // First validate the parameters.
  //
  if ((This == NULL) || (Dhcp4ModeData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP_INSTANCE_FROM_THIS (This);

  if (Instance->Signature != DHCP_PROTOCOL_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl  = NET_RAISE_TPL (NET_TPL_LOCK);
  DhcpSb  = Instance->Service;

  //
  // Caller can use GetModeData to retrieve current DHCP states
  // no matter whether it is the active child or not.
  //
  Dhcp4ModeData->State                     = (EFI_DHCP4_STATE) DhcpSb->DhcpState;
  CopyMem (&Dhcp4ModeData->ConfigData, &DhcpSb->ActiveConfig, sizeof (Dhcp4ModeData->ConfigData));
  CopyMem (&Dhcp4ModeData->ClientMacAddress, &DhcpSb->Mac, sizeof (Dhcp4ModeData->ClientMacAddress));

  Ip = HTONL (DhcpSb->ClientAddr);
  NetCopyMem (&Dhcp4ModeData->ClientAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

  Ip = HTONL (DhcpSb->Netmask);
  NetCopyMem (&Dhcp4ModeData->SubnetMask, &Ip, sizeof (EFI_IPv4_ADDRESS));

  Ip = HTONL (DhcpSb->ServerAddr);
  NetCopyMem (&Dhcp4ModeData->ServerAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

  Para = DhcpSb->Para;

  if (Para != NULL) {
    Ip = HTONL (Para->Router);
    NetCopyMem (&Dhcp4ModeData->RouterAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));
    Dhcp4ModeData->LeaseTime               = Para->Lease;
  } else {
    NetZeroMem (&Dhcp4ModeData->RouterAddress, sizeof (EFI_IPv4_ADDRESS));
    Dhcp4ModeData->LeaseTime               = 0xffffffff;
  }

  Dhcp4ModeData->ReplyPacket = DhcpSb->Selected;

  NET_RESTORE_TPL (OldTpl);
  return EFI_SUCCESS;
}


/**
  Free the resource related to the configure parameters.
  DHCP driver will make a copy of the user's configure
  such as the time out value.

  @param  Config                 The DHCP configure data

  @return None

**/
VOID
DhcpCleanConfigure (
  IN EFI_DHCP4_CONFIG_DATA  *Config
  )
{
  UINT32                    Index;

  if (Config->DiscoverTimeout != NULL) {
    NetFreePool (Config->DiscoverTimeout);
  }

  if (Config->RequestTimeout != NULL) {
    NetFreePool (Config->RequestTimeout);
  }

  if (Config->OptionList != NULL) {
    for (Index = 0; Index < Config->OptionCount; Index++) {
      if (Config->OptionList[Index] != NULL) {
        NetFreePool (Config->OptionList[Index]);
      }
    }

    NetFreePool (Config->OptionList);
  }

  NetZeroMem (Config, sizeof (EFI_DHCP4_CONFIG_DATA));
}


/**
  Allocate memory for configure parameter such as timeout value for Dst,
  then copy the configure parameter from Src to Dst.

  @param  Dst                    The destination DHCP configure data.
  @param  Src                    The source DHCP configure data.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_SUCCESS            The configure is copied.

**/
EFI_STATUS
DhcpCopyConfigure (
  IN EFI_DHCP4_CONFIG_DATA  *Dst,
  IN EFI_DHCP4_CONFIG_DATA  *Src
  )
{
  EFI_DHCP4_PACKET_OPTION   **DstOptions;
  EFI_DHCP4_PACKET_OPTION   **SrcOptions;
  INTN                      Len;
  UINT32                    Index;

  CopyMem (Dst, Src, sizeof (*Dst));
  Dst->DiscoverTimeout  = NULL;
  Dst->RequestTimeout   = NULL;
  Dst->OptionList       = NULL;

  //
  // Allocate a memory then copy DiscoverTimeout to it
  //
  if (Src->DiscoverTimeout != NULL) {
    Len                   = Src->DiscoverTryCount * sizeof (UINT32);
    Dst->DiscoverTimeout  = NetAllocatePool (Len);

    if (Dst->DiscoverTimeout == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < Src->DiscoverTryCount; Index++) {
      Dst->DiscoverTimeout[Index] = NET_MAX (Src->DiscoverTimeout[Index], 1);
    }
  }

  //
  // Allocate a memory then copy RequestTimeout to it
  //
  if (Src->RequestTimeout != NULL) {
    Len                 = Src->RequestTryCount * sizeof (UINT32);
    Dst->RequestTimeout = NetAllocatePool (Len);

    if (Dst->RequestTimeout == NULL) {
      goto ON_ERROR;
    }

    for (Index = 0; Index < Src->RequestTryCount; Index++) {
      Dst->RequestTimeout[Index] = NET_MAX (Src->RequestTimeout[Index], 1);
    }
  }

  //
  // Allocate an array of dhcp option point, then allocate memory
  // for each option and copy the source option to it
  //
  if (Src->OptionList != NULL) {
    Len             = Src->OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *);
    Dst->OptionList = NetAllocateZeroPool (Len);

    if (Dst->OptionList == NULL) {
      goto ON_ERROR;
    }

    DstOptions  = Dst->OptionList;
    SrcOptions  = Src->OptionList;

    for (Index = 0; Index < Src->OptionCount; Index++) {
      Len = sizeof (EFI_DHCP4_PACKET_OPTION) + NET_MAX (SrcOptions[Index]->Length - 1, 0);

      DstOptions[Index] = NetAllocatePool (Len);

      if (DstOptions[Index] == NULL) {
        goto ON_ERROR;
      }

      NetCopyMem (DstOptions[Index], SrcOptions[Index], Len);
    }
  }

  return EFI_SUCCESS;

ON_ERROR:
  DhcpCleanConfigure (Dst);
  return EFI_OUT_OF_RESOURCES;
}


/**
  Give up the control of the DHCP service to let other child
  resume. Don't change the service's DHCP state and the Client
  address and option list configure as required by RFC2131.

  @param  DhcpSb                 The DHCP service instance.

  @return None

**/
VOID
DhcpYieldControl (
  IN DHCP_SERVICE           *DhcpSb
  )
{
  EFI_DHCP4_CONFIG_DATA     *Config;

  Config    = &DhcpSb->ActiveConfig;

  DhcpSb->ServiceState  = DHCP_UNCONFIGED;
  DhcpSb->ActiveChild   = NULL;

  if (Config->DiscoverTimeout != NULL) {
    NetFreePool (Config->DiscoverTimeout);

    Config->DiscoverTryCount  = 0;
    Config->DiscoverTimeout   = NULL;
  }

  if (Config->RequestTimeout != NULL) {
    NetFreePool (Config->RequestTimeout);

    Config->RequestTryCount = 0;
    Config->RequestTimeout  = NULL;
  }

  Config->Dhcp4Callback   = NULL;
  Config->CallbackContext = NULL;
}


/**
  Configure the DHCP protocol instance and its underlying DHCP service
  for operation. If Dhcp4CfgData is NULL and the child is currently
  controlling the DHCP service, release the control.

  @param  This                   The DHCP protocol instance
  @param  Dhcp4CfgData           The DHCP configure data.

  @retval EFI_INVALID_PARAMETER  The parameters are invalid.
  @retval EFI_ACCESS_DENIED      The service isn't in one of configurable states,
                                 or there is already an active child.
  @retval EFI_OUT_OF_RESOURCE    Failed to allocate some resources.
  @retval EFI_SUCCESS            The child is configured.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4Configure (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN EFI_DHCP4_CONFIG_DATA  *Dhcp4CfgData       OPTIONAL
  )
{
  EFI_DHCP4_CONFIG_DATA     *Config;
  DHCP_PROTOCOL             *Instance;
  DHCP_SERVICE              *DhcpSb;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  UINT32                    Index;
  IP4_ADDR                  Ip;

  //
  // First validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Dhcp4CfgData != NULL) {
    if (Dhcp4CfgData->DiscoverTryCount && (Dhcp4CfgData->DiscoverTimeout == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if (Dhcp4CfgData->RequestTryCount && (Dhcp4CfgData->RequestTimeout == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if (Dhcp4CfgData->OptionCount && (Dhcp4CfgData->OptionList == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    NetCopyMem (&Ip, &Dhcp4CfgData->ClientAddress, sizeof (IP4_ADDR));

    if ((Ip != 0) && !Ip4IsUnicast (NTOHL (Ip), 0)) {

      return EFI_INVALID_PARAMETER;
    }
  }

  Instance = DHCP_INSTANCE_FROM_THIS (This);

  if (Instance->Signature != DHCP_PROTOCOL_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl  = NET_RAISE_TPL (NET_TPL_LOCK);

  DhcpSb  = Instance->Service;
  Config  = &DhcpSb->ActiveConfig;

  Status  = EFI_ACCESS_DENIED;

  if ((DhcpSb->DhcpState != Dhcp4Stopped) &&
      (DhcpSb->DhcpState != Dhcp4Init) &&
      (DhcpSb->DhcpState != Dhcp4InitReboot) &&
      (DhcpSb->DhcpState != Dhcp4Bound)) {

    goto ON_EXIT;
  }

  if ((DhcpSb->ActiveChild != NULL) && (DhcpSb->ActiveChild != Instance)) {
    goto ON_EXIT;
  }

  if (Dhcp4CfgData != NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DhcpCleanConfigure (Config);

    if (EFI_ERROR (DhcpCopyConfigure (Config, Dhcp4CfgData))) {
      goto ON_EXIT;
    }

    DhcpSb->UserOptionLen = 0;

    for (Index = 0; Index < Dhcp4CfgData->OptionCount; Index++) {
      DhcpSb->UserOptionLen += Dhcp4CfgData->OptionList[Index]->Length + 2;
    }

    DhcpSb->ActiveChild = Instance;

    if (DhcpSb->DhcpState == Dhcp4Stopped) {
      DhcpSb->ClientAddr = EFI_NTOHL (Dhcp4CfgData->ClientAddress);

      if (DhcpSb->ClientAddr != 0) {
        DhcpSb->DhcpState = Dhcp4InitReboot;
      } else {
        DhcpSb->DhcpState = Dhcp4Init;
      }
    }

    DhcpSb->ServiceState  = DHCP_CONFIGED;
    Status                = EFI_SUCCESS;

  } else if (DhcpSb->ActiveChild == Instance) {
    Status = EFI_SUCCESS;
    DhcpYieldControl (DhcpSb);
  }

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Start the DHCP process.

  @param  This                   The DHCP protocol instance
  @param  CompletionEvent        The event to signal is address is acquired.

  @retval EFI_INVALID_PARAMETER  The parameters are invalid.
  @retval EFI_NOT_STARTED        The protocol hasn't been configured.
  @retval EFI_ALREADY_STARTED    The DHCP process has already been started.
  @retval EFI_SUCCESS            The DHCP process is started.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4Start (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN EFI_EVENT              CompletionEvent   OPTIONAL
  )
{
  DHCP_PROTOCOL             *Instance;
  DHCP_SERVICE              *DhcpSb;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  //
  // First validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP_INSTANCE_FROM_THIS (This);

  if (Instance->Signature != DHCP_PROTOCOL_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl  = NET_RAISE_TPL (NET_TPL_LOCK);
  DhcpSb  = Instance->Service;

  if (DhcpSb->DhcpState == Dhcp4Stopped) {
    Status = EFI_NOT_STARTED;
    goto ON_ERROR;
  }

  if ((DhcpSb->DhcpState != Dhcp4Init) && (DhcpSb->DhcpState != Dhcp4InitReboot)) {
    Status = EFI_ALREADY_STARTED;
    goto ON_ERROR;
  }

  DhcpSb->IoStatus = EFI_ALREADY_STARTED;

  if (EFI_ERROR (Status = DhcpInitRequest (DhcpSb))) {
    goto ON_ERROR;
  }

  //
  // Start/Restart the receiving.
  //
  Status = UdpIoRecvDatagram (DhcpSb->UdpIo, DhcpInput, DhcpSb, 0);

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ON_ERROR;
  }

  Instance->CompletionEvent = CompletionEvent;

  //
  // Restore the TPL now, don't call poll function at NET_TPL_LOCK.
  //
  NET_RESTORE_TPL (OldTpl);

  if (CompletionEvent == NULL) {
    while (DhcpSb->IoStatus == EFI_ALREADY_STARTED) {
      DhcpSb->UdpIo->Udp->Poll (DhcpSb->UdpIo->Udp);
    }

    return DhcpSb->IoStatus;
  }

  return EFI_SUCCESS;

ON_ERROR:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Request an extra manual renew/rebind.

  @param  This                   The DHCP protocol instance
  @param  RebindRequest          TRUE if request a rebind, otherwise renew it
  @param  CompletionEvent        Event to signal when complete

  @retval EFI_INVALID_PARAMETER  The parameters are invalid
  @retval EFI_NOT_STARTED        The DHCP protocol hasn't been started.
  @retval EFI_ACCESS_DENIED      The DHCP protocol isn't in Bound state.
  @retval EFI_SUCCESS            The DHCP is renewed/rebound.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4RenewRebind (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN BOOLEAN                RebindRequest,
  IN EFI_EVENT              CompletionEvent   OPTIONAL
  )
{
  DHCP_PROTOCOL             *Instance;
  DHCP_SERVICE              *DhcpSb;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  //
  // First validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP_INSTANCE_FROM_THIS (This);

  if (Instance->Signature != DHCP_PROTOCOL_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl  = NET_RAISE_TPL (NET_TPL_LOCK);
  DhcpSb  = Instance->Service;

  if (DhcpSb->DhcpState == Dhcp4Stopped) {
    Status = EFI_NOT_STARTED;
    goto ON_ERROR;
  }

  if (DhcpSb->DhcpState != Dhcp4Bound) {
    Status = EFI_ACCESS_DENIED;
    goto ON_ERROR;
  }

  if (DHCP_IS_BOOTP (DhcpSb->Para)) {
    return EFI_SUCCESS;
  }

  //
  // Transit the states then send a extra DHCP request
  //
  if (!RebindRequest) {
    DhcpSetState (DhcpSb, Dhcp4Renewing, FALSE);
  } else {
    DhcpSetState (DhcpSb, Dhcp4Rebinding, FALSE);
  }

  Status = DhcpSendMessage (
             DhcpSb,
             DhcpSb->Selected,
             DhcpSb->Para,
             DHCP_MSG_REQUEST,
             "Extra renew/rebind by the application"
             );

  if (EFI_ERROR (Status)) {
    DhcpSetState (DhcpSb, Dhcp4Bound, FALSE);
    goto ON_ERROR;
  }

  DhcpSb->ExtraRefresh        = TRUE;
  DhcpSb->IoStatus            = EFI_ALREADY_STARTED;
  Instance->RenewRebindEvent  = CompletionEvent;

  NET_RESTORE_TPL (OldTpl);

  if (CompletionEvent == NULL) {
    while (DhcpSb->IoStatus == EFI_ALREADY_STARTED) {
      DhcpSb->UdpIo->Udp->Poll (DhcpSb->UdpIo->Udp);
    }

    return DhcpSb->IoStatus;
  }

  return EFI_SUCCESS;

ON_ERROR:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Release the current acquired lease.

  @param  This                   The DHCP protocol instance

  @retval EFI_INVALID_PARAMETER  The parameter is invalid
  @retval EFI_DEVICE_ERROR       Failed to transmit the DHCP release packet
  @retval EFI_ACCESS_DENIED      The DHCP service isn't in one of the connected
                                 state.
  @retval EFI_SUCCESS            The lease is released.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4Release (
  IN EFI_DHCP4_PROTOCOL     *This
  )
{
  DHCP_PROTOCOL             *Instance;
  DHCP_SERVICE              *DhcpSb;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  //
  // First validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP_INSTANCE_FROM_THIS (This);

  if (Instance->Signature != DHCP_PROTOCOL_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  Status  = EFI_SUCCESS;
  OldTpl  = NET_RAISE_TPL (NET_TPL_LOCK);
  DhcpSb  = Instance->Service;

  if ((DhcpSb->DhcpState != Dhcp4InitReboot) && (DhcpSb->DhcpState != Dhcp4Bound)) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  if (!DHCP_IS_BOOTP (DhcpSb->Para) && (DhcpSb->DhcpState == Dhcp4Bound)) {
    Status = DhcpSendMessage (
               DhcpSb,
               DhcpSb->Selected,
               DhcpSb->Para,
               DHCP_MSG_RELEASE,
               NULL
               );

    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
    }
  }

  DhcpCleanLease (DhcpSb);

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Stop the current DHCP process. After this, other DHCP child
  can gain control of the service, configure and use it.

  @param  This                   The DHCP protocol instance

  @retval EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval EFI_SUCCESS            The DHCP process is stopped.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4Stop (
  IN EFI_DHCP4_PROTOCOL     *This
  )
{
  DHCP_PROTOCOL             *Instance;
  DHCP_SERVICE              *DhcpSb;
  EFI_TPL                   OldTpl;

  //
  // First validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP_INSTANCE_FROM_THIS (This);

  if (Instance->Signature != DHCP_PROTOCOL_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl  = NET_RAISE_TPL (NET_TPL_LOCK);
  DhcpSb  = Instance->Service;

  DhcpCleanLease (DhcpSb);

  DhcpSb->DhcpState     = Dhcp4Stopped;
  DhcpSb->ServiceState  = DHCP_UNCONFIGED;

  NET_RESTORE_TPL (OldTpl);
  return EFI_SUCCESS;
}


/**
  Build a new DHCP packet from the seed packet. Options may be deleted or
  appended. The caller should free the NewPacket when finished using it.

  @param  This                   The DHCP protocol instance.
  @param  SeedPacket             The seed packet to start with
  @param  DeleteCount            The number of options to delete
  @param  DeleteList             The options to delete from the packet
  @param  AppendCount            The number of options to append
  @param  AppendList             The options to append to the packet
  @param  NewPacket              The new packet, allocated and built by this
                                 function.

  @retval EFI_INVALID_PARAMETER  The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory
  @retval EFI_SUCCESS            The packet is build.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4Build (
  IN EFI_DHCP4_PROTOCOL       *This,
  IN EFI_DHCP4_PACKET         *SeedPacket,
  IN UINT32                   DeleteCount,
  IN UINT8                    *DeleteList OPTIONAL,
  IN UINT32                   AppendCount,
  IN EFI_DHCP4_PACKET_OPTION  *AppendList[] OPTIONAL,
  OUT EFI_DHCP4_PACKET        **NewPacket
  )
{
  //
  // First validate the parameters
  //
  if ((This == NULL) || (NewPacket == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((SeedPacket == NULL) || (SeedPacket->Dhcp4.Magik != DHCP_OPTION_MAGIC) ||
      EFI_ERROR (DhcpValidateOptions (SeedPacket, NULL))) {

    return EFI_INVALID_PARAMETER;
  }

  if (((DeleteCount == 0) && (AppendCount == 0)) ||
      ((DeleteCount != 0) && (DeleteList == NULL)) ||
      ((AppendCount != 0) && (AppendList == NULL))) {

    return EFI_INVALID_PARAMETER;
  }

  return DhcpBuild (
           SeedPacket,
           DeleteCount,
           DeleteList,
           AppendCount,
           AppendList,
           NewPacket
           );
}


/**
  Transmit and receive a packet through this DHCP service.
  This is unsupported.

  @param  This                   The DHCP protocol instance
  @param  Token                  The transmit and receive instance

  @retval EFI_UNSUPPORTED        It always returns unsupported.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4TransmitReceive (
  IN EFI_DHCP4_PROTOCOL                *This,
  IN EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token
  )
{
  //
  // This function is for PXE, leave it for now
  //
  return EFI_UNSUPPORTED;
}


/**
  Callback function for DhcpIterateOptions. This callback sets the
  EFI_DHCP4_PACKET_OPTION array in the DHCP_PARSE_CONTEXT to point
  the individual DHCP option in the packet.

  @param  Tag                    The DHCP option type
  @param  Len                    length of the DHCP option data
  @param  Data                   The DHCP option data
  @param  Context                The context, to pass several parameters in.

  @retval EFI_SUCCESS            It always returns EFI_SUCCESS

**/
STATIC
EFI_STATUS
Dhcp4ParseCheckOption (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  )
{
  DHCP_PARSE_CONTEXT        *Parse;

  Parse = (DHCP_PARSE_CONTEXT *) Context;
  Parse->Index++;

  if (Parse->Index < Parse->OptionCount) {
    //
    // Use _CR to get the memory position of EFI_DHCP4_PACKET_OPTION for
    // the EFI_DHCP4_PACKET_OPTION->Data because DhcpIterateOptions only
    // pass in the point to option data.
    //
    Parse->Option[Parse->Index - 1] = _CR (Data, EFI_DHCP4_PACKET_OPTION, Data);
  }

  return EFI_SUCCESS;
}


/**
  Parse the DHCP options in the Packet into the PacketOptionList.
  User should allocate this array of EFI_DHCP4_PACKET_OPTION points.

  @param  This                   The DHCP protocol instance
  @param  Packet                 The DHCP packet to parse
  @param  OptionCount            On input, the size of the PacketOptionList; On
                                 output,  the actual number of options processed.
  @param  PacketOptionList       The array of EFI_DHCP4_PACKET_OPTION points

  @retval EFI_INVALID_PARAMETER  The parameters are invalid.
  @retval EFI_BUFFER_TOO_SMALL   A bigger array of points is needed.
  @retval EFI_SUCCESS            The options are parsed.

**/
STATIC
EFI_STATUS
EFIAPI
EfiDhcp4Parse (
  IN EFI_DHCP4_PROTOCOL       *This,
  IN EFI_DHCP4_PACKET         *Packet,
  IN OUT UINT32               *OptionCount,
  OUT EFI_DHCP4_PACKET_OPTION *PacketOptionList[] OPTIONAL
  )
{
  DHCP_PARSE_CONTEXT        Context;
  EFI_STATUS                Status;

  //
  // First validate the parameters
  //
  if ((This == NULL) || (Packet == NULL) || (OptionCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->Size < Packet->Length + 2 * sizeof (UINT32)) ||
      (Packet->Dhcp4.Magik != DHCP_OPTION_MAGIC) ||
      EFI_ERROR (DhcpValidateOptions (Packet, NULL))) {

    return EFI_INVALID_PARAMETER;
  }

  if ((*OptionCount != 0) && (PacketOptionList == NULL)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  NetZeroMem (PacketOptionList, *OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *));

  Context.Option      = PacketOptionList;
  Context.OptionCount = *OptionCount;
  Context.Index       = 0;

  Status              = DhcpIterateOptions (Packet, Dhcp4ParseCheckOption, &Context);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *OptionCount = Context.Index;

  if (Context.Index > Context.OptionCount) {
    return EFI_BUFFER_TOO_SMALL;
  }

  return EFI_SUCCESS;
}

EFI_DHCP4_PROTOCOL  mDhcp4ProtocolTemplate = {
  EfiDhcp4GetModeData,
  EfiDhcp4Configure,
  EfiDhcp4Start,
  EfiDhcp4RenewRebind,
  EfiDhcp4Release,
  EfiDhcp4Stop,
  EfiDhcp4Build,
  EfiDhcp4TransmitReceive,
  EfiDhcp4Parse
};
