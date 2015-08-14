/** @file
  The Common operations used by IKE Exchange Process.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Utility.h"
#include "IpSecDebug.h"
#include "IkeService.h"
#include "IpSecConfigImpl.h"

UINT16 mIkev2EncryptAlgorithmList[IKEV2_SUPPORT_ENCRYPT_ALGORITHM_NUM] = {
  IKEV2_TRANSFORM_ID_ENCR_3DES,
  IKEV2_TRANSFORM_ID_ENCR_AES_CBC, 
};

UINT16 mIkev2PrfAlgorithmList[IKEV2_SUPPORT_PRF_ALGORITHM_NUM] = {
  IKEV2_TRANSFORM_ID_PRF_HMAC_SHA1,
};

UINT16 mIkev2DhGroupAlgorithmList[IKEV2_SUPPORT_DH_ALGORITHM_NUM] = {
  IKEV2_TRANSFORM_ID_DH_1024MODP,
  IKEV2_TRANSFORM_ID_DH_2048MODP,
};

UINT16 mIkev2AuthAlgorithmList[IKEV2_SUPPORT_AUTH_ALGORITHM_NUM] = {
  IKEV2_TRANSFORM_ID_AUTH_HMAC_SHA1_96,
};

/**
  Allocate buffer for IKEV2_SA_SESSION and initialize it.

  @param[in] Private        Pointer to IPSEC_PRIVATE_DATA.
  @param[in] UdpService     Pointer to IKE_UDP_SERVICE related to this IKE SA Session.

  @return Pointer to IKEV2_SA_SESSION or NULL.

**/
IKEV2_SA_SESSION *
Ikev2SaSessionAlloc (
  IN IPSEC_PRIVATE_DATA       *Private,
  IN IKE_UDP_SERVICE          *UdpService
  )
{
  EFI_STATUS            Status;
  IKEV2_SESSION_COMMON  *SessionCommon;
  IKEV2_SA_SESSION      *IkeSaSession;

  IkeSaSession = AllocateZeroPool (sizeof (IKEV2_SA_SESSION));
  ASSERT (IkeSaSession != NULL);

  //
  // Initialize the fields of IkeSaSession and its SessionCommon.
  //
  IkeSaSession->NCookie              = NULL;
  IkeSaSession->Signature            = IKEV2_SA_SESSION_SIGNATURE;
  IkeSaSession->InitiatorCookie      = IkeGenerateCookie ();
  IkeSaSession->ResponderCookie      = 0;
  //
  // BUGBUG: Message ID starts from 2 is to match the OpenSwan requirement, but it 
  // might not match the IPv6 Logo. In its test specification, it mentions that
  // the Message ID should start from zero after the IKE_SA_INIT exchange.
  //
  IkeSaSession->MessageId            = 2;
  SessionCommon                      = &IkeSaSession->SessionCommon;
  SessionCommon->UdpService          = UdpService;
  SessionCommon->Private             = Private;
  SessionCommon->IkeSessionType      = IkeSessionTypeIkeSa;
  SessionCommon->IkeVer              = 2;
  SessionCommon->AfterEncodePayload  = NULL;
  SessionCommon->BeforeDecodePayload = NULL;

  //
  // Create a resend notfiy event for retry.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ikev2ResendNotify,
                  SessionCommon,
                  &SessionCommon->TimeoutEvent
                  );

  if (EFI_ERROR (Status)) {
    FreePool (IkeSaSession);
    return NULL;
  }

  //
  // Initialize the lists in IkeSaSession.
  //
  InitializeListHead (&IkeSaSession->ChildSaSessionList);
  InitializeListHead (&IkeSaSession->ChildSaEstablishSessionList);
  InitializeListHead (&IkeSaSession->InfoMIDList);
  InitializeListHead (&IkeSaSession->DeleteSaList);

  return IkeSaSession;
}

/**
  Register the established IKEv2 SA into Private->Ikev2EstablishedList. If there is
  IKEV2_SA_SESSION with same remote peer IP, remove the old one then register the
  new one.

  @param[in]  IkeSaSession  Pointer to IKEV2_SA_SESSION to be registered.
  @param[in]  Private       Pointer to IPSEC_PRAVATE_DATA.

**/
VOID
Ikev2SaSessionReg (
  IN IKEV2_SA_SESSION          *IkeSaSession,
  IN IPSEC_PRIVATE_DATA        *Private
  )
{
  IKEV2_SESSION_COMMON         *SessionCommon;
  IKEV2_SA_SESSION             *OldIkeSaSession;
  EFI_STATUS                   Status;
  UINT64                       Lifetime;

  //
  // Keep IKE SA exclusive to remote ip address.
  //
  SessionCommon   = &IkeSaSession->SessionCommon;
  OldIkeSaSession = Ikev2SaSessionRemove (&Private->Ikev2EstablishedList, &SessionCommon->RemotePeerIp);
  if (OldIkeSaSession != NULL) {
    //
    // TODO: It should delete all child SAs if rekey the IKE SA.
    //
    Ikev2SaSessionFree (OldIkeSaSession);
  }

  //
  // Cleanup the fields of SessionCommon for processing.
  // 
  Ikev2SessionCommonRefresh (SessionCommon);

  //
  // Insert the ready IKE SA session into established list.
  //
  Ikev2SaSessionInsert (&Private->Ikev2EstablishedList, IkeSaSession, &SessionCommon->RemotePeerIp);

  //
  // Create a notfiy event for the IKE SA life time counting.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ikev2LifetimeNotify,
                  SessionCommon,
                  &SessionCommon->TimeoutEvent
                  );
  if (EFI_ERROR(Status)){
    //
    // If TimerEvent creation failed, the SA will be alive untill user disable it or 
    // receiving a Delete Payload from peer. 
    //
    return;
  }

  //
  // Start to count the lifetime of the IKE SA.
  //
  if (IkeSaSession->Spd->Data->ProcessingPolicy->SaLifetime.HardLifetime == 0) {
    Lifetime = IKE_SA_DEFAULT_LIFETIME;
  } else {
    Lifetime = IkeSaSession->Spd->Data->ProcessingPolicy->SaLifetime.HardLifetime;
  }
  
  Status = gBS->SetTimer (
                  SessionCommon->TimeoutEvent,
                  TimerRelative,
                  MultU64x32(Lifetime, 10000000) // ms->100ns
                  );
  if (EFI_ERROR(Status)){
    //
    // If SetTimer failed, the SA will be alive untill user disable it or 
    // receiving a Delete Payload from peer. 
    //
    return ;
  }

  DEBUG ((
    DEBUG_INFO,
    "\n------IkeSa established and start to count down %d seconds lifetime\n",
    Lifetime
    ));

  return ;
}

/**
  Find a IKEV2_SA_SESSION by the remote peer IP.

  @param[in]  SaSessionList     SaSession List to be searched.
  @param[in]  RemotePeerIp      Pointer to specified IP address.

  @return Pointer to IKEV2_SA_SESSION if find one or NULL.

**/
IKEV2_SA_SESSION *
Ikev2SaSessionLookup (
  IN LIST_ENTRY           *SaSessionList,
  IN EFI_IP_ADDRESS       *RemotePeerIp
  )
{
  LIST_ENTRY        *Entry;
  IKEV2_SA_SESSION  *IkeSaSession;

  NET_LIST_FOR_EACH (Entry, SaSessionList) {
    IkeSaSession = IKEV2_SA_SESSION_BY_SESSION (Entry);

    if (CompareMem (
          &IkeSaSession->SessionCommon.RemotePeerIp,
          RemotePeerIp,
          sizeof (EFI_IP_ADDRESS)
          ) == 0) {

      return IkeSaSession;
    }
  }

  return NULL;
}

/**
  Insert a IKE_SA_SESSION into IkeSaSession list. The IkeSaSession list is either
  Private->Ikev2SaSession list or Private->Ikev2EstablishedList list.

  @param[in]  SaSessionList   Pointer to list to be inserted into.
  @param[in]  IkeSaSession    Pointer to IKEV2_SA_SESSION to be inserted. 
  @param[in]  RemotePeerIp    Pointer to EFI_IP_ADDRESSS to indicate the 
                              unique IKEV2_SA_SESSION.

**/
VOID
Ikev2SaSessionInsert (
  IN LIST_ENTRY           *SaSessionList,
  IN IKEV2_SA_SESSION     *IkeSaSession,
  IN EFI_IP_ADDRESS       *RemotePeerIp
  )
{
  Ikev2SaSessionRemove (SaSessionList, RemotePeerIp);
  InsertTailList (SaSessionList, &IkeSaSession->BySessionTable);
}

/**
  Remove the SA Session by Remote Peer IP.

  @param[in]  SaSessionList   Pointer to list to be searched.
  @param[in]  RemotePeerIp    Pointer to EFI_IP_ADDRESS to use for SA Session search.

  @retval Pointer to IKEV2_SA_SESSION with the specified remote IP address or NULL. 

**/
IKEV2_SA_SESSION *
Ikev2SaSessionRemove (
  IN LIST_ENTRY           *SaSessionList,
  IN EFI_IP_ADDRESS       *RemotePeerIp
  )
{
  LIST_ENTRY        *Entry;
  IKEV2_SA_SESSION  *IkeSaSession;

  NET_LIST_FOR_EACH (Entry, SaSessionList) {
    IkeSaSession = IKEV2_SA_SESSION_BY_SESSION (Entry);

    if (CompareMem (
          &IkeSaSession->SessionCommon.RemotePeerIp,
          RemotePeerIp,
          sizeof (EFI_IP_ADDRESS)
          ) == 0) {

      RemoveEntryList (Entry);
      return IkeSaSession;
    }
  }

  return NULL;
}

/**
  Marking a SA session as on deleting.

  @param[in]  IkeSaSession  Pointer to IKEV2_SA_SESSION.

  @retval     EFI_SUCCESS   Find the related SA session and marked it.

**/
EFI_STATUS
Ikev2SaSessionOnDeleting (
  IN IKEV2_SA_SESSION          *IkeSaSession
  )
{
  return EFI_SUCCESS;
}

/**
  Free specified Seession Common. The session common would belong to a IKE SA or 
  a Child SA.

  @param[in]   SessionCommon   Pointer to a Session Common.

**/
VOID
Ikev2SaSessionCommonFree (
  IN IKEV2_SESSION_COMMON      *SessionCommon
  )
{

  ASSERT (SessionCommon != NULL);

  if (SessionCommon->LastSentPacket != NULL) {
    IkePacketFree (SessionCommon->LastSentPacket);
  }

  if (SessionCommon->SaParams != NULL) {
    FreePool (SessionCommon->SaParams);
  }
  if (SessionCommon->TimeoutEvent != NULL) {
    gBS->CloseEvent (SessionCommon->TimeoutEvent);
  }
}

