/** @file
  Dhcp6 internal functions implementation.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Dhcp6Impl.h"

/**
  Enqueue the packet into the retry list in case of timeout.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  Packet          The pointer to the Dhcp6 packet to retry.
  @param[in]  Elapsed         The pointer to the elapsed time value in the packet.
  @param[in]  RetryCtl        The pointer to the transmission control of the packet.
                              This parameter is optional and may be NULL.

  @retval EFI_SUCCESS           Successfully enqueued the packet into the retry list according
                                to its message type.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected message type.

**/
EFI_STATUS
Dhcp6EnqueueRetry (
  IN DHCP6_INSTANCE            *Instance,
  IN EFI_DHCP6_PACKET          *Packet,
  IN UINT16                    *Elapsed,
  IN EFI_DHCP6_RETRANSMISSION  *RetryCtl     OPTIONAL
  )
{
  DHCP6_TX_CB  *TxCb;
  DHCP6_IA_CB  *IaCb;

  ASSERT (Packet != NULL);

  IaCb = &Instance->IaCb;
  TxCb = AllocateZeroPool (sizeof (DHCP6_TX_CB));

  if (TxCb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save tx packet pointer, and it will be destroyed when reply received.
  //
  TxCb->TxPacket = Packet;
  TxCb->Xid      = Packet->Dhcp6.Header.TransactionId;

  //
  // Save pointer to elapsed-time value so we can update it on retransmits.
  //
  TxCb->Elapsed = Elapsed;

  //
  // Calculate the retransmission according to the message type.
  //
  switch (Packet->Dhcp6.Header.MessageType) {
    case Dhcp6MsgSolicit:
      //
      // Calculate the retransmission threshold value for solicit packet.
      // Use the default value by rfc-3315 if user doesn't configure.
      //
      if (RetryCtl == NULL) {
        TxCb->RetryCtl.Irt = DHCP6_SOL_IRT;
        TxCb->RetryCtl.Mrc = DHCP6_SOL_MRC;
        TxCb->RetryCtl.Mrt = DHCP6_SOL_MRT;
        TxCb->RetryCtl.Mrd = DHCP6_SOL_MRD;
      } else {
        TxCb->RetryCtl.Irt = (RetryCtl->Irt != 0) ? RetryCtl->Irt : DHCP6_SOL_IRT;
        TxCb->RetryCtl.Mrc = (RetryCtl->Mrc != 0) ? RetryCtl->Mrc : DHCP6_SOL_MRC;
        TxCb->RetryCtl.Mrt = (RetryCtl->Mrt != 0) ? RetryCtl->Mrt : DHCP6_SOL_MRT;
        TxCb->RetryCtl.Mrd = (RetryCtl->Mrd != 0) ? RetryCtl->Mrd : DHCP6_SOL_MRD;
      }

      TxCb->RetryExp = Dhcp6CalculateExpireTime (
                         TxCb->RetryCtl.Irt,
                         TRUE,
                         FALSE
                         );
      break;

    case Dhcp6MsgRequest:
      //
      // Calculate the retransmission threshold value for request packet.
      //
      TxCb->RetryCtl.Irt = DHCP6_REQ_IRT;
      TxCb->RetryCtl.Mrc = DHCP6_REQ_MRC;
      TxCb->RetryCtl.Mrt = DHCP6_REQ_MRT;
      TxCb->RetryCtl.Mrd = DHCP6_REQ_MRD;
      TxCb->RetryExp     = Dhcp6CalculateExpireTime (
                             TxCb->RetryCtl.Irt,
                             TRUE,
                             TRUE
                             );
      break;

    case Dhcp6MsgConfirm:
      //
      // Calculate the retransmission threshold value for confirm packet.
      //
      TxCb->RetryCtl.Irt = DHCP6_CNF_IRT;
      TxCb->RetryCtl.Mrc = DHCP6_CNF_MRC;
      TxCb->RetryCtl.Mrt = DHCP6_CNF_MRT;
      TxCb->RetryCtl.Mrd = DHCP6_CNF_MRD;
      TxCb->RetryExp     = Dhcp6CalculateExpireTime (
                             TxCb->RetryCtl.Irt,
                             TRUE,
                             TRUE
                             );
      break;

    case Dhcp6MsgRenew:
      //
      // Calculate the retransmission threshold value for renew packet.
      //
      TxCb->RetryCtl.Irt = DHCP6_REB_IRT;
      TxCb->RetryCtl.Mrc = DHCP6_REB_MRC;
      TxCb->RetryCtl.Mrt = DHCP6_REB_MRT;
      TxCb->RetryCtl.Mrd = IaCb->T2 - IaCb->T1;
      TxCb->RetryExp     = Dhcp6CalculateExpireTime (
                             TxCb->RetryCtl.Irt,
                             TRUE,
                             TRUE
                             );
      break;

    case Dhcp6MsgRebind:
      //
      // Calculate the retransmission threshold value for rebind packet.
      //
      TxCb->RetryCtl.Irt = DHCP6_REN_IRT;
      TxCb->RetryCtl.Mrc = DHCP6_REN_MRC;
      TxCb->RetryCtl.Mrt = DHCP6_REN_MRT;
      TxCb->RetryCtl.Mrd = IaCb->AllExpireTime - IaCb->T2;
      TxCb->RetryExp     = Dhcp6CalculateExpireTime (
                             TxCb->RetryCtl.Irt,
                             TRUE,
                             TRUE
                             );
      break;

    case Dhcp6MsgDecline:
      //
      // Calculate the retransmission threshold value for decline packet.
      //
      TxCb->RetryCtl.Irt = DHCP6_DEC_IRT;
      TxCb->RetryCtl.Mrc = DHCP6_DEC_MRC;
      TxCb->RetryCtl.Mrt = DHCP6_DEC_MRT;
      TxCb->RetryCtl.Mrd = DHCP6_DEC_MRD;
      TxCb->RetryExp     = Dhcp6CalculateExpireTime (
                             TxCb->RetryCtl.Irt,
                             TRUE,
                             TRUE
                             );
      break;

    case Dhcp6MsgRelease:
      //
      // Calculate the retransmission threshold value for release packet.
      //
      TxCb->RetryCtl.Irt = DHCP6_REL_IRT;
      TxCb->RetryCtl.Mrc = DHCP6_REL_MRC;
      TxCb->RetryCtl.Mrt = DHCP6_REL_MRT;
      TxCb->RetryCtl.Mrd = DHCP6_REL_MRD;
      TxCb->RetryExp     = Dhcp6CalculateExpireTime (
                             TxCb->RetryCtl.Irt,
                             TRUE,
                             TRUE
                             );
      break;

    case Dhcp6MsgInfoRequest:
      //
      // Calculate the retransmission threshold value for info-request packet.
      // Use the default value by rfc-3315 if user doesn't configure.
      //
      if (RetryCtl == NULL) {
        TxCb->RetryCtl.Irt = DHCP6_INF_IRT;
        TxCb->RetryCtl.Mrc = DHCP6_INF_MRC;
        TxCb->RetryCtl.Mrt = DHCP6_INF_MRT;
        TxCb->RetryCtl.Mrd = DHCP6_INF_MRD;
      } else {
        TxCb->RetryCtl.Irt = (RetryCtl->Irt != 0) ? RetryCtl->Irt : DHCP6_INF_IRT;
        TxCb->RetryCtl.Mrc = (RetryCtl->Mrc != 0) ? RetryCtl->Mrc : DHCP6_INF_MRC;
        TxCb->RetryCtl.Mrt = (RetryCtl->Mrt != 0) ? RetryCtl->Mrt : DHCP6_INF_MRT;
        TxCb->RetryCtl.Mrd = (RetryCtl->Mrd != 0) ? RetryCtl->Mrd : DHCP6_INF_MRD;
      }

      TxCb->RetryExp = Dhcp6CalculateExpireTime (
                         TxCb->RetryCtl.Irt,
                         TRUE,
                         TRUE
                         );
      break;

    default:
      //
      // Unexpected message type.
      //
      FreePool (TxCb);
      return EFI_DEVICE_ERROR;
  }

  //
  // Insert into the retransmit list of the instance.
  //
  InsertTailList (&Instance->TxList, &TxCb->Link);

  return EFI_SUCCESS;
}

/**
  Dequeue the packet from retry list if reply received or timeout at last.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  PacketXid       The packet transaction id to match.
  @param[in]  NeedSignal      If TRUE, then an timeout event need be signaled when it is existed.
                              Otherwise, this parameter is ignored.

  @retval EFI_SUCCESS         Successfully dequeued the packet into retry list .
  @retval EFI_NOT_FOUND       There is no xid matched in retry list.

**/
EFI_STATUS
Dhcp6DequeueRetry (
  IN DHCP6_INSTANCE  *Instance,
  IN UINT32          PacketXid,
  IN BOOLEAN         NeedSignal
  )
{
  LIST_ENTRY    *Entry;
  LIST_ENTRY    *NextEntry;
  DHCP6_TX_CB   *TxCb;
  DHCP6_INF_CB  *InfCb;

  //
  // Seek the retransmit node in the retransmit list by packet xid.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Instance->TxList) {
    TxCb = NET_LIST_USER_STRUCT (Entry, DHCP6_TX_CB, Link);
    ASSERT (TxCb->TxPacket);

    if (TxCb->Xid == PacketXid) {
      if (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgInfoRequest) {
        //
        // Seek the info-request node in the info-request list by packet xid.
        //
        NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Instance->InfList) {
          InfCb = NET_LIST_USER_STRUCT (Entry, DHCP6_INF_CB, Link);

          if (InfCb->Xid == PacketXid) {
            //
            // Remove the info-request node, and signal the event if timeout.
            //
            if ((InfCb->TimeoutEvent != NULL) && NeedSignal) {
              gBS->SignalEvent (InfCb->TimeoutEvent);
            }

            RemoveEntryList (&InfCb->Link);
            FreePool (InfCb);
          }
        }
      }

      //
      // Remove the retransmit node.
      //
      RemoveEntryList (&TxCb->Link);
      ASSERT (TxCb->TxPacket);
      FreePool (TxCb->TxPacket);
      FreePool (TxCb);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Clean up the specific nodes in the retry list.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  Scope           The scope of cleanup nodes.

**/
VOID
Dhcp6CleanupRetry (
  IN DHCP6_INSTANCE  *Instance,
  IN UINT32          Scope
  )
{
  LIST_ENTRY    *Entry;
  LIST_ENTRY    *NextEntry;
  DHCP6_TX_CB   *TxCb;
  DHCP6_INF_CB  *InfCb;

  //
  // Clean up all the stateful messages from the retransmit list.
  //
  if ((Scope == DHCP6_PACKET_STATEFUL) || (Scope == DHCP6_PACKET_ALL)) {
    NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Instance->TxList) {
      TxCb = NET_LIST_USER_STRUCT (Entry, DHCP6_TX_CB, Link);
      ASSERT (TxCb->TxPacket);

      if (TxCb->TxPacket->Dhcp6.Header.MessageType != Dhcp6MsgInfoRequest) {
        RemoveEntryList (&TxCb->Link);
        FreePool (TxCb->TxPacket);
        FreePool (TxCb);
      }
    }
  }

  //
  // Clean up all the stateless messages from the retransmit list.
  //
  if ((Scope == DHCP6_PACKET_STATELESS) || (Scope == DHCP6_PACKET_ALL)) {
    //
    // Clean up all the retransmit list for stateless messages.
    //
    NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Instance->TxList) {
      TxCb = NET_LIST_USER_STRUCT (Entry, DHCP6_TX_CB, Link);
      ASSERT (TxCb->TxPacket);

      if (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgInfoRequest) {
        RemoveEntryList (&TxCb->Link);
        FreePool (TxCb->TxPacket);
        FreePool (TxCb);
      }
    }

    //
    // Clean up all the info-request messages list.
    //
    NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Instance->InfList) {
      InfCb = NET_LIST_USER_STRUCT (Entry, DHCP6_INF_CB, Link);

      if (InfCb->TimeoutEvent != NULL) {
        gBS->SignalEvent (InfCb->TimeoutEvent);
      }

      RemoveEntryList (&InfCb->Link);
      FreePool (InfCb);
    }
  }
}

