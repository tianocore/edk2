/** @file
  This file implement the EFI_DHCP4_PROTOCOL interface.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "Dhcp4Impl.h"

/**
  Returns the current operating mode and cached data packet for the EFI DHCPv4 Protocol driver.

  The GetModeData() function returns the current operating mode and cached data
  packet for the EFI DHCPv4 Protocol driver.

  @param[in]  This          Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[out] Dhcp4ModeData Pointer to storage for the EFI_DHCP4_MODE_DATA structure.

  @retval EFI_SUCCESS           The mode data was returned.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
EFI_STATUS
EFIAPI
EfiDhcp4GetModeData (
  IN  EFI_DHCP4_PROTOCOL    *This,
  OUT EFI_DHCP4_MODE_DATA   *Dhcp4ModeData
  );

/**
  Initializes, changes, or resets the operational settings for the EFI DHCPv4 Protocol driver.

  The Configure() function is used to initialize, change, or reset the operational
  settings of the EFI DHCPv4 Protocol driver for the communication device on which
  the EFI DHCPv4 Service Binding Protocol is installed. This function can be
  successfully called only if both of the following are true:
  * This instance of the EFI DHCPv4 Protocol driver is in the Dhcp4Stopped, Dhcp4Init,
    Dhcp4InitReboot, or Dhcp4Bound states.
  * No other EFI DHCPv4 Protocol driver instance that is controlled by this EFI
    DHCPv4 Service Binding Protocol driver instance has configured this EFI DHCPv4
    Protocol driver.
  When this driver is in the Dhcp4Stopped state, it can transfer into one of the
  following two possible initial states:
  * Dhcp4Init
  * Dhcp4InitReboot
  The driver can transfer into these states by calling Configure() with a non-NULL
  Dhcp4CfgData. The driver will transfer into the appropriate state based on the
  supplied client network address in the ClientAddress parameter and DHCP options
  in the OptionList parameter as described in RFC 2131.
  When Configure() is called successfully while Dhcp4CfgData is set to NULL, the
  default configuring data will be reset in the EFI DHCPv4 Protocol driver and
  the state of the EFI DHCPv4 Protocol driver will not be changed. If one instance
  wants to make it possible for another instance to configure the EFI DHCPv4 Protocol
  driver, it must call this function with Dhcp4CfgData set to NULL.

  @param[in]  This                   Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  Dhcp4CfgData           Pointer to the EFI_DHCP4_CONFIG_DATA.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Init or
                                Dhcp4InitReboot state, if the original state of this driver
                                was Dhcp4Stopped and the value of Dhcp4CfgData was
                                not NULL. Otherwise, the state was left unchanged.
  @retval EFI_ACCESS_DENIED     This instance of the EFI DHCPv4 Protocol driver was not in the
                                Dhcp4Stopped, Dhcp4Init, Dhcp4InitReboot, or Dhcp4Bound state;
                                Or onother instance of this EFI DHCPv4 Protocol driver is already
                                in a valid configured state.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp4Configure (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN EFI_DHCP4_CONFIG_DATA  *Dhcp4CfgData       OPTIONAL
  );

/**
  Starts the DHCP configuration process.

  The Start() function starts the DHCP configuration process. This function can
  be called only when the EFI DHCPv4 Protocol driver is in the Dhcp4Init or
  Dhcp4InitReboot state.
  If the DHCP process completes successfully, the state of the EFI DHCPv4 Protocol
  driver will be transferred through Dhcp4Selecting and Dhcp4Requesting to the
  Dhcp4Bound state. The CompletionEvent will then be signaled if it is not NULL.
  If the process aborts, either by the user or by some unexpected network error,
  the state is restored to the Dhcp4Init state. The Start() function can be called
  again to restart the process.
  Refer to RFC 2131 for precise state transitions during this process. At the
  time when each event occurs in this process, the callback function that was set
  by EFI_DHCP4_PROTOCOL.Configure() will be called and the user can take this
  opportunity to control the process.

  @param[in]  This            Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  CompletionEvent If not NULL, indicates the event that will be signaled when the
                              EFI DHCPv4 Protocol driver is transferred into the
                              Dhcp4Bound state or when the DHCP process is aborted.
                              EFI_DHCP4_PROTOCOL.GetModeData() can be called to
                              check the completion status. If NULL,
                              EFI_DHCP4_PROTOCOL.Start() will wait until the driver
                              is transferred into the Dhcp4Bound state or the process fails.

  @retval EFI_SUCCESS           The DHCP configuration process has started, or it has completed
                                when CompletionEvent is NULL.
  @retval EFI_NOT_STARTED       The EFI DHCPv4 Protocol driver is in the Dhcp4Stopped
                                state. EFI_DHCP4_PROTOCOL. Configure() needs to be called.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_TIMEOUT           The DHCP configuration process failed because no response was
                                received from the server within the specified timeout value.
  @retval EFI_ABORTED           The user aborted the DHCP process.
  @retval EFI_ALREADY_STARTED   Some other EFI DHCPv4 Protocol instance already started the
                                DHCP process.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp4Start (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN EFI_EVENT              CompletionEvent   OPTIONAL
  );

/**
  Extends the lease time by sending a request packet.

  The RenewRebind() function is used to manually extend the lease time when the
  EFI DHCPv4 Protocol driver is in the Dhcp4Bound state and the lease time has
  not expired yet. This function will send a request packet to the previously
  found server (or to any server when RebindRequest is TRUE) and transfer the
  state into the Dhcp4Renewing state (or Dhcp4Rebinding when RebindingRequest is
  TRUE). When a response is received, the state is returned to Dhcp4Bound.
  If no response is received before the try count is exceeded (the RequestTryCount
  field that is specified in EFI_DHCP4_CONFIG_DATA) but before the lease time that
  was issued by the previous server expires, the driver will return to the Dhcp4Bound
  state and the previous configuration is restored. The outgoing and incoming packets
  can be captured by the EFI_DHCP4_CALLBACK function.

  @param[in]  This            Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  RebindRequest   If TRUE, this function broadcasts the request packets and enters
                              the Dhcp4Rebinding state. Otherwise, it sends a unicast
                              request packet and enters the Dhcp4Renewing state.
  @param[in]  CompletionEvent If not NULL, this event is signaled when the renew/rebind phase
                              completes or some error occurs.
                              EFI_DHCP4_PROTOCOL.GetModeData() can be called to
                              check the completion status. If NULL,
                              EFI_DHCP4_PROTOCOL.RenewRebind() will busy-wait
                              until the DHCP process finishes.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the
                                Dhcp4Renewing state or is back to the Dhcp4Bound state.
  @retval EFI_NOT_STARTED       The EFI DHCPv4 Protocol driver is in the Dhcp4Stopped
                                state. EFI_DHCP4_PROTOCOL.Configure() needs to
                                be called.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_TIMEOUT           There was no response from the server when the try count was
                                exceeded.
  @retval EFI_ACCESS_DENIED     The driver is not in the Dhcp4Bound state.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp4RenewRebind (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN BOOLEAN                RebindRequest,
  IN EFI_EVENT              CompletionEvent   OPTIONAL
  );

/**
  Releases the current address configuration.

  The Release() function releases the current configured IP address by doing either
  of the following:
  * Sending a DHCPRELEASE packet when the EFI DHCPv4 Protocol driver is in the
    Dhcp4Bound state
  * Setting the previously assigned IP address that was provided with the
    EFI_DHCP4_PROTOCOL.Configure() function to 0.0.0.0 when the driver is in
    Dhcp4InitReboot state
  After a successful call to this function, the EFI DHCPv4 Protocol driver returns
  to the Dhcp4Init state and any subsequent incoming packets will be discarded silently.

  @param[in]  This                  Pointer to the EFI_DHCP4_PROTOCOL instance.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Init phase.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_ACCESS_DENIED     The EFI DHCPv4 Protocol driver is not Dhcp4InitReboot state.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp4Release (
  IN EFI_DHCP4_PROTOCOL     *This
  );

/**
  Stops the current address configuration.

  The Stop() function is used to stop the DHCP configuration process. After this
  function is called successfully, the EFI DHCPv4 Protocol driver is transferred
  into the Dhcp4Stopped state. EFI_DHCP4_PROTOCOL.Configure() needs to be called
  before DHCP configuration process can be started again. This function can be
  called when the EFI DHCPv4 Protocol driver is in any state.

  @param[in]  This                  Pointer to the EFI_DHCP4_PROTOCOL instance.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Stopped phase.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
EFI_STATUS
EFIAPI
EfiDhcp4Stop (
  IN EFI_DHCP4_PROTOCOL     *This
  );

/**
  Builds a DHCP packet, given the options to be appended or deleted or replaced.

  The Build() function is used to assemble a new packet from the original packet
  by replacing or deleting existing options or appending new options. This function
  does not change any state of the EFI DHCPv4 Protocol driver and can be used at
  any time.

  @param[in]  This        Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  SeedPacket  Initial packet to be used as a base for building new packet.
  @param[in]  DeleteCount Number of opcodes in the DeleteList.
  @param[in]  DeleteList  List of opcodes to be deleted from the seed packet.
                          Ignored if DeleteCount is zero.
  @param[in]  AppendCount Number of entries in the OptionList.
  @param[in]  AppendList  Pointer to a DHCP option list to be appended to SeedPacket.
                          If SeedPacket also contains options in this list, they are
                          replaced by new options (except pad option). Ignored if
                          AppendCount is zero. Type EFI_DHCP4_PACKET_OPTION
  @param[out] NewPacket   Pointer to storage for the pointer to the new allocated packet.
                          Use the EFI Boot Service FreePool() on the resulting pointer
                          when done with the packet.

  @retval EFI_SUCCESS           The new packet was built.
  @retval EFI_OUT_OF_RESOURCES  Storage for the new packet could not be allocated.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.

**/
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
  );