/**
  After IKE/Child SA is estiblished, close the time event and free sent packet.

  @param[in]   SessionCommon   Pointer to a Session Common.

**/
VOID
Ikev2SessionCommonRefresh (
  IN IKEV2_SESSION_COMMON      *SessionCommon
  )
{
  ASSERT (SessionCommon != NULL);

  gBS->CloseEvent (SessionCommon->TimeoutEvent);
  SessionCommon->TimeoutEvent     = NULL;
  SessionCommon->TimeoutInterval  = 0;
  SessionCommon->RetryCount       = 0;
  if (SessionCommon->LastSentPacket != NULL) {
    IkePacketFree (SessionCommon->LastSentPacket);
    SessionCommon->LastSentPacket = NULL;
  }

  return ;
}
/**
  Free specified IKEV2 SA Session. 

  @param[in]    IkeSaSession   Pointer to IKEV2_SA_SESSION to be freed.

**/
VOID
Ikev2SaSessionFree (
  IN IKEV2_SA_SESSION         *IkeSaSession
  )
{
  IKEV2_SESSION_KEYS      *IkeKeys;
  LIST_ENTRY              *Entry;
  IKEV2_CHILD_SA_SESSION  *ChildSa;
  IKEV2_DH_BUFFER         *DhBuffer;

  ASSERT (IkeSaSession != NULL);
  
  //
  // Delete Common Session
  //
  Ikev2SaSessionCommonFree (&IkeSaSession->SessionCommon);

  //
  // Delete ChildSaEstablish List and SAD
  //
  for (Entry = IkeSaSession->ChildSaEstablishSessionList.ForwardLink;
       Entry != &IkeSaSession->ChildSaEstablishSessionList;
      ) {

    ChildSa = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (Entry);
    Entry   = Entry->ForwardLink;
    Ikev2ChildSaSilentDelete (ChildSa->IkeSaSession, ChildSa->LocalPeerSpi);

  }

  //
  // Delete ChildSaSessionList
  //
  for ( Entry  = IkeSaSession->ChildSaSessionList.ForwardLink;
        Entry != &IkeSaSession->ChildSaSessionList;
        ){
    ChildSa = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (Entry);
    Entry   = Entry->ForwardLink;
    RemoveEntryList (Entry->BackLink);
    Ikev2ChildSaSessionFree (ChildSa);
  }

  //
  // Delete DhBuffer and Keys
  //
  if (IkeSaSession->IkeKeys != NULL) {
    IkeKeys  = IkeSaSession->IkeKeys;
    DhBuffer = IkeKeys->DhBuffer;

    //
    // Delete DhBuffer
    //
    Ikev2DhBufferFree (DhBuffer);

    //
    // Delete Keys
    //    
    if (IkeKeys->SkAiKey != NULL) {
      FreePool (IkeKeys->SkAiKey);
    }
    if (IkeKeys->SkArKey != NULL) {
      FreePool (IkeKeys->SkArKey);
    }
    if (IkeKeys->SkdKey != NULL) {
      FreePool (IkeKeys->SkdKey);
    }
    if (IkeKeys->SkEiKey != NULL) {
      FreePool (IkeKeys->SkEiKey);
    }
    if (IkeKeys->SkErKey != NULL) {
      FreePool (IkeKeys->SkErKey);
    }
    if (IkeKeys->SkPiKey != NULL) {
      FreePool (IkeKeys->SkPiKey);
    }
    if (IkeKeys->SkPrKey != NULL) {
      FreePool (IkeKeys->SkPrKey);
    }
    FreePool (IkeKeys);
  }

  if (IkeSaSession->SaData != NULL) {
    FreePool (IkeSaSession->SaData);
  }

  if (IkeSaSession->NiBlock != NULL) {
    FreePool (IkeSaSession->NiBlock);
  }

  if (IkeSaSession->NrBlock != NULL) {
    FreePool (IkeSaSession->NrBlock);
  }

  if (IkeSaSession->NCookie != NULL) {
    FreePool (IkeSaSession->NCookie);
  }

  if (IkeSaSession->InitPacket != NULL) {
    FreePool (IkeSaSession->InitPacket);
  }

  if (IkeSaSession->RespPacket != NULL) {
    FreePool (IkeSaSession->RespPacket);
  }

  FreePool (IkeSaSession);

  return ;
}

/**
  Increase the MessageID in IkeSaSession.

  @param[in] IkeSaSession Pointer to a specified IKEV2_SA_SESSION.

**/
VOID
Ikev2SaSessionIncreaseMessageId (
  IN IKEV2_SA_SESSION         *IkeSaSession
  )
{
  if (IkeSaSession->MessageId < 0xffffffff) {
    IkeSaSession->MessageId ++;
  } else {
    //
    // TODO: Trigger Rekey process.
    //
  }
}

/**
  Allocate memory for IKEV2 Child SA Session.
  
  @param[in]   UdpService     Pointer to IKE_UDP_SERVICE.
  @param[in]   IkeSaSession   Pointer to IKEV2_SA_SESSION related to this Child SA 
                              Session.

  @retval  Pointer of a new created IKEV2 Child SA Session or NULL.

**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionAlloc (
  IN IKE_UDP_SERVICE          *UdpService,
  IN IKEV2_SA_SESSION         *IkeSaSession
  )
{
  EFI_STATUS                  Status;
  IKEV2_CHILD_SA_SESSION      *ChildSaSession;
  IKEV2_SESSION_COMMON        *ChildSaCommon;
  IKEV2_SESSION_COMMON        *SaCommon;

  ChildSaSession = AllocateZeroPool (sizeof (IKEV2_CHILD_SA_SESSION));
  if (ChildSaSession == NULL) {
    return NULL;
  }

  //
  // Initialize the fields of ChildSaSession and its SessionCommon.
  //
  ChildSaSession->Signature          = IKEV2_CHILD_SA_SESSION_SIGNATURE;
  ChildSaSession->IkeSaSession       = IkeSaSession;
  ChildSaSession->MessageId          = IkeSaSession->MessageId;
  ChildSaSession->LocalPeerSpi       = IkeGenerateSpi ();
  ChildSaCommon                      = &ChildSaSession->SessionCommon;
  ChildSaCommon->UdpService          = UdpService;
  ChildSaCommon->Private             = IkeSaSession->SessionCommon.Private;
  ChildSaCommon->IkeSessionType      = IkeSessionTypeChildSa;
  ChildSaCommon->IkeVer              = 2;
  ChildSaCommon->AfterEncodePayload  = Ikev2ChildSaAfterEncodePayload;
  ChildSaCommon->BeforeDecodePayload = Ikev2ChildSaBeforeDecodePayload;
  SaCommon = &ChildSaSession->IkeSaSession->SessionCommon;

  //
  // Create a resend notfiy event for retry.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ikev2ResendNotify,
                  ChildSaCommon,
                  &ChildSaCommon->TimeoutEvent
                  );
  if (EFI_ERROR (Status)) {
    FreePool (ChildSaSession);
    return NULL;
  }

  CopyMem (&ChildSaCommon->LocalPeerIp, &SaCommon->LocalPeerIp, sizeof (EFI_IP_ADDRESS));
  CopyMem (&ChildSaCommon->RemotePeerIp, &SaCommon->RemotePeerIp, sizeof (EFI_IP_ADDRESS));

  return ChildSaSession;
}

/**
  Register a established IKEv2 Child SA into IkeSaSession->ChildSaEstablishSessionList. 
  If the there is IKEV2_CHILD_SA_SESSION with same remote peer IP, remove the old one 
  then register the new one.

  @param[in]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION to be registered.
  @param[in]  Private         Pointer to IPSEC_PRAVATE_DATA.

**/
VOID
Ikev2ChildSaSessionReg (
  IN IKEV2_CHILD_SA_SESSION    *ChildSaSession,
  IN IPSEC_PRIVATE_DATA        *Private
  )
{
  IKEV2_SESSION_COMMON         *SessionCommon;
  IKEV2_CHILD_SA_SESSION       *OldChildSaSession;
  IKEV2_SA_SESSION             *IkeSaSession;
  EFI_STATUS                   Status;
  UINT64                       Lifetime;

  //
  // Keep the IKE SA exclusive.
  //
  SessionCommon     = &ChildSaSession->SessionCommon;
  IkeSaSession      = ChildSaSession->IkeSaSession;
  OldChildSaSession = Ikev2ChildSaSessionRemove (
                        &IkeSaSession->ChildSaEstablishSessionList,
                        ChildSaSession->LocalPeerSpi,
                        IKEV2_ESTABLISHED_CHILDSA_LIST
                        );
  if (OldChildSaSession != NULL) {
    //
    // Free the old one.
    //
    Ikev2ChildSaSessionFree (OldChildSaSession);
  }

  //
  // Store the ready child SA into SAD.
  //
  Ikev2StoreSaData (ChildSaSession);

  //
  // Cleanup the fields of SessionCommon for processing.
  // 
  Ikev2SessionCommonRefresh (SessionCommon);
 
  //
  // Insert the ready child SA session into established list.
  //
  Ikev2ChildSaSessionInsert (&IkeSaSession->ChildSaEstablishSessionList, ChildSaSession);

  //
  // Create a Notify event for the IKE SA life time counting.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ikev2LifetimeNotify,
                  SessionCommon,
                  &SessionCommon->TimeoutEvent
                  );
  if (EFI_ERROR(Status)){
    return ;
  }

  //
  // Start to count the lifetime of the IKE SA.
  //
  if (ChildSaSession->Spd->Data->ProcessingPolicy->SaLifetime.HardLifetime != 0){
    Lifetime = ChildSaSession->Spd->Data->ProcessingPolicy->SaLifetime.HardLifetime;
  } else {
    Lifetime = CHILD_SA_DEFAULT_LIFETIME;
  }

  Status = gBS->SetTimer (
                  SessionCommon->TimeoutEvent,
                  TimerRelative,
                  MultU64x32(Lifetime, 10000000) // ms->100ns
                  );
  if (EFI_ERROR(Status)){
    return ;
  }

  DEBUG ((
    DEBUG_INFO,
    "\n------ChildSa established and start to count down %d seconds lifetime\n",
    Lifetime
    ));

  return ;
}

/**
  Find the ChildSaSession by it's MessagId.

  @param[in] SaSessionList  Pointer to a ChildSaSession List.
  @param[in] Mid            The messageId used to search ChildSaSession.

  @return Pointer to IKEV2_CHILD_SA_SESSION or NULL.

**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionLookupByMid (
  IN LIST_ENTRY           *SaSessionList,
  IN UINT32               Mid
  )
{
  LIST_ENTRY              *Entry;
  IKEV2_CHILD_SA_SESSION  *ChildSaSession;

  NET_LIST_FOR_EACH (Entry, SaSessionList) {
    ChildSaSession  = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (Entry);

    if (ChildSaSession->MessageId == Mid) {
      return ChildSaSession;
    }
  }
  return NULL;
}

/**
  This function find the Child SA by the specified SPI.

  This functin find a ChildSA session by searching the ChildSaSessionlist of
  the input IKEV2_SA_SESSION by specified MessageID.
  
  @param[in]  SaSessionList      Pointer to List to be searched.
  @param[in]  Spi                Specified SPI.

  @return Pointer to IKEV2_CHILD_SA_SESSION or NULL.

**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionLookupBySpi (
  IN LIST_ENTRY           *SaSessionList,
  IN UINT32               Spi
  )
{
  LIST_ENTRY              *Entry;
  IKEV2_CHILD_SA_SESSION  *ChildSaSession;

  NET_LIST_FOR_EACH (Entry, SaSessionList) {
    ChildSaSession  = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (Entry);

    if (ChildSaSession->RemotePeerSpi == Spi || ChildSaSession->LocalPeerSpi == Spi) {
      return ChildSaSession;
    }
  }

  return NULL;
}

/**
  Insert a Child SA Session into the specified ChildSa list.

  @param[in]  SaSessionList   Pointer to list to be inserted in.
  @param[in]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION to be inserted.

**/
VOID
Ikev2ChildSaSessionInsert (
  IN LIST_ENTRY               *SaSessionList,
  IN IKEV2_CHILD_SA_SESSION   *ChildSaSession
  )
{
 InsertTailList (SaSessionList, &ChildSaSession->ByIkeSa);
}

/**
  Remove the IKEV2_CHILD_SA_SESSION from IkeSaSessionList.
  
  @param[in]  SaSessionList      The SA Session List to be iterated.
  @param[in]  Spi                Spi used to identified the IKEV2_CHILD_SA_SESSION.
  @param[in]  ListType           The type of the List to indicate whether it is a 
                                 Established. 

  @return The point to IKEV2_CHILD_SA_SESSION or NULL.
  
**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionRemove (
  IN LIST_ENTRY           *SaSessionList,
  IN UINT32               Spi, 
  IN UINT8                ListType
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *NextEntry;
  IKEV2_CHILD_SA_SESSION  *ChildSaSession;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, SaSessionList) {
    
    if (ListType == IKEV2_ESTABLISHED_CHILDSA_LIST || ListType == IKEV2_ESTABLISHING_CHILDSA_LIST) {
      ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (Entry);
    } else if (ListType == IKEV2_DELET_CHILDSA_LIST) {
      ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_DEL_SA (Entry);
    } else {
      return NULL;
    }

    if (ChildSaSession->RemotePeerSpi == Spi || ChildSaSession->LocalPeerSpi == Spi) {
      RemoveEntryList (Entry);
      return ChildSaSession;
    }
  }

  return NULL;
}

/**
  Mark a specified Child SA Session as on deleting.

  @param[in]  ChildSaSession   Pointer to IKEV2_CHILD_SA_SESSION.

  @retval     EFI_SUCCESS      Operation is successful.

**/
EFI_STATUS
Ikev2ChildSaSessionOnDeleting (
  IN IKEV2_CHILD_SA_SESSION   *ChildSaSession
  )
{
  return EFI_SUCCESS;
}

