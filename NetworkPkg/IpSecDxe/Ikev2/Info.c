/** @file
  The Implementations for Information Exchange.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
  
**/

#include "Utility.h"
#include "IpSecDebug.h"
#include "IpSecConfigImpl.h"

/**
  Generate Information Packet.

  The information Packet may contain one Delete Payload, or Notify Payload, which 
  dependes on the Context's parameters.

  @param[in]  SaSession   Pointer to IKE SA Session or Child SA Session which is 
                          related to the information Exchange.
  @param[in]  Context     The Data passed from the caller. If the Context is not NULL
                          it should contain the information for Notification Data.
                          
  @retval     Pointer of IKE_PACKET generated.

**/
IKE_PACKET *
Ikev2InfoGenerator (
  IN UINT8                         *SaSession,
  IN VOID                          *Context
  )
{
  IKEV2_SA_SESSION            *IkeSaSession;
  IKEV2_CHILD_SA_SESSION      *ChildSaSession;
  IKE_PACKET                  *IkePacket;
  IKE_PAYLOAD                 *IkePayload;
  IKEV2_INFO_EXCHANGE_CONTEXT *InfoContext;

  InfoContext  = NULL;
  IkeSaSession = (IKEV2_SA_SESSION *) SaSession;
  IkePacket    = IkePacketAlloc ();
  ASSERT (IkePacket != NULL);

  //
  // Fill IkePacket Header.
  //
  IkePacket->Header->ExchangeType    = IKEV2_EXCHANGE_TYPE_INFO;
  IkePacket->Header->Version         = (UINT8) (2 << 4); 

  if (Context != NULL) {
    InfoContext = (IKEV2_INFO_EXCHANGE_CONTEXT *) Context;
  }

  //
  // For Liveness Check
  //
  if (InfoContext != NULL && 
      (InfoContext->InfoType == Ikev2InfoLiveCheck || InfoContext->InfoType == Ikev2InfoNotify) 
    ) {
    IkePacket->Header->MessageId       = InfoContext->MessageId;
    IkePacket->Header->InitiatorCookie = IkeSaSession->InitiatorCookie;
    IkePacket->Header->ResponderCookie = IkeSaSession->ResponderCookie;
    IkePacket->Header->NextPayload     = IKEV2_PAYLOAD_TYPE_NONE;
    IkePacket->Header->Flags           = IKE_HEADER_FLAGS_RESPOND;
    //
    // TODO: add Notify Payload for Notification Information.
    //
    return IkePacket;
  }
  
  //
  // For delete SAs
  //  
  if (IkeSaSession->SessionCommon.IkeSessionType == IkeSessionTypeIkeSa) {

    IkePacket->Header->InitiatorCookie = IkeSaSession->InitiatorCookie;
    IkePacket->Header->ResponderCookie = IkeSaSession->ResponderCookie;

    //
    // If the information message is response message,the MessageId should
    // be same as the request MessageId which passed through the Context.
    //
    if (InfoContext != NULL) {
      IkePacket->Header->MessageId     = InfoContext->MessageId;
    } else {
      IkePacket->Header->MessageId     = IkeSaSession->MessageId;
      Ikev2SaSessionIncreaseMessageId (IkeSaSession);
    }
    //
    // If the state is on deleting generate a Delete Payload for it.
    //
    if (IkeSaSession->SessionCommon.State == IkeStateSaDeleting ) {
      IkePayload = Ikev2GenerateDeletePayload (
                     IkeSaSession, 
                     IKEV2_PAYLOAD_TYPE_NONE, 
                     0, 
                     0, 
                     NULL
                     );  
      if (IkePayload == NULL) {
        goto ERROR_EXIT;
      }
      //
      // Fill the next payload in IkePacket's Header.
      //
      IkePacket->Header->NextPayload     = IKEV2_PAYLOAD_TYPE_DELETE;
      IKE_PACKET_APPEND_PAYLOAD (IkePacket, IkePayload);
      IkePacket->Private           = IkeSaSession->SessionCommon.Private;
      IkePacket->Spi               = 0;
      IkePacket->IsDeleteInfo      = TRUE;
            
    } else if (Context != NULL) {
      //
      // TODO: If contest is not NULL Generate a Notify Payload.
      //
    } else {
      //
      // The input parameter is not correct.
      //
      goto ERROR_EXIT;
    } 
  } else {
    //
    // Delete the Child SA Information Exchagne
    //
    ChildSaSession                     = (IKEV2_CHILD_SA_SESSION *) SaSession;
    IkeSaSession                       = ChildSaSession->IkeSaSession;
    IkePacket->Header->InitiatorCookie = ChildSaSession->IkeSaSession->InitiatorCookie;
    IkePacket->Header->ResponderCookie = ChildSaSession->IkeSaSession->ResponderCookie;

    //
    // If the information message is response message,the MessageId should
    // be same as the request MessageId which passed through the Context.
    //
    if (InfoContext != NULL && InfoContext->MessageId != 0) {
      IkePacket->Header->MessageId     = InfoContext->MessageId;
    } else {
      IkePacket->Header->MessageId     = ChildSaSession->IkeSaSession->MessageId;
      Ikev2SaSessionIncreaseMessageId (IkeSaSession);
    }
    
    IkePayload     = Ikev2GenerateDeletePayload (
                       ChildSaSession->IkeSaSession,
                       IKEV2_PAYLOAD_TYPE_DELETE,
                       4,
                       1,
                       (UINT8 *)&ChildSaSession->LocalPeerSpi
                       );
    if (IkePayload == NULL) {
      goto ERROR_EXIT;
    }
    //
    // Fill the Next Payload in IkePacket's Header.
    //
    IkePacket->Header->NextPayload     = IKEV2_PAYLOAD_TYPE_DELETE;
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, IkePayload);

    IkePacket->Private      = IkeSaSession->SessionCommon.Private;
    IkePacket->Spi          = ChildSaSession->LocalPeerSpi;
    IkePacket->IsDeleteInfo = TRUE;

    if (!ChildSaSession->SessionCommon.IsInitiator) {
      //
      // If responder, use the MessageId fromt the initiator.
      //
      IkePacket->Header->MessageId = ChildSaSession->MessageId;
    }

    //
    // Change the IsOnDeleting Flag
    //
    ChildSaSession->SessionCommon.IsOnDeleting = TRUE;
  }

  if (InfoContext == NULL) {
    IkePacket->Header->Flags = IKE_HEADER_FLAGS_INIT;
  } else {
    IkePacket->Header->Flags = IKE_HEADER_FLAGS_RESPOND;
  }
  return IkePacket;