/**
  Transmits a DHCP formatted packet and optionally waits for responses.

  The TransmitReceive() function is used to transmit a DHCP packet and optionally
  wait for the response from servers. This function does not change the state of
  the EFI DHCPv4 Protocol driver and thus can be used at any time.

  @param[in]  This    Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  Token   Pointer to the EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN structure.

  @retval EFI_SUCCESS           The packet was successfully queued for transmission.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_NOT_READY         The previous call to this function has not finished yet. Try to call
                                this function after collection process completes.
  @retval EFI_NO_MAPPING        The default station address is not available yet.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Some other unexpected error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp4TransmitReceive (
  IN EFI_DHCP4_PROTOCOL                *This,
  IN EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token
  );

/**
  Parses the packed DHCP option data.

  The Parse() function is used to retrieve the option list from a DHCP packet.
  If *OptionCount isn't zero, and there is enough space for all the DHCP options
  in the Packet, each element of PacketOptionList is set to point to somewhere in
  the Packet->Dhcp4.Option where a new DHCP option begins. If RFC3396 is supported,
  the caller should reassemble the parsed DHCP options to get the finial result.
  If *OptionCount is zero or there isn't enough space for all of them, the number
  of DHCP options in the Packet is returned in OptionCount.

  @param  This             Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  Packet           Pointer to packet to be parsed.
  @param  OptionCount      On input, the number of entries in the PacketOptionList.
                           On output, the number of entries that were written into the
                           PacketOptionList.
  @param  PacketOptionList List of packet option entries to be filled in. End option or pad
                           options are not included.

  @retval EFI_SUCCESS           The packet was successfully parsed.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_BUFFER_TOO_SMALL  One or more of the following conditions is TRUE:
                                1) *OptionCount is smaller than the number of options that
                                were found in the Packet.
                                2) PacketOptionList is NULL.

**/
EFI_STATUS
EFIAPI
EfiDhcp4Parse (
  IN EFI_DHCP4_PROTOCOL       *This,
  IN EFI_DHCP4_PACKET         *Packet,
  IN OUT UINT32               *OptionCount,
  OUT EFI_DHCP4_PACKET_OPTION *PacketOptionList[] OPTIONAL
  );

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

