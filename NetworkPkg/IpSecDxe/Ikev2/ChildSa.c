/** @file
  The operations for Child SA.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Utility.h"

/**
  Generate IKE Packet for CREATE_CHILD_SA exchange.  

  This IKE Packet would be the packet for creating new CHILD SA, or the packet for
  rekeying existing IKE SA, or the packet for existing CHILD SA.
  
  @param[in] SaSession   Pointer to related SA session. 
  @param[in] Context     The data passed by the caller.

  return a pointer of IKE packet.

**/
IKE_PACKET *
Ikev2CreateChildGenerator (
  IN UINT8               *SaSession,
  IN VOID                *Context
  )
{

  IKEV2_CHILD_SA_SESSION  *ChildSaSession;
  IKEV2_SA_SESSION        *IkeSaSession;
  IKE_PACKET              *IkePacket;
  IKE_PAYLOAD             *NotifyPayload;
  UINT32                  *MessageId;
  
  ChildSaSession  = (IKEV2_CHILD_SA_SESSION *) SaSession;
  IkePacket       = IkePacketAlloc();
  MessageId       = NULL;

  if (IkePacket == NULL) {
    return NULL;
  }
  if (ChildSaSession == NULL) {
    return NULL;
  }

  if (Context != NULL) {
    MessageId = (UINT32 *) Context;
  }
  
  IkePacket->Header->Version      = (UINT8) (2 << 4);
  IkePacket->Header->NextPayload  = IKEV2_PAYLOAD_TYPE_NOTIFY;
  IkePacket->Header->ExchangeType = IKE_XCG_TYPE_CREATE_CHILD_SA;
  
  if (ChildSaSession->SessionCommon.IkeSessionType == IkeSessionTypeChildSa) {
    //
    // 1.a Fill the IkePacket->Hdr
    //    
    IkePacket->Header->InitiatorCookie = ChildSaSession->IkeSaSession->InitiatorCookie;
    IkePacket->Header->ResponderCookie = ChildSaSession->IkeSaSession->ResponderCookie;
    
    if (MessageId != NULL) {
      IkePacket->Header->MessageId     = *MessageId;
    } else {
      IkePacket->Header->MessageId     = ChildSaSession->MessageId;
    }    
    
    if (ChildSaSession->SessionCommon.IsInitiator) {
      IkePacket->Header->Flags = IKE_HEADER_FLAGS_CHILD_INIT;
    } else {
      IkePacket->Header->Flags = IKE_HEADER_FLAGS_RESPOND;
    }
      
  } else {
    IkeSaSession  = (IKEV2_SA_SESSION *) SaSession;
    //
    // 1.a Fill the IkePacket->Hdr
    //
    IkePacket->Header->InitiatorCookie = IkeSaSession->InitiatorCookie;
    IkePacket->Header->ResponderCookie = IkeSaSession->ResponderCookie;

    if (MessageId != NULL) {
      IkePacket->Header->MessageId     = *MessageId;
    } else {
      IkePacket->Header->MessageId     = IkeSaSession->MessageId;
    }    
    
    if (IkeSaSession->SessionCommon.IsInitiator) {
      IkePacket->Header->Flags = IKE_HEADER_FLAGS_CHILD_INIT;
    } else {
      IkePacket->Header->Flags = IKE_HEADER_FLAGS_RESPOND;
    }
  } 
   
  //
  // According to RFC4306, Chapter 4.
  // A minimal implementation may support the CREATE_CHILD_SA exchange only to
  // recognize requests and reject them with a Notify payload of type NO_ADDITIONAL_SAS.
  //
  NotifyPayload = Ikev2GenerateNotifyPayload (
                    0,
                    IKEV2_PAYLOAD_TYPE_NONE,
                    0,                  
                    IKEV2_NOTIFICATION_NO_ADDITIONAL_SAS,
                    NULL,
                    NULL,
                    0
                    );
                        
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, NotifyPayload);
  //
  // TODO: Support the CREATE_CHILD_SA exchange. 
  // 
  return IkePacket;
}

/**
  Parse the IKE packet of CREATE_CHILD_SA exchange.
  
  This function parse the IKE packet and save the related information to further
  calculation. 
  
  @param[in] SaSession   Pointer to IKEv2_CHILD_SA_SESSION related to this Exchange.
  @param[in] IkePacket   Received packet to be parsed.
 

  @retval EFI_SUCCESS       The IKE Packet is acceptable.
  @retval EFI_UNSUPPORTED   Not support the CREATE_CHILD_SA request.

**/
EFI_STATUS
Ikev2CreateChildParser (
  IN UINT8                        *SaSession,
  IN IKE_PACKET                   *IkePacket
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Routine process before the payload decoding.

  @param[in] SessionCommon  Pointer to ChildSa SessionCommon.
  @param[in] PayloadBuf     Pointer to the payload.
  @param[in] PayloadSize    Size of PayloadBuf in byte.
  @param[in] PayloadType    Type of Payload.

**/
VOID
Ikev2ChildSaBeforeDecodePayload (
  IN UINT8              *SessionCommon,
  IN UINT8              *PayloadBuf,
  IN UINTN              PayloadSize,
  IN UINT8              PayloadType
  )
{

}

/**
  Routine Process after the payload encoding.

  @param[in] SessionCommon  Pointer to ChildSa SessionCommon.
  @param[in] PayloadBuf     Pointer to the payload.
  @param[in] PayloadSize    Size of PayloadBuf in byte.
  @param[in] PayloadType    Type of Payload.

**/
VOID
Ikev2ChildSaAfterEncodePayload (
  IN UINT8              *SessionCommon,
  IN UINT8              *PayloadBuf,
  IN UINTN              PayloadSize,
  IN UINT8              PayloadType
  )
{
}

IKEV2_PACKET_HANDLER  mIkev2CreateChild = {
  //
  // Create Child
  //
  Ikev2CreateChildParser,
  Ikev2CreateChildGenerator
};