/**
  Free the memory located for the specified IKEV2_CHILD_SA_SESSION. 

  @param[in]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION.

**/
VOID
Ikev2ChildSaSessionFree (
  IN IKEV2_CHILD_SA_SESSION   *ChildSaSession
  )
{
  IKEV2_SESSION_COMMON  *SessionCommon;

  SessionCommon = &ChildSaSession->SessionCommon;
  if (ChildSaSession->SaData != NULL) {
    FreePool (ChildSaSession->SaData);
  }

  if (ChildSaSession->NiBlock != NULL) {
    FreePool (ChildSaSession->NiBlock);
  }

  if (ChildSaSession->NrBlock != NULL) {
    FreePool (ChildSaSession->NrBlock);
  }

  if (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey != NULL) {
    FreePool (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey);
  }

  if (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey != NULL) {
    FreePool (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey);
  }

  if (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey != NULL) {
    FreePool (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey);
  }

  if (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey != NULL) {
    FreePool (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey);
  }

  //
  // Delete DhBuffer
  //
  Ikev2DhBufferFree (ChildSaSession->DhBuffer);

  //
  // Delete SpdSelector
  //
  if (ChildSaSession->SpdSelector != NULL) {
    if (ChildSaSession->SpdSelector->LocalAddress != NULL) {
      FreePool (ChildSaSession->SpdSelector->LocalAddress);
    }
    if (ChildSaSession->SpdSelector->RemoteAddress != NULL) {
      FreePool (ChildSaSession->SpdSelector->RemoteAddress);
    }
    FreePool (ChildSaSession->SpdSelector);
  }
  Ikev2SaSessionCommonFree (SessionCommon);
  FreePool (ChildSaSession);

  return ;
}

/**
  Delete the specified established Child SA.

  This function delete the Child SA directly and don't send the Information Packet to
  remote peer.

  @param[in]  IkeSaSession   Pointer to a IKE SA Session used to be searched for.
  @param[in]  Spi            SPI used to find the Child SA.

  @retval     EFI_NOT_FOUND  Pointer of IKE SA Session is NULL.
  @retval     EFI_NOT_FOUND  There is no specified Child SA related with the input
                             SPI under this IKE SA Session.
  @retval     EFI_SUCCESS    Delete the Child SA successfully.

**/
EFI_STATUS
Ikev2ChildSaSilentDelete (
  IN IKEV2_SA_SESSION       *IkeSaSession,
  IN UINT32                 Spi
  )
{
  EFI_STATUS                Status;
  EFI_IPSEC_CONFIG_SELECTOR *Selector;
  UINTN                     SelectorSize;
  BOOLEAN                   IsLocalFound;
  BOOLEAN                   IsRemoteFound;
  UINT32                    LocalSpi;
  UINT32                    RemoteSpi;
  IKEV2_CHILD_SA_SESSION    *ChildSession;
  EFI_IPSEC_CONFIG_SELECTOR *LocalSelector;
  EFI_IPSEC_CONFIG_SELECTOR *RemoteSelector;
  IKE_UDP_SERVICE           *UdpService;
  IPSEC_PRIVATE_DATA        *Private;

  if (IkeSaSession == NULL) {
    return EFI_NOT_FOUND;
  }

  IsLocalFound    = FALSE;
  IsRemoteFound   = FALSE;
  ChildSession    = NULL;
  LocalSelector   = NULL;
  RemoteSelector  = NULL;
  UdpService      = IkeSaSession->SessionCommon.UdpService;

  Private = IkeSaSession->SessionCommon.Private;

  //
  // Remove the Established SA from ChildSaEstablishlist.
  //
  ChildSession = Ikev2ChildSaSessionRemove(
                   &(IkeSaSession->ChildSaEstablishSessionList),
                   Spi, 
                   IKEV2_ESTABLISHED_CHILDSA_LIST
                   );
  if (ChildSession == NULL) {
    return EFI_NOT_FOUND;
  }

  LocalSpi  = ChildSession->LocalPeerSpi;
  RemoteSpi = ChildSession->RemotePeerSpi;
  
  SelectorSize  = sizeof (EFI_IPSEC_CONFIG_SELECTOR);
  Selector      = AllocateZeroPool (SelectorSize);
  ASSERT (Selector != NULL);

  

  while (1) {
    Status = EfiIpSecConfigGetNextSelector (
               &Private->IpSecConfig,
               IPsecConfigDataTypeSad,
               &SelectorSize,
               Selector
               );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (Selector);

      Selector = AllocateZeroPool (SelectorSize);
      ASSERT (Selector != NULL);
      Status   = EfiIpSecConfigGetNextSelector (
                   &Private->IpSecConfig,
                   IPsecConfigDataTypeSad,
                   &SelectorSize,
                   Selector
                   );
    }

    if (EFI_ERROR (Status)) {
      break;
    }

    if (Selector->SaId.Spi == RemoteSpi) {
      //
      // SPI is unique. There is only one SAD whose SPI is
      // same with RemoteSpi.
      //
      IsRemoteFound   = TRUE;
      RemoteSelector  = AllocateZeroPool (SelectorSize);
      ASSERT (RemoteSelector != NULL);
      CopyMem (RemoteSelector, Selector, SelectorSize);
    }

    if (Selector->SaId.Spi == LocalSpi) {
      //
      // SPI is unique. There is only one SAD whose SPI is
      // same with LocalSpi.
      //
      IsLocalFound  = TRUE;
      LocalSelector = AllocateZeroPool (SelectorSize);
      ASSERT (LocalSelector != NULL);
      CopyMem (LocalSelector, Selector, SelectorSize);
    }
  }
  //
  // Delete SA from the Variable.
  //
  if (IsLocalFound) {
    Status = EfiIpSecConfigSetData (
               &Private->IpSecConfig,
               IPsecConfigDataTypeSad,
               LocalSelector,
               NULL,
               NULL
               );
  }

  if (IsRemoteFound) {
    Status = EfiIpSecConfigSetData (
               &Private->IpSecConfig,
               IPsecConfigDataTypeSad,
               RemoteSelector,
               NULL,
               NULL
               );

  }

  DEBUG (
    (DEBUG_INFO,
    "\n------IKEV2 deleted ChildSa(local spi, remote spi):(0x%x, 0x%x)------\n",
    LocalSpi,
    RemoteSpi)
    );
  Ikev2ChildSaSessionFree (ChildSession);

  if (RemoteSelector != NULL) {
    FreePool (RemoteSelector);
  }

  if (LocalSelector != NULL) {
    FreePool (LocalSelector);
  }

  if (Selector != NULL) {
    FreePool (Selector);
  }

  return Status;
}

/**
  Free the specified DhBuffer.

  @param[in] DhBuffer   Pointer to IKEV2_DH_BUFFER to be freed.
  
**/
VOID
Ikev2DhBufferFree (
  IKEV2_DH_BUFFER *DhBuffer
) 
{
  if (DhBuffer != NULL) {
    if (DhBuffer->GxBuffer != NULL) {
      FreePool (DhBuffer->GxBuffer);
    }
    if (DhBuffer->GyBuffer != NULL) {
      FreePool (DhBuffer->GyBuffer);
    }
    if (DhBuffer->GxyBuffer != NULL) {
      FreePool (DhBuffer->GxyBuffer);
    }
    if (DhBuffer->DhContext != NULL) {
      IpSecCryptoIoFreeDh (&DhBuffer->DhContext);
    }
    FreePool (DhBuffer);
  }
}

/**
  This function is to parse a request IKE packet and return its request type.
  The request type is one of IKE CHILD SA creation, IKE SA rekeying and 
  IKE CHILD SA rekeying.

  @param[in] IkePacket  IKE packet to be prased.

  return the type of the IKE packet.

**/
IKEV2_CREATE_CHILD_REQUEST_TYPE
Ikev2ChildExchangeRequestType(
  IN IKE_PACKET               *IkePacket
  )
{
  BOOLEAN       Flag;
  LIST_ENTRY    *Entry;
  IKE_PAYLOAD   *IkePayload;

  Flag            = FALSE;

  NET_LIST_FOR_EACH (Entry, &(IkePacket)->PayloadList) {
    IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_TS_INIT) {
      //
      // Packet with Ts Payload means it is for either CHILD_SA_CREATE or CHILD_SA_REKEY.
      //
      Flag = TRUE;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_NOTIFY) { 
      if (((IKEV2_NOTIFY*)IkePayload)->MessageType == IKEV2_NOTIFICATION_REKEY_SA) {
        //
        // If notify payload with REKEY_SA message type, the IkePacket is for 
        // rekeying Child SA.
        //
        return IkeRequestTypeRekeyChildSa;
      }
    }
  };

  if (!Flag){
    //
    // The Create Child Exchange is for IKE SA rekeying.
    //
    return IkeRequestTypeRekeyIkeSa;
  } else {
    //
    // If the Notify payloaad with transport mode message type, the IkePacket is 
    // for create Child SA.
    //
    return IkeRequestTypeCreateChildSa;
  }
}

/**
  Associate a SPD selector to the Child SA Session.

  This function is called when the Child SA is not the first child SA of its 
  IKE SA. It associate a SPD to this Child SA.

  @param[in, out]  ChildSaSession     Pointer to the Child SA Session to be associated to 
                                      a SPD selector.

  @retval EFI_SUCCESS        Associate one SPD selector to this Child SA Session successfully.
  @retval EFI_NOT_FOUND      Can't find the related SPD selector.

**/
EFI_STATUS
Ikev2ChildSaAssociateSpdEntry (
  IN OUT IKEV2_CHILD_SA_SESSION *ChildSaSession
  )
{
  IpSecVisitConfigData (IPsecConfigDataTypeSpd, Ikev2MatchSpdEntry, ChildSaSession);
  if (ChildSaSession->Spd != NULL) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}


/**
  This function finds the SPI from Create Child SA Exchange Packet.
 
  @param[in] IkePacket       Pointer to IKE_PACKET to be searched.

  @retval SPI number or 0 if it is not supported.

**/
UINT32
Ikev2ChildExchangeRekeySpi (
  IN IKE_PACKET               *IkePacket
  )
{
  //
  // Not support yet.
  // 
  return 0;
}