/**
  Returns the current operating mode and cached data packet for the EFI DHCPv4 Protocol driver.

  The GetModeData() function returns the current operating mode and cached data
  packet for the EFI DHCPv4 Protocol driver.

  @param[in]  This          Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[out] Dhcp4ModeData Pointer to storage for the EFI_DHCP4_MODE_DATA structure.

  @retval EFI_SUCCESS           The mode data was returned.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
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

  OldTpl  = gBS->RaiseTPL (TPL_CALLBACK);
  DhcpSb  = Instance->Service;

  //
  // Caller can use GetModeData to retrieve current DHCP states
  // no matter whether it is the active child or not.
  //
  Dhcp4ModeData->State = (EFI_DHCP4_STATE) DhcpSb->DhcpState;
  CopyMem (&Dhcp4ModeData->ConfigData, &DhcpSb->ActiveConfig, sizeof (Dhcp4ModeData->ConfigData));
  CopyMem (&Dhcp4ModeData->ClientMacAddress, &DhcpSb->Mac, sizeof (Dhcp4ModeData->ClientMacAddress));

  Ip = HTONL (DhcpSb->ClientAddr);
  CopyMem (&Dhcp4ModeData->ClientAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

  Ip = HTONL (DhcpSb->Netmask);
  CopyMem (&Dhcp4ModeData->SubnetMask, &Ip, sizeof (EFI_IPv4_ADDRESS));

  Ip = HTONL (DhcpSb->ServerAddr);
  CopyMem (&Dhcp4ModeData->ServerAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

  Para = DhcpSb->Para;

  if (Para != NULL) {
    Ip = HTONL (Para->Router);
    CopyMem (&Dhcp4ModeData->RouterAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));
    Dhcp4ModeData->LeaseTime = Para->Lease;
  } else {
    ZeroMem (&Dhcp4ModeData->RouterAddress, sizeof (EFI_IPv4_ADDRESS));
    Dhcp4ModeData->LeaseTime = 0xffffffff;
  }

  Dhcp4ModeData->ReplyPacket = DhcpSb->Selected;

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}


/**
  Free the resource related to the configure parameters.
  DHCP driver will make a copy of the user's configure
  such as the time out value.

  @param  Config                 The DHCP configure data

**/
VOID
DhcpCleanConfigure (
  IN OUT EFI_DHCP4_CONFIG_DATA  *Config
  )
{
  UINT32                    Index;

  if (Config->DiscoverTimeout != NULL) {
    FreePool (Config->DiscoverTimeout);
  }

  if (Config->RequestTimeout != NULL) {
    FreePool (Config->RequestTimeout);
  }

  if (Config->OptionList != NULL) {
    for (Index = 0; Index < Config->OptionCount; Index++) {
      if (Config->OptionList[Index] != NULL) {
        FreePool (Config->OptionList[Index]);
      }
    }

    FreePool (Config->OptionList);
  }

  ZeroMem (Config, sizeof (EFI_DHCP4_CONFIG_DATA));
}


/**
  Allocate memory for configure parameter such as timeout value for Dst,
  then copy the configure parameter from Src to Dst.

  @param[out]  Dst                    The destination DHCP configure data.
  @param[in]   Src                    The source DHCP configure data.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_SUCCESS            The configure is copied.

**/
EFI_STATUS
DhcpCopyConfigure (
  OUT EFI_DHCP4_CONFIG_DATA  *Dst,
  IN  EFI_DHCP4_CONFIG_DATA  *Src
  )
{
  EFI_DHCP4_PACKET_OPTION   **DstOptions;
  EFI_DHCP4_PACKET_OPTION   **SrcOptions;
  UINTN                     Len;
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
    Dst->DiscoverTimeout  = AllocatePool (Len);

    if (Dst->DiscoverTimeout == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < Src->DiscoverTryCount; Index++) {
      Dst->DiscoverTimeout[Index] = MAX (Src->DiscoverTimeout[Index], 1);
    }
  }

  //
  // Allocate a memory then copy RequestTimeout to it
  //
  if (Src->RequestTimeout != NULL) {
    Len                 = Src->RequestTryCount * sizeof (UINT32);
    Dst->RequestTimeout = AllocatePool (Len);

    if (Dst->RequestTimeout == NULL) {
      goto ON_ERROR;
    }

    for (Index = 0; Index < Src->RequestTryCount; Index++) {
      Dst->RequestTimeout[Index] = MAX (Src->RequestTimeout[Index], 1);
    }
  }

  //
  // Allocate an array of dhcp option point, then allocate memory
  // for each option and copy the source option to it
  //
  if (Src->OptionList != NULL) {
    Len             = Src->OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *);
    Dst->OptionList = AllocateZeroPool (Len);

    if (Dst->OptionList == NULL) {
      goto ON_ERROR;
    }

    DstOptions  = Dst->OptionList;
    SrcOptions  = Src->OptionList;

    for (Index = 0; Index < Src->OptionCount; Index++) {
      Len = sizeof (EFI_DHCP4_PACKET_OPTION) + MAX (SrcOptions[Index]->Length - 1, 0);

      DstOptions[Index] = AllocatePool (Len);

      if (DstOptions[Index] == NULL) {
        goto ON_ERROR;
      }

      CopyMem (DstOptions[Index], SrcOptions[Index], Len);
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
    FreePool (Config->DiscoverTimeout);

    Config->DiscoverTryCount  = 0;
    Config->DiscoverTimeout   = NULL;
  }

  if (Config->RequestTimeout != NULL) {
    FreePool (Config->RequestTimeout);

    Config->RequestTryCount = 0;
    Config->RequestTimeout  = NULL;
  }

  Config->Dhcp4Callback   = NULL;
  Config->CallbackContext = NULL;
}