/**
  Check whether the TxCb is still a valid control block in the instance's retry list.

  @param[in]  Instance       The pointer to DHCP6_INSTANCE.
  @param[in]  TxCb           The control block for a transmitted message.

  @retval   TRUE      The control block is in Instance's retry list.
  @retval   FALSE     The control block is NOT in Instance's retry list.

**/
BOOLEAN
Dhcp6IsValidTxCb (
  IN  DHCP6_INSTANCE  *Instance,
  IN  DHCP6_TX_CB     *TxCb
  )
{
  LIST_ENTRY  *Entry;

  NET_LIST_FOR_EACH (Entry, &Instance->TxList) {
    if (TxCb == NET_LIST_USER_STRUCT (Entry, DHCP6_TX_CB, Link)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Clean up the session of the instance stateful exchange.

  @param[in, out]  Instance        The pointer to the Dhcp6 instance.
  @param[in]       Status          The return status from udp.

**/
VOID
Dhcp6CleanupSession (
  IN OUT DHCP6_INSTANCE  *Instance,
  IN     EFI_STATUS      Status
  )
{
  UINTN         Index;
  EFI_DHCP6_IA  *Ia;

  ASSERT (Instance->Config);
  ASSERT (Instance->IaCb.Ia);

  //
  // Clean up the retransmit list for stateful messages.
  //
  Dhcp6CleanupRetry (Instance, DHCP6_PACKET_STATEFUL);

  if (Instance->Unicast != NULL) {
    FreePool (Instance->Unicast);
  }

  if (Instance->AdSelect != NULL) {
    FreePool (Instance->AdSelect);
  }

  if (Instance->IaCb.Ia->ReplyPacket != NULL) {
    FreePool (Instance->IaCb.Ia->ReplyPacket);
  }

  //
  // Reinitialize the Ia fields of the instance.
  //
  Instance->UdpSts             = Status;
  Instance->AdSelect           = NULL;
  Instance->AdPref             = 0;
  Instance->Unicast            = NULL;
  Instance->IaCb.T1            = 0;
  Instance->IaCb.T2            = 0;
  Instance->IaCb.AllExpireTime = 0;
  Instance->IaCb.LeaseTime     = 0;

  //
  // Clear start time
  //
  Instance->StartTime = 0;

  Ia              = Instance->IaCb.Ia;
  Ia->State       = Dhcp6Init;
  Ia->ReplyPacket = NULL;

  //
  // Set the addresses as zero lifetime, and then the notify
  // function in Ip6Config will remove these timeout address.
  //
  for (Index = 0; Index < Ia->IaAddressCount; Index++) {
    Ia->IaAddress[Index].PreferredLifetime = 0;
    Ia->IaAddress[Index].ValidLifetime     = 0;
  }

  //
  //
  // Signal the Ia information updated event to informal user.
  //
  if (Instance->Config->IaInfoEvent != NULL) {
    gBS->SignalEvent (Instance->Config->IaInfoEvent);
  }
}

/**
  Callback to user when Dhcp6 transmit/receive occurs.

  @param[in]      Instance        The pointer to the Dhcp6 instance.
  @param[in]      Event           The current Dhcp6 event.
  @param[in, out] Packet          The pointer to the packet sending or received.

  @retval EFI_SUCCESS           The user function returns success.
  @retval EFI_NOT_READY         Direct the caller to continue collecting the offer.
  @retval EFI_ABORTED           The user function ask it to abort.

**/
EFI_STATUS
EFIAPI
Dhcp6CallbackUser (
  IN     DHCP6_INSTANCE    *Instance,
  IN     EFI_DHCP6_EVENT   Event,
  IN OUT EFI_DHCP6_PACKET  **Packet
  )
{
  EFI_STATUS          Status;
  EFI_DHCP6_PACKET    *NewPacket;
  EFI_DHCP6_CALLBACK  Callback;
  VOID                *Context;

  ASSERT (Packet != NULL);
  ASSERT (Instance->Config != NULL);
  ASSERT (Instance->IaCb.Ia != NULL);

  NewPacket = NULL;
  Status    = EFI_SUCCESS;
  Callback  = Instance->Config->Dhcp6Callback;
  Context   = Instance->Config->CallbackContext;

  //
  // Callback to user with the new message if has.
  //
  if (Callback != NULL) {
    Status = Callback (
               &Instance->Dhcp6,
               Context,
               Instance->IaCb.Ia->State,
               Event,
               *Packet,
               &NewPacket
               );
    //
    // Updated the new packet from user to replace the original one.
    //
    if (NewPacket != NULL) {
      ASSERT (*Packet != NULL);
      FreePool (*Packet);
      *Packet = NewPacket;
    }
  }

  return Status;
}

/**
  Update Ia according to the new reply message.

  @param[in, out]  Instance        The pointer to the Dhcp6 instance.
  @param[in]       Packet          The pointer to reply messages.

  @retval EFI_SUCCESS         Updated the Ia information successfully.
  @retval EFI_DEVICE_ERROR    An unexpected error.

**/
EFI_STATUS
Dhcp6UpdateIaInfo (
  IN OUT DHCP6_INSTANCE    *Instance,
  IN     EFI_DHCP6_PACKET  *Packet
  )
{
  EFI_STATUS  Status;
  UINT8       *Option;
  UINT32      OptionLen;
  UINT8       *IaInnerOpt;
  UINT16      IaInnerLen;
  UINT16      StsCode;
  UINT32      T1;
  UINT32      T2;

  T1 = 0;
  T2 = 0;

  ASSERT (Instance->Config != NULL);

  // OptionLen is the length of the Options excluding the DHCP header.
  // Length of the EFI_DHCP6_PACKET from the first byte of the Header field to the last
  // byte of the Option[] field.
  OptionLen = Packet->Length - sizeof (Packet->Dhcp6.Header);

  //
  // If the reply was received in response to a solicit with rapid commit option,
  // request, renew or rebind message, the client updates the information it has
  // recorded about IAs from the IA options contained in the reply message:
  //   1. record the T1 and T2 times
  //   2. add any new addresses in the IA
  //   3. discard any addresses from the IA, that have a valid lifetime of 0
  //   4. update lifetimes for any addresses that already recorded
  //   5. leave unchanged any information about addresses
  //
  // See details in the section-18.1.8 of rfc-3315.
  //
  Option = Dhcp6SeekIaOption (
             Packet->Dhcp6.Option,
             OptionLen,
             &Instance->Config->IaDescriptor
             );
  if (Option == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Calculate the distance from Packet->Dhcp6.Option to the IA option.
  //
  // Packet->Size and Packet->Length are both UINT32 type, and Packet->Size is
  // the size of the whole packet, including the DHCP header, and Packet->Length
  // is the length of the DHCP message body, excluding the DHCP header.
  //
  // (*Option - Packet->Dhcp6.Option) is the number of bytes from the start of
  // DHCP6 option area to the start of the IA option.
  //
  // Dhcp6SeekInnerOptionSafe() is searching starting from the start of the
  // IA option to the end of the DHCP6 option area, thus subtract the space
  // up until this option
  //
  OptionLen = OptionLen - (UINT32)(Option - Packet->Dhcp6.Option);

  //
  // The format of the IA_NA option is:
  //
  //     0                   1                   2                   3
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |          OPTION_IA_NA         |          option-len           |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                        IAID (4 octets)                        |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                              T1                               |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                              T2                               |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                                                               |
  //    .                         IA_NA-options                         .
  //    .                                                               .
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //
  // The format of the IA_TA option is:
  //
  //     0                   1                   2                   3
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |         OPTION_IA_TA          |          option-len           |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                        IAID (4 octets)                        |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                                                               |
  //    .                         IA_TA-options                         .
  //    .                                                               .
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  //
  // Seek the inner option
  //
  if (EFI_ERROR (
        Dhcp6SeekInnerOptionSafe (
          Instance->Config->IaDescriptor.Type,
          Option,
          OptionLen,
          &IaInnerOpt,
          &IaInnerLen
          )
        ))
  {
    return EFI_DEVICE_ERROR;
  }

  if (Instance->Config->IaDescriptor.Type == Dhcp6OptIana) {
    T1 = NTOHL (ReadUnaligned32 ((UINT32 *)(DHCP6_OFFSET_OF_IA_NA_T1 (Option))));
    T2 = NTOHL (ReadUnaligned32 ((UINT32 *)(DHCP6_OFFSET_OF_IA_NA_T2 (Option))));
    //
    // Refer to RFC3155 Chapter 22.4. If a client receives an IA_NA with T1 greater than T2,
    // and both T1 and T2 are greater than 0, the client discards the IA_NA option and processes
    // the remainder of the message as though the server had not included the invalid IA_NA option.
    //
    if ((T1 > T2) && (T2 > 0)) {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // The format of the Status Code option is:
  //
  //   0                   1                   2                   3
  //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  |       OPTION_STATUS_CODE      |         option-len            |
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  |          status-code          |                               |
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
  //  .                                                               .
  //  .                        status-message                         .
  //  .                                                               .
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  //
  // sizeof (option-code + option-len) = 4
  //
  StsCode = Dhcp6StsSuccess;
  Option  = Dhcp6SeekOption (IaInnerOpt, IaInnerLen, Dhcp6OptStatusCode);

  if (Option != NULL) {
    StsCode = NTOHS (ReadUnaligned16 ((UINT16 *)(DHCP6_OFFSET_OF_STATUS_CODE (Option))));
    if (StsCode != Dhcp6StsSuccess) {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Generate control block for the Ia.
  //
  Status = Dhcp6GenerateIaCb (
             Instance,
             IaInnerOpt,
             IaInnerLen,
             T1,
             T2
             );

  return Status;
}

/**
  Seeks the Inner Options from a DHCP6 Option

  @param[in]  IaType          The type of the IA option.
  @param[in]  Option          The pointer to the DHCP6 Option.
  @param[in]  OptionLen       The length of the DHCP6 Option.
  @param[out] IaInnerOpt      The pointer to the IA inner option.
  @param[out] IaInnerLen      The length of the IA inner option.

  @retval EFI_SUCCESS         Seek the inner option successfully.
  @retval EFI_DEVICE_ERROR    The OptionLen is invalid. On Error,
                              the pointers are not modified
**/
EFI_STATUS
Dhcp6SeekInnerOptionSafe (
  IN  UINT16  IaType,
  IN  UINT8   *Option,
  IN  UINT32  OptionLen,
  OUT UINT8   **IaInnerOpt,
  OUT UINT16  *IaInnerLen
  )
{
  UINT16  IaInnerLenTmp;
  UINT8   *IaInnerOptTmp;

  if (Option == NULL) {
    ASSERT (Option != NULL);
    return EFI_DEVICE_ERROR;
  }

  if (IaInnerOpt == NULL) {
    ASSERT (IaInnerOpt != NULL);
    return EFI_DEVICE_ERROR;
  }

  if (IaInnerLen == NULL) {
    ASSERT (IaInnerLen != NULL);
    return EFI_DEVICE_ERROR;
  }

  if (IaType == Dhcp6OptIana) {
    //
    // Verify we have a fully formed IA_NA
    //
    if (OptionLen < DHCP6_MIN_SIZE_OF_IA_NA) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Get the IA Inner Option and Length
    //
    IaInnerOptTmp = DHCP6_OFFSET_OF_IA_NA_INNER_OPT (Option);

    //
    // Verify the IaInnerLen is valid.
    //
    IaInnerLenTmp = (UINT16)NTOHS (ReadUnaligned16 ((UINT16 *)DHCP6_OFFSET_OF_OPT_LEN (Option)));
    if (IaInnerLenTmp < DHCP6_SIZE_OF_COMBINED_IAID_T1_T2) {
      return EFI_DEVICE_ERROR;
    }

    IaInnerLenTmp -= DHCP6_SIZE_OF_COMBINED_IAID_T1_T2;
  } else if (IaType == Dhcp6OptIata) {
    //
    // Verify the OptionLen is valid.
    //
    if (OptionLen < DHCP6_MIN_SIZE_OF_IA_TA) {
      return EFI_DEVICE_ERROR;
    }

    IaInnerOptTmp = DHCP6_OFFSET_OF_IA_TA_INNER_OPT (Option);

    //
    // Verify the IaInnerLen is valid.
    //
    IaInnerLenTmp = (UINT16)NTOHS (ReadUnaligned16 ((UINT16 *)(DHCP6_OFFSET_OF_OPT_LEN (Option))));
    if (IaInnerLenTmp < DHCP6_SIZE_OF_IAID) {
      return EFI_DEVICE_ERROR;
    }

    IaInnerLenTmp -= DHCP6_SIZE_OF_IAID;
  } else {
    return EFI_DEVICE_ERROR;
  }

  *IaInnerOpt = IaInnerOptTmp;
  *IaInnerLen = IaInnerLenTmp;

  return EFI_SUCCESS;
}

/**
  Seek StatusCode Option in package. A Status Code option may appear in the
  options field of a DHCP message and/or in the options field of another option.
  See details in section 22.13, RFC3315.

  @param[in]       Instance        The pointer to the Dhcp6 instance.
  @param[in]       Packet          The pointer to reply messages.
  @param[out]      Option          The pointer to status code option.

  @retval EFI_SUCCESS              Seek status code option successfully.
  @retval EFI_DEVICE_ERROR         An unexpected error.

**/
EFI_STATUS
Dhcp6SeekStsOption (
  IN     DHCP6_INSTANCE    *Instance,
  IN     EFI_DHCP6_PACKET  *Packet,
  OUT    UINT8             **Option
  )
{
  UINT8   *IaInnerOpt;
  UINT16  IaInnerLen;
  UINT16  StsCode;
  UINT32  OptionLen;

  // OptionLen is the length of the Options excluding the DHCP header.
  // Length of the EFI_DHCP6_PACKET from the first byte of the Header field to the last
  // byte of the Option[] field.
  OptionLen = Packet->Length - sizeof (Packet->Dhcp6.Header);

  //
  // Seek StatusCode option directly in DHCP message body. That is, search in
  // non-encapsulated option fields.
  //
  *Option = Dhcp6SeekOption (
              Packet->Dhcp6.Option,
              OptionLen,
              Dhcp6OptStatusCode
              );

  if (*Option != NULL) {
    StsCode = NTOHS (ReadUnaligned16 ((UINT16 *)(DHCP6_OFFSET_OF_STATUS_CODE (*Option))));
    if (StsCode != Dhcp6StsSuccess) {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Seek in encapsulated options, IA_NA and IA_TA.
  //
  *Option = Dhcp6SeekIaOption (
              Packet->Dhcp6.Option,
              OptionLen,
              &Instance->Config->IaDescriptor
              );
  if (*Option == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Calculate the distance from Packet->Dhcp6.Option to the IA option.
  //
  // Packet->Size and Packet->Length are both UINT32 type, and Packet->Size is
  // the size of the whole packet, including the DHCP header, and Packet->Length
  // is the length of the DHCP message body, excluding the DHCP header.
  //
  // (*Option - Packet->Dhcp6.Option) is the number of bytes from the start of
  // DHCP6 option area to the start of the IA option.
  //
  // Dhcp6SeekInnerOptionSafe() is searching starting from the start of the
  // IA option to the end of the DHCP6 option area, thus subtract the space
  // up until this option
  //
  OptionLen = OptionLen - (UINT32)(*Option - Packet->Dhcp6.Option);

  //
  // Seek the inner option
  //
  if (EFI_ERROR (
        Dhcp6SeekInnerOptionSafe (
          Instance->Config->IaDescriptor.Type,
          *Option,
          OptionLen,
          &IaInnerOpt,
          &IaInnerLen
          )
        ))
  {
    return EFI_DEVICE_ERROR;
  }

  //
  // The format of the Status Code option is:
  //
  //   0                   1                   2                   3
  //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  |       OPTION_STATUS_CODE      |         option-len            |
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  |          status-code          |                               |
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
  //  .                                                               .
  //  .                        status-message                         .
  //  .                                                               .
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  //
  // sizeof (option-code + option-len) = 4
  //
  *Option = Dhcp6SeekOption (IaInnerOpt, IaInnerLen, Dhcp6OptStatusCode);
  if (*Option != NULL) {
    StsCode = NTOHS (ReadUnaligned16 ((UINT16 *)((DHCP6_OFFSET_OF_STATUS_CODE (*Option)))));
    if (StsCode != Dhcp6StsSuccess) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Transmit Dhcp6 message by udpio.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  Packet          The pointer to transmit message.
  @param[in]  Elapsed         The pointer to the elapsed time value to fill in.

  @retval EFI_SUCCESS           Successfully transmitted the packet.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Failed to transmit the packet.

**/
EFI_STATUS
Dhcp6TransmitPacket (
  IN DHCP6_INSTANCE    *Instance,
  IN EFI_DHCP6_PACKET  *Packet,
  IN UINT16            *Elapsed
  )
{
  EFI_STATUS     Status;
  NET_BUF        *Wrap;
  NET_FRAGMENT   Frag;
  UDP_END_POINT  EndPt;
  DHCP6_SERVICE  *Service;

  Service = Instance->Service;

  //
  // Wrap it into a netbuf then send it.
  //
  Frag.Bulk = (UINT8 *)&Packet->Dhcp6.Header;
  Frag.Len  = Packet->Length;

  //
  // Do not register free packet here, which will be handled in retry list.
  //
  Wrap = NetbufFromExt (&Frag, 1, 0, 0, Dhcp6DummyExtFree, NULL);

  if (Wrap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Multicast the Dhcp6 message, unless get the unicast server address by option.
  //
  ZeroMem (&EndPt, sizeof (UDP_END_POINT));

  if (Instance->Unicast != NULL) {
    CopyMem (
      &EndPt.RemoteAddr,
      Instance->Unicast,
      sizeof (EFI_IPv6_ADDRESS)
      );
  } else {
    CopyMem (
      &EndPt.RemoteAddr,
      &mAllDhcpRelayAndServersAddress,
      sizeof (EFI_IPv6_ADDRESS)
      );
  }

  EndPt.RemotePort = DHCP6_PORT_SERVER;
  EndPt.LocalPort  = DHCP6_PORT_CLIENT;

  //
  // Update the elapsed time value.
  //
  if (Elapsed != NULL) {
    SetElapsedTime (Elapsed, Instance);
  }

  //
  // Send out the message by the configured Udp6Io.
  //
  Status = UdpIoSendDatagram (
             Service->UdpIo,
             Wrap,
             &EndPt,
             NULL,
             Dhcp6OnTransmitted,
             NULL
             );

  if (EFI_ERROR (Status)) {
    NetbufFree (Wrap);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Create the solicit message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.

  @retval EFI_SUCCESS           Created and sent the solicit message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Failed to send the solicit message.

**/
EFI_STATUS
Dhcp6SendSolicitMsg   (
  IN DHCP6_INSTANCE  *Instance
  )
{
  EFI_STATUS               Status;
  EFI_DHCP6_PACKET         *Packet;
  EFI_DHCP6_PACKET_OPTION  *UserOpt;
  EFI_DHCP6_DUID           *ClientId;
  DHCP6_SERVICE            *Service;
  UINT8                    *Cursor;
  UINT16                   *Elapsed;
  UINT32                   UserLen;
  UINTN                    Index;
  UINT16                   Length;

  Service  = Instance->Service;
  ClientId = Service->ClientId;
  UserLen  = 0;

  ASSERT (Service->ClientId != NULL);
  ASSERT (Instance->Config != NULL);
  ASSERT (Instance->IaCb.Ia != NULL);

  //
  // Calculate the added length of customized option list.
  //
  for (Index = 0; Index < Instance->Config->OptionCount; Index++) {
    UserLen += (NTOHS (Instance->Config->OptionList[Index]->OpLen) + 4);
  }

  //
  // Create the Dhcp6 packet and initialize common fields.
  //
  Packet = AllocateZeroPool (DHCP6_BASE_PACKET_SIZE + UserLen);
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Packet->Size                       = DHCP6_BASE_PACKET_SIZE + UserLen;
  Packet->Length                     = sizeof (EFI_DHCP6_HEADER);
  Packet->Dhcp6.Header.MessageType   = Dhcp6MsgSolicit;
  Packet->Dhcp6.Header.TransactionId = Service->Xid++;

  //
  // Assembly Dhcp6 options for solicit message.
  //
  Cursor = Packet->Dhcp6.Option;

  Length = HTONS (ClientId->Length);
  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptClientId),
             Length,
             ClientId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendETOption (
             Packet,
             &Cursor,
             Instance,
             &Elapsed
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendIaOption (
             Packet,
             &Cursor,
             Instance->IaCb.Ia,
             Instance->IaCb.T1,
             Instance->IaCb.T2,
             Packet->Dhcp6.Header.MessageType
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Append user-defined when configurate Dhcp6 service.
  //
  for (Index = 0; Index < Instance->Config->OptionCount; Index++) {
    UserOpt = Instance->Config->OptionList[Index];
    Status  = Dhcp6AppendOption (
                Packet,
                &Cursor,
                UserOpt->OpCode,
                UserOpt->OpLen,
                UserOpt->Data
                );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  ASSERT (Packet->Size > Packet->Length + 8);

  //
  // Callback to user with the packet to be sent and check the user's feedback.
  //
  Status = Dhcp6CallbackUser (Instance, Dhcp6SendSolicit, &Packet);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Send solicit packet with the state transition from Dhcp6init to
  // Dhcp6selecting.
  //
  Instance->IaCb.Ia->State = Dhcp6Selecting;
  //
  // Clear initial time for current transaction.
  //
  Instance->StartTime = 0;

  Status = Dhcp6TransmitPacket (Instance, Packet, Elapsed);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Enqueue the sent packet for the retransmission in case reply timeout.
  //
  return Dhcp6EnqueueRetry (
           Instance,
           Packet,
           Elapsed,
           Instance->Config->SolicitRetransmission
           );

ON_ERROR:

  if (Packet) {
    FreePool (Packet);
  }

  return Status;
}

/**
  Configure some parameter to initiate SolicitMsg.

  @param[in]  Instance          The pointer to the Dhcp6 instance.

  @retval EFI_SUCCESS           Created and sent the solicit message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Failed to send the solicit message.

**/
EFI_STATUS
Dhcp6InitSolicitMsg   (
  IN DHCP6_INSTANCE  *Instance
  )
{
  Instance->IaCb.T1                 = 0;
  Instance->IaCb.T2                 = 0;
  Instance->IaCb.Ia->IaAddressCount = 0;

  return Dhcp6SendSolicitMsg (Instance);
}

/**
  Create the request message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.

  @retval EFI_SUCCESS           Created and sent the request message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the request message.

**/
EFI_STATUS
Dhcp6SendRequestMsg (
  IN DHCP6_INSTANCE  *Instance
  )
{
  EFI_STATUS               Status;
  EFI_DHCP6_PACKET         *Packet;
  EFI_DHCP6_PACKET_OPTION  *UserOpt;
  EFI_DHCP6_DUID           *ClientId;
  EFI_DHCP6_DUID           *ServerId;
  DHCP6_SERVICE            *Service;
  UINT8                    *Option;
  UINT8                    *Cursor;
  UINT16                   *Elapsed;
  UINT32                   UserLen;
  UINTN                    Index;
  UINT16                   Length;

  ASSERT (Instance->AdSelect != NULL);
  ASSERT (Instance->Config != NULL);
  ASSERT (Instance->IaCb.Ia != NULL);
  ASSERT (Instance->Service != NULL);

  Service  = Instance->Service;
  ClientId = Service->ClientId;

  ASSERT (ClientId != NULL);

  //
  // Get the server Id from the selected advertisement message.
  //
  Option = Dhcp6SeekOption (
             Instance->AdSelect->Dhcp6.Option,
             Instance->AdSelect->Length - sizeof (EFI_DHCP6_HEADER),
             Dhcp6OptServerId
             );
  if (Option == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ServerId = (EFI_DHCP6_DUID *)(Option + 2);

  //
  // Calculate the added length of customized option list.
  //
  UserLen = 0;
  for (Index = 0; Index < Instance->Config->OptionCount; Index++) {
    UserLen += (NTOHS (Instance->Config->OptionList[Index]->OpLen) + 4);
  }

  //
  // Create the Dhcp6 packet and initialize common fields.
  //
  Packet = AllocateZeroPool (DHCP6_BASE_PACKET_SIZE + UserLen);
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Packet->Size                       = DHCP6_BASE_PACKET_SIZE + UserLen;
  Packet->Length                     = sizeof (EFI_DHCP6_HEADER);
  Packet->Dhcp6.Header.MessageType   = Dhcp6MsgRequest;
  Packet->Dhcp6.Header.TransactionId = Service->Xid++;

  //
  // Assembly Dhcp6 options for request message.
  //
  Cursor = Packet->Dhcp6.Option;

  Length = HTONS (ClientId->Length);
  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptClientId),
             Length,
             ClientId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendETOption (
             Packet,
             &Cursor,
             Instance,
             &Elapsed
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptServerId),
             ServerId->Length,
             ServerId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendIaOption (
             Packet,
             &Cursor,
             Instance->IaCb.Ia,
             Instance->IaCb.T1,
             Instance->IaCb.T2,
             Packet->Dhcp6.Header.MessageType
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Append user-defined when configurate Dhcp6 service.
  //
  for (Index = 0; Index < Instance->Config->OptionCount; Index++) {
    UserOpt = Instance->Config->OptionList[Index];
    Status  = Dhcp6AppendOption (
                Packet,
                &Cursor,
                UserOpt->OpCode,
                UserOpt->OpLen,
                UserOpt->Data
                );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  ASSERT (Packet->Size > Packet->Length + 8);

  //
  // Callback to user with the packet to be sent and check the user's feedback.
  //
  Status = Dhcp6CallbackUser (Instance, Dhcp6SendRequest, &Packet);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Send request packet with the state transition from Dhcp6selecting to
  // Dhcp6requesting.
  //
  Instance->IaCb.Ia->State = Dhcp6Requesting;
  //
  // Clear initial time for current transaction.
  //
  Instance->StartTime = 0;

  Status = Dhcp6TransmitPacket (Instance, Packet, Elapsed);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Enqueue the sent packet for the retransmission in case reply timeout.
  //
  return Dhcp6EnqueueRetry (Instance, Packet, Elapsed, NULL);

ON_ERROR:

  if (Packet) {
    FreePool (Packet);
  }

  return Status;
}

/**
  Create the decline message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  DecIa           The pointer to the decline Ia.

  @retval EFI_SUCCESS           Created and sent the decline message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the decline message.

**/
EFI_STATUS
Dhcp6SendDeclineMsg (
  IN DHCP6_INSTANCE  *Instance,
  IN EFI_DHCP6_IA    *DecIa
  )
{
  EFI_STATUS        Status;
  EFI_DHCP6_PACKET  *Packet;
  EFI_DHCP6_PACKET  *LastReply;
  EFI_DHCP6_DUID    *ClientId;
  EFI_DHCP6_DUID    *ServerId;
  DHCP6_SERVICE     *Service;
  UINT8             *Option;
  UINT8             *Cursor;
  UINT16            *Elapsed;
  UINT16            Length;

  ASSERT (Instance->Config != NULL);
  ASSERT (Instance->IaCb.Ia != NULL);
  ASSERT (Instance->Service != NULL);

  Service   = Instance->Service;
  ClientId  = Service->ClientId;
  LastReply = Instance->IaCb.Ia->ReplyPacket;

  ASSERT (ClientId != NULL);
  ASSERT (LastReply != NULL);

  //
  // Get the server Id from the last reply message.
  //
  Option = Dhcp6SeekOption (
             LastReply->Dhcp6.Option,
             LastReply->Length - sizeof (EFI_DHCP6_HEADER),
             Dhcp6OptServerId
             );
  if (Option == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // EFI_DHCP6_DUID contains a length field of 2 bytes.
  //
  ServerId = (EFI_DHCP6_DUID *)(Option + 2);

  //
  // Create the Dhcp6 packet and initialize common fields.
  //
  Packet = AllocateZeroPool (DHCP6_BASE_PACKET_SIZE);
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Packet->Size                       = DHCP6_BASE_PACKET_SIZE;
  Packet->Length                     = sizeof (EFI_DHCP6_HEADER);
  Packet->Dhcp6.Header.MessageType   = Dhcp6MsgDecline;
  Packet->Dhcp6.Header.TransactionId = Service->Xid++;

  //
  // Assembly Dhcp6 options for rebind/renew message.
  //
  Cursor = Packet->Dhcp6.Option;

  Length = HTONS (ClientId->Length);
  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptClientId),
             Length,
             ClientId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendETOption (
             Packet,
             &Cursor,
             Instance,
             &Elapsed
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptServerId),
             ServerId->Length,
             ServerId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendIaOption (
             Packet,
             &Cursor,
             DecIa,
             0,
             0,
             Packet->Dhcp6.Header.MessageType
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  ASSERT (Packet->Size > Packet->Length + 8);

  //
  // Callback to user with the packet to be sent and check the user's feedback.
  //
  Status = Dhcp6CallbackUser (Instance, Dhcp6SendDecline, &Packet);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Send decline packet with the state transition from Dhcp6bound to
  // Dhcp6declining.
  //
  Instance->IaCb.Ia->State = Dhcp6Declining;
  //
  // Clear initial time for current transaction.
  //
  Instance->StartTime = 0;

  Status = Dhcp6TransmitPacket (Instance, Packet, Elapsed);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Enqueue the sent packet for the retransmission in case reply timeout.
  //
  return Dhcp6EnqueueRetry (Instance, Packet, Elapsed, NULL);

ON_ERROR:

  if (Packet) {
    FreePool (Packet);
  }

  return Status;
}

/**
  Create the release message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  RelIa           The pointer to the release Ia.

  @retval EFI_SUCCESS           Created and sent the release message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the release message.

**/
EFI_STATUS
Dhcp6SendReleaseMsg (
  IN DHCP6_INSTANCE  *Instance,
  IN EFI_DHCP6_IA    *RelIa
  )
{
  EFI_STATUS        Status;
  EFI_DHCP6_PACKET  *Packet;
  EFI_DHCP6_PACKET  *LastReply;
  EFI_DHCP6_DUID    *ClientId;
  EFI_DHCP6_DUID    *ServerId;
  DHCP6_SERVICE     *Service;
  UINT8             *Option;
  UINT8             *Cursor;
  UINT16            *Elapsed;
  UINT16            Length;

  ASSERT (Instance->Config);
  ASSERT (Instance->IaCb.Ia);

  Service   = Instance->Service;
  ClientId  = Service->ClientId;
  LastReply = Instance->IaCb.Ia->ReplyPacket;

  ASSERT (ClientId);
  ASSERT (LastReply);

  //
  // Get the server Id from the last reply message.
  //
  Option = Dhcp6SeekOption (
             LastReply->Dhcp6.Option,
             LastReply->Length - sizeof (EFI_DHCP6_HEADER),
             Dhcp6OptServerId
             );
  if (Option == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ServerId = (EFI_DHCP6_DUID *)(Option + 2);

  //
  // Create the Dhcp6 packet and initialize common fields.
  //
  Packet = AllocateZeroPool (DHCP6_BASE_PACKET_SIZE);
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Packet->Size                       = DHCP6_BASE_PACKET_SIZE;
  Packet->Length                     = sizeof (EFI_DHCP6_HEADER);
  Packet->Dhcp6.Header.MessageType   = Dhcp6MsgRelease;
  Packet->Dhcp6.Header.TransactionId = Service->Xid++;

  //
  // Assembly Dhcp6 options for rebind/renew message
  //
  Cursor = Packet->Dhcp6.Option;

  Length = HTONS (ClientId->Length);
  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptClientId),
             Length,
             ClientId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // ServerId is extracted from packet, it's network order.
  //
  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptServerId),
             ServerId->Length,
             ServerId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendETOption (
             Packet,
             &Cursor,
             Instance,
             &Elapsed
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendIaOption (
             Packet,
             &Cursor,
             RelIa,
             0,
             0,
             Packet->Dhcp6.Header.MessageType
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  ASSERT (Packet->Size > Packet->Length + 8);

  //
  // Callback to user with the packet to be sent and check the user's feedback.
  //
  Status = Dhcp6CallbackUser (Instance, Dhcp6SendRelease, &Packet);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Send release packet with the state transition from Dhcp6bound to
  // Dhcp6releasing.
  //
  Instance->IaCb.Ia->State = Dhcp6Releasing;

  Status = Dhcp6TransmitPacket (Instance, Packet, Elapsed);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Enqueue the sent packet for the retransmission in case reply timeout.
  //
  return Dhcp6EnqueueRetry (Instance, Packet, Elapsed, NULL);

ON_ERROR:

  if (Packet) {
    FreePool (Packet);
  }

  return Status;
}

/**
  Create the renew/rebind message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  RebindRequest   If TRUE, it is a Rebind type message.
                              Otherwise, it is a Renew type message.

  @retval EFI_SUCCESS           Created and sent the renew/rebind message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the renew/rebind message.

**/
EFI_STATUS
Dhcp6SendRenewRebindMsg (
  IN DHCP6_INSTANCE  *Instance,
  IN BOOLEAN         RebindRequest
  )
{
  EFI_STATUS               Status;
  EFI_DHCP6_PACKET         *Packet;
  EFI_DHCP6_PACKET         *LastReply;
  EFI_DHCP6_PACKET_OPTION  *UserOpt;
  EFI_DHCP6_DUID           *ClientId;
  EFI_DHCP6_DUID           *ServerId;
  EFI_DHCP6_STATE          State;
  EFI_DHCP6_EVENT          Event;
  DHCP6_SERVICE            *Service;
  UINT8                    *Option;
  UINT8                    *Cursor;
  UINT16                   *Elapsed;
  UINT32                   UserLen;
  UINTN                    Index;
  UINT16                   Length;

  ASSERT (Instance->Config);
  ASSERT (Instance->IaCb.Ia);

  Service  = Instance->Service;
  ClientId = Service->ClientId;

  ASSERT (ClientId);

  //
  // Calculate the added length of customized option list.
  //
  UserLen = 0;
  for (Index = 0; Index < Instance->Config->OptionCount; Index++) {
    UserLen += (NTOHS (Instance->Config->OptionList[Index]->OpLen) + 4);
  }

  //
  // Create the Dhcp6 packet and initialize common fields.
  //
  Packet = AllocateZeroPool (DHCP6_BASE_PACKET_SIZE + UserLen);
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Packet->Size                       = DHCP6_BASE_PACKET_SIZE + UserLen;
  Packet->Length                     = sizeof (EFI_DHCP6_HEADER);
  Packet->Dhcp6.Header.MessageType   = RebindRequest ? Dhcp6MsgRebind : Dhcp6MsgRenew;
  Packet->Dhcp6.Header.TransactionId = Service->Xid++;

  //
  // Assembly Dhcp6 options for rebind/renew message.
  //
  Cursor = Packet->Dhcp6.Option;

  Length = HTONS (ClientId->Length);
  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptClientId),
             Length,
             ClientId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendETOption (
             Packet,
             &Cursor,
             Instance,
             &Elapsed
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendIaOption (
             Packet,
             &Cursor,
             Instance->IaCb.Ia,
             Instance->IaCb.T1,
             Instance->IaCb.T2,
             Packet->Dhcp6.Header.MessageType
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  if (!RebindRequest) {
    //
    // Get the server Id from the last reply message and
    // insert it for rebind request.
    //
    LastReply = Instance->IaCb.Ia->ReplyPacket;
    ASSERT (LastReply);

    Option = Dhcp6SeekOption (
               LastReply->Dhcp6.Option,
               LastReply->Length - sizeof (EFI_DHCP6_HEADER),
               Dhcp6OptServerId
               );
    if (Option == NULL) {
      Status = EFI_DEVICE_ERROR;
      goto ON_ERROR;
    }

    ServerId = (EFI_DHCP6_DUID *)(Option + 2);

    Status = Dhcp6AppendOption (
               Packet,
               &Cursor,
               HTONS (Dhcp6OptServerId),
               ServerId->Length,
               ServerId->Duid
               );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  //
  // Append user-defined when configurate Dhcp6 service.
  //
  for (Index = 0; Index < Instance->Config->OptionCount; Index++) {
    UserOpt = Instance->Config->OptionList[Index];
    Status  = Dhcp6AppendOption (
                Packet,
                &Cursor,
                UserOpt->OpCode,
                UserOpt->OpLen,
                UserOpt->Data
                );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  ASSERT (Packet->Size > Packet->Length + 8);

  //
  // Callback to user with the packet to be sent and check the user's feedback.
  //
  State = (RebindRequest) ? Dhcp6Rebinding : Dhcp6Renewing;
  Event = (RebindRequest) ? Dhcp6EnterRebinding : Dhcp6EnterRenewing;

  Status = Dhcp6CallbackUser (Instance, Event, &Packet);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Send renew/rebind packet with the state transition from Dhcp6bound to
  // Dhcp6renew/rebind.
  // And sync the lease time when send renew/rebind, in case that user send
  // renew/rebind actively.
  //
  Instance->IaCb.Ia->State = State;
  Instance->IaCb.LeaseTime = (RebindRequest) ? Instance->IaCb.T2 : Instance->IaCb.T1;
  //
  // Clear initial time for current transaction.
  //
  Instance->StartTime = 0;

  Status = Dhcp6TransmitPacket (Instance, Packet, Elapsed);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Enqueue the sent packet for the retransmission in case reply timeout.
  //
  return Dhcp6EnqueueRetry (Instance, Packet, Elapsed, NULL);

ON_ERROR:

  if (Packet) {
    FreePool (Packet);
  }

  return Status;
}

/**
  Start the information request process.

  @param[in]  Instance          The pointer to the Dhcp6 instance.
  @param[in]  SendClientId      If TRUE, the client identifier option will be included in
                                information request message. Otherwise, the client identifier
                                option will not be included.
  @param[in]  OptionRequest     The pointer to the option request option.
  @param[in]  OptionCount       The number options in the OptionList.
  @param[in]  OptionList        The array pointers to the appended options.
  @param[in]  Retransmission    The pointer to the retransmission control.
  @param[in]  TimeoutEvent      The event of timeout.
  @param[in]  ReplyCallback     The callback function when the reply was received.
  @param[in]  CallbackContext   The pointer to the parameter passed to the callback.

  @retval EFI_SUCCESS           Start the info-request process successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_NO_MAPPING        No source address is available for use.
  @retval Others                Failed to start the info-request process.

**/
EFI_STATUS
Dhcp6StartInfoRequest (
  IN DHCP6_INSTANCE            *Instance,
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
  EFI_STATUS     Status;
  DHCP6_INF_CB   *InfCb;
  DHCP6_SERVICE  *Service;
  EFI_TPL        OldTpl;

  Service = Instance->Service;

  OldTpl           = gBS->RaiseTPL (TPL_CALLBACK);
  Instance->UdpSts = EFI_ALREADY_STARTED;
  //
  // Create and initialize the control block for the info-request.
  //
  InfCb = AllocateZeroPool (sizeof (DHCP6_INF_CB));

  if (InfCb == NULL) {
    gBS->RestoreTPL (OldTpl);
    return EFI_OUT_OF_RESOURCES;
  }

  InfCb->ReplyCallback   = ReplyCallback;
  InfCb->CallbackContext = CallbackContext;
  InfCb->TimeoutEvent    = TimeoutEvent;

  InsertTailList (&Instance->InfList, &InfCb->Link);

  //
  // Send the info-request message to start exchange process.
  //
  Status = Dhcp6SendInfoRequestMsg (
             Instance,
             InfCb,
             SendClientId,
             OptionRequest,
             OptionCount,
             OptionList,
             Retransmission
             );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Register receive callback for the stateless exchange process.
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
  return EFI_SUCCESS;

ON_ERROR:
  gBS->RestoreTPL (OldTpl);
  RemoveEntryList (&InfCb->Link);
  FreePool (InfCb);

  return Status;
}

/**
  Create the information request message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  InfCb           The pointer to the information request control block.
  @param[in]  SendClientId    If TRUE, the client identifier option will be included in
                              information request message. Otherwise, the client identifier
                              option will not be included.
  @param[in]  OptionRequest   The pointer to the option request option.
  @param[in]  OptionCount     The number options in the OptionList.
  @param[in]  OptionList      The array pointers to the appended options.
  @param[in]  Retransmission  The pointer to the retransmission control.

  @retval EFI_SUCCESS           Created and sent the info-request message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Failed to send the info-request message.

**/
EFI_STATUS
Dhcp6SendInfoRequestMsg (
  IN DHCP6_INSTANCE            *Instance,
  IN DHCP6_INF_CB              *InfCb,
  IN BOOLEAN                   SendClientId,
  IN EFI_DHCP6_PACKET_OPTION   *OptionRequest,
  IN UINT32                    OptionCount,
  IN EFI_DHCP6_PACKET_OPTION   *OptionList[],
  IN EFI_DHCP6_RETRANSMISSION  *Retransmission
  )
{
  EFI_STATUS               Status;
  EFI_DHCP6_PACKET         *Packet;
  EFI_DHCP6_PACKET_OPTION  *UserOpt;
  EFI_DHCP6_DUID           *ClientId;
  DHCP6_SERVICE            *Service;
  UINT8                    *Cursor;
  UINT16                   *Elapsed;
  UINT32                   UserLen;
  UINTN                    Index;
  UINT16                   Length;

  ASSERT (OptionRequest);

  Service  = Instance->Service;
  ClientId = Service->ClientId;
  UserLen  = NTOHS (OptionRequest->OpLen) + 4;

  ASSERT (ClientId);

  //
  // Calculate the added length of customized option list.
  //
  for (Index = 0; Index < OptionCount; Index++) {
    UserLen += (NTOHS (OptionList[Index]->OpLen) + 4);
  }

  //
  // Create the Dhcp6 packet and initialize common fields.
  //
  Packet = AllocateZeroPool (DHCP6_BASE_PACKET_SIZE + UserLen);
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Packet->Size                       = DHCP6_BASE_PACKET_SIZE + UserLen;
  Packet->Length                     = sizeof (EFI_DHCP6_HEADER);
  Packet->Dhcp6.Header.MessageType   = Dhcp6MsgInfoRequest;
  Packet->Dhcp6.Header.TransactionId = Service->Xid++;

  InfCb->Xid = Packet->Dhcp6.Header.TransactionId;

  //
  // Assembly Dhcp6 options for info-request message.
  //
  Cursor = Packet->Dhcp6.Option;

  if (SendClientId) {
    Length = HTONS (ClientId->Length);
    Status = Dhcp6AppendOption (
               Packet,
               &Cursor,
               HTONS (Dhcp6OptClientId),
               Length,
               ClientId->Duid
               );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  Status = Dhcp6AppendETOption (
             Packet,
             &Cursor,
             Instance,
             &Elapsed
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             OptionRequest->OpCode,
             OptionRequest->OpLen,
             OptionRequest->Data
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Append user-defined when configurate Dhcp6 service.
  //
  for (Index = 0; Index < OptionCount; Index++) {
    UserOpt = OptionList[Index];
    Status  = Dhcp6AppendOption (
                Packet,
                &Cursor,
                UserOpt->OpCode,
                UserOpt->OpLen,
                UserOpt->Data
                );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  ASSERT (Packet->Size > Packet->Length + 8);

  //
  // Clear initial time for current transaction.
  //
  Instance->StartTime = 0;

  //
  // Send info-request packet with no state.
  //
  Status = Dhcp6TransmitPacket (Instance, Packet, Elapsed);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Enqueue the sent packet for the retransmission in case reply timeout.
  //
  return Dhcp6EnqueueRetry (Instance, Packet, Elapsed, Retransmission);

ON_ERROR:

  if (Packet) {
    FreePool (Packet);
  }

  return Status;
}

/**
  Create the Confirm message and send it.

  @param[in]  Instance          The pointer to the Dhcp6 instance.

  @retval EFI_SUCCESS           Created and sent the confirm message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the confirm message.

**/
EFI_STATUS
Dhcp6SendConfirmMsg (
  IN DHCP6_INSTANCE  *Instance
  )
{
  UINT8                    *Cursor;
  UINTN                    Index;
  UINT16                   Length;
  UINT32                   UserLen;
  EFI_STATUS               Status;
  DHCP6_SERVICE            *Service;
  EFI_DHCP6_DUID           *ClientId;
  EFI_DHCP6_PACKET         *Packet;
  EFI_DHCP6_PACKET_OPTION  *UserOpt;
  UINT16                   *Elapsed;

  ASSERT (Instance->Config != NULL);
  ASSERT (Instance->IaCb.Ia != NULL);
  ASSERT (Instance->Service != NULL);

  Service  = Instance->Service;
  ClientId = Service->ClientId;
  ASSERT (ClientId != NULL);

  //
  // Calculate the added length of customized option list.
  //
  UserLen = 0;
  for (Index = 0; Index < Instance->Config->OptionCount; Index++) {
    UserLen += (NTOHS (Instance->Config->OptionList[Index]->OpLen) + 4);
  }

  //
  // Create the Dhcp6 packet and initialize common fields.
  //
  Packet = AllocateZeroPool (DHCP6_BASE_PACKET_SIZE + UserLen);
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Packet->Size                       = DHCP6_BASE_PACKET_SIZE + UserLen;
  Packet->Length                     = sizeof (EFI_DHCP6_HEADER);
  Packet->Dhcp6.Header.MessageType   = Dhcp6MsgConfirm;
  Packet->Dhcp6.Header.TransactionId = Service->Xid++;

  //
  // Assembly Dhcp6 options for solicit message.
  //
  Cursor = Packet->Dhcp6.Option;

  Length = HTONS (ClientId->Length);
  Status = Dhcp6AppendOption (
             Packet,
             &Cursor,
             HTONS (Dhcp6OptClientId),
             Length,
             ClientId->Duid
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendETOption (
             Packet,
             &Cursor,
             Instance,
             &Elapsed
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp6AppendIaOption (
             Packet,
             &Cursor,
             Instance->IaCb.Ia,
             Instance->IaCb.T1,
             Instance->IaCb.T2,
             Packet->Dhcp6.Header.MessageType
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Append user-defined when configurate Dhcp6 service.
  //
  for (Index = 0; Index < Instance->Config->OptionCount; Index++) {
    UserOpt = Instance->Config->OptionList[Index];
    Status  = Dhcp6AppendOption (
                Packet,
                &Cursor,
                UserOpt->OpCode,
                UserOpt->OpLen,
                UserOpt->Data
                );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  ASSERT (Packet->Size > Packet->Length + 8);

  //
  // Callback to user with the packet to be sent and check the user's feedback.
  //
  Status = Dhcp6CallbackUser (Instance, Dhcp6SendConfirm, &Packet);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Send confirm packet with the state transition from Dhcp6Bound to
  // Dhcp6Confirming.
  //
  Instance->IaCb.Ia->State = Dhcp6Confirming;
  //
  // Clear initial time for current transaction.
  //
  Instance->StartTime = 0;

  Status = Dhcp6TransmitPacket (Instance, Packet, Elapsed);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Enqueue the sent packet for the retransmission in case reply timeout.
  //
  return Dhcp6EnqueueRetry (Instance, Packet, Elapsed, NULL);

ON_ERROR:

  if (Packet) {
    FreePool (Packet);
  }

  return Status;
}

/**
  Handle with the Dhcp6 reply message.

  @param[in]  Instance        The pointer to Dhcp6 instance.
  @param[in]  Packet          The pointer to the Dhcp6 reply message.

  @retval EFI_SUCCESS           Processed the reply message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to process the reply message.

**/
EFI_STATUS
Dhcp6HandleReplyMsg (
  IN DHCP6_INSTANCE    *Instance,
  IN EFI_DHCP6_PACKET  *Packet
  )
{
  EFI_STATUS  Status;
  UINT8       *Option;
  UINT16      StsCode;

  ASSERT (Instance->Config != NULL);
  ASSERT (Instance->IaCb.Ia != NULL);
  ASSERT (Packet != NULL);

  Status = EFI_SUCCESS;

  if (Packet->Dhcp6.Header.MessageType != Dhcp6MsgReply) {
    return EFI_DEVICE_ERROR;
  }

  //
  // If the client subsequently receives a valid reply message that includes a
  // rapid commit option since send a solicit with rapid commit option before,
  // preocess the reply message and discard any reply messages received in
  // response to the request message.
  // See details in the section-17.1.4 of rfc-3315.
  //
  Option = Dhcp6SeekOption (
             Packet->Dhcp6.Option,
             Packet->Length - sizeof (EFI_DHCP6_HEADER),
             Dhcp6OptRapidCommit
             );

  if (((Option != NULL) && !Instance->Config->RapidCommit) || ((Option == NULL) && Instance->Config->RapidCommit)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // As to a valid reply packet in response to a request/renew/rebind packet,
  // ignore the packet if not contains the Ia option
  //
  if ((Instance->IaCb.Ia->State == Dhcp6Requesting) ||
      (Instance->IaCb.Ia->State == Dhcp6Renewing) ||
      (Instance->IaCb.Ia->State == Dhcp6Rebinding)
      )
  {
    Option = Dhcp6SeekIaOption (
               Packet->Dhcp6.Option,
               Packet->Length,
               &Instance->Config->IaDescriptor
               );
    if (Option == NULL) {
      return EFI_SUCCESS;
    }
  }

  //
  // Callback to user with the received packet and check the user's feedback.
  //
  Status = Dhcp6CallbackUser (Instance, Dhcp6RcvdReply, &Packet);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // When receive a valid reply packet in response to a decline/release packet,
  // the client considers the decline/release event completed regardless of the
  // status code.
  //
  if ((Instance->IaCb.Ia->State == Dhcp6Declining) || (Instance->IaCb.Ia->State == Dhcp6Releasing)) {
    if (Instance->IaCb.Ia->IaAddressCount != 0) {
      Instance->IaCb.Ia->State = Dhcp6Bound;
    } else {
      ASSERT (Instance->IaCb.Ia->ReplyPacket);
      FreePool (Instance->IaCb.Ia->ReplyPacket);
      Instance->IaCb.Ia->ReplyPacket = NULL;
      Instance->IaCb.Ia->State       = Dhcp6Init;
    }

    //
    // For sync, set the success flag out of polling in decline/release.
    //
    Instance->UdpSts = EFI_SUCCESS;

    //
    // For async, signal the Ia event to inform Ia information update.
    //
    if (Instance->Config->IaInfoEvent != NULL) {
      gBS->SignalEvent (Instance->Config->IaInfoEvent);
    }

    //
    // Reset start time for next exchange.
    //
    Instance->StartTime = 0;

    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Upon the receipt of a valid reply packet in response to a solicit, request,
  // confirm, renew and rebind, the behavior depends on the status code option.
  // See the details in the section-18.1.8 of rfc-3315.
  //
  Option = NULL;
  Status = Dhcp6SeekStsOption (
             Instance,
             Packet,
             &Option
             );

  if (!EFI_ERROR (Status)) {
    //
    // No status code or no error status code means succeed to reply.
    //
    Status = Dhcp6UpdateIaInfo (Instance, Packet);
    if (!EFI_ERROR (Status)) {
      //
      // Reset start time for next exchange.
      //
      Instance->StartTime = 0;

      //
      // Set bound state and store the reply packet.
      //
      if (Instance->IaCb.Ia->ReplyPacket != NULL) {
        FreePool (Instance->IaCb.Ia->ReplyPacket);
      }

      Instance->IaCb.Ia->ReplyPacket = AllocateZeroPool (Packet->Size);

      if (Instance->IaCb.Ia->ReplyPacket == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }

      CopyMem (Instance->IaCb.Ia->ReplyPacket, Packet, Packet->Size);

      Instance->IaCb.Ia->State = Dhcp6Bound;

      //
      // For sync, set the success flag out of polling in start/renewrebind.
      //
      Instance->UdpSts = EFI_SUCCESS;

      //
      // Maybe this is a new round DHCP process due to some reason, such as NotOnLink
      // ReplyMsg for ConfirmMsg should trigger new round to acquire new address. In that
      // case, clear old address.ValidLifetime and append to new address. Therefore, DHCP
      // consumers can be notified to flush old address.
      //
      Dhcp6AppendCacheIa (Instance);

      //
      // For async, signal the Ia event to inform Ia information update.
      //
      if (Instance->Config->IaInfoEvent != NULL) {
        gBS->SignalEvent (Instance->Config->IaInfoEvent);
      }
    } else if (Status == EFI_NOT_FOUND) {
      //
      // Refer to RFC3315 Chapter 18.1.8, for each IA in the original Renew or Rebind message,
      // the client sends a Renew or Rebind if the IA is not in the Reply message.
      // Return EFI_SUCCESS so we can continue to restart the Renew/Rebind process.
      //
      return EFI_SUCCESS;
    }

    goto ON_EXIT;
  } else if (Option != NULL) {
    //
    // Any error status code option is found.
    //
    StsCode = NTOHS (ReadUnaligned16 ((UINT16 *)((DHCP6_OFFSET_OF_STATUS_CODE (Option)))));
    switch (StsCode) {
      case Dhcp6StsUnspecFail:
        //
        // It indicates the server is unable to process the message due to an
        // unspecified failure condition, so just retry if possible.
        //
        break;

      case Dhcp6StsUseMulticast:
        //
        // It indicates the server receives a message via unicast from a client
        // to which the server has not sent a unicast option, so retry it by
        // multi-cast address.
        //
        if (Instance->Unicast != NULL) {
          FreePool (Instance->Unicast);
          Instance->Unicast = NULL;
        }

        break;

      case Dhcp6StsNotOnLink:
        if (Instance->IaCb.Ia->State == Dhcp6Confirming) {
          //
          // Before initiate new round DHCP, cache the current IA.
          //
          Status = Dhcp6CacheIa (Instance);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          //
          // Restart S.A.R.R process to acquire new address.
          //
          Status = Dhcp6InitSolicitMsg (Instance);
          if (EFI_ERROR (Status)) {
            return Status;
          }
        }

        break;

      case Dhcp6StsNoBinding:
        if ((Instance->IaCb.Ia->State == Dhcp6Renewing) || (Instance->IaCb.Ia->State == Dhcp6Rebinding)) {
          //
          // Refer to RFC3315 Chapter 18.1.8, for each IA in the original Renew or Rebind message, the client
          // sends a Request message if the IA contained a Status Code option with the NoBinding status.
          //
          Status = Dhcp6SendRequestMsg (Instance);
          if (EFI_ERROR (Status)) {
            return Status;
          }
        }

        break;

      default:
        //
        // The other status code, just restart solicitation.
        //
        break;
    }
  }

  return EFI_SUCCESS;

ON_EXIT:

  if (!EFI_ERROR (Status)) {
    Status = Dhcp6DequeueRetry (
               Instance,
               Packet->Dhcp6.Header.TransactionId,
               FALSE
               );
  }

  return Status;
}

/**
  Select the appointed Dhcp6 advertisement message.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  AdSelect        The pointer to the selected Dhcp6 advertisement message.

  @retval EFI_SUCCESS           Selected the right advertisement message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Failed to select the advertise message.

**/
EFI_STATUS
Dhcp6SelectAdvertiseMsg (
  IN DHCP6_INSTANCE    *Instance,
  IN EFI_DHCP6_PACKET  *AdSelect
  )
{
  EFI_STATUS  Status;
  UINT8       *Option;

  ASSERT (AdSelect != NULL);

  //
  // Callback to user with the selected advertisement packet, and the user
  // might overwrite it.
  //
  Status = Dhcp6CallbackUser (Instance, Dhcp6SelectAdvertise, &AdSelect);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Instance->AdSelect = AdSelect;

  //
  // Dequeue the sent packet for the retransmission since advertisement selected.
  //
  Status = Dhcp6DequeueRetry (
             Instance,
             AdSelect->Dhcp6.Header.TransactionId,
             FALSE
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check whether there is server unicast option in the selected advertise
  // packet, and update it.
  //
  Option = Dhcp6SeekOption (
             AdSelect->Dhcp6.Option,
             AdSelect->Length - sizeof (EFI_DHCP6_HEADER),
             Dhcp6OptServerUnicast
             );

  if (Option != NULL) {
    Instance->Unicast = AllocateZeroPool (sizeof (EFI_IPv6_ADDRESS));

    if (Instance->Unicast == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Instance->Unicast, DHCP6_OFFSET_OF_OPT_DATA (Option), sizeof (EFI_IPv6_ADDRESS));
  }

  //
  // Update the information of the Ia by the selected advertisement message.
  //
  Status = Dhcp6UpdateIaInfo (Instance, AdSelect);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Send the request message to continue the S.A.R.R. process.
  //
  return Dhcp6SendRequestMsg (Instance);
}

/**
  Handle with the Dhcp6 advertisement message.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  Packet          The pointer to the Dhcp6 advertisement message.

  @retval EFI_SUCCESS           Processed the advertisement message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to process the advertise message.

**/
EFI_STATUS
Dhcp6HandleAdvertiseMsg (
  IN DHCP6_INSTANCE    *Instance,
  IN EFI_DHCP6_PACKET  *Packet
  )
{
  EFI_STATUS  Status;
  UINT8       *Option;
  BOOLEAN     Timeout;

  ASSERT (Instance->Config);
  ASSERT (Instance->IaCb.Ia);

  Timeout = FALSE;

  //
  // If the client does receives a valid reply message that includes a rapid
  // commit option since a solicit with rapid commit option sent before, select
  // this reply message. Or else, process the advertise messages as normal.
  // See details in the section-17.1.4 of rfc-3315.
  //
  Option = Dhcp6SeekOption (
             Packet->Dhcp6.Option,
             Packet->Length - sizeof (EFI_DHCP6_HEADER),
             Dhcp6OptRapidCommit
             );

  if ((Option != NULL) && Instance->Config->RapidCommit && (Packet->Dhcp6.Header.MessageType == Dhcp6MsgReply)) {
    return Dhcp6HandleReplyMsg (Instance, Packet);
  }

  if (Packet->Dhcp6.Header.MessageType != Dhcp6MsgAdvertise) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Client must ignore any advertise message that includes a status code option
  // containing the value noaddrsavail, with the exception that the client may
  // display the associated status message to the user.
  // See the details in the section-17.1.3 of rfc-3315.
  //
  Status = Dhcp6SeekStsOption (
             Instance,
             Packet,
             &Option
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Callback to user with the received packet and check the user's feedback.
  //
  Status = Dhcp6CallbackUser (Instance, Dhcp6RcvdAdvertise, &Packet);

  if (!EFI_ERROR (Status)) {
    //
    // Success means user choose the current advertisement packet.
    //
    if (Instance->AdSelect != NULL) {
      FreePool (Instance->AdSelect);
    }

    //
    // Store the selected advertisement packet and set a flag.
    //
    Instance->AdSelect = AllocateZeroPool (Packet->Size);

    if (Instance->AdSelect == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Instance->AdSelect, Packet, Packet->Size);

    Instance->AdPref = 0xff;
  } else if (Status == EFI_NOT_READY) {
    //
    // Not_ready means user wants to continue to receive more advertise packets.
    //
    if ((Instance->AdPref == 0xff) && (Instance->AdSelect == NULL)) {
      //
      // It's a tricky point. The timer routine set adpref as 0xff if the first
      // rt timeout and no advertisement received, which means any advertisement
      // received will be selected after the first rt.
      //
      Timeout = TRUE;
    }

    //
    // Check whether the current packet has a 255 preference option or not.
    // Take non-preference option as 0 value.
    //
    Option = Dhcp6SeekOption (
               Packet->Dhcp6.Option,
               Packet->Length - 4,
               Dhcp6OptPreference
               );

    if ((Instance->AdSelect == NULL) || ((Option != NULL) && (*(Option + 4) > Instance->AdPref))) {
      //
      // No advertisements received before or preference is more than other
      // advertisements received before. Then store the new packet and the
      // preference value.
      //
      if (Instance->AdSelect != NULL) {
        FreePool (Instance->AdSelect);
      }

      Instance->AdSelect = AllocateZeroPool (Packet->Size);

      if (Instance->AdSelect == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (Instance->AdSelect, Packet, Packet->Size);

      if (Option != NULL) {
        Instance->AdPref = *(DHCP6_OFFSET_OF_OPT_DATA (Option));
      }
    } else {
      //
      // Non-preference and other advertisements received before or current
      // preference is less than other advertisements received before.
      // Leave the packet alone.
    }
  } else {
    //
    // Other error status means termination.
    //
    return Status;
  }

  //
  // Client must collect advertise messages as more as possible until the first
  // RT has elapsed, or get a highest preference 255 advertise.
  // See details in the section-17.1.2 of rfc-3315.
  //
  if ((Instance->AdPref == 0xff) || Timeout) {
    Status = Dhcp6SelectAdvertiseMsg (Instance, Instance->AdSelect);
  }

  return Status;
}

/**
  The Dhcp6 stateful exchange process routine.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  Packet          The pointer to the received Dhcp6 message.

**/
VOID
Dhcp6HandleStateful (
  IN DHCP6_INSTANCE    *Instance,
  IN EFI_DHCP6_PACKET  *Packet
  )
{
  EFI_STATUS      Status;
  EFI_DHCP6_DUID  *ClientId;
  DHCP6_SERVICE   *Service;
  UINT8           *Option;

  Service  = Instance->Service;
  ClientId = Service->ClientId;
  Status   = EFI_SUCCESS;

  if (Instance->Config == NULL) {
    goto ON_CONTINUE;
  }

  ASSERT (ClientId);
  ASSERT (Instance->Config);
  ASSERT (Instance->IaCb.Ia);

  //
  // Discard the packet if not advertisement or reply packet.
  //
  if ((Packet->Dhcp6.Header.MessageType != Dhcp6MsgAdvertise) && (Packet->Dhcp6.Header.MessageType != Dhcp6MsgReply)) {
    goto ON_CONTINUE;
  }

  //
  // Check whether include client Id or not.
  //
  Option = Dhcp6SeekOption (
             Packet->Dhcp6.Option,
             Packet->Length - DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN,
             Dhcp6OptClientId
             );

  if ((Option == NULL) || (CompareMem (DHCP6_OFFSET_OF_OPT_DATA (Option), ClientId->Duid, ClientId->Length) != 0)) {
    goto ON_CONTINUE;
  }

  //
  // Check whether include server Id or not.
  //
  Option = Dhcp6SeekOption (
             Packet->Dhcp6.Option,
             Packet->Length - DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN,
             Dhcp6OptServerId
             );

  if (Option == NULL) {
    goto ON_CONTINUE;
  }

  switch (Instance->IaCb.Ia->State) {
    case Dhcp6Selecting:
      //
      // Handle the advertisement message when in the Dhcp6Selecting state.
      // Do not need check return status, if failed, just continue to the next.
      //
      Dhcp6HandleAdvertiseMsg (Instance, Packet);
      break;

    case Dhcp6Requesting:
    case Dhcp6Confirming:
    case Dhcp6Renewing:
    case Dhcp6Rebinding:
    case Dhcp6Releasing:
    case Dhcp6Declining:
      //
      // Handle the reply message when in the Dhcp6Requesting,  Dhcp6Renewing
      // Dhcp6Rebinding, Dhcp6Releasing and Dhcp6Declining state.
      // If failed here, it should reset the current session.
      //
      Status = Dhcp6HandleReplyMsg (Instance, Packet);
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      break;
    default:
      //
      // Other state has not supported yet.
      //
      break;
  }

ON_CONTINUE:
  //
  // Continue to receive the following Dhcp6 message.
  //
  Status = UdpIoRecvDatagram (
             Service->UdpIo,
             Dhcp6ReceivePacket,
             Service,
             0
             );
ON_EXIT:
  if (EFI_ERROR (Status)) {
    Dhcp6CleanupSession (Instance, Status);
  }
}

/**
  The Dhcp6 stateless exchange process routine.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  Packet          The pointer to the received Dhcp6 message.

**/
VOID
Dhcp6HandleStateless (
  IN DHCP6_INSTANCE    *Instance,
  IN EFI_DHCP6_PACKET  *Packet
  )
{
  EFI_STATUS     Status;
  DHCP6_SERVICE  *Service;
  DHCP6_INF_CB   *InfCb;
  UINT8          *Option;
  BOOLEAN        IsMatched;

  Service   = Instance->Service;
  Status    = EFI_SUCCESS;
  IsMatched = FALSE;
  InfCb     = NULL;

  if (Packet->Dhcp6.Header.MessageType != Dhcp6MsgReply) {
    goto ON_EXIT;
  }

  //
  // Check whether it's a desired Info-request message by Xid.
  //
  while (!IsListEmpty (&Instance->InfList)) {
    InfCb = NET_LIST_HEAD (&Instance->InfList, DHCP6_INF_CB, Link);
    if (InfCb->Xid == Packet->Dhcp6.Header.TransactionId) {
      IsMatched = TRUE;
      break;
    }
  }

  if (!IsMatched) {
    goto ON_EXIT;
  }

  //
  // Check whether include server Id or not.
  //
  Option = Dhcp6SeekOption (
             Packet->Dhcp6.Option,
             Packet->Length - sizeof (EFI_DHCP6_HEADER),
             Dhcp6OptServerId
             );

  if (Option == NULL) {
    goto ON_EXIT;
  }

  //
  // Callback to user with the received packet and check the user's feedback.
  //
  Status = InfCb->ReplyCallback (
                    &Instance->Dhcp6,
                    InfCb->CallbackContext,
                    Packet
                    );

  if (Status == EFI_NOT_READY) {
    //
    // Success or aborted will both stop this info-request exchange process,
    // but not ready means user wants to continue to receive reply.
    //
    goto ON_EXIT;
  }

  //
  // Dequeue the sent packet from the txlist if the xid matched, and ignore
  // if no xid matched.
  //
  Dhcp6DequeueRetry (
    Instance,
    Packet->Dhcp6.Header.TransactionId,
    FALSE
    );

  //
  // For sync, set the status out of polling for info-request.
  //
  Instance->UdpSts = Status;

ON_EXIT:

  Status = UdpIoRecvDatagram (
             Service->UdpIo,
             Dhcp6ReceivePacket,
             Service,
             0
             );

  if (EFI_ERROR (Status)) {
    Dhcp6CleanupRetry (Instance, DHCP6_PACKET_STATELESS);
  }
}

/**
  The receive callback function for Dhcp6 exchange process.

  @param[in]  Udp6Wrap        The pointer to the received net buffer.
  @param[in]  EndPoint        The pointer to the udp end point.
  @param[in]  IoStatus        The return status from udp io.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
Dhcp6ReceivePacket (
  IN NET_BUF        *Udp6Wrap,
  IN UDP_END_POINT  *EndPoint,
  IN EFI_STATUS     IoStatus,
  IN VOID           *Context
  )
{
  EFI_DHCP6_HEADER  *Head;
  EFI_DHCP6_PACKET  *Packet;
  DHCP6_SERVICE     *Service;
  DHCP6_INSTANCE    *Instance;
  DHCP6_TX_CB       *TxCb;
  UINT32            Size;
  BOOLEAN           IsDispatched;
  BOOLEAN           IsStateless;
  LIST_ENTRY        *Entry1;
  LIST_ENTRY        *Next1;
  LIST_ENTRY        *Entry2;
  LIST_ENTRY        *Next2;
  EFI_STATUS        Status;

  ASSERT (Udp6Wrap != NULL);
  ASSERT (Context != NULL);

  Service      = (DHCP6_SERVICE *)Context;
  Instance     = NULL;
  Packet       = NULL;
  IsDispatched = FALSE;
  IsStateless  = FALSE;

  if (EFI_ERROR (IoStatus)) {
    return;
  }

  if (Udp6Wrap->TotalSize < sizeof (EFI_DHCP6_HEADER)) {
    goto ON_CONTINUE;
  }

  //
  // Copy the net buffer received from upd6 to a Dhcp6 packet.
  //
  Size   = sizeof (EFI_DHCP6_PACKET) + Udp6Wrap->TotalSize;
  Packet = (EFI_DHCP6_PACKET *)AllocateZeroPool (Size);

  if (Packet == NULL) {
    goto ON_CONTINUE;
  }

  Packet->Size   = Size;
  Head           = &Packet->Dhcp6.Header;
  Packet->Length = NetbufCopy (Udp6Wrap, 0, Udp6Wrap->TotalSize, (UINT8 *)Head);

  if (Packet->Length == 0) {
    goto ON_CONTINUE;
  }

  //
  // Dispatch packet to right instance by transaction id.
  //
  NET_LIST_FOR_EACH_SAFE (Entry1, Next1, &Service->Child) {
    Instance = NET_LIST_USER_STRUCT (Entry1, DHCP6_INSTANCE, Link);

    NET_LIST_FOR_EACH_SAFE (Entry2, Next2, &Instance->TxList) {
      TxCb = NET_LIST_USER_STRUCT (Entry2, DHCP6_TX_CB, Link);

      if (Packet->Dhcp6.Header.TransactionId == TxCb->Xid) {
        //
        // Find the corresponding packet in tx list, and check it whether belongs
        // to stateful exchange process.
        //
        if (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgInfoRequest) {
          IsStateless = TRUE;
        }

        IsDispatched = TRUE;
        break;
      }
    }

    if (IsDispatched) {
      break;
    }
  }

  //
  // Skip this packet if not dispatched to any instance.
  //
  if (!IsDispatched) {
    goto ON_CONTINUE;
  }

  //
  // Dispatch the received packet ot the right instance.
  //
  if (IsStateless) {
    Dhcp6HandleStateless (Instance, Packet);
  } else {
    Dhcp6HandleStateful (Instance, Packet);
  }

ON_CONTINUE:

  if (!IsDispatched) {
    Status = UdpIoRecvDatagram (
               Service->UdpIo,
               Dhcp6ReceivePacket,
               Service,
               0
               );
    if (EFI_ERROR (Status)) {
      NET_LIST_FOR_EACH_SAFE (Entry1, Next1, &Service->Child) {
        Instance = NET_LIST_USER_STRUCT (Entry1, DHCP6_INSTANCE, Link);
        Dhcp6CleanupRetry (Instance, DHCP6_PACKET_ALL);
      }
    }
  }

  NetbufFree (Udp6Wrap);

  if (Packet != NULL) {
    FreePool (Packet);
  }
}

/**
  Detect Link movement for specified network device.

  This routine will try to invoke Snp->GetStatus() to get the media status.
  If media present status switches from unpresent to present, a link movement
  is detected. Note that the underlying UNDI driver may not support reporting
  media status from GET_STATUS command. If that, fail to detect link movement.

  @param[in]  Instance       The pointer to DHCP6_INSTANCE.

  @retval     TRUE           A link movement is detected.
  @retval     FALSE          A link movement is not detected.

**/
BOOLEAN
Dhcp6LinkMovDetect (
  IN  DHCP6_INSTANCE  *Instance
  )
{
  UINT32                       InterruptStatus;
  BOOLEAN                      MediaPresent;
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;

  ASSERT (Instance != NULL);
  Snp          = Instance->Service->Snp;
  MediaPresent = Instance->MediaPresent;

  //
  // Check whether SNP support media detection
  //
  if (!Snp->Mode->MediaPresentSupported) {
    return FALSE;
  }

  //
  // Invoke Snp->GetStatus() to refresh MediaPresent field in SNP mode data
  //
  Status = Snp->GetStatus (Snp, &InterruptStatus, NULL);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Instance->MediaPresent = Snp->Mode->MediaPresent;
  //
  // Media transimit Unpresent to Present means new link movement is detected.
  //
  if (!MediaPresent && Instance->MediaPresent) {
    return TRUE;
  }

  return FALSE;
}

/**
  The timer routine of the Dhcp6 instance for each second.

  @param[in]  Event           The timer event.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
Dhcp6OnTimerTick (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  LIST_ENTRY      *Entry;
  LIST_ENTRY      *NextEntry;
  DHCP6_INSTANCE  *Instance;
  DHCP6_TX_CB     *TxCb;
  DHCP6_IA_CB     *IaCb;
  UINT32          LossTime;
  EFI_STATUS      Status;

  ASSERT (Context != NULL);

  Instance = (DHCP6_INSTANCE *)Context;

  //
  // 1. Loop the tx list, count live time of every tx packet to check whether
  //    need re-transmit or not.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Instance->TxList) {
    TxCb = NET_LIST_USER_STRUCT (Entry, DHCP6_TX_CB, Link);

    TxCb->TickTime++;

    if (TxCb->TickTime > TxCb->RetryExp) {
      //
      // Handle the first rt in the transmission of solicit specially.
      //
      if (((TxCb->RetryCnt == 0) || TxCb->SolicitRetry) && (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgSolicit)) {
        if (Instance->AdSelect == NULL) {
          //
          // Set adpref as 0xff here to indicate select any advertisement
          // afterwards.
          //
          Instance->AdPref = 0xff;
        } else {
          //
          // Select the advertisement received before.
          //
          Status = Dhcp6SelectAdvertiseMsg (Instance, Instance->AdSelect);
          if (Status == EFI_ABORTED) {
            goto ON_CLOSE;
          } else if (EFI_ERROR (Status)) {
            TxCb->RetryCnt++;
          }

          return;
        }
      }

      //
      // Increase the retry count for the packet and add up the total loss time.
      //
      TxCb->RetryCnt++;
      TxCb->RetryLos += TxCb->RetryExp;

      //
      // Check whether overflow the max retry count limit for this packet
      //
      if ((TxCb->RetryCtl.Mrc != 0) && (TxCb->RetryCtl.Mrc < TxCb->RetryCnt)) {
        Status = EFI_NO_RESPONSE;
        goto ON_CLOSE;
      }

      //
      // Check whether overflow the max retry duration for this packet
      //
      if ((TxCb->RetryCtl.Mrd != 0) && (TxCb->RetryCtl.Mrd <= TxCb->RetryLos)) {
        Status = EFI_NO_RESPONSE;
        goto ON_CLOSE;
      }

      //
      // Re-calculate retry expire timeout for the next time.
      //
      // Firstly, Check the new calculated time whether overflow the max retry
      // expire time.
      //
      TxCb->RetryExp = Dhcp6CalculateExpireTime (
                         TxCb->RetryExp,
                         FALSE,
                         TRUE
                         );

      if ((TxCb->RetryCtl.Mrt != 0) && (TxCb->RetryCtl.Mrt < TxCb->RetryExp)) {
        TxCb->RetryExp = Dhcp6CalculateExpireTime (
                           TxCb->RetryCtl.Mrt,
                           TRUE,
                           TRUE
                           );
      }

      //
      // Secondly, Check the new calculated time whether overflow the max retry
      // duration time.
      //
      LossTime = TxCb->RetryLos + TxCb->RetryExp;
      if ((TxCb->RetryCtl.Mrd != 0) && (TxCb->RetryCtl.Mrd < LossTime)) {
        TxCb->RetryExp = TxCb->RetryCtl.Mrd - TxCb->RetryLos;
      }

      //
      // Reset the tick time for the next retransmission
      //
      TxCb->TickTime = 0;

      //
      // Retransmit the last sent packet again.
      //
      Dhcp6TransmitPacket (Instance, TxCb->TxPacket, TxCb->Elapsed);
      TxCb->SolicitRetry = FALSE;
      if (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgSolicit) {
        TxCb->SolicitRetry = TRUE;
      }
    }
  }

  //
  // 2. Check the configured Ia, count lease time of every valid Ia to check
  // whether need to renew or rebind this Ia.
  //
  IaCb = &Instance->IaCb;

  if ((Instance->Config == NULL) || (IaCb->Ia == NULL)) {
    return;
  }

  if ((IaCb->Ia->State == Dhcp6Bound) || (IaCb->Ia->State == Dhcp6Renewing) || (IaCb->Ia->State == Dhcp6Rebinding)) {
    IaCb->LeaseTime++;

    if ((IaCb->LeaseTime > IaCb->T2) && (IaCb->Ia->State == Dhcp6Bound)) {
      //
      // Exceed t2, send rebind packet to extend the Ia lease.
      //
      Dhcp6SendRenewRebindMsg (Instance, TRUE);
    } else if ((IaCb->LeaseTime > IaCb->T1) && (IaCb->Ia->State == Dhcp6Bound)) {
      //
      // Exceed t1, send renew packet to extend the Ia lease.
      //
      Dhcp6SendRenewRebindMsg (Instance, FALSE);
    }
  }

  //
  // 3. In any situation when a client may have moved to a new link, the
  //    client MUST initiate a Confirm/Reply message exchange.
  //
  if (Dhcp6LinkMovDetect (Instance) && (IaCb->Ia->State == Dhcp6Bound)) {
    Dhcp6SendConfirmMsg (Instance);
  }

  return;

ON_CLOSE:

  if (Dhcp6IsValidTxCb (Instance, TxCb) &&
      (TxCb->TxPacket != NULL) &&
      ((TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgInfoRequest) ||
       (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgRenew) ||
       (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgConfirm))
      )
  {
    //
    // The failure of renew/Confirm will still switch to the bound state.
    //
    if ((TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgRenew) ||
        (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgConfirm))
    {
      ASSERT (Instance->IaCb.Ia);
      Instance->IaCb.Ia->State = Dhcp6Bound;
    }

    //
    // The failure of info-request will return no response.
    //
    if (TxCb->TxPacket->Dhcp6.Header.MessageType == Dhcp6MsgInfoRequest) {
      Instance->UdpSts = EFI_NO_RESPONSE;
    }

    Dhcp6DequeueRetry (
      Instance,
      TxCb->Xid,
      TRUE
      );
  } else {
    //
    // The failure of the others will terminate current state machine if timeout.
    //
    Dhcp6CleanupSession (Instance, Status);
  }
}