/**
  Validate the IKE header of received IKE packet.

  @param[in]   IkeSaSession  Pointer to IKEV2_SA_SESSION related to this IKE packet.
  @param[in]   IkeHdr        Pointer to IKE header of received IKE packet.

  @retval TRUE   If the IKE header is valid.
  @retval FALSE  If the IKE header is invalid.

**/
BOOLEAN
Ikev2ValidateHeader (
  IN IKEV2_SA_SESSION         *IkeSaSession,
  IN IKE_HEADER               *IkeHdr
  )
{

  IKEV2_SESSION_STATE State;

  State = IkeSaSession->SessionCommon.State;
  if (State == IkeStateInit) {
    //
    // For the IKE Initial Exchange, the MessagId should be zero.
    //
    if (IkeHdr->MessageId != 0) {
      return FALSE;
    }
  } else {
    if (State == IkeStateAuth) {
      if (IkeHdr->MessageId != 1) {
        return FALSE;
      }
    }
    if (IkeHdr->InitiatorCookie != IkeSaSession->InitiatorCookie ||
        IkeHdr->ResponderCookie != IkeSaSession->ResponderCookie
        ) {
      //
      // TODO: send notification INVALID-COOKIE
      //
      return FALSE;
    }
  }

  //
  // Information Exchagne and Create Child Exchange can be started from each part.
  //
  if (IkeHdr->ExchangeType != IKEV2_EXCHANGE_TYPE_INFO && 
      IkeHdr->ExchangeType != IKEV2_EXCHANGE_TYPE_CREATE_CHILD
      ) {
    if (IkeSaSession->SessionCommon.IsInitiator) {
      if (IkeHdr->InitiatorCookie != IkeSaSession->InitiatorCookie) {
        //
        // TODO: send notification INVALID-COOKIE
        //
        return FALSE;
      }
      if (IkeHdr->Flags != IKE_HEADER_FLAGS_RESPOND) {
        return FALSE;
      }
    } else {
      if (IkeHdr->Flags != IKE_HEADER_FLAGS_INIT) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

/**
  Create and intialize IKEV2_SA_DATA for speicifed IKEV2_SESSION_COMMON.

  This function will be only called by the initiator. The responder's IKEV2_SA_DATA
  will be generated during parsed the initiator packet.

  @param[in]  SessionCommon  Pointer to IKEV2_SESSION_COMMON related to.

  @retval a Pointer to a new IKEV2_SA_DATA or NULL.

**/
IKEV2_SA_DATA *
Ikev2InitializeSaData (
  IN IKEV2_SESSION_COMMON     *SessionCommon
  )
{
  IKEV2_CHILD_SA_SESSION      *ChildSaSession;
  IKEV2_SA_DATA               *SaData;
  IKEV2_PROPOSAL_DATA         *ProposalData;
  IKEV2_TRANSFORM_DATA        *TransformData;
  IKE_SA_ATTRIBUTE            *Attribute;

  ASSERT (SessionCommon != NULL);
  //
  // TODO: Remove the hard code of the support Alogrithm. Those data should be
  // get from the SPD/PAD data.
  //
  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    SaData = AllocateZeroPool (
               sizeof (IKEV2_SA_DATA) +
               sizeof (IKEV2_PROPOSAL_DATA) * 2 +
               sizeof (IKEV2_TRANSFORM_DATA) * 4 * 2
               );
  } else {
    SaData = AllocateZeroPool (
               sizeof (IKEV2_SA_DATA) +
               sizeof (IKEV2_PROPOSAL_DATA) * 2 +
               sizeof (IKEV2_TRANSFORM_DATA) * 3 * 2
               );
  }
  if (SaData == NULL) {
    return NULL;
  }

  //
  // First proposal payload: 3DES + SHA1 + DH
  //
  SaData->NumProposals          = 2;
  ProposalData                  = (IKEV2_PROPOSAL_DATA *) (SaData + 1);
  ProposalData->ProposalIndex   = 1;

  //
  // If SA data for IKE_SA_INIT exchage, contains 4 transforms. If SA data for 
  // IKE_AUTH exchange contains 3 transforms.
  //
  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    ProposalData->NumTransforms   = 4;
  } else {
    ProposalData->NumTransforms   = 3;
  }


  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    ProposalData->ProtocolId    = IPSEC_PROTO_ISAKMP;
  } else {
    ChildSaSession              = IKEV2_CHILD_SA_SESSION_FROM_COMMON (SessionCommon);
    ProposalData->ProtocolId    = IPSEC_PROTO_IPSEC_ESP;
    ProposalData->Spi           = AllocateZeroPool (sizeof (ChildSaSession->LocalPeerSpi));
    ASSERT (ProposalData->Spi != NULL);
    CopyMem (
      ProposalData->Spi,
      &ChildSaSession->LocalPeerSpi,
      sizeof(ChildSaSession->LocalPeerSpi)
    );
  }

  //
  // Set transform attribute for Encryption Algorithm - 3DES
  //
  TransformData                 = (IKEV2_TRANSFORM_DATA *) (ProposalData + 1);
  TransformData->TransformIndex = 0;
  TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_ENCR;
  TransformData->TransformId    = IKEV2_TRANSFORM_ID_ENCR_3DES;

  //
  // Set transform attribute for Integrity Algorithm - SHA1_96
  //
  TransformData                 = (IKEV2_TRANSFORM_DATA *) (TransformData + 1);
  TransformData->TransformIndex = 1;
  TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_INTEG;
  TransformData->TransformId    = IKEV2_TRANSFORM_ID_AUTH_HMAC_SHA1_96;

  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    //
    // Set transform attribute for Pseduo-Random Function - HAMC_SHA1
    //
    TransformData                 = (IKEV2_TRANSFORM_DATA *) (TransformData + 1);
    TransformData->TransformIndex = 2;
    TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_PRF;
    TransformData->TransformId    = IKEV2_TRANSFORM_ID_PRF_HMAC_SHA1;
  }

  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    //
    // Set transform attribute for DH Group - DH 1024
    //
    TransformData                 = (IKEV2_TRANSFORM_DATA *) (TransformData + 1);
    TransformData->TransformIndex = 3;
    TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_DH;
    TransformData->TransformId    = IKEV2_TRANSFORM_ID_DH_1024MODP;
  } else {
    //
    // Transform type for Extended Sequence Numbers. Currently not support Extended
    // Sequence Number.
    //
    TransformData                 = (IKEV2_TRANSFORM_DATA *) (TransformData + 1);
    TransformData->TransformIndex = 2;
    TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_ESN;
    TransformData->TransformId    = 0;
  }

  //
  // Second proposal payload: 3DES + SHA1 + DH
  //
  ProposalData                  = (IKEV2_PROPOSAL_DATA *) (TransformData + 1);
  ProposalData->ProposalIndex   = 2;

  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    ProposalData->ProtocolId      = IPSEC_PROTO_ISAKMP;
    ProposalData->NumTransforms   = 4;
  } else {

    ChildSaSession              = IKEV2_CHILD_SA_SESSION_FROM_COMMON (SessionCommon);
    ProposalData->ProtocolId    = IPSEC_PROTO_IPSEC_ESP;
    ProposalData->NumTransforms = 3;
    ProposalData->Spi           = AllocateZeroPool (sizeof (ChildSaSession->LocalPeerSpi));
    ASSERT (ProposalData->Spi != NULL);
    CopyMem (
      ProposalData->Spi,
      &ChildSaSession->LocalPeerSpi,
      sizeof(ChildSaSession->LocalPeerSpi)
    );
  }

  //
  // Set transform attribute for Encryption Algorithm - AES-CBC
  //
  TransformData                 = (IKEV2_TRANSFORM_DATA *) (ProposalData + 1);
  TransformData->TransformIndex = 0;
  TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_ENCR;
  TransformData->TransformId    = IKEV2_TRANSFORM_ID_ENCR_AES_CBC;
  Attribute                     = &TransformData->Attribute;
  Attribute->AttrType           = IKEV2_ATTRIBUTE_TYPE_KEYLEN;
  Attribute->Attr.AttrLength    = (UINT16) (8 * IpSecGetEncryptKeyLength (IKEV2_TRANSFORM_ID_ENCR_AES_CBC));

  //
  // Set transform attribute for Integrity Algorithm - SHA1_96
  //
  TransformData                 = (IKEV2_TRANSFORM_DATA *) (TransformData + 1);
  TransformData->TransformIndex = 1;
  TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_INTEG;
  TransformData->TransformId    = IKEV2_TRANSFORM_ID_AUTH_HMAC_SHA1_96;

  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    //
    // Set transform attribute for Pseduo-Random Function - HAMC_SHA1
    //
    TransformData                 = (IKEV2_TRANSFORM_DATA *) (TransformData + 1);
    TransformData->TransformIndex = 2;
    TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_PRF;
    TransformData->TransformId    = IKEV2_TRANSFORM_ID_PRF_HMAC_SHA1;
  }

  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    //
    // Set transform attrbiute for DH Group - DH-1024
    //
    TransformData                 = (IKEV2_TRANSFORM_DATA *) (TransformData + 1);
    TransformData->TransformIndex = 3;
    TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_DH;
    TransformData->TransformId    = IKEV2_TRANSFORM_ID_DH_1024MODP;
  } else {
    //
    // Transform type for Extended Sequence Numbers. Currently not support Extended
    // Sequence Number.
    //
    TransformData                 = (IKEV2_TRANSFORM_DATA *) (TransformData + 1);
    TransformData->TransformIndex = 2;
    TransformData->TransformType  = IKEV2_TRANSFORM_TYPE_ESN;
    TransformData->TransformId    = 0;
  }

  return SaData;
}