/**
  Initializes, changes, or resets the operational settings for the EFI DHCPv4 Protocol driver.

  The Configure() function is used to initialize, change, or reset the operational
  settings of the EFI DHCPv4 Protocol driver for the communication device on which
  the EFI DHCPv4 Service Binding Protocol is installed. This function can be
  successfully called only if both of the following are true:
  * This instance of the EFI DHCPv4 Protocol driver is in the Dhcp4Stopped, Dhcp4Init,
    Dhcp4InitReboot, or Dhcp4Bound states.
  * No other EFI DHCPv4 Protocol driver instance that is controlled by this EFI
    DHCPv4 Service Binding Protocol driver instance has configured this EFI DHCPv4
    Protocol driver.
  When this driver is in the Dhcp4Stopped state, it can transfer into one of the
  following two possible initial states:
  * Dhcp4Init
  * Dhcp4InitReboot
  The driver can transfer into these states by calling Configure() with a non-NULL
  Dhcp4CfgData. The driver will transfer into the appropriate state based on the
  supplied client network address in the ClientAddress parameter and DHCP options
  in the OptionList parameter as described in RFC 2131.
  When Configure() is called successfully while Dhcp4CfgData is set to NULL, the
  default configuring data will be reset in the EFI DHCPv4 Protocol driver and
  the state of the EFI DHCPv4 Protocol driver will not be changed. If one instance
  wants to make it possible for another instance to configure the EFI DHCPv4 Protocol
  driver, it must call this function with Dhcp4CfgData set to NULL.

  @param[in]  This                   Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  Dhcp4CfgData           Pointer to the EFI_DHCP4_CONFIG_DATA.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Init or
                                Dhcp4InitReboot state, if the original state of this driver
                                was Dhcp4Stopped and the value of Dhcp4CfgData was
                                not NULL. Otherwise, the state was left unchanged.
  @retval EFI_ACCESS_DENIED     This instance of the EFI DHCPv4 Protocol driver was not in the
                                Dhcp4Stopped, Dhcp4Init, Dhcp4InitReboot, or Dhcp4Bound state;
                                Or onother instance of this EFI DHCPv4 Protocol driver is already
                                in a valid configured state.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
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
    if ((Dhcp4CfgData->DiscoverTryCount != 0) && (Dhcp4CfgData->DiscoverTimeout == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((Dhcp4CfgData->RequestTryCount != 0) && (Dhcp4CfgData->RequestTimeout == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((Dhcp4CfgData->OptionCount != 0) && (Dhcp4CfgData->OptionList == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    CopyMem (&Ip, &Dhcp4CfgData->ClientAddress, sizeof (IP4_ADDR));
    if (IP4_IS_LOCAL_BROADCAST(NTOHL (Ip))) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Instance = DHCP_INSTANCE_FROM_THIS (This);

  if (Instance->Signature != DHCP_PROTOCOL_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl  = gBS->RaiseTPL (TPL_CALLBACK);

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
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Starts the DHCP configuration process.

  The Start() function starts the DHCP configuration process. This function can
  be called only when the EFI DHCPv4 Protocol driver is in the Dhcp4Init or
  Dhcp4InitReboot state.
  If the DHCP process completes successfully, the state of the EFI DHCPv4 Protocol
  driver will be transferred through Dhcp4Selecting and Dhcp4Requesting to the
  Dhcp4Bound state. The CompletionEvent will then be signaled if it is not NULL.
  If the process aborts, either by the user or by some unexpected network error,
  the state is restored to the Dhcp4Init state. The Start() function can be called
  again to restart the process.
  Refer to RFC 2131 for precise state transitions during this process. At the
  time when each event occurs in this process, the callback function that was set
  by EFI_DHCP4_PROTOCOL.Configure() will be called and the user can take this
  opportunity to control the process.

  @param[in]  This            Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  CompletionEvent If not NULL, indicates the event that will be signaled when the
                              EFI DHCPv4 Protocol driver is transferred into the
                              Dhcp4Bound state or when the DHCP process is aborted.
                              EFI_DHCP4_PROTOCOL.GetModeData() can be called to
                              check the completion status. If NULL,
                              EFI_DHCP4_PROTOCOL.Start() will wait until the driver
                              is transferred into the Dhcp4Bound state or the process fails.

  @retval EFI_SUCCESS           The DHCP configuration process has started, or it has completed
                                when CompletionEvent is NULL.
  @retval EFI_NOT_STARTED       The EFI DHCPv4 Protocol driver is in the Dhcp4Stopped
                                state. EFI_DHCP4_PROTOCOL. Configure() needs to be called.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_TIMEOUT           The DHCP configuration process failed because no response was
                                received from the server within the specified timeout value.
  @retval EFI_ABORTED           The user aborted the DHCP process.
  @retval EFI_ALREADY_STARTED   Some other EFI DHCPv4 Protocol instance already started the
                                DHCP process.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_NO_MEDIA          There was a media error.

**/
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
  EFI_STATUS                MediaStatus;

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

  OldTpl  = gBS->RaiseTPL (TPL_CALLBACK);
  DhcpSb  = Instance->Service;

  if (DhcpSb->DhcpState == Dhcp4Stopped) {
    Status = EFI_NOT_STARTED;
    goto ON_ERROR;
  }

  if ((DhcpSb->DhcpState != Dhcp4Init) && (DhcpSb->DhcpState != Dhcp4InitReboot)) {
    Status = EFI_ALREADY_STARTED;
    goto ON_ERROR;
  }

  //
  // Check Media Satus.
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (DhcpSb->Controller, DHCP_CHECK_MEDIA_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    Status = EFI_NO_MEDIA;
    goto ON_ERROR;
  }

  DhcpSb->IoStatus = EFI_ALREADY_STARTED;

  if (EFI_ERROR (Status = DhcpInitRequest (DhcpSb))) {
    goto ON_ERROR;
  }


  Instance->CompletionEvent = CompletionEvent;

  //
  // Restore the TPL now, don't call poll function at TPL_CALLBACK.
  //
  gBS->RestoreTPL (OldTpl);

  if (CompletionEvent == NULL) {
    while (DhcpSb->IoStatus == EFI_ALREADY_STARTED) {
      DhcpSb->UdpIo->Protocol.Udp4->Poll (DhcpSb->UdpIo->Protocol.Udp4);
    }

    return DhcpSb->IoStatus;
  }

  return EFI_SUCCESS;

