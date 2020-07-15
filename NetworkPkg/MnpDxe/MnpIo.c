/** @file
  Implementation of Managed Network Protocol I/O functions.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MnpImpl.h"
#include "MnpVlan.h"

/**
  Validates the Mnp transmit token.

  @param[in]  Instance            Pointer to the Mnp instance context data.
  @param[in]  Token               Pointer to the transmit token to check.

  @return The Token is valid or not.

**/
BOOLEAN
MnpIsValidTxToken (
  IN MNP_INSTANCE_DATA                       *Instance,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN    *Token
  )
{
  MNP_SERVICE_DATA                  *MnpServiceData;
  EFI_MANAGED_NETWORK_TRANSMIT_DATA *TxData;
  UINT32                            Index;
  UINT32                            TotalLength;
  EFI_MANAGED_NETWORK_FRAGMENT_DATA *FragmentTable;

  MnpServiceData = Instance->MnpServiceData;
  NET_CHECK_SIGNATURE (MnpServiceData, MNP_SERVICE_DATA_SIGNATURE);

  TxData = Token->Packet.TxData;

  if ((Token->Event == NULL) || (TxData == NULL) || (TxData->FragmentCount == 0)) {
    //
    // The token is invalid if the Event is NULL, or the TxData is NULL, or
    // the fragment count is zero.
    //
    DEBUG ((EFI_D_WARN, "MnpIsValidTxToken: Invalid Token.\n"));
    return FALSE;
  }

  if ((TxData->DestinationAddress != NULL) && (TxData->HeaderLength != 0)) {
    //
    // The token is invalid if the HeaderLength isn't zero while the DestinationAddress
    // is NULL (The destination address is already put into the packet).
    //
    DEBUG ((EFI_D_WARN, "MnpIsValidTxToken: DestinationAddress isn't NULL, HeaderLength must be 0.\n"));
    return FALSE;
  }

  TotalLength   = 0;
  FragmentTable = TxData->FragmentTable;
  for (Index = 0; Index < TxData->FragmentCount; Index++) {

    if ((FragmentTable[Index].FragmentLength == 0) || (FragmentTable[Index].FragmentBuffer == NULL)) {
      //
      // The token is invalid if any FragmentLength is zero or any FragmentBuffer is NULL.
      //
      DEBUG ((EFI_D_WARN, "MnpIsValidTxToken: Invalid FragmentLength or FragmentBuffer.\n"));
      return FALSE;
    }

    TotalLength += FragmentTable[Index].FragmentLength;
  }

  if ((TxData->DestinationAddress == NULL) && (FragmentTable[0].FragmentLength < TxData->HeaderLength)) {
    //
    // Media header is split between fragments.
    //
    return FALSE;
  }

  if (TotalLength != (TxData->DataLength + TxData->HeaderLength)) {
    //
    // The length calculated from the fragment information doesn't equal to the
    // sum of the DataLength and the HeaderLength.
    //
    DEBUG ((EFI_D_WARN, "MnpIsValidTxData: Invalid Datalength compared with the sum of fragment length.\n"));
    return FALSE;
  }

  if (TxData->DataLength > MnpServiceData->Mtu) {
    //
    // The total length is larger than the MTU.
    //
    DEBUG ((EFI_D_WARN, "MnpIsValidTxData: TxData->DataLength exceeds Mtu.\n"));
    return FALSE;
  }

  return TRUE;
}