/**
  Store the SA into SAD.

  @param[in]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION.

**/
VOID
Ikev2StoreSaData (
  IN IKEV2_CHILD_SA_SESSION   *ChildSaSession
  )
{
  EFI_STATUS                  Status;
  EFI_IPSEC_SA_ID             SaId;
  EFI_IPSEC_SA_DATA2           SaData;
  IKEV2_SESSION_COMMON        *SessionCommon;
  IPSEC_PRIVATE_DATA          *Private;
  UINT32                      TempAddressCount;
  EFI_IP_ADDRESS_INFO         *TempAddressInfo;

  SessionCommon             = &ChildSaSession->SessionCommon;
  Private                   = SessionCommon->Private;

  ZeroMem (&SaId, sizeof (EFI_IPSEC_SA_ID));
  ZeroMem (&SaData, sizeof (EFI_IPSEC_SA_DATA2));

  //
  // Create a SpdSelector. In this implementation, one SPD represents
  // 2 direction traffic, so in here, there needs to reverse the local address 
  // and remote address for Remote Peer's SA, then reverse again for the locate
  // SA. 
  //
  TempAddressCount = ChildSaSession->SpdSelector->LocalAddressCount;
  TempAddressInfo  = ChildSaSession->SpdSelector->LocalAddress;

  ChildSaSession->SpdSelector->LocalAddressCount = ChildSaSession->SpdSelector->RemoteAddressCount;
  ChildSaSession->SpdSelector->LocalAddress      = ChildSaSession->SpdSelector->RemoteAddress;

  ChildSaSession->SpdSelector->RemoteAddress     = TempAddressInfo;
  ChildSaSession->SpdSelector->RemoteAddressCount= TempAddressCount;

  //
  // Set the SaId and SaData.
  //
  SaId.Spi                 = ChildSaSession->LocalPeerSpi;
  SaId.Proto               = EfiIPsecESP;
  SaData.AntiReplayWindows = 16;
  SaData.SNCount           = 0;
  SaData.Mode              = ChildSaSession->Spd->Data->ProcessingPolicy->Mode;

  //
  // If it is tunnel mode, should add the TunnelDest and TunnelSource for SaData.
  //
  if (SaData.Mode == EfiIPsecTunnel) {
    CopyMem (
      &SaData.TunnelSourceAddress, 
      &ChildSaSession->Spd->Data->ProcessingPolicy->TunnelOption->RemoteTunnelAddress,
      sizeof (EFI_IP_ADDRESS)
      );
    CopyMem (
      &SaData.TunnelDestinationAddress,
      &ChildSaSession->Spd->Data->ProcessingPolicy->TunnelOption->LocalTunnelAddress,
      sizeof (EFI_IP_ADDRESS)
      );
  }

  CopyMem (&SaId.DestAddress, &ChildSaSession->SessionCommon.LocalPeerIp, sizeof (EFI_IP_ADDRESS));
  CopyMem (&SaData.AlgoInfo, &ChildSaSession->ChildKeymats.LocalPeerInfo, sizeof (EFI_IPSEC_ALGO_INFO));
  SaData.SpdSelector = ChildSaSession->SpdSelector;

  //
  // Store the remote SA into SAD.
  //
  Status = EfiIpSecConfigSetData (
             &Private->IpSecConfig,
             IPsecConfigDataTypeSad,
             (EFI_IPSEC_CONFIG_SELECTOR *) &SaId,
             &SaData,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Store the local SA into SAD.
  //  
  ChildSaSession->SpdSelector->RemoteAddressCount = ChildSaSession->SpdSelector->LocalAddressCount;
  ChildSaSession->SpdSelector->RemoteAddress      = ChildSaSession->SpdSelector->LocalAddress;

  ChildSaSession->SpdSelector->LocalAddress       = TempAddressInfo;
  ChildSaSession->SpdSelector->LocalAddressCount  = TempAddressCount;
  
  SaId.Spi = ChildSaSession->RemotePeerSpi;

  CopyMem (&SaId.DestAddress, &ChildSaSession->SessionCommon.RemotePeerIp, sizeof (EFI_IP_ADDRESS));
  CopyMem (&SaData.AlgoInfo, &ChildSaSession->ChildKeymats.RemotePeerInfo, sizeof (EFI_IPSEC_ALGO_INFO));
  SaData.SpdSelector = ChildSaSession->SpdSelector;

  //
  // If it is tunnel mode, should add the TunnelDest and TunnelSource for SaData.
  //
  if (SaData.Mode == EfiIPsecTunnel) {
    CopyMem (
      &SaData.TunnelSourceAddress,
      &ChildSaSession->Spd->Data->ProcessingPolicy->TunnelOption->LocalTunnelAddress,
      sizeof (EFI_IP_ADDRESS)
      );
    CopyMem (
      &SaData.TunnelDestinationAddress,
      &ChildSaSession->Spd->Data->ProcessingPolicy->TunnelOption->RemoteTunnelAddress,
      sizeof (EFI_IP_ADDRESS)
      );
  }

  Status = EfiIpSecConfigSetData (
             &Private->IpSecConfig,
             IPsecConfigDataTypeSad,
             (EFI_IPSEC_CONFIG_SELECTOR *) &SaId,
             &SaData,
             NULL
             );

  ASSERT_EFI_ERROR (Status);
}

/**
  Call back function of the IKE life time is over.

  This function will mark the related IKE SA Session as deleting and trigger a 
  Information negotiation.

  @param[in]    Event     The signaled Event.
  @param[in]    Context   Pointer to data passed by caller.
  
**/
VOID
EFIAPI
Ikev2LifetimeNotify (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  IKEV2_SA_SESSION            *IkeSaSession;
  IKEV2_CHILD_SA_SESSION      *ChildSaSession;
  IKEV2_SESSION_COMMON        *SessionCommon;

  ASSERT (Context != NULL);
  SessionCommon = (IKEV2_SESSION_COMMON *) Context;

  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
    IkeSaSession = IKEV2_SA_SESSION_FROM_COMMON (SessionCommon);
    DEBUG ((
      DEBUG_INFO,
      "\n---IkeSa Lifetime is out(cookie_i, cookie_r):(0x%lx, 0x%lx)---\n",
      IkeSaSession->InitiatorCookie,
      IkeSaSession->ResponderCookie
      ));

    //
    // Change the  IKE SA Session's State to IKE_STATE_SA_DELETING.
    //
    IKEV2_DUMP_STATE (IkeSaSession->SessionCommon.State, IkeStateSaDeleting);
    IkeSaSession->SessionCommon.State = IkeStateSaDeleting;

  } else {
    ChildSaSession = IKEV2_CHILD_SA_SESSION_FROM_COMMON (SessionCommon);
    IkeSaSession   = ChildSaSession->IkeSaSession;

    //
    // Link the timeout child SA to the DeleteSaList.
    //
    InsertTailList (&IkeSaSession->DeleteSaList, &ChildSaSession->ByDelete);

    //
    // Change the Child SA Session's State to IKE_STATE_SA_DELETING.
    //    
    DEBUG ((
      DEBUG_INFO,
      "\n------ChildSa Lifetime is out(SPI):(0x%x)------\n",
      ChildSaSession->LocalPeerSpi
      ));
  }

  //
  // TODO: Send the delete info packet or delete silently
  //
  mIkev2Exchange.NegotiateInfo ((UINT8 *) IkeSaSession, NULL);
}

/**
  This function will be called if the TimeOut Event is signaled.

  @param[in]  Event      The signaled Event.
  @param[in]  Context    The data passed by caller.

**/
VOID
EFIAPI
Ikev2ResendNotify (
  IN EFI_EVENT                 Event,
  IN VOID                      *Context
  )
{
  IPSEC_PRIVATE_DATA           *Private;
  IKEV2_SA_SESSION             *IkeSaSession;
  IKEV2_CHILD_SA_SESSION       *ChildSaSession;
  IKEV2_SESSION_COMMON         *SessionCommon;
  LIST_ENTRY                   *ChildSaEntry;
  UINT8                        Value;
  EFI_STATUS                   Status;

  ASSERT (Context != NULL); 
  IkeSaSession   = NULL;
  ChildSaSession = NULL;
  SessionCommon  = (IKEV2_SESSION_COMMON *) Context;
  Private        = SessionCommon->Private;

  //
  // Remove the SA session from the processing list if exceed the max retry.
  //
  if (SessionCommon->RetryCount > IKE_MAX_RETRY) {
    if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {
      IkeSaSession = IKEV2_SA_SESSION_FROM_COMMON (SessionCommon);
      if (IkeSaSession->SessionCommon.State == IkeStateSaDeleting) {

        //
        // If the IkeSaSession is initiator, delete all its Child SAs before removing IKE SA.
        // If the IkesaSession is responder, all ChildSa has been remove in Ikev2HandleInfo();
        //
        for (ChildSaEntry = IkeSaSession->ChildSaEstablishSessionList.ForwardLink;
             ChildSaEntry != &IkeSaSession->ChildSaEstablishSessionList;
        ) {
          ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (ChildSaEntry);
          //
          // Move to next ChildSa Entry.
          //
          ChildSaEntry = ChildSaEntry->ForwardLink;
          //
          // Delete LocalSpi & RemoteSpi and remove the ChildSaSession from the
          // EstablishedChildSaList.
          //
          Ikev2ChildSaSilentDelete (IkeSaSession, ChildSaSession->LocalPeerSpi);
        }

        //
        // If the IKE SA Delete Payload wasn't sent out successfully, Delete it from the EstablishedList.
        //
        Ikev2SaSessionRemove (&Private->Ikev2EstablishedList, &SessionCommon->RemotePeerIp);

        if (Private != NULL && Private->IsIPsecDisabling) {
            //
            // After all IKE SAs were deleted, set the IPSEC_STATUS_DISABLED value in
            // IPsec status variable.
            //
            if (IsListEmpty (&Private->Ikev1EstablishedList) && IsListEmpty (&Private->Ikev2EstablishedList)) {
              Value = IPSEC_STATUS_DISABLED;
              Status = gRT->SetVariable (
                              IPSECCONFIG_STATUS_NAME,
                              &gEfiIpSecConfigProtocolGuid,
                              EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                              sizeof (Value),
                              &Value
                              );
              if (!EFI_ERROR (Status)) {
                //
                // Set the Disabled Flag in Private data.
                //
                Private->IpSec.DisabledFlag = TRUE;
                Private->IsIPsecDisabling   = FALSE;
              }
            }
          }
      } else {
        Ikev2SaSessionRemove (&Private->Ikev2SessionList, &SessionCommon->RemotePeerIp);
      }
      Ikev2SaSessionFree (IkeSaSession);

    } else {

      //
      // If the packet sent by Child SA.
      //
      ChildSaSession = IKEV2_CHILD_SA_SESSION_FROM_COMMON (SessionCommon);
      IkeSaSession   = ChildSaSession->IkeSaSession;
      if (ChildSaSession->SessionCommon.State == IkeStateSaDeleting) {

        //
        // Established Child SA should be remove from the SAD entry and 
        // DeleteList. The function of Ikev2DeleteChildSaSilent() will remove 
        // the childSA from the IkeSaSession->ChildSaEstablishedList. So there 
        // is no need to remove it here.
        //
        Ikev2ChildSaSilentDelete (IkeSaSession, ChildSaSession->LocalPeerSpi);
        Ikev2ChildSaSessionRemove (
          &IkeSaSession->DeleteSaList,
          ChildSaSession->LocalPeerSpi,
          IKEV2_DELET_CHILDSA_LIST
          );
      } else {
        Ikev2ChildSaSessionRemove (
          &IkeSaSession->ChildSaSessionList,
          ChildSaSession->LocalPeerSpi,
          IKEV2_ESTABLISHING_CHILDSA_LIST
          );
      }

      Ikev2ChildSaSessionFree (ChildSaSession);
    }
    return ;
  }

  //
  // Increase the retry count.
  //
  SessionCommon->RetryCount++;
  DEBUG ((DEBUG_INFO, ">>>Resending the last packet ...\n"));

  //
  // Resend the last packet.
  //
  Ikev2SendIkePacket (
    SessionCommon->UdpService,
    (UINT8*)SessionCommon,
    SessionCommon->LastSentPacket,
    0
    );
}

/**
  Copy ChildSaSession->Spd->Selector to ChildSaSession->SpdSelector.

  ChildSaSession->SpdSelector stores the real Spdselector for its SA. Sometime,
  the SpdSelector in ChildSaSession is more accurated or the scope is smaller 
  than the one in ChildSaSession->Spd, especially for the tunnel mode.
    
  @param[in, out]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION related to.
  
**/
VOID
Ikev2ChildSaSessionSpdSelectorCreate (
  IN OUT IKEV2_CHILD_SA_SESSION *ChildSaSession
  ) 
{
  if (ChildSaSession->Spd != NULL && ChildSaSession->Spd->Selector != NULL) {
    if (ChildSaSession->SpdSelector == NULL) {
      ChildSaSession->SpdSelector = AllocateZeroPool (sizeof (EFI_IPSEC_SPD_SELECTOR));
      ASSERT (ChildSaSession->SpdSelector != NULL);
    }
    CopyMem (
      ChildSaSession->SpdSelector, 
      ChildSaSession->Spd->Selector, 
      sizeof (EFI_IPSEC_SPD_SELECTOR)
      );
    ChildSaSession->SpdSelector->RemoteAddress = AllocateCopyPool (
                                                   ChildSaSession->Spd->Selector->RemoteAddressCount * 
                                                   sizeof (EFI_IP_ADDRESS_INFO), 
                                                   ChildSaSession->Spd->Selector->RemoteAddress
                                                   );
    ChildSaSession->SpdSelector->LocalAddress = AllocateCopyPool (
                                                  ChildSaSession->Spd->Selector->LocalAddressCount * 
                                                  sizeof (EFI_IP_ADDRESS_INFO), 
                                                  ChildSaSession->Spd->Selector->LocalAddress
                                                  );

    ASSERT (ChildSaSession->SpdSelector->LocalAddress != NULL);
    ASSERT (ChildSaSession->SpdSelector->RemoteAddress != NULL);

    ChildSaSession->SpdSelector->RemoteAddressCount = ChildSaSession->Spd->Selector->RemoteAddressCount;
    ChildSaSession->SpdSelector->LocalAddressCount = ChildSaSession->Spd->Selector->LocalAddressCount; 
  }
}

