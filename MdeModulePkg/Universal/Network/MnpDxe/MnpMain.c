/** @file

Copyright (c) 2005 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MnpMain.c

Abstract:

  Implementation of Managed Network Protocol public services.


**/

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>

#include "MnpImpl.h"


/**
  Get configuration data of this instance.

  @param  This                   Pointer to the Managed Network Protocol.
  @param  MnpConfigData          Pointer to strorage for MNP operational
                                 parameters.
  @param  SnpModeData            Pointer to strorage for SNP operational
                                 parameters.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured The default values are returned in
                                 MnpConfigData if it is not NULL.

**/
EFI_STATUS
EFIAPI
MnpGetModeData (
  IN  EFI_MANAGED_NETWORK_PROTOCOL     *This,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData OPTIONAL
  )
{
  MNP_INSTANCE_DATA           *Instance;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_TPL                     OldTpl;
  EFI_STATUS                  Status;

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
    Snp           = Instance->MnpServiceData->Snp;
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
  Set or clear the operational parameters for the MNP child driver.

  @param  This                   Pointer to the Managed Network Protocol.
  @param  MnpConfigData          Pointer to the configuration data that will be
                                 assigned to the MNP child driver instance. If
                                 NULL, the MNP child driver instance is reset to
                                 startup defaults and all pending transmit and
                                 receive requests are flushed.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Required system resources (usually memory) could
                                 not be allocated.
  @retval EFI_UNSUPPORTED        EnableReceiveTimestamps is TRUE, this
                                 implementation doesn't support it.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Other                  The MNP child driver instance has been reset to
                                 startup defaults.

**/
EFI_STATUS
EFIAPI
MnpConfigure (
  IN EFI_MANAGED_NETWORK_PROTOCOL     *This,
  IN EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData OPTIONAL
  )
{
  MNP_INSTANCE_DATA  *Instance;
  EFI_TPL            OldTpl;
  EFI_STATUS         Status;

  if ((This == NULL) ||
    ((MnpConfigData != NULL) &&
    (MnpConfigData->ProtocolTypeFilter > 0) &&
    (MnpConfigData->ProtocolTypeFilter <= 1500))) {

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
  Translate a multicast IP address to a multicast hardware (MAC) address.

  @param  This                   Pointer to the Managed Network Protocol.
  @param  Ipv6Flag               Set to TRUE if IpAddress is an IPv6 multicast
                                 address. Set to FALSE if IpAddress is an IPv4
                                 multicast address.
  @param  IpAddress              Pointer to the multicast IP address to convert.
  @param  MacAddress             Pointer to the resulting multicast MAC address.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more parameter is invalid.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_UNSUPPORTED        Ipv6Flag is TRUE, this implementation doesn't
                                 supported it.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Other                  The address could not be converted.

**/
EFI_STATUS
EFIAPI
MnpMcastIpToMac (
  IN  EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN  BOOLEAN                       Ipv6Flag,
  IN  EFI_IP_ADDRESS                *IpAddress,
  OUT EFI_MAC_ADDRESS               *MacAddress
  )
{
  EFI_STATUS                  Status;
  MNP_INSTANCE_DATA           *Instance;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_TPL                     OldTpl;

  if ((This == NULL) || (IpAddress == NULL) || (MacAddress == NULL)) {

    return EFI_INVALID_PARAMETER;
  }

  if (Ipv6Flag) {
    //
    // Currently IPv6 isn't supported.
    //
    return EFI_UNSUPPORTED;
  }

  if (!IP4_IS_MULTICAST (EFI_NTOHL (*IpAddress))) {
    //
    // The IPv4 address passed in is not a multicast address.
    //
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (!Instance->Configured) {

    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  Snp = Instance->MnpServiceData->Snp;
  ASSERT (Snp != NULL);

  if (Snp->Mode->IfType == NET_IFTYPE_ETHERNET) {
    //
    // Translate the IPv4 address into a multicast MAC address if the NIC is an
    // ethernet NIC.
    //
    MacAddress->Addr[0] = 0x01;
    MacAddress->Addr[1] = 0x00;
    MacAddress->Addr[2] = 0x5E;
    MacAddress->Addr[3] = (UINT8) (IpAddress->v4.Addr[1] & 0x7F);
    MacAddress->Addr[4] = IpAddress->v4.Addr[2];
    MacAddress->Addr[5] = IpAddress->v4.Addr[3];

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
  Enable or disable receie filters for multicast address.

  @param  This                   Pointer to the Managed Network Protocol.
  @param  JoinFlag               Set to TRUE to join this multicast group. Set to
                                 FALSE to leave this multicast group.
  @param  MacAddress             Pointer to the multicast MAC group (address) to
                                 join or leave.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more parameter is invalid
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_ALREADY_STARTED    The supplied multicast group is already joined.
  @retval EFI_NOT_FOUND          The supplied multicast group is not joined.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Other                  The requested operation could not be completed.
                                 The MNP multicast group settings are unchanged.

**/
EFI_STATUS
EFIAPI
MnpGroups (
  IN EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                       JoinFlag,
  IN EFI_MAC_ADDRESS               *MacAddress OPTIONAL
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
  SnpMode   = Instance->MnpServiceData->Snp->Mode;

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
  Place an outgoing packet into the transmit queue.

  @param  This                   Pointer to the Managed Network Protocol.
  @param  Token                  Pointer to a token associated with the transmit
                                 data descriptor.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more parameter is invalid
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
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
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
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
  MnpBuildTxPacket (MnpServiceData, Token->Packet.TxData, &PktBuf, &PktLen);

  //
  //  OK, send the packet synchronously.
  //
  Status = MnpSyncSendPacket (MnpServiceData, PktBuf, PktLen, Token);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Place an asynchronous receiving request into the receiving queue.

  @param  This                   Pointer to the EFI_MANAGED_NETWORK_PROTOCOL
                                 instance.
  @param  Token                  Pointer to a token associated with the receive
                                 data descriptor.

  @retval EFI_SUCCESS            The receive completion token was cached.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_INVALID_PARAMETER  One or more parameter is invalid.
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
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
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
    NetLibDispatchDpc ();
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Abort a pending transmit or receive request.

  @param  This                   Pointer to the EFI_MANAGED_NETWORK_PROTOCOL
                                 instance.
  @param  Token                  Pointer to a token that has been issued by
                                 EFI_MANAGED_NETWORK_PROTOCOL.Transmit() or
                                 EFI_MANAGED_NETWORK_PROTOCOL.Receive(). If NULL,
                                 all pending tokens are aborted.

  @retval EFI_SUCCESS            The asynchronous I/O request was aborted and
                                 Token->Event was signaled.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_FOUND          The asynchronous I/O request was not found in the
                                 transmit or receive queue. It has either completed
                                 or was not issued by Transmit() and Receive().

**/
EFI_STATUS
EFIAPI
MnpCancel (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token OPTIONAL
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
  NetLibDispatchDpc ();

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Poll the network interface to do transmit/receive work.

  @param  This                   Pointer to the EFI_MANAGED_NETWORK_PROTOCOL
                                 instance.

  @retval EFI_SUCCESS            Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The MNP child driver instance has been reset to
                                 startup defaults.
  @retval EFI_NOT_READY          No incoming or outgoing data was processed.
  @retval EFI_TIMEOUT            Data was dropped out of the transmit and/or
                                 receive queue.

**/
EFI_STATUS
EFIAPI
MnpPoll (
  IN EFI_MANAGED_NETWORK_PROTOCOL  *This
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
  Status = MnpReceivePacket (Instance->MnpServiceData);

  NetLibDispatchDpc ();

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