/**
  Build the packet to transmit from the TxData passed in.

  @param[in]   MnpServiceData      Pointer to the mnp service context data.
  @param[in]   TxData              Pointer to the transmit data containing the information
                                   to build the packet.
  @param[out]  PktBuf              Pointer to record the address of the packet.
  @param[out]  PktLen              Pointer to a UINT32 variable used to record the packet's
                                   length.

  @retval EFI_SUCCESS           TxPackage is built.
  @retval EFI_OUT_OF_RESOURCES  The deliver fails due to lack of memory resource.

**/
EFI_STATUS
MnpBuildTxPacket (
  IN     MNP_SERVICE_DATA                    *MnpServiceData,
  IN     EFI_MANAGED_NETWORK_TRANSMIT_DATA   *TxData,
     OUT UINT8                               **PktBuf,
     OUT UINT32                              *PktLen
  )
{
  EFI_SIMPLE_NETWORK_MODE *SnpMode;
  UINT8                   *DstPos;
  UINT16                  Index;
  MNP_DEVICE_DATA         *MnpDeviceData;
  UINT8                   *TxBuf;

  MnpDeviceData = MnpServiceData->MnpDeviceData;

  TxBuf = MnpAllocTxBuf (MnpDeviceData);
  if (TxBuf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Reserve space for vlan tag if needed.
  //
  if (MnpServiceData->VlanId != 0) {
    *PktBuf = TxBuf + NET_VLAN_TAG_LEN;
  } else {
    *PktBuf = TxBuf;
  }

  if ((TxData->DestinationAddress == NULL) && (TxData->FragmentCount == 1)) {
    CopyMem (
        *PktBuf,
        TxData->FragmentTable[0].FragmentBuffer,
        TxData->FragmentTable[0].FragmentLength
        );

    *PktLen = TxData->FragmentTable[0].FragmentLength;
  } else {
    //
    // Either media header isn't in FragmentTable or there is more than
    // one fragment, copy the data into the packet buffer. Reserve the
    // media header space if necessary.
    //
    SnpMode = MnpDeviceData->Snp->Mode;
    DstPos  = *PktBuf;
    *PktLen = 0;
    if (TxData->DestinationAddress != NULL) {
      //
      // If dest address is not NULL, move DstPos to reserve space for the
      // media header. Add the media header length to buflen.
      //
      DstPos += SnpMode->MediaHeaderSize;
      *PktLen += SnpMode->MediaHeaderSize;
    }

    for (Index = 0; Index < TxData->FragmentCount; Index++) {
      //
      // Copy the data.
      //
      CopyMem (
        DstPos,
        TxData->FragmentTable[Index].FragmentBuffer,
        TxData->FragmentTable[Index].FragmentLength
        );
      DstPos += TxData->FragmentTable[Index].FragmentLength;
    }

    //
    // Set the buffer length.
    //
    *PktLen += TxData->DataLength + TxData->HeaderLength;
  }

  return EFI_SUCCESS;
}


/**
  Synchronously send out the packet.

  This function places the packet buffer to SNP driver's tansmit queue. The packet
  can be considered successfully sent out once SNP accept the packet, while the
  packet buffer recycle is deferred for better performance.

  @param[in]       MnpServiceData      Pointer to the mnp service context data.
  @param[in]       Packet              Pointer to the packet buffer.
  @param[in]       Length              The length of the packet.
  @param[in, out]  Token               Pointer to the token the packet generated from.

  @retval EFI_SUCCESS                  The packet is sent out.
  @retval EFI_TIMEOUT                  Time out occurs, the packet isn't sent.
  @retval EFI_DEVICE_ERROR             An unexpected network error occurs.

**/
EFI_STATUS
MnpSyncSendPacket (
  IN     MNP_SERVICE_DATA                        *MnpServiceData,
  IN     UINT8                                   *Packet,
  IN     UINT32                                  Length,
  IN OUT EFI_MANAGED_NETWORK_COMPLETION_TOKEN    *Token
  )
{
  EFI_STATUS                        Status;
  EFI_SIMPLE_NETWORK_PROTOCOL       *Snp;
  EFI_MANAGED_NETWORK_TRANSMIT_DATA *TxData;
  UINT32                            HeaderSize;
  MNP_DEVICE_DATA                   *MnpDeviceData;
  UINT16                            ProtocolType;

  MnpDeviceData = MnpServiceData->MnpDeviceData;
  Snp           = MnpDeviceData->Snp;
  TxData        = Token->Packet.TxData;
  Token->Status = EFI_SUCCESS;
  HeaderSize    = Snp->Mode->MediaHeaderSize - TxData->HeaderLength;

  //
  // Check media status before transmit packet.
  // Note: media status will be updated by periodic timer MediaDetectTimer.
  //
  if (Snp->Mode->MediaPresentSupported && !Snp->Mode->MediaPresent) {
    //
    // Media not present, skip packet transmit and report EFI_NO_MEDIA
    //
    DEBUG ((EFI_D_WARN, "MnpSyncSendPacket: No network cable detected.\n"));
    Token->Status = EFI_NO_MEDIA;
    goto SIGNAL_TOKEN;
  }


  if (MnpServiceData->VlanId != 0) {
    //
    // Insert VLAN tag
    //
    MnpInsertVlanTag (MnpServiceData, TxData, &ProtocolType, &Packet, &Length);
  } else {
    ProtocolType = TxData->ProtocolType;
  }

  //
  // Transmit the packet through SNP.
  //
  Status = Snp->Transmit (
                  Snp,
                  HeaderSize,
                  Length,
                  Packet,
                  TxData->SourceAddress,
                  TxData->DestinationAddress,
                  &ProtocolType
                  );
  if (Status == EFI_NOT_READY) {
    Status = MnpRecycleTxBuf (MnpDeviceData);
    if (EFI_ERROR (Status)) {
      Token->Status = EFI_DEVICE_ERROR;
      goto SIGNAL_TOKEN;
    }

    Status = Snp->Transmit (
                    Snp,
                    HeaderSize,
                    Length,
                    Packet,
                    TxData->SourceAddress,
                    TxData->DestinationAddress,
                    &ProtocolType
                    );
  }

  if (EFI_ERROR (Status)) {
    Token->Status = EFI_DEVICE_ERROR;
  }

SIGNAL_TOKEN:

  gBS->SignalEvent (Token->Event);

  //
  // Dispatch the DPC queued by the NotifyFunction of Token->Event.
  //
  DispatchDpc ();

  return EFI_SUCCESS;
}


/**
  Try to deliver the received packet to the instance.

  @param[in, out]  Instance     Pointer to the mnp instance context data.

  @retval EFI_SUCCESS           The received packet is delivered, or there is no
                                packet to deliver, or there is no available receive
                                token.
  @retval EFI_OUT_OF_RESOURCES  The deliver fails due to lack of memory resource.

**/
EFI_STATUS
MnpInstanceDeliverPacket (
  IN OUT MNP_INSTANCE_DATA   *Instance
  )
{
  MNP_DEVICE_DATA                       *MnpDeviceData;
  MNP_RXDATA_WRAP                       *RxDataWrap;
  NET_BUF                               *DupNbuf;
  EFI_MANAGED_NETWORK_RECEIVE_DATA      *RxData;
  EFI_SIMPLE_NETWORK_MODE               *SnpMode;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *RxToken;

  MnpDeviceData = Instance->MnpServiceData->MnpDeviceData;
  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  if (NetMapIsEmpty (&Instance->RxTokenMap) || IsListEmpty (&Instance->RcvdPacketQueue)) {
    //
    // No pending received data or no available receive token, return.
    //
    return EFI_SUCCESS;
  }

  ASSERT (Instance->RcvdPacketQueueSize != 0);

  RxDataWrap = NET_LIST_HEAD (&Instance->RcvdPacketQueue, MNP_RXDATA_WRAP, WrapEntry);
  if (RxDataWrap->Nbuf->RefCnt > 2) {
    //
    // There are other instances share this Nbuf, duplicate to get a
    // copy to allow the instance to do R/W operations.
    //
    DupNbuf = MnpAllocNbuf (MnpDeviceData);
    if (DupNbuf == NULL) {
      DEBUG ((EFI_D_WARN, "MnpDeliverPacket: Failed to allocate a free Nbuf.\n"));

      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Duplicate the net buffer.
    //
    NetbufDuplicate (RxDataWrap->Nbuf, DupNbuf, 0);
    MnpFreeNbuf (MnpDeviceData, RxDataWrap->Nbuf);
    RxDataWrap->Nbuf = DupNbuf;
  }

  //
  // All resources are OK, remove the packet from the queue.
  //
  NetListRemoveHead (&Instance->RcvdPacketQueue);
  Instance->RcvdPacketQueueSize--;

  RxData  = &RxDataWrap->RxData;
  SnpMode = MnpDeviceData->Snp->Mode;

  //
  // Set all the buffer pointers.
  //
  RxData->MediaHeader         = NetbufGetByte (RxDataWrap->Nbuf, 0, NULL);
  RxData->DestinationAddress  = RxData->MediaHeader;
  RxData->SourceAddress       = (UINT8 *) RxData->MediaHeader + SnpMode->HwAddressSize;
  RxData->PacketData          = (UINT8 *) RxData->MediaHeader + SnpMode->MediaHeaderSize;

  //
  // Insert this RxDataWrap into the delivered queue.
  //
  InsertTailList (&Instance->RxDeliveredPacketQueue, &RxDataWrap->WrapEntry);

  //
  // Get the receive token from the RxTokenMap.
  //
  RxToken = NetMapRemoveHead (&Instance->RxTokenMap, NULL);

  //
  // Signal this token's event.
  //
  RxToken->Packet.RxData  = &RxDataWrap->RxData;
  RxToken->Status         = EFI_SUCCESS;
  gBS->SignalEvent (RxToken->Event);

  return EFI_SUCCESS;
}


/**
  Deliver the received packet for the instances belonging to the MnpServiceData.

  @param[in]  MnpServiceData        Pointer to the mnp service context data.

**/
VOID
MnpDeliverPacket (
  IN MNP_SERVICE_DATA    *MnpServiceData
  )
{
  LIST_ENTRY        *Entry;
  MNP_INSTANCE_DATA *Instance;

  NET_CHECK_SIGNATURE (MnpServiceData, MNP_SERVICE_DATA_SIGNATURE);

  NET_LIST_FOR_EACH (Entry, &MnpServiceData->ChildrenList) {
    Instance = NET_LIST_USER_STRUCT (Entry, MNP_INSTANCE_DATA, InstEntry);
    NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

    //
    // Try to deliver packet for this instance.
    //
    MnpInstanceDeliverPacket (Instance);
  }
}


/**
  Recycle the RxData and other resources used to hold and deliver the received
  packet.

  @param[in]  Event               The event this notify function registered to.
  @param[in]  Context             Pointer to the context data registered to the Event.

**/
VOID
EFIAPI
MnpRecycleRxData (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  MNP_RXDATA_WRAP *RxDataWrap;
  MNP_DEVICE_DATA *MnpDeviceData;

  ASSERT (Context != NULL);

  RxDataWrap = (MNP_RXDATA_WRAP *) Context;
  NET_CHECK_SIGNATURE (RxDataWrap->Instance, MNP_INSTANCE_DATA_SIGNATURE);

  ASSERT (RxDataWrap->Nbuf != NULL);

  MnpDeviceData = RxDataWrap->Instance->MnpServiceData->MnpDeviceData;
  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  //
  // Free this Nbuf.
  //
  MnpFreeNbuf (MnpDeviceData, RxDataWrap->Nbuf);
  RxDataWrap->Nbuf = NULL;

  //
  // Close the recycle event.
  //
  gBS->CloseEvent (RxDataWrap->RxData.RecycleEvent);

  //
  // Remove this Wrap entry from the list.
  //
  RemoveEntryList (&RxDataWrap->WrapEntry);

  FreePool (RxDataWrap);
}


/**
  Queue the received packet into instance's receive queue.

  @param[in, out]  Instance        Pointer to the mnp instance context data.
  @param[in, out]  RxDataWrap      Pointer to the Wrap structure containing the
                                   received data and other information.
**/
VOID
MnpQueueRcvdPacket (
  IN OUT MNP_INSTANCE_DATA   *Instance,
  IN OUT MNP_RXDATA_WRAP     *RxDataWrap
  )
{
  MNP_RXDATA_WRAP *OldRxDataWrap;

  NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

  //
  // Check the queue size. If it exceeds the limit, drop one packet
  // from the head.
  //
  if (Instance->RcvdPacketQueueSize == MNP_MAX_RCVD_PACKET_QUE_SIZE) {

    DEBUG ((EFI_D_WARN, "MnpQueueRcvdPacket: Drop one packet bcz queue size limit reached.\n"));

    //
    // Get the oldest packet.
    //
    OldRxDataWrap = NET_LIST_HEAD (
                      &Instance->RcvdPacketQueue,
                      MNP_RXDATA_WRAP,
                      WrapEntry
                      );

    //
    // Recycle this OldRxDataWrap, this entry will be removed by the callee.
    //
    MnpRecycleRxData (NULL, (VOID *) OldRxDataWrap);
    Instance->RcvdPacketQueueSize--;
  }

  //
  // Update the timeout tick using the configured parameter.
  //
  RxDataWrap->TimeoutTick = Instance->ConfigData.ReceivedQueueTimeoutValue;

  //
  // Insert this Wrap into the instance queue.
  //
  InsertTailList (&Instance->RcvdPacketQueue, &RxDataWrap->WrapEntry);
  Instance->RcvdPacketQueueSize++;
}


/**
  Match the received packet with the instance receive filters.

  @param[in]  Instance          Pointer to the mnp instance context data.
  @param[in]  RxData            Pointer to the EFI_MANAGED_NETWORK_RECEIVE_DATA.
  @param[in]  GroupAddress      Pointer to the GroupAddress, the GroupAddress is
                                non-NULL and it contains the destination multicast
                                mac address of the received packet if the packet
                                destinated to a multicast mac address.
  @param[in]  PktAttr           The received packets attribute.

  @return The received packet matches the instance's receive filters or not.

**/
BOOLEAN
MnpMatchPacket (
  IN MNP_INSTANCE_DATA                   *Instance,
  IN EFI_MANAGED_NETWORK_RECEIVE_DATA    *RxData,
  IN MNP_GROUP_ADDRESS                   *GroupAddress OPTIONAL,
  IN UINT8                               PktAttr
  )
{
  EFI_MANAGED_NETWORK_CONFIG_DATA *ConfigData;
  LIST_ENTRY                      *Entry;
  MNP_GROUP_CONTROL_BLOCK         *GroupCtrlBlk;

  NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

  ConfigData = &Instance->ConfigData;

  //
  // Check the protocol type.
  //
  if ((ConfigData->ProtocolTypeFilter != 0) && (ConfigData->ProtocolTypeFilter != RxData->ProtocolType)) {
    return FALSE;
  }

  if (ConfigData->EnablePromiscuousReceive) {
    //
    // Always match if this instance is configured to be promiscuous.
    //
    return TRUE;
  }

  //
  // The protocol type is matched, check receive filter, include unicast and broadcast.
  //
  if ((Instance->ReceiveFilter & PktAttr) != 0) {
    return TRUE;
  }

  //
  // Check multicast addresses.
  //
  if (ConfigData->EnableMulticastReceive && RxData->MulticastFlag) {

    ASSERT (GroupAddress != NULL);

    NET_LIST_FOR_EACH (Entry, &Instance->GroupCtrlBlkList) {

      GroupCtrlBlk = NET_LIST_USER_STRUCT (Entry, MNP_GROUP_CONTROL_BLOCK, CtrlBlkEntry);
      if (GroupCtrlBlk->GroupAddress == GroupAddress) {
        //
        // The instance is configured to receiveing packets destinated to this
        // multicast address.
        //
        return TRUE;
      }
    }
  }

  //
  // No match.
  //
  return FALSE;
}


/**
  Analyse the received packets.

  @param[in]       MnpServiceData    Pointer to the mnp service context data.
  @param[in]       Nbuf              Pointer to the net buffer holding the received
                                     packet.
  @param[in, out]  RxData            Pointer to the buffer used to save the analysed
                                     result in EFI_MANAGED_NETWORK_RECEIVE_DATA.
  @param[out]      GroupAddress      Pointer to pointer to a MNP_GROUP_ADDRESS used to
                                     pass out the address of the multicast address the
                                     received packet destinated to.
  @param[out]      PktAttr           Pointer to the buffer used to save the analysed
                                     packet attribute.

**/
VOID
MnpAnalysePacket (
  IN     MNP_SERVICE_DATA                    *MnpServiceData,
  IN     NET_BUF                             *Nbuf,
  IN OUT EFI_MANAGED_NETWORK_RECEIVE_DATA    *RxData,
     OUT MNP_GROUP_ADDRESS                   **GroupAddress,
     OUT UINT8                               *PktAttr
  )
{
  EFI_SIMPLE_NETWORK_MODE *SnpMode;
  MNP_DEVICE_DATA         *MnpDeviceData;
  UINT8                   *BufPtr;
  LIST_ENTRY              *Entry;

  MnpDeviceData = MnpServiceData->MnpDeviceData;
  SnpMode       = MnpDeviceData->Snp->Mode;

  //
  // Get the packet buffer.
  //
  BufPtr = NetbufGetByte (Nbuf, 0, NULL);
  ASSERT (BufPtr != NULL);

  //
  // Set the initial values.
  //
  RxData->BroadcastFlag   = FALSE;
  RxData->MulticastFlag   = FALSE;
  RxData->PromiscuousFlag = FALSE;
  *PktAttr                = UNICAST_PACKET;

  if (!NET_MAC_EQUAL (&SnpMode->CurrentAddress, BufPtr, SnpMode->HwAddressSize)) {
    //
    // This packet isn't destinated to our current mac address, it't not unicast.
    //
    *PktAttr = 0;

    if (NET_MAC_EQUAL (&SnpMode->BroadcastAddress, BufPtr, SnpMode->HwAddressSize)) {
      //
      // It's broadcast.
      //
      RxData->BroadcastFlag = TRUE;
      *PktAttr              = BROADCAST_PACKET;
    } else if ((*BufPtr & 0x01) == 0x1) {
      //
      // It's multicast, try to match the multicast filters.
      //
      NET_LIST_FOR_EACH (Entry, &MnpDeviceData->GroupAddressList) {

        *GroupAddress = NET_LIST_USER_STRUCT (Entry, MNP_GROUP_ADDRESS, AddrEntry);
        if (NET_MAC_EQUAL (BufPtr, &((*GroupAddress)->Address), SnpMode->HwAddressSize)) {
          RxData->MulticastFlag = TRUE;
          break;
        }
      }

      if (!RxData->MulticastFlag) {
        //
        // No match, set GroupAddress to NULL. This multicast packet must
        // be the result of PROMISUCOUS or PROMISUCOUS_MULTICAST flag is on.
        //
        *GroupAddress           = NULL;
        RxData->PromiscuousFlag = TRUE;

        if (MnpDeviceData->PromiscuousCount == 0) {
          //
          // Skip the below code, there is no receiver of this packet.
          //
          return ;
        }
      }
    } else {
      RxData->PromiscuousFlag = TRUE;
    }
  }

  ZeroMem (&RxData->Timestamp, sizeof (EFI_TIME));

  //
  // Fill the common parts of RxData.
  //
  RxData->PacketLength  = Nbuf->TotalSize;
  RxData->HeaderLength  = SnpMode->MediaHeaderSize;
  RxData->AddressLength = SnpMode->HwAddressSize;
  RxData->DataLength    = RxData->PacketLength - RxData->HeaderLength;
  RxData->ProtocolType  = NTOHS (*(UINT16 *) (BufPtr + 2 * SnpMode->HwAddressSize));
}


/**
  Wrap the RxData.

  @param[in]  Instance           Pointer to the mnp instance context data.
  @param[in]  RxData             Pointer to the receive data to wrap.

  @return Pointer to a MNP_RXDATA_WRAP which wraps the RxData.

**/
MNP_RXDATA_WRAP *
MnpWrapRxData (
  IN MNP_INSTANCE_DATA                   *Instance,
  IN EFI_MANAGED_NETWORK_RECEIVE_DATA    *RxData
  )
{
  EFI_STATUS      Status;
  MNP_RXDATA_WRAP *RxDataWrap;

  //
  // Allocate memory.
  //
  RxDataWrap = AllocatePool (sizeof (MNP_RXDATA_WRAP));
  if (RxDataWrap == NULL) {
    DEBUG ((EFI_D_ERROR, "MnpDispatchPacket: Failed to allocate a MNP_RXDATA_WRAP.\n"));
    return NULL;
  }

  RxDataWrap->Instance = Instance;

  //
  // Fill the RxData in RxDataWrap,
  //
  CopyMem (&RxDataWrap->RxData, RxData, sizeof (RxDataWrap->RxData));

  //
  // Create the recycle event.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  MnpRecycleRxData,
                  RxDataWrap,
                  &RxDataWrap->RxData.RecycleEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MnpDispatchPacket: gBS->CreateEvent failed, %r.\n", Status));

    FreePool (RxDataWrap);
    return NULL;
  }

  return RxDataWrap;
}


/**
  Enqueue the received the packets to the instances belonging to the
  MnpServiceData.

  @param[in]  MnpServiceData    Pointer to the mnp service context data.
  @param[in]  Nbuf              Pointer to the net buffer representing the received
                                packet.

**/
VOID
MnpEnqueuePacket (
  IN MNP_SERVICE_DATA    *MnpServiceData,
  IN NET_BUF             *Nbuf
  )
{
  LIST_ENTRY                        *Entry;
  MNP_INSTANCE_DATA                 *Instance;
  EFI_MANAGED_NETWORK_RECEIVE_DATA  RxData;
  UINT8                             PktAttr;
  MNP_GROUP_ADDRESS                 *GroupAddress;
  MNP_RXDATA_WRAP                   *RxDataWrap;


  GroupAddress = NULL;
  //
  // First, analyse the packet header.
  //
  MnpAnalysePacket (MnpServiceData, Nbuf, &RxData, &GroupAddress, &PktAttr);

  if (RxData.PromiscuousFlag && (MnpServiceData->MnpDeviceData->PromiscuousCount == 0)) {
    //
    // No receivers, no more action need.
    //
    return ;
  }

  //
  // Iterate the children to find match.
  //
  NET_LIST_FOR_EACH (Entry, &MnpServiceData->ChildrenList) {

    Instance = NET_LIST_USER_STRUCT (Entry, MNP_INSTANCE_DATA, InstEntry);
    NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

    if (!Instance->Configured) {
      continue;
    }

    //
    // Check the packet against the instance receive filters.
    //
    if (MnpMatchPacket (Instance, &RxData, GroupAddress, PktAttr)) {
      //
      // Wrap the RxData.
      //
      RxDataWrap = MnpWrapRxData (Instance, &RxData);
      if (RxDataWrap == NULL) {
        continue;
      }

      //
      // Associate RxDataWrap with Nbuf and increase the RefCnt.
      //
      RxDataWrap->Nbuf = Nbuf;
      NET_GET_REF (RxDataWrap->Nbuf);

      //
      // Queue the packet into the instance queue.
      //
      MnpQueueRcvdPacket (Instance, RxDataWrap);
    }
  }
}


/**
  Try to receive a packet and deliver it.

  @param[in, out]  MnpDeviceData        Pointer to the mnp device context data.

  @retval EFI_SUCCESS           add return value to function comment
  @retval EFI_NOT_STARTED       The simple network protocol is not started.
  @retval EFI_NOT_READY         No packet received.
  @retval EFI_DEVICE_ERROR      An unexpected error occurs.

**/
EFI_STATUS
MnpReceivePacket (
  IN OUT MNP_DEVICE_DATA   *MnpDeviceData
  )
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  NET_BUF                     *Nbuf;
  UINT8                       *BufPtr;
  UINTN                       BufLen;
  UINTN                       HeaderSize;
  UINT32                      Trimmed;
  MNP_SERVICE_DATA            *MnpServiceData;
  UINT16                      VlanId;
  BOOLEAN                     IsVlanPacket;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  Snp = MnpDeviceData->Snp;
  if (Snp->Mode->State != EfiSimpleNetworkInitialized) {
    //
    // The simple network protocol is not started.
    //
    return EFI_NOT_STARTED;
  }

  if (MnpDeviceData->RxNbufCache == NULL) {
    //
    // Try to get a new buffer as there may be buffers recycled.
    //
    MnpDeviceData->RxNbufCache = MnpAllocNbuf (MnpDeviceData);

    if (MnpDeviceData->RxNbufCache == NULL) {
      //
      // No available buffer in the buffer pool.
      //
      return EFI_DEVICE_ERROR;
    }

    NetbufAllocSpace (
      MnpDeviceData->RxNbufCache,
      MnpDeviceData->BufferLength,
      NET_BUF_TAIL
      );
  }

  Nbuf    = MnpDeviceData->RxNbufCache;
  BufLen  = Nbuf->TotalSize;
  BufPtr  = NetbufGetByte (Nbuf, 0, NULL);
  ASSERT (BufPtr != NULL);

  //
  // Receive packet through Snp.
  //
  Status = Snp->Receive (Snp, &HeaderSize, &BufLen, BufPtr, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG_CODE (
      if (Status != EFI_NOT_READY) {
        DEBUG ((EFI_D_WARN, "MnpReceivePacket: Snp->Receive() = %r.\n", Status));
      }
    );

    return Status;
  }

  //
  // Sanity check.
  //
  if ((HeaderSize != Snp->Mode->MediaHeaderSize) || (BufLen < HeaderSize)) {
    DEBUG (
      (EFI_D_WARN,
      "MnpReceivePacket: Size error, HL:TL = %d:%d.\n",
      HeaderSize,
      BufLen)
      );
    return EFI_DEVICE_ERROR;
  }

  Trimmed = 0;
  if (Nbuf->TotalSize != BufLen) {
    //
    // Trim the packet from tail.
    //
    Trimmed = NetbufTrim (Nbuf, Nbuf->TotalSize - (UINT32) BufLen, NET_BUF_TAIL);
    ASSERT (Nbuf->TotalSize == BufLen);
  }

  VlanId = 0;
  if (MnpDeviceData->NumberOfVlan != 0) {
    //
    // VLAN is configured, remove the VLAN tag if any
    //
    IsVlanPacket = MnpRemoveVlanTag (MnpDeviceData, Nbuf, &VlanId);
  } else {
    IsVlanPacket = FALSE;
  }

  MnpServiceData = MnpFindServiceData (MnpDeviceData, VlanId);
  if (MnpServiceData == NULL) {
    //
    // VLAN is not set for this tagged frame, ignore this packet
    //
    if (Trimmed > 0) {
      NetbufAllocSpace (Nbuf, Trimmed, NET_BUF_TAIL);
    }

    if (IsVlanPacket) {
      NetbufAllocSpace (Nbuf, NET_VLAN_TAG_LEN, NET_BUF_HEAD);
    }

    goto EXIT;
  }

  //
  // Enqueue the packet to the matched instances.
  //
  MnpEnqueuePacket (MnpServiceData, Nbuf);

  if (Nbuf->RefCnt > 2) {
    //
    // RefCnt > 2 indicates there is at least one receiver of this packet.
    // Free the current RxNbufCache and allocate a new one.
    //
    MnpFreeNbuf (MnpDeviceData, Nbuf);

    Nbuf                       = MnpAllocNbuf (MnpDeviceData);
    MnpDeviceData->RxNbufCache = Nbuf;
    if (Nbuf == NULL) {
      DEBUG ((EFI_D_ERROR, "MnpReceivePacket: Alloc packet for receiving cache failed.\n"));
      return EFI_DEVICE_ERROR;
    }

    NetbufAllocSpace (Nbuf, MnpDeviceData->BufferLength, NET_BUF_TAIL);
  } else {
    //
    // No receiver for this packet.
    //
    if (Trimmed > 0) {
      NetbufAllocSpace (Nbuf, Trimmed, NET_BUF_TAIL);
    }
    if (IsVlanPacket) {
      NetbufAllocSpace (Nbuf, NET_VLAN_TAG_LEN, NET_BUF_HEAD);
    }

    goto EXIT;
  }
  //
  // Deliver the queued packets.
  //
  MnpDeliverPacket (MnpServiceData);

EXIT:

  ASSERT (Nbuf->TotalSize == MnpDeviceData->BufferLength);

  return Status;
}