ON_ERROR:
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Extends the lease time by sending a request packet.

  The RenewRebind() function is used to manually extend the lease time when the
  EFI DHCPv4 Protocol driver is in the Dhcp4Bound state and the lease time has
  not expired yet. This function will send a request packet to the previously
  found server (or to any server when RebindRequest is TRUE) and transfer the
  state into the Dhcp4Renewing state (or Dhcp4Rebinding when RebindingRequest is
  TRUE). When a response is received, the state is returned to Dhcp4Bound.
  If no response is received before the try count is exceeded (the RequestTryCount
  field that is specified in EFI_DHCP4_CONFIG_DATA) but before the lease time that
  was issued by the previous server expires, the driver will return to the Dhcp4Bound
  state and the previous configuration is restored. The outgoing and incoming packets
  can be captured by the EFI_DHCP4_CALLBACK function.

  @param[in]  This            Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  RebindRequest   If TRUE, this function broadcasts the request packets and enters
                              the Dhcp4Rebinding state. Otherwise, it sends a unicast
                              request packet and enters the Dhcp4Renewing state.
  @param[in]  CompletionEvent If not NULL, this event is signaled when the renew/rebind phase
                              completes or some error occurs.
                              EFI_DHCP4_PROTOCOL.GetModeData() can be called to
                              check the completion status. If NULL,
                              EFI_DHCP4_PROTOCOL.RenewRebind() will busy-wait
                              until the DHCP process finishes.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the
                                Dhcp4Renewing state or is back to the Dhcp4Bound state.
  @retval EFI_NOT_STARTED       The EFI DHCPv4 Protocol driver is in the Dhcp4Stopped
                                state. EFI_DHCP4_PROTOCOL.Configure() needs to
                                be called.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_TIMEOUT           There was no response from the server when the try count was
                                exceeded.
  @retval EFI_ACCESS_DENIED     The driver is not in the Dhcp4Bound state.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
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

  OldTpl  = gBS->RaiseTPL (TPL_CALLBACK);
  DhcpSb  = Instance->Service;

  if (DhcpSb->DhcpState == Dhcp4Stopped) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (DhcpSb->DhcpState != Dhcp4Bound) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  if (DHCP_IS_BOOTP (DhcpSb->Para)) {
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Transit the states then send a extra DHCP request
  //
  if (!RebindRequest) {
    DhcpSetState (DhcpSb, Dhcp4Renewing, FALSE);
  } else {
    DhcpSetState (DhcpSb, Dhcp4Rebinding, FALSE);
  }

  //
  // Clear initial time to make sure that elapsed-time
  // is set to 0 for first REQUEST in renewal process.
  //
  Instance->ElaspedTime = 0;

  Status = DhcpSendMessage (
             DhcpSb,
             DhcpSb->Selected,
             DhcpSb->Para,
             DHCP_MSG_REQUEST,
             (UINT8 *) "Extra renew/rebind by the application"
             );

  if (EFI_ERROR (Status)) {
    DhcpSetState (DhcpSb, Dhcp4Bound, FALSE);
    goto ON_EXIT;
  }

  DhcpSb->ExtraRefresh        = TRUE;
  DhcpSb->IoStatus            = EFI_ALREADY_STARTED;
  Instance->RenewRebindEvent  = CompletionEvent;

  gBS->RestoreTPL (OldTpl);

  if (CompletionEvent == NULL) {
    while (DhcpSb->IoStatus == EFI_ALREADY_STARTED) {
      DhcpSb->UdpIo->Protocol.Udp4->Poll (DhcpSb->UdpIo->Protocol.Udp4);

    }

    return DhcpSb->IoStatus;
  }

  return EFI_SUCCESS;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Releases the current address configuration.

  The Release() function releases the current configured IP address by doing either
  of the following:
  * Sending a DHCPRELEASE packet when the EFI DHCPv4 Protocol driver is in the
    Dhcp4Bound state
  * Setting the previously assigned IP address that was provided with the
    EFI_DHCP4_PROTOCOL.Configure() function to 0.0.0.0 when the driver is in
    Dhcp4InitReboot state
  After a successful call to this function, the EFI DHCPv4 Protocol driver returns
  to the Dhcp4Init state and any subsequent incoming packets will be discarded silently.

  @param[in]  This                  Pointer to the EFI_DHCP4_PROTOCOL instance.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Init phase.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_ACCESS_DENIED     The EFI DHCPv4 Protocol driver is not Dhcp4InitReboot state.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
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
  OldTpl  = gBS->RaiseTPL (TPL_CALLBACK);
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
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Stops the current address configuration.

  The Stop() function is used to stop the DHCP configuration process. After this
  function is called successfully, the EFI DHCPv4 Protocol driver is transferred
  into the Dhcp4Stopped state. EFI_DHCP4_PROTOCOL.Configure() needs to be called
  before DHCP configuration process can be started again. This function can be
  called when the EFI DHCPv4 Protocol driver is in any state.

  @param[in]  This                  Pointer to the EFI_DHCP4_PROTOCOL instance.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Stopped phase.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
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

  OldTpl  = gBS->RaiseTPL (TPL_CALLBACK);
  DhcpSb  = Instance->Service;

  DhcpCleanLease (DhcpSb);

  DhcpSb->DhcpState     = Dhcp4Stopped;
  DhcpSb->ServiceState  = DHCP_UNCONFIGED;

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}


/**
  Builds a DHCP packet, given the options to be appended or deleted or replaced.

  The Build() function is used to assemble a new packet from the original packet
  by replacing or deleting existing options or appending new options. This function
  does not change any state of the EFI DHCPv4 Protocol driver and can be used at
  any time.

  @param[in]  This        Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  SeedPacket  Initial packet to be used as a base for building new packet.
  @param[in]  DeleteCount Number of opcodes in the DeleteList.
  @param[in]  DeleteList  List of opcodes to be deleted from the seed packet.
                          Ignored if DeleteCount is zero.
  @param[in]  AppendCount Number of entries in the OptionList.
  @param[in]  AppendList  Pointer to a DHCP option list to be appended to SeedPacket.
                          If SeedPacket also contains options in this list, they are
                          replaced by new options (except pad option). Ignored if
                          AppendCount is zero. Type EFI_DHCP4_PACKET_OPTION
  @param[out] NewPacket   Pointer to storage for the pointer to the new allocated packet.
                          Use the EFI Boot Service FreePool() on the resulting pointer
                          when done with the packet.

  @retval EFI_SUCCESS           The new packet was built.
  @retval EFI_OUT_OF_RESOURCES  Storage for the new packet could not be allocated.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.

**/
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
  Callback by UdpIoCreatePort() when creating UdpIo for this Dhcp4 instance.

  @param[in] UdpIo      The UdpIo being created.
  @param[in] Context    Dhcp4 instance.

  @retval EFI_SUCCESS              UdpIo is configured successfully.
  @retval EFI_INVALID_PARAMETER    Class E IP address is not supported or other parameters
                                   are not valid.
  @retval other                    Other error occurs.
