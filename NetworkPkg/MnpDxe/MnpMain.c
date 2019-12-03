/** @file
  Implementation of Managed Network Protocol public services.

Copyright (c) 2005 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MnpImpl.h"

/**
  Returns the operational parameters for the current MNP child driver. May also
  support returning the underlying SNP driver mode data.

  The GetModeData() function is used to read the current mode data (operational
  parameters) from the MNP or the underlying SNP.

  @param[in]  This          Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[out] MnpConfigData Pointer to storage for MNP operational parameters. Type
                            EFI_MANAGED_NETWORK_CONFIG_DATA is defined in "Related
                            Definitions" below.
  @param[out] SnpModeData   Pointer to storage for SNP operational parameters. This
                            feature may be unsupported. Type EFI_SIMPLE_NETWORK_MODE
                            is defined in the EFI_SIMPLE_NETWORK_PROTOCOL.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this
                                MNP implementation.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been
                                configured. The default values are returned in
                                MnpConfigData if it is not NULL.
  @retval Others                The mode data could not be read.

**/
EFI_STATUS
EFIAPI
MnpGetModeData (
  IN     EFI_MANAGED_NETWORK_PROTOCOL      *This,
     OUT EFI_MANAGED_NETWORK_CONFIG_DATA   *MnpConfigData OPTIONAL,
     OUT EFI_SIMPLE_NETWORK_MODE           *SnpModeData OPTIONAL
  )
{
  MNP_INSTANCE_DATA           *Instance;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_TPL                     OldTpl;
  EFI_STATUS                  Status;
  UINT32                      InterruptStatus;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (MnpConfigData != NULL) {
    //
    // Copy the instance configuration data.
    //
    CopyMem (MnpConfigData, &Instance->ConfigData, sizeof (*MnpConfigData));
  }

  if (SnpModeData != NULL) {
    //
    // Copy the underlayer Snp mode data.
    //
    Snp = Instance->MnpServiceData->MnpDeviceData->Snp;

    //
    // Upon successful return of GetStatus(), the Snp->Mode->MediaPresent
    // will be updated to reflect any change of media status
    //
    Snp->GetStatus (Snp, &InterruptStatus, NULL);
    CopyMem (SnpModeData, Snp->Mode, sizeof (*SnpModeData));
  }

  if (!Instance->Configured) {
    Status = EFI_NOT_STARTED;
  } else {
    Status = EFI_SUCCESS;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Sets or clears the operational parameters for the MNP child driver.

  The Configure() function is used to set, change, or reset the operational
  parameters for the MNP child driver instance. Until the operational parameters
  have been set, no network traffic can be sent or received by this MNP child
  driver instance. Once the operational parameters have been reset, no more
  traffic can be sent or received until the operational parameters have been set
  again.
  Each MNP child driver instance can be started and stopped independently of
  each other by setting or resetting their receive filter settings with the
  Configure() function.
  After any successful call to Configure(), the MNP child driver instance is
  started. The internal periodic timer (if supported) is enabled. Data can be
  transmitted and may be received if the receive filters have also been enabled.
  Note: If multiple MNP child driver instances will receive the same packet
  because of overlapping receive filter settings, then the first MNP child
  driver instance will receive the original packet and additional instances will
  receive copies of the original packet.
  Note: Warning: Receive filter settings that overlap will consume extra
  processor and/or DMA resources and degrade system and network performance.

  @param[in]  This           Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  MnpConfigData  Pointer to configuration data that will be assigned
                             to the MNP child driver instance. If NULL, the MNP
                             child driver instance is reset to startup defaults
                             and all pending transmit and receive requests are
                             flushed. Type EFI_MANAGED_NETWORK_CONFIG_DATA is
                             defined in EFI_MANAGED_NETWORK_PROTOCOL.GetModeData().

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is
                                 TRUE:
                                 * This is NULL.
                                 * MnpConfigData.ProtocolTypeFilter is not
                                   valid.
                                 The operational data for the MNP child driver
                                 instance is unchanged.
  @retval EFI_OUT_OF_RESOURCES   Required system resources (usually memory)
                                 could not be allocated.
                                 The MNP child driver instance has been reset to
                                 startup defaults.
  @retval EFI_UNSUPPORTED        The requested feature is unsupported in
                                 this [MNP] implementation. The operational data
                                 for the MNP child driver instance is unchanged.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error
                                 occurred. The MNP child driver instance has
                                 been reset to startup defaults.
  @retval Others                 The MNP child driver instance has been reset to
                                 startup defaults.

**/
EFI_STATUS
EFIAPI
MnpConfigure (
  IN EFI_MANAGED_NETWORK_PROTOCOL        *This,
  IN EFI_MANAGED_NETWORK_CONFIG_DATA     *MnpConfigData OPTIONAL
  )
{
  MNP_INSTANCE_DATA  *Instance;
  EFI_TPL            OldTpl;
  EFI_STATUS         Status;

  if ((This == NULL) ||
      ((MnpConfigData != NULL) &&
       (MnpConfigData->ProtocolTypeFilter > 0) &&
       (MnpConfigData->ProtocolTypeFilter <= 1500))
     ) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if ((MnpConfigData == NULL) && (!Instance->Configured)) {
    //
    // If the instance is not configured and a reset is requested, just return.
    //
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Configure the instance.
  //
  Status = MnpConfigureInstance (Instance, MnpConfigData);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Translates an IP multicast address to a hardware (MAC) multicast address. This
  function may be unsupported in some MNP implementations.

  The McastIpToMac() function translates an IP multicast address to a hardware
  (MAC) multicast address. This function may be implemented by calling the
  underlying EFI_SIMPLE_NETWORK. MCastIpToMac() function, which may also be
  unsupported in some MNP implementations.

  @param[in]  This        Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  Ipv6Flag    Set to TRUE to if IpAddress is an IPv6 multicast address.
                          Set to FALSE if IpAddress is an IPv4 multicast address.
  @param[in]  IpAddress   Pointer to the multicast IP address (in network byte
                          order) to convert.
  @param[out] MacAddress  Pointer to the resulting multicast MAC address.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER One of the following conditions is TRUE:
                                 * This is NULL.
                                 * IpAddress is NULL.
                                 * IpAddress is not a valid multicast IP
                                   address.
                                 * MacAddress is NULL.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been
                                configured.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this
                                MNP implementation.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred.
  @retval Others                The address could not be converted.
**/
EFI_STATUS
EFIAPI
MnpMcastIpToMac (
  IN     EFI_MANAGED_NETWORK_PROTOCOL    *This,
  IN     BOOLEAN                         Ipv6Flag,
  IN     EFI_IP_ADDRESS                  *IpAddress,
     OUT EFI_MAC_ADDRESS                 *MacAddress
  )
{
  EFI_STATUS                  Status;
  MNP_INSTANCE_DATA           *Instance;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_TPL                     OldTpl;
  EFI_IPv6_ADDRESS            *Ip6Address;

  if ((This == NULL) || (IpAddress == NULL) || (MacAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Ip6Address = &IpAddress->v6;

  if ((Ipv6Flag && !IP6_IS_MULTICAST (Ip6Address)) ||
      (!Ipv6Flag && !IP4_IS_MULTICAST (EFI_NTOHL (*IpAddress)))
      ) {
    //
    // The IP address passed in is not a multicast address.
    //
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (!Instance->Configured) {

    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  Snp = Instance->MnpServiceData->MnpDeviceData->Snp;
  ASSERT (Snp != NULL);

  ZeroMem (MacAddress, sizeof (EFI_MAC_ADDRESS));

  if (Snp->Mode->IfType == NET_IFTYPE_ETHERNET) {
    if (!Ipv6Flag) {
      //
      // Translate the IPv4 address into a multicast MAC address if the NIC is an
      // ethernet NIC according to RFC1112..
      //
      MacAddress->Addr[0] = 0x01;
      MacAddress->Addr[1] = 0x00;
      MacAddress->Addr[2] = 0x5E;
      MacAddress->Addr[3] = (UINT8) (IpAddress->v4.Addr[1] & 0x7F);
      MacAddress->Addr[4] = IpAddress->v4.Addr[2];
      MacAddress->Addr[5] = IpAddress->v4.Addr[3];
    } else {
      //
      // Translate the IPv6 address into a multicast MAC address if the NIC is an
      // ethernet NIC according to RFC2464.
      //

      MacAddress->Addr[0] = 0x33;
      MacAddress->Addr[1] = 0x33;
      MacAddress->Addr[2] = Ip6Address->Addr[12];
      MacAddress->Addr[3] = Ip6Address->Addr[13];
      MacAddress->Addr[4] = Ip6Address->Addr[14];
      MacAddress->Addr[5] = Ip6Address->Addr[15];
    }

    Status = EFI_SUCCESS;
  } else {
    //
    // Invoke Snp to translate the multicast IP address.
    //
    Status = Snp->MCastIpToMac (
                    Snp,
                    Ipv6Flag,
                    IpAddress,
                    MacAddress
                    );
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Enables and disables receive filters for multicast address. This function may
  be unsupported in some MNP implementations.

  The Groups() function only adds and removes multicast MAC addresses from the
  filter list. The MNP driver does not transmit or process Internet Group
  Management Protocol (IGMP) packets. If JoinFlag is FALSE and MacAddress is
  NULL, then all joined groups are left.

  @param[in]  This        Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  JoinFlag    Set to TRUE to join this multicast group.
                          Set to FALSE to leave this multicast group.
  @param[in]  MacAddress  Pointer to the multicast MAC group (address) to join or
                          leave.

  @retval EFI_SUCCESS           The requested operation completed successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                * This is NULL.
                                * JoinFlag is TRUE and MacAddress is NULL.
                                * MacAddress is not a valid multicast MAC
                                  address.
                                * The MNP multicast group settings are
                                  unchanged.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been
                                configured.
  @retval EFI_ALREADY_STARTED   The supplied multicast group is already joined.
  @retval EFI_NOT_FOUND         The supplied multicast group is not joined.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred.
                                The MNP child driver instance has been reset to
                                startup defaults.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this MNP
                                implementation.
  @retval Others                The requested operation could not be completed.
                                The MNP multicast group settings are unchanged.

**/
EFI_STATUS
EFIAPI
MnpGroups (
  IN EFI_MANAGED_NETWORK_PROTOCOL    *This,
  IN BOOLEAN                         JoinFlag,
  IN EFI_MAC_ADDRESS                 *MacAddress OPTIONAL
  )
{
  MNP_INSTANCE_DATA       *Instance;
  EFI_SIMPLE_NETWORK_MODE *SnpMode;
  MNP_GROUP_CONTROL_BLOCK *GroupCtrlBlk;
  MNP_GROUP_ADDRESS       *GroupAddress;
  LIST_ENTRY              *ListEntry;
  BOOLEAN                 AddressExist;
  EFI_TPL                 OldTpl;
  EFI_STATUS              Status;

  if (This == NULL || (JoinFlag && (MacAddress == NULL))) {
    //
    // This is NULL, or it's a join operation but MacAddress is NULL.
    //
    return EFI_INVALID_PARAMETER;
  }

  Instance  = MNP_INSTANCE_DATA_FROM_THIS (This);
  SnpMode   = Instance->MnpServiceData->MnpDeviceData->Snp->Mode;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (!Instance->Configured) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if ((!Instance->ConfigData.EnableMulticastReceive) ||
    ((MacAddress != NULL) && !NET_MAC_IS_MULTICAST (MacAddress, &SnpMode->BroadcastAddress, SnpMode->HwAddressSize))) {
    //
    // The instance isn't configured to do mulitcast receive. OR
    // the passed in MacAddress is not a mutlticast mac address.
    //
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Status       = EFI_SUCCESS;
  AddressExist = FALSE;
  GroupCtrlBlk = NULL;

  if (MacAddress != NULL) {
    //
    // Search the instance's GroupCtrlBlkList to find the specific address.
    //
    NET_LIST_FOR_EACH (ListEntry, &Instance->GroupCtrlBlkList) {

      GroupCtrlBlk = NET_LIST_USER_STRUCT (
                      ListEntry,
                      MNP_GROUP_CONTROL_BLOCK,
                      CtrlBlkEntry
                      );
      GroupAddress = GroupCtrlBlk->GroupAddress;
      if (0 == CompareMem (
                MacAddress,
                &GroupAddress->Address,
                SnpMode->HwAddressSize
                )) {
        //
        // There is already the same multicast mac address configured.
        //
        AddressExist = TRUE;
        break;
      }
    }

    if (JoinFlag && AddressExist) {
      //
      // The multicast mac address to join already exists.
      //
      Status = EFI_ALREADY_STARTED;
    }

    if (!JoinFlag && !AddressExist) {
      //
      // The multicast mac address to leave doesn't exist in this instance.
      //
      Status = EFI_NOT_FOUND;
    }

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  } else if (IsListEmpty (&Instance->GroupCtrlBlkList)) {
    //
    // The MacAddress is NULL and there is no configured multicast mac address,
    // just return.
    //
    goto ON_EXIT;
  }

  //
  // OK, it is time to take action.
  //
  Status = MnpGroupOp (Instance, JoinFlag, MacAddress, GroupCtrlBlk);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Places asynchronous outgoing data packets into the transmit queue.

  The Transmit() function places a completion token into the transmit packet
  queue. This function is always asynchronous.
  The caller must fill in the Token.Event and Token.TxData fields in the
  completion token, and these fields cannot be NULL. When the transmit operation
  completes, the MNP updates the Token.Status field and the Token.Event is
  signaled.
  Note: There may be a performance penalty if the packet needs to be
  defragmented before it can be transmitted by the network device. Systems in
  which performance is critical should review the requirements and features of
  the underlying communications device and drivers.


  @param[in]  This    Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  Token   Pointer to a token associated with the transmit data
                      descriptor. Type EFI_MANAGED_NETWORK_COMPLETION_TOKEN
                      is defined in "Related Definitions" below.

  @retval EFI_SUCCESS            The transmit completion token was cached.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is
                                 TRUE:
                                 * This is NULL.
                                 * Token is NULL.
                                 * Token.Event is NULL.
                                 * Token.TxData is NULL.
                                 * Token.TxData.DestinationAddress is not
                                   NULL and Token.TxData.HeaderLength is zero.
                                 * Token.TxData.FragmentCount is zero.
                                 * (Token.TxData.HeaderLength +
                                   Token.TxData.DataLength) is not equal to the
                                   sum of the
                                   Token.TxData.FragmentTable[].FragmentLength
                                   fields.
                                 * One or more of the
                                   Token.TxData.FragmentTable[].FragmentLength
                                   fields is zero.
                                 * One or more of the
                                   Token.TxData.FragmentTable[].FragmentBufferfields
                                   is NULL.
                                 * Token.TxData.DataLength is greater than MTU.
  @retval EFI_ACCESS_DENIED      The transmit completion token is already in the
                                 transmit queue.
  @retval EFI_OUT_OF_RESOURCES   The transmit data could not be queued due to a
                                 lack of system resources (usually memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The MNP child driver instance has been reset to
                                 startup defaults.
  @retval EFI_NOT_READY          The transmit request could not be queued because
                                 the transmit queue is full.

**/
EFI_STATUS
EFIAPI
MnpTransmit (
  IN EFI_MANAGED_NETWORK_PROTOCOL            *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN    *Token
  )
{
  EFI_STATUS        Status;
  MNP_INSTANCE_DATA *Instance;
  MNP_SERVICE_DATA  *MnpServiceData;
  UINT8             *PktBuf;
  UINT32            PktLen;
  EFI_TPL           OldTpl;

  if ((This == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (!Instance->Configured) {

    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (!MnpIsValidTxToken (Instance, Token)) {
    //
    // The Token is invalid.
    //
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  MnpServiceData = Instance->MnpServiceData;
  NET_CHECK_SIGNATURE (MnpServiceData, MNP_SERVICE_DATA_SIGNATURE);

  //
  // Build the tx packet
  //
  Status = MnpBuildTxPacket (MnpServiceData, Token->Packet.TxData, &PktBuf, &PktLen);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  //  OK, send the packet synchronously.
  //
  Status = MnpSyncSendPacket (MnpServiceData, PktBuf, PktLen, Token);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Places an asynchronous receiving request into the receiving queue.

  The Receive() function places a completion token into the receive packet
  queue. This function is always asynchronous.
  The caller must fill in the Token.Event field in the completion token, and
  this field cannot be NULL. When the receive operation completes, the MNP
  updates the Token.Status and Token.RxData fields and the Token.Event is
  signaled.

  @param[in]  This      Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  Token     Pointer to a token associated with the receive
                        data descriptor. Type
                        EFI_MANAGED_NETWORK_COMPLETION_TOKEN is defined in
                        EFI_MANAGED_NETWORK_PROTOCOL.Transmit().

  @retval EFI_SUCCESS            The receive completion token was cached.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is
                                 TRUE:
                                 * This is NULL.
                                 * Token is NULL.
                                 * Token.Event is NULL
  @retval EFI_OUT_OF_RESOURCES   The transmit data could not be queued due to a
                                 lack of system resources (usually memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The MNP child driver instance has been reset to
                                 startup defaults.
  @retval EFI_ACCESS_DENIED      The receive completion token was already in the
                                 receive queue.
  @retval EFI_NOT_READY          The receive request could not be queued because
                                 the receive queue is full.

**/
EFI_STATUS
EFIAPI
MnpReceive (
  IN EFI_MANAGED_NETWORK_PROTOCOL            *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN    *Token
  )
{
  EFI_STATUS         Status;
  MNP_INSTANCE_DATA  *Instance;
  EFI_TPL            OldTpl;

  if ((This == NULL) || (Token == NULL) || (Token->Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (!Instance->Configured) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  //
  // Check whether this token(event) is already in the rx token queue.
  //
  Status = NetMapIterate (&Instance->RxTokenMap, MnpTokenExist, (VOID *) Token);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Insert the Token into the RxTokenMap.
  //
  Status = NetMapInsertTail (&Instance->RxTokenMap, (VOID *) Token, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // Try to deliver any buffered packets.
    //
    Status = MnpInstanceDeliverPacket (Instance);

    //
    // Dispatch the DPC queued by the NotifyFunction of Token->Event.
    //
    DispatchDpc ();
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Aborts an asynchronous transmit or receive request.

  The Cancel() function is used to abort a pending transmit or receive request.
  If the token is in the transmit or receive request queues, after calling this
  function, Token.Status will be set to EFI_ABORTED and then Token.Event will be
  signaled. If the token is not in one of the queues, which usually means that
  the asynchronous operation has completed, this function will not signal the
  token and EFI_NOT_FOUND is returned.

  @param[in]  This     Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  Token    Pointer to a token that has been issued by
                       EFI_MANAGED_NETWORK_PROTOCOL.Transmit() or
                       EFI_MANAGED_NETWORK_PROTOCOL.Receive(). If NULL, all
                       pending tokens are aborted.

  @retval EFI_SUCCESS            The asynchronous I/O request was aborted and
                                 Token.Event was signaled. When Token is NULL,
                                 all pending requests were aborted and their
                                 events were signaled.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_FOUND          When Token is not NULL, the asynchronous I/O
                                 request was not found in the transmit or
                                 receive queue. It has either completed or was
                                 not issued by Transmit() and Receive().

**/
EFI_STATUS
EFIAPI
MnpCancel (
  IN EFI_MANAGED_NETWORK_PROTOCOL            *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN    *Token OPTIONAL
  )
{
  EFI_STATUS         Status;
  MNP_INSTANCE_DATA  *Instance;
  EFI_TPL            OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (!Instance->Configured) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  //
  // Iterate the RxTokenMap to cancel the specified Token.
  //
  Status = NetMapIterate (&Instance->RxTokenMap, MnpCancelTokens, (VOID *) Token);
  if (Token != NULL) {
    Status = (Status == EFI_ABORTED) ? EFI_SUCCESS : EFI_NOT_FOUND;
  }

  //
  // Dispatch the DPC queued by the NotifyFunction of the cancled token's events.
  //
  DispatchDpc ();

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to
  increase the rate that data packets are moved between the communications
  device and the transmit and receive queues.
  Normally, a periodic timer event internally calls the Poll() function. But, in
  some systems, the periodic timer event may not call Poll() fast enough to
  transmit and/or receive all data packets without missing packets. Drivers and
  applications that are experiencing packet loss should try calling the Poll()
  function more often.

  @param[in]  This         Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.

  @retval EFI_SUCCESS      Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED  This MNP child driver instance has not been
                           configured.
  @retval EFI_DEVICE_ERROR An unexpected system or network error occurred. The
                           MNP child driver instance has been reset to startup
                           defaults.
  @retval EFI_NOT_READY    No incoming or outgoing data was processed. Consider
                           increasing the polling rate.
  @retval EFI_TIMEOUT      Data was dropped out of the transmit and/or receive
                           queue. Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
MnpPoll (
  IN EFI_MANAGED_NETWORK_PROTOCOL    *This
  )
{
  EFI_STATUS         Status;
  MNP_INSTANCE_DATA  *Instance;
  EFI_TPL            OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (!Instance->Configured) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  //
  // Try to receive packets.
  //
  Status = MnpReceivePacket (Instance->MnpServiceData->MnpDeviceData);

  //
  // Dispatch the DPC queued by the NotifyFunction of rx token's events.
  //
  DispatchDpc ();

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
