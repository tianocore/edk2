/** @file
  This EFI_DHCP6_PROTOCOL interface implementation.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Dhcp6Impl.h"

//
// Well-known multi-cast address defined in section-24.1 of rfc-3315
//
//   ALL_DHCP_Relay_Agents_and_Servers address: FF02::1:2
//
EFI_IPv6_ADDRESS  mAllDhcpRelayAndServersAddress = {
  { 0xFF, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2 }
};

EFI_DHCP6_PROTOCOL  gDhcp6ProtocolTemplate = {
  EfiDhcp6GetModeData,
  EfiDhcp6Configure,
  EfiDhcp6Start,
  EfiDhcp6InfoRequest,
  EfiDhcp6RenewRebind,
  EfiDhcp6Decline,
  EfiDhcp6Release,
  EfiDhcp6Stop,
  EfiDhcp6Parse
};

/**
  Starts the DHCPv6 standard S.A.R.R. process.

  The Start() function starts the DHCPv6 standard process. This function can
  be called only when the state of Dhcp6 instance is in the Dhcp6Init state.
  If the DHCP process completes successfully, the state of the Dhcp6 instance
  will be transferred through Dhcp6Selecting and Dhcp6Requesting to the
  Dhcp6Bound state.
  Refer to rfc-3315 for precise state transitions during this process. At the
  time when each event occurs in this process, the callback function that was set
  by EFI_DHCP6_PROTOCOL.Configure() will be called, and the user can take this
  opportunity to control the process.

  @param[in]  This              The pointer to Dhcp6 protocol.

  @retval EFI_SUCCESS           The DHCPv6 standard process has started, or it has
                                completed when CompletionEvent is NULL.
  @retval EFI_ACCESS_DENIED     The EFI DHCPv6 Child instance hasn't been configured.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_TIMEOUT           The DHCPv6 configuration process failed because no
                                response was received from the server within the
                                specified timeout value.
  @retval EFI_ABORTED           The user aborted the DHCPv6 process.
  @retval EFI_ALREADY_STARTED   Some other Dhcp6 instance already started the DHCPv6
                                standard process.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_NO_MEDIA          There was a media error.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Start (
  IN EFI_DHCP6_PROTOCOL  *This
  )
{
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;
  DHCP6_INSTANCE  *Instance;
  DHCP6_SERVICE   *Service;
  EFI_STATUS      MediaStatus;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;

  //
  // The instance hasn't been configured.
  //
  if (Instance->Config == NULL) {
    return EFI_ACCESS_DENIED;
  }

  ASSERT (Instance->IaCb.Ia != NULL);

  //
  // The instance has already been started.
  //
  if (Instance->IaCb.Ia->State != Dhcp6Init) {
    return EFI_ALREADY_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Check Media Status.
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Service->Controller, DHCP_CHECK_MEDIA_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    Status = EFI_NO_MEDIA;
    goto ON_ERROR;
  }

  Instance->UdpSts = EFI_ALREADY_STARTED;

  //
  // Send the solicit message to start S.A.R.R process.
  //
  Status = Dhcp6SendSolicitMsg (Instance);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Register receive callback for the stateful exchange process.
  //
  Status = UdpIoRecvDatagram (
             Service->UdpIo,
             Dhcp6ReceivePacket,
             Service,
             0
             );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ON_ERROR;
  }

  gBS->RestoreTPL (OldTpl);

  //
  // Poll udp out of the net tpl if synchronous call.
  //
  if (Instance->Config->IaInfoEvent == NULL) {
    while (Instance->UdpSts == EFI_ALREADY_STARTED) {
      Service->UdpIo->Protocol.Udp6->Poll (Service->UdpIo->Protocol.Udp6);
    }

    return Instance->UdpSts;
  }

  return EFI_SUCCESS;

ON_ERROR:

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Stops the DHCPv6 standard S.A.R.R. process.

  The Stop() function is used to stop the DHCPv6 standard process. After this
  function is called successfully, the state of Dhcp6 instance is transferred
  into Dhcp6Init. EFI_DHCP6_PROTOCOL.Configure() needs to be called
  before DHCPv6 standard process can be started again. This function can be
  called when the Dhcp6 instance is in any state.

  @param[in]  This              The pointer to the Dhcp6 protocol.

  @retval EFI_SUCCESS           The Dhcp6 instance is now in the Dhcp6Init state.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Stop (
  IN EFI_DHCP6_PROTOCOL  *This
  )
{
  EFI_TPL            OldTpl;
  EFI_STATUS         Status;
  EFI_UDP6_PROTOCOL  *Udp6;
  DHCP6_INSTANCE     *Instance;
  DHCP6_SERVICE      *Service;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;
  Udp6     = Service->UdpIo->Protocol.Udp6;
  Status   = EFI_SUCCESS;

  //
  // The instance hasn't been configured.
  //
  if (Instance->Config == NULL) {
    return Status;
  }

  ASSERT (Instance->IaCb.Ia != NULL);

  //
  // No valid REPLY message received yet, cleanup this instance directly.
  //
  if ((Instance->IaCb.Ia->State == Dhcp6Init) ||
      (Instance->IaCb.Ia->State == Dhcp6Selecting) ||
      (Instance->IaCb.Ia->State == Dhcp6Requesting)
      )
  {
    goto ON_EXIT;
  }

  //
  // Release the current ready Ia.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance->UdpSts = EFI_ALREADY_STARTED;
  Status           = Dhcp6SendReleaseMsg (Instance, Instance->IaCb.Ia);
  gBS->RestoreTPL (OldTpl);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Poll udp out of the net tpl if synchronous call.
  //
  if (Instance->Config->IaInfoEvent == NULL) {
    ASSERT (Udp6 != NULL);
    while (Instance->UdpSts == EFI_ALREADY_STARTED) {
      Udp6->Poll (Udp6);
    }

    Status = Instance->UdpSts;
  }

ON_EXIT:
  //
  // Clean up the session data for the released Ia.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Dhcp6CleanupSession (Instance, EFI_SUCCESS);
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Returns the current operating mode data for the Dhcp6 instance.

  The GetModeData() function returns the current operating mode and
  cached data packet for the Dhcp6 instance.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[out] Dhcp6ModeData     The pointer to the Dhcp6 mode data.
  @param[out] Dhcp6ConfigData   The pointer to the Dhcp6 configure data.

  @retval EFI_SUCCESS           The mode data was returned.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_ACCESS_DENIED     The EFI DHCPv6 Protocol instance was not
                                configured.
**/
EFI_STATUS
EFIAPI
EfiDhcp6GetModeData (
  IN  EFI_DHCP6_PROTOCOL     *This,
  OUT EFI_DHCP6_MODE_DATA    *Dhcp6ModeData      OPTIONAL,
  OUT EFI_DHCP6_CONFIG_DATA  *Dhcp6ConfigData    OPTIONAL
  )
{
  EFI_TPL         OldTpl;
  EFI_DHCP6_IA    *Ia;
  DHCP6_INSTANCE  *Instance;
  DHCP6_SERVICE   *Service;
  UINT32          IaSize;
  UINT32          IdSize;

  if ((This == NULL) || ((Dhcp6ModeData == NULL) && (Dhcp6ConfigData == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;

  if ((Instance->Config == NULL) && (Dhcp6ConfigData != NULL)) {
    return EFI_ACCESS_DENIED;
  }

  ASSERT (Service->ClientId != NULL);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // User needs a copy of instance config data.
  //
  if (Dhcp6ConfigData != NULL) {
    ZeroMem (Dhcp6ConfigData, sizeof (EFI_DHCP6_CONFIG_DATA));
    //
    // Duplicate config data, including all reference buffers.
    //
    if (EFI_ERROR (Dhcp6CopyConfigData (Dhcp6ConfigData, Instance->Config))) {
      goto ON_ERROR;
    }
  }

  //
  // User need a copy of instance mode data.
  //
  if (Dhcp6ModeData != NULL) {
    ZeroMem (Dhcp6ModeData, sizeof (EFI_DHCP6_MODE_DATA));
    //
    // Duplicate a copy of EFI_DHCP6_DUID for client Id.
    //
    IdSize = Service->ClientId->Length + sizeof (Service->ClientId->Length);

    Dhcp6ModeData->ClientId = AllocateZeroPool (IdSize);
    if (Dhcp6ModeData->ClientId == NULL) {
      goto ON_ERROR;
    }

    CopyMem (
      Dhcp6ModeData->ClientId,
      Service->ClientId,
      IdSize
      );

    Ia = Instance->IaCb.Ia;
    if (Ia != NULL) {
      //
      // Duplicate a copy of EFI_DHCP6_IA for configured Ia.
      //
      IaSize = sizeof (EFI_DHCP6_IA) + (Ia->IaAddressCount -1) * sizeof (EFI_DHCP6_IA_ADDRESS);

      Dhcp6ModeData->Ia = AllocateZeroPool (IaSize);
      if (Dhcp6ModeData->Ia == NULL) {
        goto ON_ERROR;
      }

      CopyMem (
        Dhcp6ModeData->Ia,
        Ia,
        IaSize
        );

      //
      // Duplicate a copy of reply packet if has.
      //
      if (Ia->ReplyPacket != NULL) {
        Dhcp6ModeData->Ia->ReplyPacket = AllocateZeroPool (Ia->ReplyPacket->Size);
        if (Dhcp6ModeData->Ia->ReplyPacket == NULL) {
          goto ON_ERROR;
        }

        CopyMem (
          Dhcp6ModeData->Ia->ReplyPacket,
          Ia->ReplyPacket,
          Ia->ReplyPacket->Size
          );
      }
    }
  }

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_ERROR:

  if (Dhcp6ConfigData != NULL) {
    Dhcp6CleanupConfigData (Dhcp6ConfigData);
  }

  if (Dhcp6ModeData != NULL) {
    Dhcp6CleanupModeData (Dhcp6ModeData);
  }

  gBS->RestoreTPL (OldTpl);

  return EFI_OUT_OF_RESOURCES;
}

/**
  Initializes, changes, or resets the operational settings for the Dhcp6 instance.

  The Configure() function is used to initialize or clean up the configuration
  data of the Dhcp6 instance:
  - When Dhcp6CfgData is not NULL and Configure() is called successfully, the
    configuration data will be initialized in the Dhcp6 instance, and the state
    of the configured IA will be transferred into Dhcp6Init.
  - When Dhcp6CfgData is NULL and Configure() is called successfully, the
    configuration data will be cleaned up and no IA will be associated with
    the Dhcp6 instance.
  To update the configuration data for an Dhcp6 instance, the original data
  must be cleaned up before setting the new configuration data.

  @param[in]  This                   The pointer to the Dhcp6 protocol
  @param[in]  Dhcp6CfgData           The pointer to the EFI_DHCP6_CONFIG_DATA.

  @retval EFI_SUCCESS           The Dhcp6 is configured successfully with the
                                Dhcp6Init state, or cleaned up the original
                                configuration setting.
  @retval EFI_ACCESS_DENIED     The Dhcp6 instance was already configured.
                                The Dhcp6 instance has already started the
                                DHCPv6 S.A.R.R when Dhcp6CfgData is NULL.
  @retval EFI_INVALID_PARAMETER Some of the parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Configure (
  IN EFI_DHCP6_PROTOCOL     *This,
  IN EFI_DHCP6_CONFIG_DATA  *Dhcp6CfgData    OPTIONAL
  )
{
  EFI_TPL         OldTpl;
  EFI_STATUS      Status;
  LIST_ENTRY      *Entry;
  DHCP6_INSTANCE  *Other;
  DHCP6_INSTANCE  *Instance;
  DHCP6_SERVICE   *Service;
  UINTN           Index;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;

  //
  // Check the parameter of configure data.
  //
  if (Dhcp6CfgData != NULL) {
    if ((Dhcp6CfgData->OptionCount > 0) && (Dhcp6CfgData->OptionList == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if (Dhcp6CfgData->OptionList != NULL) {
      for (Index = 0; Index < Dhcp6CfgData->OptionCount; Index++) {
        if ((Dhcp6CfgData->OptionList[Index]->OpCode == Dhcp6OptClientId) ||
            (Dhcp6CfgData->OptionList[Index]->OpCode == Dhcp6OptRapidCommit) ||
            (Dhcp6CfgData->OptionList[Index]->OpCode == Dhcp6OptReconfigureAccept) ||
            (Dhcp6CfgData->OptionList[Index]->OpCode == Dhcp6OptIana) ||
            (Dhcp6CfgData->OptionList[Index]->OpCode == Dhcp6OptIata)
            )
        {
          return EFI_INVALID_PARAMETER;
        }
      }
    }

    if ((Dhcp6CfgData->IaDescriptor.Type != EFI_DHCP6_IA_TYPE_NA) &&
        (Dhcp6CfgData->IaDescriptor.Type != EFI_DHCP6_IA_TYPE_TA)
        )
    {
      return EFI_INVALID_PARAMETER;
    }

    if ((Dhcp6CfgData->IaInfoEvent == NULL) && (Dhcp6CfgData->SolicitRetransmission == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((Dhcp6CfgData->SolicitRetransmission != NULL) &&
        (Dhcp6CfgData->SolicitRetransmission->Mrc == 0) &&
        (Dhcp6CfgData->SolicitRetransmission->Mrd == 0)
        )
    {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Make sure the (IaId, IaType) is unique over all the instances.
    //
    NET_LIST_FOR_EACH (Entry, &Service->Child) {
      Other = NET_LIST_USER_STRUCT (Entry, DHCP6_INSTANCE, Link);
      if ((Other->IaCb.Ia != NULL) &&
          (Other->IaCb.Ia->Descriptor.Type == Dhcp6CfgData->IaDescriptor.Type) &&
          (Other->IaCb.Ia->Descriptor.IaId == Dhcp6CfgData->IaDescriptor.IaId)
          )
      {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Dhcp6CfgData != NULL) {
    //
    // It's not allowed to configure one instance twice without configure null.
    //
    if (Instance->Config != NULL) {
      gBS->RestoreTPL (OldTpl);
      return EFI_ACCESS_DENIED;
    }

    //
    // Duplicate config data including all reference buffers.
    //
    Instance->Config = AllocateZeroPool (sizeof (EFI_DHCP6_CONFIG_DATA));
    if (Instance->Config == NULL) {
      gBS->RestoreTPL (OldTpl);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = Dhcp6CopyConfigData (Instance->Config, Dhcp6CfgData);
    if (EFI_ERROR (Status)) {
      FreePool (Instance->Config);
      gBS->RestoreTPL (OldTpl);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Initialize the Ia descriptor from the config data, and leave the other
    // fields of the Ia as default value 0.
    //
    Instance->IaCb.Ia = AllocateZeroPool (sizeof (EFI_DHCP6_IA));
    if (Instance->IaCb.Ia == NULL) {
      Dhcp6CleanupConfigData (Instance->Config);
      FreePool (Instance->Config);
      gBS->RestoreTPL (OldTpl);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (
      &Instance->IaCb.Ia->Descriptor,
      &Dhcp6CfgData->IaDescriptor,
      sizeof (EFI_DHCP6_IA_DESCRIPTOR)
      );
  } else {
    if (Instance->Config == NULL) {
      ASSERT (Instance->IaCb.Ia == NULL);
      gBS->RestoreTPL (OldTpl);
      return EFI_SUCCESS;
    }

    //
    // It's not allowed to configure a started instance as null.
    //
    if (Instance->IaCb.Ia->State != Dhcp6Init) {
      gBS->RestoreTPL (OldTpl);
      return EFI_ACCESS_DENIED;
    }

    Dhcp6CleanupConfigData (Instance->Config);
    FreePool (Instance->Config);
    Instance->Config = NULL;

    FreePool (Instance->IaCb.Ia);
    Instance->IaCb.Ia = NULL;
  }

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**
  Request configuration information without the assignment of any
  Ia addresses of the client.

  The InfoRequest() function is used to request configuration information
  without the assignment of any IPv6 address of the client. The client sends
  out an Information Request packet to obtain the required configuration
  information, and DHCPv6 server responds with a Reply packet containing
  the information for the client. The received Reply packet will be passed
  to the user by ReplyCallback function. If the user returns EFI_NOT_READY from
  ReplyCallback, the Dhcp6 instance will continue to receive other Reply
  packets unless timeout according to the Retransmission parameter.
  Otherwise, the Information Request exchange process will be finished
  successfully if user returns EFI_SUCCESS from ReplyCallback.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[in]  SendClientId      If TRUE, the DHCPv6 protocol instance will build Client
                                Identifier option and include it into Information Request
                                packet. Otherwise, Client Identifier option will not be included.
  @param[in]  OptionRequest     The pointer to the buffer of option request options.
  @param[in]  OptionCount       The option number in the OptionList.
  @param[in]  OptionList        The list of appended options.
  @param[in]  Retransmission    The pointer to the retransmission of the message.
  @param[in]  TimeoutEvent      The event of timeout.
  @param[in]  ReplyCallback     The callback function when the reply was received.
  @param[in]  CallbackContext   The pointer to the parameter passed to the callback.

  @retval EFI_SUCCESS           The DHCPv6 information request exchange process
                                completed when TimeoutEvent is NULL. Information
                                Request packet has been sent to DHCPv6 server when
                                TimeoutEvent is not NULL.
  @retval EFI_NO_RESPONSE       The DHCPv6 information request exchange process failed
                                because of no response, or not all requested-options
                                are responded by DHCPv6 servers when Timeout happened.
  @retval EFI_ABORTED           The DHCPv6 information request exchange process was aborted
                                by user.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6InfoRequest (
  IN EFI_DHCP6_PROTOCOL        *This,
  IN BOOLEAN                   SendClientId,
  IN EFI_DHCP6_PACKET_OPTION   *OptionRequest,
  IN UINT32                    OptionCount,
  IN EFI_DHCP6_PACKET_OPTION   *OptionList[]    OPTIONAL,
  IN EFI_DHCP6_RETRANSMISSION  *Retransmission,
  IN EFI_EVENT                 TimeoutEvent     OPTIONAL,
  IN EFI_DHCP6_INFO_CALLBACK   ReplyCallback,
  IN VOID                      *CallbackContext OPTIONAL
  )
{
  EFI_STATUS      Status;
  DHCP6_INSTANCE  *Instance;
  DHCP6_SERVICE   *Service;
  UINTN           Index;
  EFI_EVENT       Timer;
  EFI_STATUS      TimerStatus;
  UINTN           GetMappingTimeOut;

  if ((This == NULL) || (OptionRequest == NULL) || (Retransmission == NULL) || (ReplyCallback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Retransmission != NULL) && (Retransmission->Mrc == 0) && (Retransmission->Mrd == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((OptionCount > 0) && (OptionList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (OptionList != NULL) {
    for (Index = 0; Index < OptionCount; Index++) {
      if ((OptionList[Index]->OpCode == Dhcp6OptClientId) || (OptionList[Index]->OpCode == Dhcp6OptRequestOption)) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;

  Status = Dhcp6StartInfoRequest (
             Instance,
             SendClientId,
             OptionRequest,
             OptionCount,
             OptionList,
             Retransmission,
             TimeoutEvent,
             ReplyCallback,
             CallbackContext
             );
  if (Status == EFI_NO_MAPPING) {
    //
    // The link local address is not ready, wait for some time and restart
    // the DHCP6 information request process.
    //
    Status = Dhcp6GetMappingTimeOut (Service->Ip6Cfg, &GetMappingTimeOut);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gBS->CreateEvent (EVT_TIMER, TPL_CALLBACK, NULL, NULL, &Timer);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Start the timer, wait for link local address DAD to finish.
    //
    Status = gBS->SetTimer (Timer, TimerRelative, GetMappingTimeOut);
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (Timer);
      return Status;
    }

    do {
      TimerStatus = gBS->CheckEvent (Timer);
      if (!EFI_ERROR (TimerStatus)) {
        Status = Dhcp6StartInfoRequest (
                   Instance,
                   SendClientId,
                   OptionRequest,
                   OptionCount,
                   OptionList,
                   Retransmission,
                   TimeoutEvent,
                   ReplyCallback,
                   CallbackContext
                   );
      }
    } while (TimerStatus == EFI_NOT_READY);

    gBS->CloseEvent (Timer);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Poll udp out of the net tpl if synchronous call.
  //
  if (TimeoutEvent == NULL) {
    while (Instance->UdpSts == EFI_ALREADY_STARTED) {
      Service->UdpIo->Protocol.Udp6->Poll (Service->UdpIo->Protocol.Udp6);
    }

    return Instance->UdpSts;
  }

  return EFI_SUCCESS;
}

/**
  Manually extend the valid and preferred lifetimes for the IPv6 addresses
  of the configured IA and update other configuration parameters by sending a
  Renew or Rebind packet.

  The RenewRebind() function is used to manually extend the valid and preferred
  lifetimes for the IPv6 addresses of the configured IA, and update other
  configuration parameters by sending Renew or Rebind packet.
  - When RebindRequest is FALSE and the state of the configured IA is Dhcp6Bound,
    it sends Renew packet to the previously DHCPv6 server and transfer the
    state of the configured IA to Dhcp6Renewing. If valid Reply packet received,
    the state transfers to Dhcp6Bound and the valid and preferred timer restarts.
    If fails, the state transfers to Dhcp6Bound, but the timer continues.
  - When RebindRequest is TRUE and the state of the configured IA is Dhcp6Bound,
    it will send a Rebind packet. If valid Reply packet is received, the state transfers
    to Dhcp6Bound and the valid and preferred timer restarts. If it fails, the state
    transfers to Dhcp6Init, and the IA can't be used.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[in]  RebindRequest     If TRUE, Rebind packet will be sent and enter Dhcp6Rebinding state.
                                Otherwise, Renew packet will be sent and enter Dhcp6Renewing state.

  @retval EFI_SUCCESS           The DHCPv6 renew/rebind exchange process has
                                completed and at least one IPv6 address of the
                                configured IA has been bound again when
                                EFI_DHCP6_CONFIG_DATA.IaInfoEvent is NULL.
                                The EFI DHCPv6 Protocol instance has sent Renew
                                or Rebind packet when
                                EFI_DHCP6_CONFIG_DATA.IaInfoEvent is not NULL.
  @retval EFI_ACCESS_DENIED     The Dhcp6 instance hasn't been configured, or the
                                state of the configured IA is not in Dhcp6Bound.
  @retval EFI_ALREADY_STARTED   The state of the configured IA has already entered
                                Dhcp6Renewing when RebindRequest is FALSE.
                                The state of the configured IA has already entered
                                Dhcp6Rebinding when RebindRequest is TRUE.
  @retval EFI_ABORTED           The DHCPv6 renew/rebind exchange process aborted
                                by the user.
  @retval EFI_NO_RESPONSE       The DHCPv6 renew/rebind exchange process failed
                                because of no response.
  @retval EFI_NO_MAPPING        No IPv6 address has been bound to the configured
                                IA after the DHCPv6 renew/rebind exchange process.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6RenewRebind (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN BOOLEAN             RebindRequest
  )
{
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;
  DHCP6_INSTANCE  *Instance;
  DHCP6_SERVICE   *Service;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;

  //
  // The instance hasn't been configured.
  //
  if (Instance->Config == NULL) {
    return EFI_ACCESS_DENIED;
  }

  ASSERT (Instance->IaCb.Ia != NULL);

  //
  // The instance has already entered renewing or rebinding state.
  //
  if (((Instance->IaCb.Ia->State == Dhcp6Rebinding) && RebindRequest) ||
      ((Instance->IaCb.Ia->State == Dhcp6Renewing) && !RebindRequest)
      )
  {
    return EFI_ALREADY_STARTED;
  }

  if (Instance->IaCb.Ia->State != Dhcp6Bound) {
    return EFI_ACCESS_DENIED;
  }

  OldTpl           = gBS->RaiseTPL (TPL_CALLBACK);
  Instance->UdpSts = EFI_ALREADY_STARTED;

  //
  // Send renew/rebind message to start exchange process.
  //
  Status = Dhcp6SendRenewRebindMsg (Instance, RebindRequest);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Register receive callback for the stateful exchange process.
  //
  Status = UdpIoRecvDatagram (
             Service->UdpIo,
             Dhcp6ReceivePacket,
             Service,
             0
             );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ON_ERROR;
  }

  gBS->RestoreTPL (OldTpl);

  //
  // Poll udp out of the net tpl if synchronous call.
  //
  if (Instance->Config->IaInfoEvent == NULL) {
    while (Instance->UdpSts == EFI_ALREADY_STARTED) {
      Service->UdpIo->Protocol.Udp6->Poll (Service->UdpIo->Protocol.Udp6);
    }

    return Instance->UdpSts;
  }

  return EFI_SUCCESS;

ON_ERROR:

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Inform that one or more addresses assigned by a server are already
  in use by another node.

  The Decline() function is used to manually decline the assignment of
  IPv6 addresses, which have been already used by another node. If all
  IPv6 addresses of the configured IA are declined through this function,
  the state of the IA will switch through Dhcp6Declining to Dhcp6Init.
  Otherwise, the state of the IA will restore to Dhcp6Bound after the
  declining process. The Decline() can only be called when the IA is in
  Dhcp6Bound state. If the EFI_DHCP6_CONFIG_DATA.IaInfoEvent is NULL,
  this function is a blocking operation. It will return after the
  declining process finishes, or aborted by user.

  @param[in]  This              The pointer to EFI_DHCP6_PROTOCOL.
  @param[in]  AddressCount      The number of declining addresses.
  @param[in]  Addresses         The pointer to the buffer stored the declining
                                addresses.

  @retval EFI_SUCCESS           The DHCPv6 decline exchange process completed
                                when EFI_DHCP6_CONFIG_DATA.IaInfoEvent was NULL.
                                The Dhcp6 instance sent Decline packet when
                                EFI_DHCP6_CONFIG_DATA.IaInfoEvent was not NULL.
  @retval EFI_ACCESS_DENIED     The Dhcp6 instance hasn't been configured, or the
                                state of the configured IA is not in Dhcp6Bound.
  @retval EFI_ABORTED           The DHCPv6 decline exchange process aborted by user.
  @retval EFI_NOT_FOUND         Any specified IPv6 address is not correlated with
                                the configured IA for this instance.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Decline (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN UINT32              AddressCount,
  IN EFI_IPv6_ADDRESS    *Addresses
  )
{
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;
  EFI_DHCP6_IA    *DecIa;
  DHCP6_INSTANCE  *Instance;
  DHCP6_SERVICE   *Service;

  if ((This == NULL) || (AddressCount == 0) || (Addresses == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;

  //
  // The instance hasn't been configured.
  //
  if (Instance->Config == NULL) {
    return EFI_ACCESS_DENIED;
  }

  ASSERT (Instance->IaCb.Ia != NULL);

  if (Instance->IaCb.Ia->State != Dhcp6Bound) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Check whether all the declined addresses belongs to the configured Ia.
  //
  Status = Dhcp6CheckAddress (Instance->IaCb.Ia, AddressCount, Addresses);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  OldTpl           = gBS->RaiseTPL (TPL_CALLBACK);
  Instance->UdpSts = EFI_ALREADY_STARTED;

  //
  // Deprive of all the declined addresses from the configured Ia, and create a
  // DeclineIa used to create decline message.
  //
  DecIa = Dhcp6DepriveAddress (Instance->IaCb.Ia, AddressCount, Addresses);

  if (DecIa == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Send the decline message to start exchange process.
  //
  Status = Dhcp6SendDeclineMsg (Instance, DecIa);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Register receive callback for the stateful exchange process.
  //
  Status = UdpIoRecvDatagram (
             Service->UdpIo,
             Dhcp6ReceivePacket,
             Service,
             0
             );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ON_ERROR;
  }

  FreePool (DecIa);
  gBS->RestoreTPL (OldTpl);

  //
  // Poll udp out of the net tpl if synchronous call.
  //
  if (Instance->Config->IaInfoEvent == NULL) {
    while (Instance->UdpSts == EFI_ALREADY_STARTED) {
      Service->UdpIo->Protocol.Udp6->Poll (Service->UdpIo->Protocol.Udp6);
    }

    return Instance->UdpSts;
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (DecIa != NULL) {
    FreePool (DecIa);
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Release one or more addresses associated with the configured Ia
  for current instance.

  The Release() function is used to manually release one or more
  IPv6 addresses. If AddressCount is zero, it will release all IPv6
  addresses of the configured IA. If all IPv6 addresses of the IA are
  released through this function, the state of the IA will switch
  through Dhcp6Releasing to Dhcp6Init, otherwise, the state of the
  IA will restore to Dhcp6Bound after the releasing process.
  The Release() can only be called when the IA is in Dhcp6Bound state.
  If the EFI_DHCP6_CONFIG_DATA.IaInfoEvent is NULL, the function is
  a blocking operation. It will return after the releasing process
  finishes, or is aborted by user.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[in]  AddressCount      The number of releasing addresses.
  @param[in]  Addresses         The pointer to the buffer stored the releasing
                                addresses.

  @retval EFI_SUCCESS           The DHCPv6 release exchange process
                                completed when EFI_DHCP6_CONFIG_DATA.IaInfoEvent
                                was NULL. The Dhcp6 instance was sent Release
                                packet when EFI_DHCP6_CONFIG_DATA.IaInfoEvent
                                was not NULL.
  @retval EFI_ACCESS_DENIED     The Dhcp6 instance hasn't been configured, or the
                                state of the configured IA is not in Dhcp6Bound.
  @retval EFI_ABORTED           The DHCPv6 release exchange process aborted by user.
  @retval EFI_NOT_FOUND         Any specified IPv6 address is not correlated with
                                the configured IA for this instance.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Release (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN UINT32              AddressCount,
  IN EFI_IPv6_ADDRESS    *Addresses
  )
{
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;
  EFI_DHCP6_IA    *RelIa;
  DHCP6_INSTANCE  *Instance;
  DHCP6_SERVICE   *Service;

  if ((This == NULL) || ((AddressCount != 0) && (Addresses == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;

  //
  // The instance hasn't been configured.
  //
  if (Instance->Config == NULL) {
    return EFI_ACCESS_DENIED;
  }

  ASSERT (Instance->IaCb.Ia != NULL);

  if (Instance->IaCb.Ia->State != Dhcp6Bound) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Check whether all the released addresses belongs to the configured Ia.
  //
  Status = Dhcp6CheckAddress (Instance->IaCb.Ia, AddressCount, Addresses);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  OldTpl           = gBS->RaiseTPL (TPL_CALLBACK);
  Instance->UdpSts = EFI_ALREADY_STARTED;

  //
  // Deprive of all the released addresses from the configured Ia, and create a
  // ReleaseIa used to create release message.
  //
  RelIa = Dhcp6DepriveAddress (Instance->IaCb.Ia, AddressCount, Addresses);

  if (RelIa == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Send the release message to start exchange process.
  //
  Status = Dhcp6SendReleaseMsg (Instance, RelIa);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Register receive callback for the stateful exchange process.
  //
  Status = UdpIoRecvDatagram (
             Service->UdpIo,
             Dhcp6ReceivePacket,
             Service,
             0
             );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ON_ERROR;
  }

  FreePool (RelIa);
  gBS->RestoreTPL (OldTpl);

  //
  // Poll udp out of the net tpl if synchronous call.
  //
  if (Instance->Config->IaInfoEvent == NULL) {
    while (Instance->UdpSts == EFI_ALREADY_STARTED) {
      Service->UdpIo->Protocol.Udp6->Poll (Service->UdpIo->Protocol.Udp6);
    }

    return Instance->UdpSts;
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (RelIa != NULL) {
    FreePool (RelIa);
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Parse the option data in the Dhcp6 packet.

  The Parse() function is used to retrieve the option list in the DHCPv6 packet.

  @param[in]      This              The pointer to the Dhcp6 protocol.
  @param[in]      Packet            The pointer to the Dhcp6 packet.
  @param[in, out] OptionCount       The number of option in the packet.
  @param[out]     PacketOptionList  The array of pointers to each option in the packet.

  @retval EFI_SUCCESS           The packet was successfully parsed.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_BUFFER_TOO_SMALL  *OptionCount is smaller than the number of options
                                that were found in the Packet.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Parse (
  IN EFI_DHCP6_PROTOCOL        *This,
  IN EFI_DHCP6_PACKET          *Packet,
  IN OUT UINT32                *OptionCount,
  OUT EFI_DHCP6_PACKET_OPTION  *PacketOptionList[]  OPTIONAL
  )
{
  UINT32  OptCnt;
  UINT32  OptLen;
  UINT16  DataLen;
  UINT8   *Start;
  UINT8   *End;

  if ((This == NULL) || (Packet == NULL) || (OptionCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*OptionCount != 0) && (PacketOptionList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->Length > Packet->Size) || (Packet->Length < sizeof (EFI_DHCP6_HEADER))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  //  The format of Dhcp6 option:
  //
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |          option-code          |   option-len (option data)    |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                          option-data                          |
  //    |                      (option-len octets)                      |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  OptCnt = 0;
  OptLen = Packet->Length - sizeof (EFI_DHCP6_HEADER);
  Start  = Packet->Dhcp6.Option;
  End    = Start + OptLen;

  //
  // Calculate the number of option in the packet.
  //
  while (Start < End) {
    DataLen = ((EFI_DHCP6_PACKET_OPTION *)Start)->OpLen;
    Start  += (NTOHS (DataLen) + 4);
    OptCnt++;
  }

  //
  // It will return buffer too small if pass-in option count is smaller than the
  // actual count of options in the packet.
  //
  if (OptCnt > *OptionCount) {
    *OptionCount = OptCnt;
    return EFI_BUFFER_TOO_SMALL;
  }

  ZeroMem (
    PacketOptionList,
    (*OptionCount * sizeof (EFI_DHCP6_PACKET_OPTION *))
    );

  OptCnt = 0;
  Start  = Packet->Dhcp6.Option;

  while (Start < End) {
    PacketOptionList[OptCnt] = (EFI_DHCP6_PACKET_OPTION *)Start;
    DataLen                  = ((EFI_DHCP6_PACKET_OPTION *)Start)->OpLen;
    Start                   += (NTOHS (DataLen) + 4);
    OptCnt++;
  }

  return EFI_SUCCESS;
}