/**
  Remove the received packets if timeout occurs.

  @param[in]  Event        The event this notify function registered to.
  @param[in]  Context      Pointer to the context data registered to the event.

**/
VOID
EFIAPI
MnpCheckPacketTimeout (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  MNP_DEVICE_DATA   *MnpDeviceData;
  MNP_SERVICE_DATA  *MnpServiceData;
  LIST_ENTRY        *Entry;
  LIST_ENTRY        *ServiceEntry;
  LIST_ENTRY        *RxEntry;
  LIST_ENTRY        *NextEntry;
  MNP_INSTANCE_DATA *Instance;
  MNP_RXDATA_WRAP   *RxDataWrap;
  EFI_TPL           OldTpl;

  MnpDeviceData = (MNP_DEVICE_DATA *) Context;
  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  NET_LIST_FOR_EACH (ServiceEntry, &MnpDeviceData->ServiceList) {
    MnpServiceData = MNP_SERVICE_DATA_FROM_LINK (ServiceEntry);

    NET_LIST_FOR_EACH (Entry, &MnpServiceData->ChildrenList) {

      Instance = NET_LIST_USER_STRUCT (Entry, MNP_INSTANCE_DATA, InstEntry);
      NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

      if (!Instance->Configured || (Instance->ConfigData.ReceivedQueueTimeoutValue == 0)) {
        //
        // This instance is not configured or there is no receive time out,
        // just skip to the next instance.
        //
        continue;
      }

      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

      NET_LIST_FOR_EACH_SAFE (RxEntry, NextEntry, &Instance->RcvdPacketQueue) {

        RxDataWrap = NET_LIST_USER_STRUCT (RxEntry, MNP_RXDATA_WRAP, WrapEntry);

        //
        // TimeoutTick unit is microsecond, MNP_TIMEOUT_CHECK_INTERVAL unit is 100ns.
        //
        if (RxDataWrap->TimeoutTick >= (MNP_TIMEOUT_CHECK_INTERVAL / 10)) {
          RxDataWrap->TimeoutTick -= (MNP_TIMEOUT_CHECK_INTERVAL / 10);
        } else {
          //
          // Drop the timeout packet.
          //
          DEBUG ((EFI_D_WARN, "MnpCheckPacketTimeout: Received packet timeout.\n"));
          MnpRecycleRxData (NULL, RxDataWrap);
          Instance->RcvdPacketQueueSize--;
        }
      }

      gBS->RestoreTPL (OldTpl);
    }
  }
}