/**
  Generate a ChildSa Session and insert it into related IkeSaSession.

  @param[in]  IkeSaSession    Pointer to related IKEV2_SA_SESSION.
  @param[in]  UdpService      Pointer to related IKE_UDP_SERVICE.

  @return pointer of IKEV2_CHILD_SA_SESSION.

**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionCreate (
  IN IKEV2_SA_SESSION   *IkeSaSession,
  IN IKE_UDP_SERVICE     *UdpService
  )
{
  IKEV2_CHILD_SA_SESSION    *ChildSaSession;
  IKEV2_SESSION_COMMON      *ChildSaCommon;

  //
  // Create a new ChildSaSession.Insert it into processing list and initiate the common parameters.
  //
  ChildSaSession = Ikev2ChildSaSessionAlloc (UdpService, IkeSaSession);
  ASSERT (ChildSaSession != NULL);

  //
  // Set the specific parameters.
  // 
  ChildSaSession->Spd        = IkeSaSession->Spd;
  ChildSaCommon              = &ChildSaSession->SessionCommon;
  ChildSaCommon->IsInitiator = IkeSaSession->SessionCommon.IsInitiator;
  if (IkeSaSession->SessionCommon.State == IkeStateAuth) {
    ChildSaCommon->State     = IkeStateAuth;
    IKEV2_DUMP_STATE (ChildSaCommon->State, IkeStateAuth);
  } else {
    ChildSaCommon->State     = IkeStateCreateChild;
    IKEV2_DUMP_STATE (ChildSaCommon->State, IkeStateCreateChild);
  }

  //
  // If SPD->Selector is not NULL, copy it to the ChildSaSession->SpdSelector.
  // The ChildSaSession->SpdSelector might be changed after the traffic selector
  // negoniation and it will be copied into the SAData after ChildSA established.
  //
  Ikev2ChildSaSessionSpdSelectorCreate (ChildSaSession);

  //
  // Copy first NiBlock and NrBlock to ChildSa Session
  //
  ChildSaSession->NiBlock   = AllocateZeroPool (IkeSaSession->NiBlkSize);
  ASSERT (ChildSaSession->NiBlock != NULL);
  ChildSaSession->NiBlkSize = IkeSaSession->NiBlkSize;
  CopyMem (ChildSaSession->NiBlock, IkeSaSession->NiBlock, IkeSaSession->NiBlkSize);

  ChildSaSession->NrBlock   = AllocateZeroPool (IkeSaSession->NrBlkSize);
  ASSERT (ChildSaSession->NrBlock != NULL);
  ChildSaSession->NrBlkSize = IkeSaSession->NrBlkSize;
  CopyMem (ChildSaSession->NrBlock, IkeSaSession->NrBlock, IkeSaSession->NrBlkSize);

  //
  //  Only if the Create Child SA is called for the IKE_INIT Exchange and 
  //  IkeSaSession is initiator (Only Initiator's SPD is not NULL), Set the 
  //  Traffic Selectors related information here.
  //
  if (IkeSaSession->SessionCommon.State == IkeStateAuth && IkeSaSession->Spd != NULL) {
    ChildSaSession->ProtoId = IkeSaSession->Spd->Selector->NextLayerProtocol;
    ChildSaSession->LocalPort = IkeSaSession->Spd->Selector->LocalPort;
    ChildSaSession->RemotePort = IkeSaSession->Spd->Selector->RemotePort;
  }

  //
  // Insert the new ChildSaSession into processing child SA list.
  //
  Ikev2ChildSaSessionInsert (&IkeSaSession->ChildSaSessionList, ChildSaSession);
  return ChildSaSession;
}

/**
  Check if the SPD is related to the input Child SA Session.

  This function is the subfunction of Ikev1AssociateSpdEntry(). It is the call
  back function of IpSecVisitConfigData(). 
  

  @param[in]  Type               Type of the input Config Selector.
  @param[in]  Selector           Pointer to the Configure Selector to be checked. 
  @param[in]  Data               Pointer to the Configure Selector's Data passed 
                                 from the caller.
  @param[in]  SelectorSize       The buffer size of Selector.
  @param[in]  DataSize           The buffer size of the Data.
  @param[in]  Context            The data passed from the caller. It is a Child
                                 SA Session in this context.

  @retval EFI_SUCCESS        The SPD Selector is not related to the Child SA Session. 
  @retval EFI_ABORTED        The SPD Selector is related to the Child SA session and 
                             set the ChildSaSession->Spd to point to this SPD Selector.

**/
EFI_STATUS
Ikev2MatchSpdEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE     Type,
  IN EFI_IPSEC_CONFIG_SELECTOR      *Selector,
  IN VOID                           *Data,
  IN UINTN                          SelectorSize,
  IN UINTN                          DataSize,
  IN VOID                           *Context
  )
{
  IKEV2_CHILD_SA_SESSION  *ChildSaSession;
  EFI_IPSEC_SPD_SELECTOR  *SpdSelector;
  EFI_IPSEC_SPD_DATA      *SpdData;
  BOOLEAN                 IsMatch;
  UINT8                   IpVersion;

  ASSERT (Type == IPsecConfigDataTypeSpd);
  SpdData = (EFI_IPSEC_SPD_DATA *) Data;
  //
  // Bypass all non-protect SPD entry first
  //
  if (SpdData->Action != EfiIPsecActionProtect) {
    return EFI_SUCCESS;
  }

  ChildSaSession  = (IKEV2_CHILD_SA_SESSION *) Context;
  IpVersion       = ChildSaSession->SessionCommon.UdpService->IpVersion;
  SpdSelector     = (EFI_IPSEC_SPD_SELECTOR *) Selector;  
  IsMatch         = TRUE;

  if (SpdSelector->NextLayerProtocol == EFI_IP_PROTO_UDP &&
      SpdSelector->LocalPort == IKE_DEFAULT_PORT &&
      SpdSelector->LocalPortRange == 0 &&
      SpdSelector->RemotePort == IKE_DEFAULT_PORT &&
      SpdSelector->RemotePortRange == 0
      ) {
    //
    // TODO: Skip IKE Policy here or set a SPD entry?
    //
    return EFI_SUCCESS;
  }

  if (SpdSelector->NextLayerProtocol != EFI_IPSEC_ANY_PROTOCOL &&
      SpdSelector->NextLayerProtocol != ChildSaSession->ProtoId
      ) {
    IsMatch = FALSE;
  }

  if (SpdSelector->LocalPort != EFI_IPSEC_ANY_PORT && SpdSelector->LocalPort != ChildSaSession->LocalPort) {
    IsMatch = FALSE;
  }

  if (SpdSelector->RemotePort != EFI_IPSEC_ANY_PORT && SpdSelector->RemotePort != ChildSaSession->RemotePort) {
    IsMatch = FALSE;
  }

  IsMatch = (BOOLEAN) (IsMatch && 
                       IpSecMatchIpAddress (
                         IpVersion,
                         &ChildSaSession->SessionCommon.LocalPeerIp,
                         SpdSelector->LocalAddress,
                         SpdSelector->LocalAddressCount
                         ));

  IsMatch = (BOOLEAN) (IsMatch && 
                       IpSecMatchIpAddress (
                         IpVersion,
                         &ChildSaSession->SessionCommon.RemotePeerIp,
                         SpdSelector->RemoteAddress,
                         SpdSelector->RemoteAddressCount
                         ));

  if (IsMatch) {
    ChildSaSession->Spd = IkeSearchSpdEntry (SpdSelector);
    return EFI_ABORTED;
  } else {
    return EFI_SUCCESS;
  }
}