**/
EFI_STATUS
EFIAPI
Dhcp4InstanceConfigUdpIo (
  IN UDP_IO       *UdpIo,
  IN VOID         *Context
  )
{
  DHCP_PROTOCOL                     *Instance;
  DHCP_SERVICE                      *DhcpSb;
  EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token;
  EFI_UDP4_CONFIG_DATA              UdpConfigData;
  IP4_ADDR                          ClientAddr;
  IP4_ADDR                          Ip;
  INTN                              Class;
  IP4_ADDR                          SubnetMask;

  Instance = (DHCP_PROTOCOL *) Context;
  DhcpSb   = Instance->Service;
  Token    = Instance->Token;

  ZeroMem (&UdpConfigData, sizeof (EFI_UDP4_CONFIG_DATA));

  UdpConfigData.AcceptBroadcast    = TRUE;
  UdpConfigData.AllowDuplicatePort = TRUE;
  UdpConfigData.TimeToLive         = 64;
  UdpConfigData.DoNotFragment      = TRUE;

  ClientAddr = EFI_NTOHL (Token->Packet->Dhcp4.Header.ClientAddr);
  Ip = HTONL (ClientAddr);
  CopyMem (&UdpConfigData.StationAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

  if (DhcpSb->Netmask == 0) {
    //
    // The Dhcp4.TransmitReceive() API should be able to used at any time according to
    // UEFI spec, while in classless addressing network, the netmask must be explicitly
    // provided together with the station address.
    // If the DHCP instance haven't be configured with a valid netmask, we could only
    // compute it according to the classful addressing rule.
    //
    Class = NetGetIpClass (ClientAddr);
    //
    //  Class E IP address is not supported here!
    //
    ASSERT (Class < IP4_ADDR_CLASSE);
    if (Class >= IP4_ADDR_CLASSE) {
      return EFI_INVALID_PARAMETER;
    }

    SubnetMask = gIp4AllMasks[Class << 3];
  } else {
    SubnetMask = DhcpSb->Netmask;
  }

  Ip = HTONL (SubnetMask);
  CopyMem (&UdpConfigData.SubnetMask, &Ip, sizeof (EFI_IPv4_ADDRESS));

  if ((Token->ListenPointCount == 0) || (Token->ListenPoints[0].ListenPort == 0)) {
    UdpConfigData.StationPort = DHCP_CLIENT_PORT;
  } else {
    UdpConfigData.StationPort = Token->ListenPoints[0].ListenPort;
  }

  return UdpIo->Protocol.Udp4->Configure (UdpIo->Protocol.Udp4, &UdpConfigData);
}

/**
  Create UdpIo for this Dhcp4 instance.

  @param Instance   The Dhcp4 instance.

  @retval EFI_SUCCESS                UdpIo is created successfully.
  @retval EFI_OUT_OF_RESOURCES       Fails to create UdpIo because of limited
                                     resources or configuration failure.
**/
EFI_STATUS
Dhcp4InstanceCreateUdpIo (
  IN OUT DHCP_PROTOCOL  *Instance
  )
{
  DHCP_SERVICE  *DhcpSb;
  EFI_STATUS    Status;
  VOID          *Udp4;

  ASSERT (Instance->Token != NULL);

  DhcpSb          = Instance->Service;
  Instance->UdpIo = UdpIoCreateIo (
                      DhcpSb->Controller,
                      DhcpSb->Image,
                      Dhcp4InstanceConfigUdpIo,
                      UDP_IO_UDP4_VERSION,
                      Instance
                      );
  if (Instance->UdpIo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  } else {
    Status = gBS->OpenProtocol (
                    Instance->UdpIo->UdpHandle,
                    &gEfiUdp4ProtocolGuid,
                    (VOID **) &Udp4,
                    Instance->Service->Image,
                    Instance->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (EFI_ERROR (Status)) {
      UdpIoFreeIo (Instance->UdpIo);
      Instance->UdpIo = NULL;
    }
    return Status;
  }
}

/**
  Callback of Dhcp packet. Does nothing.

  @param Arg           The context.

**/
VOID
EFIAPI
DhcpDummyExtFree (
  IN VOID                   *Arg
  )
{
}

/**
  Callback of UdpIoRecvDatagram() that handles a Dhcp4 packet.

  Only BOOTP responses will be handled that correspond to the Xid of the request
  sent out. The packet will be queued to the response queue.

  @param UdpPacket        The Dhcp4 packet.
  @param EndPoint         Udp4 address pair.
  @param IoStatus         Status of the input.
  @param Context          Extra info for the input.

**/
VOID
EFIAPI
PxeDhcpInput (
  NET_BUF                   *UdpPacket,
  UDP_END_POINT             *EndPoint,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  )
{
  DHCP_PROTOCOL                     *Instance;
  EFI_DHCP4_HEADER                  *Head;
  NET_BUF                           *Wrap;
  EFI_DHCP4_PACKET                  *Packet;
  EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token;
  UINT32                            Len;
  EFI_STATUS                        Status;

  Wrap     = NULL;
  Instance = (DHCP_PROTOCOL *) Context;
  Token    = Instance->Token;

  //
  // Don't restart receive if error occurs or DHCP is destroyed.
  //
  if (EFI_ERROR (IoStatus)) {
    return ;
  }

  ASSERT (UdpPacket != NULL);

  //
  // Validate the packet received
  //
  if (UdpPacket->TotalSize < sizeof (EFI_DHCP4_HEADER)) {
    goto RESTART;
  }

  //
  // Copy the DHCP message to a continuous memory block, make the buffer size
  // of the EFI_DHCP4_PACKET a multiple of 4-byte.
  //
  Len  = NET_ROUNDUP (sizeof (EFI_DHCP4_PACKET) + UdpPacket->TotalSize - sizeof (EFI_DHCP4_HEADER), 4);
  Wrap = NetbufAlloc (Len);
  if (Wrap == NULL) {
    goto RESTART;
  }

  Packet         = (EFI_DHCP4_PACKET *) NetbufAllocSpace (Wrap, Len, NET_BUF_TAIL);
  ASSERT (Packet != NULL);

  Packet->Size   = Len;
  Head           = &Packet->Dhcp4.Header;
  Packet->Length = NetbufCopy (UdpPacket, 0, UdpPacket->TotalSize, (UINT8 *) Head);

  if (Packet->Length != UdpPacket->TotalSize) {
    goto RESTART;
  }

  //
  // Is this packet the answer to our packet?
  //
  if ((Head->OpCode != BOOTP_REPLY) ||
      (Head->Xid != Token->Packet->Dhcp4.Header.Xid) ||
      (CompareMem (&Token->Packet->Dhcp4.Header.ClientHwAddr[0], Head->ClientHwAddr, Head->HwAddrLen) != 0)) {
    goto RESTART;
  }

  //
  // Validate the options and retrieve the interested options
  //
  if ((Packet->Length > sizeof (EFI_DHCP4_HEADER) + sizeof (UINT32)) &&
      (Packet->Dhcp4.Magik == DHCP_OPTION_MAGIC) &&
      EFI_ERROR (DhcpValidateOptions (Packet, NULL))) {

    goto RESTART;
  }

  //
  // Keep this packet in the ResponseQueue.
  //
  NET_GET_REF (Wrap);
  NetbufQueAppend (&Instance->ResponseQueue, Wrap);

RESTART:

  NetbufFree (UdpPacket);

  if (Wrap != NULL) {
    NetbufFree (Wrap);
  }

  Status = UdpIoRecvDatagram (Instance->UdpIo, PxeDhcpInput, Instance, 0);
  if (EFI_ERROR (Status)) {
    PxeDhcpDone (Instance);
  }
}

/**
  Complete a Dhcp4 transaction and signal the upper layer.

  @param Instance      Dhcp4 instance.

**/
VOID
PxeDhcpDone (
  IN DHCP_PROTOCOL  *Instance
  )
{
  EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token;

  Token = Instance->Token;

  Token->ResponseCount = Instance->ResponseQueue.BufNum;
  if (Token->ResponseCount != 0) {
    Token->ResponseList = (EFI_DHCP4_PACKET *) AllocatePool (Instance->ResponseQueue.BufSize);
    if (Token->ResponseList == NULL) {
      Token->Status = EFI_OUT_OF_RESOURCES;
      goto SIGNAL_USER;
    }

    //
    // Copy the received DHCP responses.
    //
    NetbufQueCopy (&Instance->ResponseQueue, 0, Instance->ResponseQueue.BufSize, (UINT8 *) Token->ResponseList);
    Token->Status = EFI_SUCCESS;
  } else {
    Token->ResponseList = NULL;
    Token->Status       = EFI_TIMEOUT;
  }

SIGNAL_USER:
  //
  // Clean up the resources dedicated for this transmit receive transaction.
  //
  NetbufQueFlush (&Instance->ResponseQueue);
  UdpIoCleanIo (Instance->UdpIo);
  gBS->CloseProtocol (
         Instance->UdpIo->UdpHandle,
         &gEfiUdp4ProtocolGuid,
         Instance->Service->Image,
         Instance->Handle
         );
  UdpIoFreeIo (Instance->UdpIo);
  Instance->UdpIo = NULL;
  Instance->Token = NULL;

  if (Token->CompletionEvent != NULL) {
    gBS->SignalEvent (Token->CompletionEvent);
  }
}


/**
  Transmits a DHCP formatted packet and optionally waits for responses.

  The TransmitReceive() function is used to transmit a DHCP packet and optionally
  wait for the response from servers. This function does not change the state of
  the EFI DHCPv4 Protocol driver and thus can be used at any time.

  @param[in]  This    Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param[in]  Token   Pointer to the EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN structure.

  @retval EFI_SUCCESS           The packet was successfully queued for transmission.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_NOT_READY         The previous call to this function has not finished yet. Try to call
                                this function after collection process completes.
  @retval EFI_NO_MAPPING        The default station address is not available yet.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Some other unexpected error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp4TransmitReceive (
  IN EFI_DHCP4_PROTOCOL                *This,
  IN EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token
  )
{
  DHCP_PROTOCOL  *Instance;
  EFI_TPL        OldTpl;
  EFI_STATUS     Status;
  NET_FRAGMENT   Frag;
  NET_BUF        *Wrap;
  UDP_END_POINT  EndPoint;
  IP4_ADDR       Ip;
  DHCP_SERVICE   *DhcpSb;
  EFI_IP_ADDRESS Gateway;
  IP4_ADDR       ClientAddr;

  if ((This == NULL) || (Token == NULL) || (Token->Packet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP_INSTANCE_FROM_THIS (This);
  DhcpSb   = Instance->Service;

  if (Instance->Token != NULL) {
    //
    // The previous call to TransmitReceive is not finished.
    //
    return EFI_NOT_READY;
  }

  if ((Token->Packet->Dhcp4.Magik != DHCP_OPTION_MAGIC)                   ||
      (NTOHL (Token->Packet->Dhcp4.Header.Xid) == Instance->Service->Xid) ||
      (Token->TimeoutValue == 0)                                          ||
      ((Token->ListenPointCount != 0) && (Token->ListenPoints == NULL))   ||
      EFI_ERROR (DhcpValidateOptions (Token->Packet, NULL))               ||
      EFI_IP4_EQUAL (&Token->RemoteAddress, &mZeroIp4Addr)
      ) {
    //
    // The DHCP packet isn't well-formed, the Transaction ID is already used,
    // the timeout value is zero, the ListenPoint is invalid, or the
    // RemoteAddress is zero.
    //
    return EFI_INVALID_PARAMETER;
  }

  ClientAddr = EFI_NTOHL (Token->Packet->Dhcp4.Header.ClientAddr);

  if (ClientAddr == 0) {
    return EFI_NO_MAPPING;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Save the token and the timeout value.
  //
  Instance->Token   = Token;
  Instance->Timeout = Token->TimeoutValue;

  //
  // Create a UDP IO for this transmit receive transaction.
  //
  Status = Dhcp4InstanceCreateUdpIo (Instance);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Save the Client Address is sent out
  //
  CopyMem (
    &DhcpSb->ClientAddressSendOut[0],
    &Token->Packet->Dhcp4.Header.ClientHwAddr[0],
    Token->Packet->Dhcp4.Header.HwAddrLen
    );

  //
  // Wrap the DHCP packet into a net buffer.
  //
  Frag.Bulk = (UINT8 *) &Token->Packet->Dhcp4;
  Frag.Len  = Token->Packet->Length;
  Wrap      = NetbufFromExt (&Frag, 1, 0, 0, DhcpDummyExtFree, NULL);
  if (Wrap == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Set the local address and local port to ZERO.
  //
  ZeroMem (&EndPoint, sizeof (UDP_END_POINT));

  //
  // Set the destination address and destination port.
  //
  CopyMem (&Ip, &Token->RemoteAddress, sizeof (EFI_IPv4_ADDRESS));
  EndPoint.RemoteAddr.Addr[0] = NTOHL (Ip);

  if (Token->RemotePort == 0) {
    EndPoint.RemotePort = DHCP_SERVER_PORT;
  } else {
    EndPoint.RemotePort = Token->RemotePort;
  }

  //
  // Get the gateway.
  //
  ZeroMem (&Gateway, sizeof (Gateway));
  if (!IP4_NET_EQUAL (ClientAddr, EndPoint.RemoteAddr.Addr[0], DhcpSb->Netmask)) {
    CopyMem (&Gateway.v4, &Token->GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
    Gateway.Addr[0] = NTOHL (Gateway.Addr[0]);
  }

  //
  // Transmit the DHCP packet.
  //
  Status = UdpIoSendDatagram (Instance->UdpIo, Wrap, &EndPoint, &Gateway, DhcpOnPacketSent, NULL);
  if (EFI_ERROR (Status)) {
    NetbufFree (Wrap);
    goto ON_ERROR;
  }

  //
  // Start to receive the DHCP response.
  //
  Status = UdpIoRecvDatagram (Instance->UdpIo, PxeDhcpInput, Instance, 0);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

ON_ERROR:

  if (EFI_ERROR (Status) && (Instance->UdpIo != NULL)) {
    UdpIoCleanIo (Instance->UdpIo);
    gBS->CloseProtocol (
           Instance->UdpIo->UdpHandle,
           &gEfiUdp4ProtocolGuid,
           Instance->Service->Image,
           Instance->Handle
           );
    UdpIoFreeIo (Instance->UdpIo);
    Instance->UdpIo = NULL;
    Instance->Token = NULL;
  }

  gBS->RestoreTPL (OldTpl);

  if (!EFI_ERROR (Status) && (Token->CompletionEvent == NULL)) {
    //
    // Keep polling until timeout if no error happens and the CompletionEvent
    // is NULL.
    //
    while (TRUE) {
      OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
      //
      // Raise TPL to protect the UDPIO in instance, in case that DhcpOnTimerTick
      // free it when timeout.
      //
      if (Instance->Timeout > 0) {
        Instance->UdpIo->Protocol.Udp4->Poll (Instance->UdpIo->Protocol.Udp4);
        gBS->RestoreTPL (OldTpl);
      } else {
        gBS->RestoreTPL (OldTpl);
        break;
      }
    }
  }

  return Status;
}


/**
  Callback function for DhcpIterateOptions. This callback sets the
  EFI_DHCP4_PACKET_OPTION array in the DHCP_PARSE_CONTEXT to point
  the individual DHCP option in the packet.

  @param[in]  Tag                    The DHCP option type
  @param[in]  Len                    Length of the DHCP option data
  @param[in]  Data                   The DHCP option data
  @param[in]  Context                The context, to pass several parameters in.

  @retval EFI_SUCCESS            It always returns EFI_SUCCESS

**/
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

  if (Parse->Index <= Parse->OptionCount) {
    //
    // Use BASE_CR to get the memory position of EFI_DHCP4_PACKET_OPTION for
    // the EFI_DHCP4_PACKET_OPTION->Data because DhcpIterateOptions only
    // pass in the point to option data.
    //
    Parse->Option[Parse->Index - 1] = BASE_CR (Data, EFI_DHCP4_PACKET_OPTION, Data);
  }

  return EFI_SUCCESS;
}


/**
  Parses the packed DHCP option data.

  The Parse() function is used to retrieve the option list from a DHCP packet.
  If *OptionCount isn't zero, and there is enough space for all the DHCP options
  in the Packet, each element of PacketOptionList is set to point to somewhere in
  the Packet->Dhcp4.Option where a new DHCP option begins. If RFC3396 is supported,
  the caller should reassemble the parsed DHCP options to get the finial result.
  If *OptionCount is zero or there isn't enough space for all of them, the number
  of DHCP options in the Packet is returned in OptionCount.

  @param  This             Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  Packet           Pointer to packet to be parsed.
  @param  OptionCount      On input, the number of entries in the PacketOptionList.
                           On output, the number of entries that were written into the
                           PacketOptionList.
  @param  PacketOptionList List of packet option entries to be filled in. End option or pad
                           options are not included.

  @retval EFI_SUCCESS           The packet was successfully parsed.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_BUFFER_TOO_SMALL  One or more of the following conditions is TRUE:
                                1) *OptionCount is smaller than the number of options that
                                were found in the Packet.
                                2) PacketOptionList is NULL.

**/
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

  ZeroMem (PacketOptionList, *OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *));

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

/**
  Set the elapsed time based on the given instance and the pointer to the
  elapsed time option.

  @param[in]      Elapsed       The pointer to the position to append.
  @param[in]      Instance      The pointer to the Dhcp4 instance.
**/
VOID
SetElapsedTime (
  IN     UINT16                 *Elapsed,
  IN     DHCP_PROTOCOL          *Instance
  )
{
  WriteUnaligned16 (Elapsed, HTONS(Instance->ElaspedTime));
}