ERROR_EXIT:
   if (IkePacket != NULL) {
     FreePool (IkePacket);
   }
   return NULL;

}

/**
  Parse the Info Exchange.

  @param[in]  SaSession   Pointer to IKEV2_SA_SESSION.
  @param[in]  IkePacket   Pointer to IkePacket related to the Information Exchange.

  @retval  EFI_SUCCESS    The operation finised successed.

**/
EFI_STATUS
Ikev2InfoParser (
  IN UINT8                         *SaSession,
  IN IKE_PACKET                    *IkePacket
  )
{
  IKEV2_CHILD_SA_SESSION *ChildSaSession;
  IKEV2_SA_SESSION       *IkeSaSession;
  IKE_PAYLOAD            *DeletePayload;
  IKE_PAYLOAD            *IkePayload;
  IKEV2_DELETE           *Delete;
  LIST_ENTRY             *Entry;
  LIST_ENTRY             *ListEntry;
  UINT8                  Index;
  UINT32                 Spi;
  UINT8                  *SpiBuffer;
  IPSEC_PRIVATE_DATA     *Private;
  UINT8                  Value;
  EFI_STATUS             Status;
  IKE_PACKET             *RespondPacket;
  
  IKEV2_INFO_EXCHANGE_CONTEXT Context;
  
  IkeSaSession   = (IKEV2_SA_SESSION *) SaSession;

  DeletePayload  = NULL;
  Private        = NULL;
  RespondPacket  = NULL;
  Status         = EFI_SUCCESS;
  
  //
  // For Liveness Check
  //
  if (IkePacket->Header->NextPayload == IKEV2_PAYLOAD_TYPE_NONE &&
      (IkePacket->PayloadTotalSize == 0)
      ) {
    if (IkePacket->Header->Flags == IKE_HEADER_FLAGS_INIT) {
      //
      // If it is Liveness check request, reply it.
      //
      Context.InfoType  = Ikev2InfoLiveCheck;
      Context.MessageId = IkePacket->Header->MessageId;
      RespondPacket     = Ikev2InfoGenerator ((UINT8 *)IkeSaSession, &Context);

      if (RespondPacket == NULL) {
        Status = EFI_INVALID_PARAMETER;
        return Status;
      }
      Status = Ikev2SendIkePacket (
                 IkeSaSession->SessionCommon.UdpService,
                 (UINT8 *)(&IkeSaSession->SessionCommon),
                 RespondPacket,
                 0
                 );

    } else {
      //
      // Todo: verify the liveness check response packet.
      //
    }
    return Status;
  }

  //
  // For SA Delete
  //
  NET_LIST_FOR_EACH (Entry, &(IkePacket)->PayloadList) {   

  //
  // Iterate payloads to find the Delete/Notify Payload.
  //
    IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);
    
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_DELETE) {
      DeletePayload = IkePayload;
      Delete = (IKEV2_DELETE *)DeletePayload->PayloadBuf;

      if (Delete->SpiSize == 0) {
        //
        // Delete IKE SA.
        //
        if (IkeSaSession->SessionCommon.State == IkeStateSaDeleting) {
          RemoveEntryList (&IkeSaSession->BySessionTable);
          Ikev2SaSessionFree (IkeSaSession);
          //
          // Checking the Private status.
          //
          //
          // when all IKE SAs were disabled by calling "IPsecConfig -disable", the IPsec
          // status should be changed.
          //
          Private = IkeSaSession->SessionCommon.Private;
          if (Private != NULL && Private->IsIPsecDisabling) {
            //
            // After all IKE SAs were deleted, set the IPSEC_STATUS_DISABLED value in
            // IPsec status variable.
            //
            if (IsListEmpty (&Private->Ikev1EstablishedList) && 
                (IsListEmpty (&Private->Ikev2EstablishedList))
               ) {
              Value  = IPSEC_STATUS_DISABLED;
              Status = gRT->SetVariable (
                         IPSECCONFIG_STATUS_NAME,
                         &gEfiIpSecConfigProtocolGuid,
                         EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                         sizeof (Value),
                         &Value
                         );
              if (!EFI_ERROR (Status)) {
                //
                // Set the DisabledFlag in Private data.
                //
                Private->IpSec.DisabledFlag = TRUE;
                Private->IsIPsecDisabling   = FALSE;
              }
            }
          }
        } else {
          IkeSaSession->SessionCommon.State = IkeStateSaDeleting;
          Context.InfoType                  = Ikev2InfoDelete;
          Context.MessageId                 = IkePacket->Header->MessageId;

          RespondPacket = Ikev2InfoGenerator ((UINT8 *)IkeSaSession, &Context);
          if (RespondPacket == NULL) {
            Status = EFI_INVALID_PARAMETER;
            return Status;
          }
          Status = Ikev2SendIkePacket (
                     IkeSaSession->SessionCommon.UdpService, 
                     (UINT8 *)(&IkeSaSession->SessionCommon), 
                     RespondPacket, 
                     0
                     );
        }
      } else if (Delete->SpiSize == 4) {
        //
        // Move the Child SAs to DeleteList
        //
        SpiBuffer = (UINT8 *)(Delete + 1);
        for (Index = 0; Index < Delete->NumSpis; Index++) {
          Spi = ReadUnaligned32 ((UINT32 *)SpiBuffer);
          for (ListEntry = IkeSaSession->ChildSaEstablishSessionList.ForwardLink;
               ListEntry != &IkeSaSession->ChildSaEstablishSessionList;
          ) {
            ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (ListEntry);
            ListEntry = ListEntry->ForwardLink;

            if (ChildSaSession->RemotePeerSpi == HTONL(Spi)) {
              if (ChildSaSession->SessionCommon.State != IkeStateSaDeleting) {

                //
                // Insert the ChildSa Session into Delete List.
                //
                InsertTailList (&IkeSaSession->DeleteSaList, &ChildSaSession->ByDelete);
                ChildSaSession->SessionCommon.State       = IkeStateSaDeleting;
                ChildSaSession->SessionCommon.IsInitiator = FALSE;
                ChildSaSession->MessageId                 = IkePacket->Header->MessageId;

                Context.InfoType = Ikev2InfoDelete;
                Context.MessageId = IkePacket->Header->MessageId;
          
                RespondPacket = Ikev2InfoGenerator ((UINT8 *)ChildSaSession, &Context);
                if (RespondPacket == NULL) {
                  Status = EFI_INVALID_PARAMETER;
                  return Status;
                }
                Status = Ikev2SendIkePacket (
                           ChildSaSession->SessionCommon.UdpService,
                           (UINT8 *)(&ChildSaSession->SessionCommon),
                           RespondPacket, 
                           0
                           );
              } else {
                //
                // Delete the Child SA.
                //
                Ikev2ChildSaSilentDelete (IkeSaSession, Spi);
                RemoveEntryList (&ChildSaSession->ByDelete);
              }
            }
          }
          SpiBuffer = SpiBuffer + sizeof (Spi);
        }
      }
    }
  }
  
  return Status;
}

GLOBAL_REMOVE_IF_UNREFERENCED IKEV2_PACKET_HANDLER  mIkev2Info = {
  Ikev2InfoParser,
  Ikev2InfoGenerator
};