/**
  Poll to update MediaPresent field in SNP ModeData by Snp->GetStatus().

  @param[in]  Event        The event this notify function registered to.
  @param[in]  Context      Pointer to the context data registered to the event.

**/
VOID
EFIAPI
MnpCheckMediaStatus (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  MNP_DEVICE_DATA             *MnpDeviceData;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  UINT32                      InterruptStatus;

  MnpDeviceData = (MNP_DEVICE_DATA *) Context;
  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  Snp = MnpDeviceData->Snp;
  if (Snp->Mode->MediaPresentSupported) {
    //
    // Upon successful return of GetStatus(), the MediaPresent field of
    // EFI_SIMPLE_NETWORK_MODE will be updated to reflect any change of media status
    //
    Snp->GetStatus (Snp, &InterruptStatus, NULL);
  }
}

/**
  Poll to receive the packets from Snp. This function is either called by upperlayer
  protocols/applications or the system poll timer notify mechanism.

  @param[in]  Event        The event this notify function registered to.
  @param[in]  Context      Pointer to the context data registered to the event.

**/
VOID
EFIAPI
MnpSystemPoll (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  MNP_DEVICE_DATA  *MnpDeviceData;

  MnpDeviceData = (MNP_DEVICE_DATA *) Context;
  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  //
  // Try to receive packets from Snp.
  //
  MnpReceivePacket (MnpDeviceData);

  //
  // Dispatch the DPC queued by the NotifyFunction of rx token's events.
  //
  DispatchDpc ();
}