/**
  Check if the Algorithm ID is supported.

  @param[in]  AlgorithmId The specified Algorithm ID.
  @param[in]  Type        The type used to indicate the Algorithm is for Encrypt or
                          Authentication.

  @retval     TRUE        If the Algorithm ID is supported.
  @retval     FALSE       If the Algorithm ID is not supported.

**/
BOOLEAN
Ikev2IsSupportAlg (
  IN UINT16 AlgorithmId,
  IN UINT8  Type
  )
{
  UINT8 Index;
  switch (Type) {
  case IKE_ENCRYPT_TYPE :
    for (Index = 0; Index < IKEV2_SUPPORT_ENCRYPT_ALGORITHM_NUM; Index++) {
      if (mIkev2EncryptAlgorithmList[Index] == AlgorithmId) {
        return TRUE;
      }
    }
    break;

  case IKE_AUTH_TYPE :
    for (Index = 0; Index < IKEV2_SUPPORT_AUTH_ALGORITHM_NUM; Index++) {
      if (mIkev2AuthAlgorithmList[Index] == AlgorithmId) {
        return TRUE;
      }
    }
    break;

  case IKE_DH_TYPE :
    for (Index = 0; Index < IKEV2_SUPPORT_DH_ALGORITHM_NUM; Index++) {
      if (mIkev2DhGroupAlgorithmList[Index] == AlgorithmId) {
        return TRUE;
      }
    }
    break;

  case IKE_PRF_TYPE :
    for (Index = 0; Index < IKEV2_SUPPORT_PRF_ALGORITHM_NUM; Index++) {
      if (mIkev2PrfAlgorithmList[Index] == AlgorithmId) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**
  Get the preferred algorithm types from ProposalData.

  @param[in]  ProposalData              Pointer to related IKEV2_PROPOSAL_DATA.
  @param[out] PreferEncryptAlgorithm    Output of preferred encrypt algorithm.
  @param[out] PreferIntegrityAlgorithm  Output of preferred integrity algorithm. 
  @param[out] PreferPrfAlgorithm        Output of preferred PRF algorithm. Only 
                                        for IKE SA.
  @param[out] PreferDhGroup             Output of preferred DH group. Only for 
                                        IKE SA.
  @param[out] PreferEncryptKeylength    Output of preferred encrypt key length 
                                        in bytes.
  @param[out] IsSupportEsn              Output of value about the Extented Sequence
                                        Number is support or not. Only for Child SA.
  @param[in]  IsChildSa                 If it is ture, the ProposalData is for IKE
                                        SA. Otherwise the proposalData is for Child SA.

**/
VOID
Ikev2ParseProposalData (
  IN     IKEV2_PROPOSAL_DATA  *ProposalData, 
     OUT UINT16               *PreferEncryptAlgorithm,
     OUT UINT16               *PreferIntegrityAlgorithm,
     OUT UINT16               *PreferPrfAlgorithm,
     OUT UINT16               *PreferDhGroup,
     OUT UINTN                *PreferEncryptKeylength,
     OUT BOOLEAN              *IsSupportEsn,
  IN     BOOLEAN              IsChildSa
) 
{
  IKEV2_TRANSFORM_DATA *TransformData;
  UINT8                TransformIndex;

  //
  // Check input parameters.
  //
  if (ProposalData == NULL ||
      PreferEncryptAlgorithm == NULL || 
      PreferIntegrityAlgorithm == NULL ||
      PreferEncryptKeylength == NULL
      ) {
    return;
  }

  if (IsChildSa) {
    if (IsSupportEsn == NULL) {
      return;
    }
  } else {
    if (PreferPrfAlgorithm == NULL || PreferDhGroup == NULL) {
      return;
    }
  }  

  TransformData = (IKEV2_TRANSFORM_DATA *)(ProposalData + 1);
  for (TransformIndex = 0; TransformIndex < ProposalData->NumTransforms; TransformIndex++) {
    switch (TransformData->TransformType) {          
    //
    // For IKE SA there are four algorithm types. Encryption Algorithm, Pseudo-random Function, 
    // Integrity Algorithm, Diffie-Hellman Group. For Child SA, there are three algorithm types. 
    // Encryption Algorithm, Integrity Algorithm, Extended Sequence Number.
    //
    case IKEV2_TRANSFORM_TYPE_ENCR:
      if (*PreferEncryptAlgorithm == 0 && Ikev2IsSupportAlg (TransformData->TransformId, IKE_ENCRYPT_TYPE)) {
        //
        // Check the attribute value. According to RFC, only Keylength is support.
        //
        if (TransformData->Attribute.AttrType == IKEV2_ATTRIBUTE_TYPE_KEYLEN) {
          //
          // If the Keylength is not support, continue to check the next one.
          //
          if (IpSecGetEncryptKeyLength ((UINT8)TransformData->TransformId) != (UINTN)(TransformData->Attribute.Attr.AttrValue >> 3)){
            break;
          } else {
            *PreferEncryptKeylength = TransformData->Attribute.Attr.AttrValue;
          }
        }
        *PreferEncryptAlgorithm = TransformData->TransformId;
      }
      break;

    case IKEV2_TRANSFORM_TYPE_PRF :
      if (!IsChildSa) {
        if (*PreferPrfAlgorithm == 0 && Ikev2IsSupportAlg (TransformData->TransformId, IKE_PRF_TYPE)) {
          *PreferPrfAlgorithm = TransformData->TransformId;
        }
      }       
      break;

    case IKEV2_TRANSFORM_TYPE_INTEG :
      if (*PreferIntegrityAlgorithm == 0 && Ikev2IsSupportAlg (TransformData->TransformId, IKE_AUTH_TYPE)) {
        *PreferIntegrityAlgorithm = TransformData->TransformId;
      }
      break;
      
    case IKEV2_TRANSFORM_TYPE_DH :
      if (!IsChildSa) {
        if (*PreferDhGroup == 0 && Ikev2IsSupportAlg (TransformData->TransformId, IKE_DH_TYPE)) {
          *PreferDhGroup = TransformData->TransformId;
        }
      }        
      break;
    
    case IKEV2_TRANSFORM_TYPE_ESN :
      if (IsChildSa) {
        if (TransformData->TransformId != 0) {
          *IsSupportEsn = TRUE;
        }
      }        
      break;

    default:
      break;
    }
    TransformData = (IKEV2_TRANSFORM_DATA *)(TransformData + 1);
  }
}

/**
  Parse the received Initial Exchange Packet.
  
  This function parse the SA Payload and Key Payload to find out the cryptographic 
  suite for the further IKE negotiation and fill it into the IKE SA Session's 
  CommonSession->SaParams.

  @param[in, out]  IkeSaSession  Pointer to related IKEV2_SA_SESSION.
  @param[in]       SaPayload     The received packet.
  @param[in]       Type          The received packet IKE header flag. 

  @retval          TRUE          If the SA proposal in Packet is acceptable.
  @retval          FALSE         If the SA proposal in Packet is not acceptable.

**/
BOOLEAN
Ikev2SaParseSaPayload (
  IN OUT IKEV2_SA_SESSION *IkeSaSession,
  IN     IKE_PAYLOAD      *SaPayload,
  IN     UINT8            Type
  )
{
  IKEV2_PROPOSAL_DATA  *ProposalData;
  UINT8                ProposalIndex;
  UINT16               PreferEncryptAlgorithm;
  UINT16               PreferIntegrityAlgorithm;
  UINT16               PreferPrfAlgorithm;
  UINT16               PreferDhGroup;
  UINTN                PreferEncryptKeylength;
  UINT16               EncryptAlgorithm;
  UINT16               IntegrityAlgorithm;
  UINT16               PrfAlgorithm;
  UINT16               DhGroup;
  UINTN                EncryptKeylength;
  BOOLEAN              IsMatch;
  UINTN                SaDataSize;

  PreferPrfAlgorithm       = 0;
  PreferIntegrityAlgorithm = 0;
  PreferDhGroup            = 0;
  PreferEncryptAlgorithm   = 0;
  PreferEncryptKeylength   = 0;
  PrfAlgorithm             = 0;
  IntegrityAlgorithm       = 0;
  DhGroup                  = 0;
  EncryptAlgorithm         = 0;
  EncryptKeylength         = 0;
  IsMatch                  = FALSE;

  if (Type == IKE_HEADER_FLAGS_INIT) {
    ProposalData   = (IKEV2_PROPOSAL_DATA *)((IKEV2_SA_DATA *)SaPayload->PayloadBuf + 1);
    for (ProposalIndex = 0; ProposalIndex < ((IKEV2_SA_DATA *)SaPayload->PayloadBuf)->NumProposals; ProposalIndex++) {
      //
      // Iterate each proposal to find the perfered one.
      //
      if (ProposalData->ProtocolId == IPSEC_PROTO_ISAKMP && ProposalData->NumTransforms >= 4) {
        //
        // Get the preferred algorithms.
        //
        Ikev2ParseProposalData (
          ProposalData, 
          &PreferEncryptAlgorithm,
          &PreferIntegrityAlgorithm,
          &PreferPrfAlgorithm,
          &PreferDhGroup,
          &PreferEncryptKeylength,
          NULL,
          FALSE
          );

        if (PreferEncryptAlgorithm != 0 &&
              PreferIntegrityAlgorithm != 0 &&
              PreferPrfAlgorithm != 0 && 
              PreferDhGroup != 0
              ) {
            //
            // Find the matched one. 
            //
            IkeSaSession->SessionCommon.SaParams = AllocateZeroPool (sizeof (IKEV2_SA_PARAMS));
            ASSERT (IkeSaSession->SessionCommon.SaParams != NULL);
            IkeSaSession->SessionCommon.SaParams->EncAlgId   = PreferEncryptAlgorithm;
            IkeSaSession->SessionCommon.SaParams->EnckeyLen  = PreferEncryptKeylength;
            IkeSaSession->SessionCommon.SaParams->DhGroup    = PreferDhGroup;
            IkeSaSession->SessionCommon.SaParams->Prf        = PreferPrfAlgorithm;
            IkeSaSession->SessionCommon.SaParams->IntegAlgId = PreferIntegrityAlgorithm;
            IkeSaSession->SessionCommon.PreferDhGroup        = PreferDhGroup;

            //
            // Save the matched one in IKEV2_SA_DATA for furthure calculation.
            //
            SaDataSize           = sizeof (IKEV2_SA_DATA) +
                                   sizeof (IKEV2_PROPOSAL_DATA) +
                                   sizeof (IKEV2_TRANSFORM_DATA) * 4;
            IkeSaSession->SaData = AllocateZeroPool (SaDataSize);
            ASSERT (IkeSaSession->SaData != NULL);

            IkeSaSession->SaData->NumProposals  = 1;

            //
            // BUGBUG: Suppose the matched proposal only has 4 transforms. If
            // The matched Proposal has more than 4 transforms means it contains
            // one than one transform with same type.
            //
            CopyMem (
              (IKEV2_PROPOSAL_DATA *) (IkeSaSession->SaData + 1), 
               ProposalData, 
               SaDataSize - sizeof (IKEV2_SA_DATA)
              );

            ((IKEV2_PROPOSAL_DATA *) (IkeSaSession->SaData + 1))->ProposalIndex = 1;
            return TRUE;
          } else {
            PreferEncryptAlgorithm   = 0;
            PreferIntegrityAlgorithm = 0;
            PreferPrfAlgorithm       = 0;
            PreferDhGroup            = 0;
            PreferEncryptKeylength   = 0;
          }
      }
      //
      // Point to next Proposal.
      //
      ProposalData = (IKEV2_PROPOSAL_DATA*)((UINT8*)(ProposalData + 1) + 
                     ProposalData->NumTransforms * sizeof (IKEV2_TRANSFORM_DATA));
    }
  } else if (Type == IKE_HEADER_FLAGS_RESPOND) {
    //
    // First check the SA proposal's ProtoctolID and Transform Numbers. Since it is 
    // the responded SA proposal, suppose it only has one proposal and the transform Numbers 
    // is 4. 
    //
    ProposalData  = (IKEV2_PROPOSAL_DATA *)((IKEV2_SA_DATA *) SaPayload->PayloadBuf + 1);
    if (ProposalData->ProtocolId != IPSEC_PROTO_ISAKMP || ProposalData->NumTransforms != 4) {
      return FALSE;
    }
    //
    // Get the preferred algorithms. 
    //
    Ikev2ParseProposalData (
      ProposalData,
      &PreferEncryptAlgorithm,
      &PreferIntegrityAlgorithm,
      &PreferPrfAlgorithm,
      &PreferDhGroup,
      &PreferEncryptKeylength,
      NULL, 
      FALSE
      );
    // 
    // Check if the Sa proposal data from received packet is in the IkeSaSession->SaData.
    //
    ProposalData = (IKEV2_PROPOSAL_DATA *) (IkeSaSession->SaData + 1);

    for (ProposalIndex = 0; ProposalIndex < IkeSaSession->SaData->NumProposals && (!IsMatch); ProposalIndex++) {
      Ikev2ParseProposalData (
          ProposalData, 
          &EncryptAlgorithm,
          &IntegrityAlgorithm,
          &PrfAlgorithm,
          &DhGroup,
          &EncryptKeylength,
          NULL,
          FALSE
          );
      if (EncryptAlgorithm == PreferEncryptAlgorithm &&
          EncryptKeylength == PreferEncryptKeylength &&
          IntegrityAlgorithm == PreferIntegrityAlgorithm &&
          PrfAlgorithm == PreferPrfAlgorithm &&
          DhGroup      == PreferDhGroup
          ) {
        IsMatch = TRUE;
      } else {
        EncryptAlgorithm   = 0;
        IntegrityAlgorithm = 0;
        PrfAlgorithm       = 0;
        DhGroup            = 0;
        EncryptKeylength   = 0; 
      }

      ProposalData = (IKEV2_PROPOSAL_DATA*)((UINT8*)(ProposalData + 1) + 
                     ProposalData->NumTransforms * sizeof (IKEV2_TRANSFORM_DATA));    
    }

    if (IsMatch) {
        IkeSaSession->SessionCommon.SaParams = AllocateZeroPool (sizeof (IKEV2_SA_PARAMS));
        ASSERT (IkeSaSession->SessionCommon.SaParams != NULL);
        IkeSaSession->SessionCommon.SaParams->EncAlgId   = PreferEncryptAlgorithm;
        IkeSaSession->SessionCommon.SaParams->EnckeyLen  = PreferEncryptKeylength;
        IkeSaSession->SessionCommon.SaParams->DhGroup    = PreferDhGroup;
        IkeSaSession->SessionCommon.SaParams->Prf        = PreferPrfAlgorithm;
        IkeSaSession->SessionCommon.SaParams->IntegAlgId = PreferIntegrityAlgorithm;
        IkeSaSession->SessionCommon.PreferDhGroup        = PreferDhGroup;
      
        return TRUE;
    }
  }
  return FALSE;
}

/**
  Parse the received Authentication Exchange Packet.
  
  This function parse the SA Payload and Key Payload to find out the cryptographic
  suite for the ESP and fill it into the Child SA Session's CommonSession->SaParams.
  
  @param[in, out]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION related to 
                                   this Authentication Exchange.
  @param[in]       SaPayload       The received packet.
  @param[in]       Type            The IKE header's flag of received packet . 
  
  @retval          TRUE            If the SA proposal in Packet is acceptable.
  @retval          FALSE           If the SA proposal in Packet is not acceptable.

**/
BOOLEAN
Ikev2ChildSaParseSaPayload (
  IN OUT IKEV2_CHILD_SA_SESSION *ChildSaSession,
  IN     IKE_PAYLOAD            *SaPayload,
  IN     UINT8                  Type
  )
{
  IKEV2_PROPOSAL_DATA  *ProposalData;
  UINT8                ProposalIndex;
  UINT16               PreferEncryptAlgorithm;
  UINT16               PreferIntegrityAlgorithm;
  UINTN                PreferEncryptKeylength;
  BOOLEAN              PreferIsSupportEsn;
  UINT16               EncryptAlgorithm;
  UINT16               IntegrityAlgorithm;
  UINTN                EncryptKeylength;
  BOOLEAN              IsSupportEsn;
  BOOLEAN              IsMatch;
  UINTN                SaDataSize;


  PreferIntegrityAlgorithm = 0;
  PreferEncryptAlgorithm   = 0;
  PreferEncryptKeylength   = 0;
  IntegrityAlgorithm       = 0;
  EncryptAlgorithm         = 0;
  EncryptKeylength         = 0;
  IsMatch                  = TRUE;
  IsSupportEsn             = FALSE;
  PreferIsSupportEsn       = FALSE;

  if (Type == IKE_HEADER_FLAGS_INIT) {
    ProposalData   = (IKEV2_PROPOSAL_DATA *)((IKEV2_SA_DATA *) SaPayload->PayloadBuf + 1);
    for (ProposalIndex = 0; ProposalIndex < ((IKEV2_SA_DATA *) SaPayload->PayloadBuf)->NumProposals; ProposalIndex++) {
      //
      // Iterate each proposal to find the preferred one.
      //
      if (ProposalData->ProtocolId == IPSEC_PROTO_IPSEC_ESP && ProposalData->NumTransforms >= 3) {
        //
        // Get the preferred algorithm.
        //
        Ikev2ParseProposalData (
          ProposalData,
          &PreferEncryptAlgorithm,
          &PreferIntegrityAlgorithm,
          NULL,
          NULL,
          &PreferEncryptKeylength,
          &IsSupportEsn,
          TRUE
          );
        //
        // Don't support the ESN now.
        //
        if (PreferEncryptAlgorithm != 0 && 
            PreferIntegrityAlgorithm != 0 &&
            !IsSupportEsn
            ) {
          //
          // Find the matched one. 
          //
          ChildSaSession->SessionCommon.SaParams = AllocateZeroPool (sizeof (IKEV2_SA_PARAMS));
          ASSERT (ChildSaSession->SessionCommon.SaParams != NULL);
          ChildSaSession->SessionCommon.SaParams->EncAlgId   = PreferEncryptAlgorithm;
          ChildSaSession->SessionCommon.SaParams->EnckeyLen  = PreferEncryptKeylength;
          ChildSaSession->SessionCommon.SaParams->IntegAlgId = PreferIntegrityAlgorithm;
          CopyMem (&ChildSaSession->RemotePeerSpi, ProposalData->Spi, sizeof (ChildSaSession->RemotePeerSpi));

          //
          // Save the matched one in IKEV2_SA_DATA for furthure calculation.
          //
          SaDataSize           = sizeof (IKEV2_SA_DATA) +
                                 sizeof (IKEV2_PROPOSAL_DATA) +
                                 sizeof (IKEV2_TRANSFORM_DATA) * 4;

          ChildSaSession->SaData = AllocateZeroPool (SaDataSize);
          ASSERT (ChildSaSession->SaData != NULL);

          ChildSaSession->SaData->NumProposals  = 1;

          //
          // BUGBUG: Suppose there are 4 transforms in the matched proposal. If
          // the matched Proposal has more than 4 transforms that means there 
          // are more than one transform with same type.
          //
          CopyMem (
            (IKEV2_PROPOSAL_DATA *) (ChildSaSession->SaData + 1),
             ProposalData,
             SaDataSize - sizeof (IKEV2_SA_DATA)
            );

          ((IKEV2_PROPOSAL_DATA *) (ChildSaSession->SaData + 1))->ProposalIndex = 1;

          ((IKEV2_PROPOSAL_DATA *) (ChildSaSession->SaData + 1))->Spi = AllocateCopyPool (
                                                                          sizeof (ChildSaSession->LocalPeerSpi), 
                                                                          &ChildSaSession->LocalPeerSpi
                                                                          );
          ASSERT (((IKEV2_PROPOSAL_DATA *) (ChildSaSession->SaData + 1))->Spi != NULL);
          return TRUE;

        } else {
          PreferEncryptAlgorithm   = 0;
          PreferIntegrityAlgorithm = 0;
          IsSupportEsn             = TRUE;
        }
      }
      //
      // Point to next Proposal
      //
      ProposalData = (IKEV2_PROPOSAL_DATA *)((UINT8 *)(ProposalData + 1) + 
                     ProposalData->NumTransforms * sizeof (IKEV2_TRANSFORM_DATA));
    }
  } else if (Type == IKE_HEADER_FLAGS_RESPOND) {
    //
    // First check the SA proposal's ProtoctolID and Transform Numbers. Since it is 
    // the responded SA proposal, suppose it only has one proposal and the transform Numbers 
    // is 3. 
    //
    ProposalData  = (IKEV2_PROPOSAL_DATA *)((IKEV2_SA_DATA *)SaPayload->PayloadBuf + 1);
    if (ProposalData->ProtocolId != IPSEC_PROTO_IPSEC_ESP || ProposalData->NumTransforms != 3) {
      return FALSE;
    }
    //
    // Get the preferred algorithms.
    //
    Ikev2ParseProposalData (
      ProposalData,
      &PreferEncryptAlgorithm,
      &PreferIntegrityAlgorithm,
      NULL,
      NULL,
      &PreferEncryptKeylength,
      &PreferIsSupportEsn,
      TRUE
      );

    ProposalData = (IKEV2_PROPOSAL_DATA *) (ChildSaSession->SaData + 1);

    for (ProposalIndex = 0; ProposalIndex < ChildSaSession->SaData->NumProposals && (!IsMatch); ProposalIndex++) {
      Ikev2ParseProposalData (
          ProposalData, 
          &EncryptAlgorithm,
          &IntegrityAlgorithm,
          NULL,
          NULL,
          &EncryptKeylength,
          &IsSupportEsn,
          TRUE
          );
      if (EncryptAlgorithm == PreferEncryptAlgorithm &&
          EncryptKeylength == PreferEncryptKeylength &&
          IntegrityAlgorithm == PreferIntegrityAlgorithm &&
          IsSupportEsn == PreferIsSupportEsn          
          ) {
        IsMatch = TRUE;
      } else {
        PreferEncryptAlgorithm   = 0;
        PreferIntegrityAlgorithm = 0;
        IsSupportEsn             = TRUE;
      }
       ProposalData = (IKEV2_PROPOSAL_DATA*)((UINT8*)(ProposalData + 1) + 
                     ProposalData->NumTransforms * sizeof (IKEV2_TRANSFORM_DATA));  
    }
  
    ProposalData  = (IKEV2_PROPOSAL_DATA *)((IKEV2_SA_DATA *)SaPayload->PayloadBuf + 1);
    if (IsMatch) {
        ChildSaSession->SessionCommon.SaParams = AllocateZeroPool (sizeof (IKEV2_SA_PARAMS));
        ASSERT (ChildSaSession->SessionCommon.SaParams != NULL);
        ChildSaSession->SessionCommon.SaParams->EncAlgId   = PreferEncryptAlgorithm;
        ChildSaSession->SessionCommon.SaParams->EnckeyLen  = PreferEncryptKeylength;
        ChildSaSession->SessionCommon.SaParams->IntegAlgId = PreferIntegrityAlgorithm;
        CopyMem (&ChildSaSession->RemotePeerSpi, ProposalData->Spi, sizeof (ChildSaSession->RemotePeerSpi));

        return TRUE;
    }
  }
  return FALSE;
}

/**
  Generate Key buffer from fragments.

  If the digest length of specified HashAlgId is larger than or equal with the 
  required output key length, derive the key directly. Otherwise, Key Material 
  needs to be PRF-based concatenation according to 2.13 of RFC 4306: 
  prf+ (K,S) = T1 | T2 | T3 | T4 | ..., T1 = prf (K, S | 0x01),
  T2 = prf (K, T1 | S | 0x02), T3 = prf (K, T2 | S | 0x03),T4 = prf (K, T3 | S | 0x04)
  then derive the key from this key material.
  
  @param[in]       HashAlgId        The Hash Algorithm ID used to generate key.
  @param[in]       HashKey          Pointer to a key buffer which contains hash key.
  @param[in]       HashKeyLength    The length of HashKey in bytes.
  @param[in, out]  OutputKey        Pointer to buffer which is used to receive the 
                                    output key.
  @param[in]       OutputKeyLength  The length of OutPutKey buffer.
  @param[in]       Fragments        Pointer to the data to be used to generate key.
  @param[in]       NumFragments     The numbers of the Fragement.

  @retval EFI_SUCCESS            The operation complete successfully.
  @retval EFI_INVALID_PARAMETER  If NumFragments is zero.
  @retval EFI_OUT_OF_RESOURCES   If the required resource can't be allocated.
  @retval Others                 The operation is failed.

**/
EFI_STATUS
Ikev2SaGenerateKey (
  IN     UINT8                 HashAlgId,
  IN     UINT8                 *HashKey,
  IN     UINTN                 HashKeyLength,
  IN OUT UINT8                 *OutputKey,
  IN     UINTN                 OutputKeyLength,
  IN     PRF_DATA_FRAGMENT    *Fragments,
  IN     UINTN                 NumFragments
  )
{
  EFI_STATUS          Status;
  PRF_DATA_FRAGMENT   LocalFragments[3];
  UINT8               *Digest;
  UINTN               DigestSize;
  UINTN               Round;
  UINTN               Index;
  UINTN               AuthKeyLength;
  UINTN               FragmentsSize;
  UINT8               TailData;

  Status = EFI_SUCCESS;

  if (NumFragments == 0) {
    return EFI_INVALID_PARAMETER;
  }

  LocalFragments[0].Data = NULL;
  LocalFragments[1].Data = NULL;
  LocalFragments[2].Data = NULL;

  AuthKeyLength = IpSecGetHmacDigestLength (HashAlgId);
  DigestSize    = AuthKeyLength;
  Digest        = AllocateZeroPool (AuthKeyLength);

  if (Digest == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // If the required output key length is less than the digest size,
  // copy the digest into OutputKey.
  //
  if (OutputKeyLength <=  DigestSize) {
    Status = IpSecCryptoIoHmac (
               HashAlgId,
               HashKey, 
               HashKeyLength, 
               (HASH_DATA_FRAGMENT *) Fragments, 
               NumFragments, 
               Digest, 
               DigestSize
               );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    CopyMem (OutputKey, Digest, OutputKeyLength);
    goto Exit;
  }

  //
  //Otherwise, Key Material need to be PRF-based concatenation according to 2.13
  //of RFC 4306: prf+ (K,S) = T1 | T2 | T3 | T4 | ..., T1 = prf (K, S | 0x01),
  //T2 = prf (K, T1 | S | 0x02), T3 = prf (K, T2 | S | 0x03),T4 = prf (K, T3 | S | 0x04)
  //then derive the key from this key material.
  //
  FragmentsSize = 0;
  for (Index = 0; Index < NumFragments; Index++) {
    FragmentsSize = FragmentsSize + Fragments[Index].DataSize;
  }

  LocalFragments[1].Data     = AllocateZeroPool (FragmentsSize);
  ASSERT (LocalFragments[1].Data != NULL);
  LocalFragments[1].DataSize = FragmentsSize;

  //
  // Copy all input fragments into LocalFragments[1];
  //
  FragmentsSize = 0;
  for (Index = 0; Index < NumFragments; Index++) {
    CopyMem (
      LocalFragments[1].Data + FragmentsSize, 
      Fragments[Index].Data,
      Fragments[Index].DataSize
      );
    FragmentsSize = FragmentsSize + Fragments[Index].DataSize;
  }

  //
  // Prepare 0x01 as the first tail data.
  //
  TailData                   = 0x01;
  LocalFragments[2].Data     = &TailData;
  LocalFragments[2].DataSize = sizeof (TailData);
  //
  // Allocate buffer for the first fragment
  //
  LocalFragments[0].Data     = AllocateZeroPool (AuthKeyLength);
  ASSERT (LocalFragments[0].Data != NULL);
  LocalFragments[0].DataSize = AuthKeyLength;

  Round = (OutputKeyLength - 1) / AuthKeyLength + 1;
  for (Index = 0; Index < Round; Index++) {
    Status = IpSecCryptoIoHmac (
               HashAlgId, 
               HashKey, 
               HashKeyLength, 
               (HASH_DATA_FRAGMENT *)(Index == 0 ? &LocalFragments[1] : LocalFragments),
               Index == 0 ? 2 : 3, 
               Digest,
               DigestSize
               );
    if (EFI_ERROR(Status)) {
      goto Exit;
    }
    CopyMem (
      LocalFragments[0].Data, 
      Digest, 
      DigestSize
      );
    if (OutputKeyLength > DigestSize * (Index + 1)) {
      CopyMem (
        OutputKey + Index * DigestSize, 
        Digest, 
        DigestSize
        );
      LocalFragments[0].DataSize = DigestSize;
      TailData ++;
    } else {
      // 
      // The last round
      //
      CopyMem (
        OutputKey + Index * DigestSize, 
        Digest, 
        OutputKeyLength - Index * DigestSize
      );
    }
  }

Exit:
  //
  // Only First and second Framgement Data need to be freed.
  //
  for (Index = 0 ; Index < 2; Index++) {
    if (LocalFragments[Index].Data != NULL) {
      FreePool (LocalFragments[Index].Data);
    }
  }
  if (Digest != NULL) {
    FreePool (Digest);
  }
  return Status;
}

