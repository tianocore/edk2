/** @file
  Provide IPsec Key Exchange (IKE) service general interfaces.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IkeService.h"
#include "IpSecConfigImpl.h"
#include "Ikev2/Utility.h"

IKE_EXCHANGE_INTERFACE  *mIkeExchange[] = {
  &mIkev1Exchange,
  &mIkev2Exchange
};

EFI_UDP4_CONFIG_DATA    mUdp4Conf = {
  FALSE,
  FALSE,
  FALSE,
  TRUE,
  //
  // IO parameters
  //
  0,
  64,
  FALSE,
  0,
  1000000,
  FALSE,
  {{0,0,0,0}},
  {{0,0,0,0}},
  IKE_DEFAULT_PORT,
  {{0,0,0,0}},
  0
};

EFI_UDP6_CONFIG_DATA    mUdp6Conf = {
  FALSE,
  FALSE,
  TRUE,
  //
  // IO parameters
  //
  0,
  128,
  0,
  1000000,
  //Access Point
  {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
  IKE_DEFAULT_PORT,
  {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
  0
};

/**
  Check if the NIC handle is binded to a Udp service.

  @param[in]  Private    Pointer of IPSEC_PRIVATE_DATA.
  @param[in]  Handle     The Handle of the NIC card.
  @param[in]  IpVersion  The version of the IP stack.

  @return a pointer of IKE_UDP_SERVICE.

**/
IKE_UDP_SERVICE *
IkeLookupUdp (
  IN IPSEC_PRIVATE_DATA     *Private,
  IN EFI_HANDLE             Handle,
  IN UINT8                  IpVersion
  )
{
  LIST_ENTRY      *Head;
  LIST_ENTRY      *Entry;
  LIST_ENTRY      *Next;
  IKE_UDP_SERVICE *Udp;

  Udp   = NULL;
  Head  = (IpVersion == IP_VERSION_4) ? &Private->Udp4List : &Private->Udp6List;

  NET_LIST_FOR_EACH_SAFE (Entry, Next, Head) {

    Udp = IPSEC_UDP_SERVICE_FROM_LIST (Entry);
    //
    // Find the right udp service which installed on the appointed NIC handle.
    //
    if (Handle == Udp->NicHandle) {
      break;
    } else {
      Udp = NULL;
    }
  }

  return Udp;
}

/**
  Configure a UDPIO's UDP4 instance.

  This fuction is called by the UdpIoCreateIo() to configures a
  UDP4 instance.

  @param[in] UdpIo         The UDP_IO to be configured.
  @param[in] Context       User-defined data when calling UdpIoCreateIo().

  @retval EFI_SUCCESS      The configuration succeeded.
  @retval Others           The UDP4 instance fails to configure.

**/
EFI_STATUS
EFIAPI
IkeConfigUdp4 (
  IN UDP_IO                 *UdpIo,
  IN VOID                   *Context
  )
{
  EFI_UDP4_CONFIG_DATA  Udp4Cfg;
  EFI_UDP4_PROTOCOL     *Udp4;

  ZeroMem (&Udp4Cfg, sizeof (EFI_UDP4_CONFIG_DATA));

  Udp4 = UdpIo->Protocol.Udp4;
  CopyMem (
    &Udp4Cfg,
    &mUdp4Conf,
    sizeof (EFI_UDP4_CONFIG_DATA)
    );

  if (Context != NULL) {
    //
    // Configure udp4 io with local default address.
    //
    Udp4Cfg.UseDefaultAddress = TRUE;
  }

  return Udp4->Configure (Udp4, &Udp4Cfg);
}

/**
  Configure a UDPIO's UDP6 instance.

  This fuction is called by the UdpIoCreateIo()to configure a
  UDP6 instance.

  @param[in] UdpIo         The UDP_IO to be configured.
  @param[in] Context       User-defined data when calling UdpIoCreateIo().

  @retval EFI_SUCCESS      The configuration succeeded.
  @retval Others           The configuration fails.

**/
EFI_STATUS
EFIAPI
IkeConfigUdp6 (
  IN UDP_IO                 *UdpIo,
  IN VOID                   *Context
  )
{
  EFI_UDP6_PROTOCOL     *Udp6;
  EFI_UDP6_CONFIG_DATA  Udp6Cfg;

  ZeroMem (&Udp6Cfg, sizeof (EFI_UDP6_CONFIG_DATA));

  Udp6 = UdpIo->Protocol.Udp6;
  CopyMem (
    &Udp6Cfg,
    &mUdp6Conf,
    sizeof (EFI_UDP6_CONFIG_DATA)
    );

  if (Context != NULL) {
    //
    // Configure instance with a destination address to start source address
    // selection, and then get the configure data from the mode data to store
    // the source address.
    //
    CopyMem (
      &Udp6Cfg.RemoteAddress,
      Context,
      sizeof (EFI_IPv6_ADDRESS)
      );
  }

  return Udp6->Configure (Udp6, &Udp6Cfg);
}

/**
  Open and configure the related output UDPIO for IKE packet sending.

  If the UdpService is not configured, this fuction calls UdpIoCreatIo() to
  create UDPIO to bind this UdpService for IKE packet sending. If the UdpService
  has already been configured, then return.

  @param[in] UdpService     The UDP_IO to be configured.
  @param[in] RemoteIp       User-defined data when calling UdpIoCreateIo().

  @retval EFI_SUCCESS      The configuration is successful.
  @retval Others           The configuration fails.

**/
EFI_STATUS
IkeOpenOutputUdp (
  IN IKE_UDP_SERVICE           *UdpService,
  IN EFI_IP_ADDRESS            *RemoteIp
  )
{
  EFI_STATUS              Status;
  EFI_IP4_CONFIG_PROTOCOL *Ip4Cfg;
  EFI_IP4_IPCONFIG_DATA   *Ip4CfgData;
  UINTN                   BufSize;
  EFI_IP6_MODE_DATA       Ip6ModeData;
  EFI_UDP6_PROTOCOL       *Udp6;

  Status      = EFI_SUCCESS;
  Ip4CfgData  = NULL;
  BufSize     = 0;

  //
  // Check whether the input and output udp io are both configured.
  //
  if (UdpService->IsConfigured) {
    goto ON_EXIT;
  }

  if (UdpService->IpVersion == UDP_IO_UDP4_VERSION) {
    //
    // Handle ip4config protocol to get local default address.
    //
    Status = gBS->HandleProtocol (
                    UdpService->NicHandle,
                    &gEfiIp4ConfigProtocolGuid,
                    (VOID **) &Ip4Cfg
                    );

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    Status = Ip4Cfg->GetData (Ip4Cfg, &BufSize, NULL);

    if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
      goto ON_EXIT;
    }

    Ip4CfgData = AllocateZeroPool (BufSize);

    if (Ip4CfgData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    Status = Ip4Cfg->GetData (Ip4Cfg, &BufSize, Ip4CfgData);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    CopyMem (
      &UdpService->DefaultAddress.v4,
      &Ip4CfgData->StationAddress,
      sizeof (EFI_IPv4_ADDRESS)
      );

    //
    // Create udp4 io for output with local default address.
    //
    UdpService->Output = UdpIoCreateIo (
                           UdpService->NicHandle,
                           UdpService->ImageHandle,
                           IkeConfigUdp4,
                           UDP_IO_UDP4_VERSION,
                           &UdpService->DefaultAddress
                           );

    if (UdpService->Output == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

  } else {
    //
    // Create udp6 io for output with remote address.
    //
    UdpService->Output = UdpIoCreateIo (
                           UdpService->NicHandle,
                           UdpService->ImageHandle,
                           IkeConfigUdp6,
                           UDP_IO_UDP6_VERSION,
                           RemoteIp
                           );

    if (UdpService->Output == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }
    //
    // Get ip6 mode data to get the result of source address selection.
    //
    ZeroMem (&Ip6ModeData, sizeof (EFI_IP6_MODE_DATA));

    Udp6    = UdpService->Output->Protocol.Udp6;
    Status  = Udp6->GetModeData (Udp6, NULL, &Ip6ModeData, NULL, NULL);

    if (EFI_ERROR (Status)) {
      UdpIoFreeIo (UdpService->Output);
      goto ON_EXIT;
    }
    //
    // Reconfigure udp6 io without remote address.
    //
    Udp6->Configure (Udp6, NULL);
    Status = IkeConfigUdp6 (UdpService->Output, NULL);

    //
    // Record the selected source address for ipsec process later.
    //
    CopyMem (
      &UdpService->DefaultAddress.v6,
      &Ip6ModeData.ConfigData.StationAddress,
      sizeof (EFI_IPv6_ADDRESS)
      );
  }

  UdpService->IsConfigured = TRUE;

ON_EXIT:
  if (Ip4CfgData != NULL) {
    FreePool (Ip4CfgData);
  }

  return Status;
}

/**
  Open and configure a UDPIO of Udp4 for IKE packet receiving.

  This function is called at the IPsecDriverBinding start. IPsec create a UDP4 and
  UDP4 IO for each NIC handle.

  @param[in] Private        Point to IPSEC_PRIVATE_DATA
  @param[in] Controller     Handler for NIC card.
  @param[in] ImageHandle    The handle that contains the EFI_DRIVER_BINDING_PROTOCOL instance.

  @retval EFI_SUCCESS             The Operation is successful.
  @retval EFI_OUT_OF_RESOURCE     The required system resource can't be allocated.

**/
EFI_STATUS
IkeOpenInputUdp4 (
  IN IPSEC_PRIVATE_DATA           *Private,
  IN EFI_HANDLE                   Controller,
  IN EFI_HANDLE                   ImageHandle
  )
{
  IKE_UDP_SERVICE *Udp4Srv;

  //
  // Check whether udp4 io of the controller has already been opened.
  //
  Udp4Srv = IkeLookupUdp (Private, Controller, IP_VERSION_4);

  if (Udp4Srv != NULL) {
    return EFI_ALREADY_STARTED;
  }

  Udp4Srv = AllocateZeroPool (sizeof (IKE_UDP_SERVICE));

  if (Udp4Srv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Create udp4 io for iutput.
  //
  Udp4Srv->Input = UdpIoCreateIo (
                     Controller,
                     ImageHandle,
                     IkeConfigUdp4,
                     UDP_IO_UDP4_VERSION,
                     NULL
                     );

  if (Udp4Srv->Input == NULL) {
    FreePool (Udp4Srv);
    return EFI_OUT_OF_RESOURCES;
  }

  Udp4Srv->NicHandle    = Controller;
  Udp4Srv->ImageHandle  = ImageHandle;
  Udp4Srv->ListHead     = &(Private->Udp4List);
  Udp4Srv->IpVersion    = UDP_IO_UDP4_VERSION;
  Udp4Srv->IsConfigured = FALSE;

  ZeroMem (&Udp4Srv->DefaultAddress, sizeof (EFI_IP_ADDRESS));

  //
  // Insert the udp4 io into the list and increase the count.
  //
  InsertTailList (&Private->Udp4List, &Udp4Srv->List);

  Private->Udp4Num++;

  UdpIoRecvDatagram (Udp4Srv->Input, IkeDispatch, Udp4Srv, 0);

  return EFI_SUCCESS;
}

/**
  Open and configure a UDPIO of Udp6 for IKE packet receiving.

  This function is called at the IPsecDriverBinding start. IPsec create a UDP6 and UDP6
  IO for each NIC handle.

  @param[in] Private        Point to IPSEC_PRIVATE_DATA
  @param[in] Controller     Handler for NIC card.
  @param[in] ImageHandle    The handle that contains the EFI_DRIVER_BINDING_PROTOCOL instance.

  @retval EFI_SUCCESS             The Operation is successful.
  @retval EFI_OUT_OF_RESOURCE     The required system resource can't be allocated.

**/
EFI_STATUS
IkeOpenInputUdp6 (
  IN IPSEC_PRIVATE_DATA           *Private,
  IN EFI_HANDLE                   Controller,
  IN EFI_HANDLE                   ImageHandle
  )
{
  IKE_UDP_SERVICE *Udp6Srv;

  Udp6Srv = IkeLookupUdp (Private, Controller, IP_VERSION_6);

  if (Udp6Srv != NULL) {
    return EFI_ALREADY_STARTED;
  }

  Udp6Srv = AllocateZeroPool (sizeof (IKE_UDP_SERVICE));

  if (Udp6Srv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Create udp6 io for input.
  //
  Udp6Srv->Input = UdpIoCreateIo (
                     Controller,
                     ImageHandle,
                     IkeConfigUdp6,
                     UDP_IO_UDP6_VERSION,
                     NULL
                     );

  if (Udp6Srv->Input == NULL) {
    FreePool (Udp6Srv);
    return EFI_OUT_OF_RESOURCES;
  }

  Udp6Srv->NicHandle    = Controller;
  Udp6Srv->ImageHandle  = ImageHandle;
  Udp6Srv->ListHead     = &(Private->Udp6List);
  Udp6Srv->IpVersion    = UDP_IO_UDP6_VERSION;
  Udp6Srv->IsConfigured = FALSE;

  ZeroMem (&Udp6Srv->DefaultAddress, sizeof (EFI_IP_ADDRESS));

  //
  // Insert the udp6 io into the list and increase the count.
  //
  InsertTailList (&Private->Udp6List, &Udp6Srv->List);

  Private->Udp6Num++;

  UdpIoRecvDatagram (Udp6Srv->Input, IkeDispatch, Udp6Srv, 0);

  return EFI_SUCCESS;
}

/**
  The general interface of starting IPsec Key Exchange.

  This function is called when a IKE negotiation to start getting a Key.

  @param[in] UdpService   Point to IKE_UDP_SERVICE which will be used for
                          IKE packet sending.
  @param[in] SpdEntry     Point to the SPD entry related to the IKE negotiation.
  @param[in] RemoteIp     Point to EFI_IP_ADDRESS related to the IKE negotiation.

  @retval EFI_SUCCESS            The Operation is successful.
  @retval EFI_ACCESS_DENIED      No related PAD entry was found.
  @retval EFI_INVALID_PARAMETER  The IKE version is not supported.

**/
EFI_STATUS
IkeNegotiate (
  IN IKE_UDP_SERVICE       *UdpService,
  IN IPSEC_SPD_ENTRY       *SpdEntry,
  IN EFI_IP_ADDRESS        *RemoteIp
  )
{
  EFI_STATUS               Status;
  UINT8                    *IkeSaSession;
  IKE_EXCHANGE_INTERFACE   *Exchange;
  IPSEC_PRIVATE_DATA       *Private;
  IPSEC_PAD_ENTRY          *PadEntry;
  UINT8                    IkeVersion;

  Private = (UdpService->IpVersion == IP_VERSION_4) ?
             IPSEC_PRIVATE_DATA_FROM_UDP4LIST(UdpService->ListHead) :
             IPSEC_PRIVATE_DATA_FROM_UDP6LIST(UdpService->ListHead);

  //
  // Try to open udp io for output if it hasn't.
  //
  Status = IkeOpenOutputUdp (UdpService, RemoteIp);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Try to find the IKE SA session in the IKEv1 and IKEv2 established SA session list.
  //
  IkeSaSession = (UINT8 *) Ikev2SaSessionLookup (&Private->Ikev2EstablishedList, RemoteIp);


  if (IkeSaSession == NULL) {
    //
    // Find the pad entry by the remote ip address.
    //
    PadEntry = IpSecLookupPadEntry (UdpService->IpVersion, RemoteIp);
    if (PadEntry == NULL) {
      return EFI_ACCESS_DENIED;
    }
    //
    // Determine the IKE exchange instance by the auth protocol in pad entry.
    //
    ASSERT (PadEntry->Data->AuthProtocol < EfiIPsecAuthProtocolMaximum);
    if (PadEntry->Data->AuthProtocol == EfiIPsecAuthProtocolIKEv1) {
      return EFI_INVALID_PARAMETER;
    }
    Exchange = mIkeExchange[PadEntry->Data->AuthProtocol];
    //
    // Start the main mode stage to negotiate IKE SA.
    //
    Status = Exchange->NegotiateSa (UdpService, SpdEntry, PadEntry, RemoteIp);
  } else {
    //
    // Determine the IKE exchange instance by the IKE version in IKE SA session.
    //
    IkeVersion = IkeGetVersionFromSession (IkeSaSession);
    if (IkeVersion != 2) {
      return EFI_INVALID_PARAMETER;
    }

    Exchange = mIkeExchange[IkeVersion - 1];
    //
    // Start the quick mode stage to negotiate child SA.
    //
    Status = Exchange->NegotiateChildSa (IkeSaSession, SpdEntry, NULL);
  }

  return Status;
}

/**
  The generic interface when receive a IKE packet.

  This function is called when UDP IO receives a IKE packet.

  @param[in] Packet       Point to received IKE packet.
  @param[in] EndPoint     Point to UDP_END_POINT which contains the information of
                          Remote IP and Port.
  @param[in] IoStatus     The Status of Recieve Token.
  @param[in] Context      Point to data passed from the caller.

**/
VOID
EFIAPI
IkeDispatch (
  IN NET_BUF                        *Packet,
  IN UDP_END_POINT                  *EndPoint,
  IN EFI_STATUS                     IoStatus,
  IN VOID                           *Context
  )
{
  IPSEC_PRIVATE_DATA                *Private;
  IKE_PACKET                        *IkePacket;
  IKE_HEADER                        *IkeHdr;
  IKE_UDP_SERVICE                   *UdpService;
  IKE_EXCHANGE_INTERFACE            *Exchange;
  EFI_STATUS                        Status;

  UdpService = (IKE_UDP_SERVICE *) Context;
  IkePacket  = NULL;
  Private    = (UdpService->IpVersion == IP_VERSION_4) ?
               IPSEC_PRIVATE_DATA_FROM_UDP4LIST(UdpService->ListHead) :
               IPSEC_PRIVATE_DATA_FROM_UDP6LIST(UdpService->ListHead);

  if (EFI_ERROR (IoStatus)) {
    goto ON_EXIT;
  }
  //
  // Check whether the ipsec is enabled or not.
  //
  if (Private->IpSec.DisabledFlag == TRUE) {
    goto ON_EXIT;
  }

  if (EndPoint->RemotePort != IKE_DEFAULT_PORT) {
    goto ON_EXIT;
  }

  //
  // Build IKE packet from the received netbuf.
  //
  IkePacket = IkePacketFromNetbuf (Packet);

  if (IkePacket == NULL) {
    goto ON_EXIT;
  }
  //
  // Get the remote address from the IKE packet.
  //
  if (UdpService->IpVersion == IP_VERSION_4) {
    *(UINT32 *) IkePacket->RemotePeerIp.Addr = HTONL ((*(UINT32 *) EndPoint->RemoteAddr.Addr));
  } else {
    CopyMem (
      &IkePacket->RemotePeerIp,
      NTOHLLL (&EndPoint->RemoteAddr.v6),
      sizeof (EFI_IPv6_ADDRESS)
      );
  }
  //
  // Try to open udp io for output if hasn't.
  //
  Status = IkeOpenOutputUdp (UdpService, &IkePacket->RemotePeerIp);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  IkeHdr = IkePacket->Header;

  //
  // Determine the IKE exchange instance by the IKE version in IKE header.
  //
  if (IKE_MAJOR_VERSION (IkeHdr->Version) == 2) {
    Exchange = mIkeExchange[IKE_MAJOR_VERSION (IkeHdr->Version) - 1];
  } else {
    goto ON_EXIT;
  }

  switch (IkeHdr->ExchangeType) {
  case IKE_XCG_TYPE_IDENTITY_PROTECT:
  case IKE_XCG_TYPE_SA_INIT:
  case IKE_XCG_TYPE_AUTH:
    Exchange->HandleSa (UdpService, IkePacket);
    break;

  case IKE_XCG_TYPE_QM:
  case IKE_XCG_TYPE_CREATE_CHILD_SA:
    Exchange->HandleChildSa (UdpService, IkePacket);
    break;

  case IKE_XCG_TYPE_INFO:
  case IKE_XCG_TYPE_INFO2:
    Exchange->HandleInfo (UdpService, IkePacket);
    break;

  default:
    break;
  }

ON_EXIT:
  if (IkePacket != NULL) {
    IkePacketFree (IkePacket);
  }

  if (Packet != NULL) {
    NetbufFree (Packet);
  }

  UdpIoRecvDatagram (UdpService->Input, IkeDispatch, UdpService, 0);

  return ;
}

/**
  Delete all established IKE SAs and related Child SAs.

  This function is the subfunction of the IpSecCleanupAllSa(). It first calls
  IkeDeleteChildSa() to delete all Child SAs then send out the related
  Information packet.

  @param[in]  Private           Pointer of the IPSEC_PRIVATE_DATA
  @param[in]  IsDisableIpsec    Indicate whether needs to disable IPsec.

**/
VOID
IkeDeleteAllSas (
  IN IPSEC_PRIVATE_DATA  *Private,
  IN BOOLEAN             IsDisableIpsec
  )
{
  LIST_ENTRY             *Entry;
  LIST_ENTRY             *NextEntry;
  IKEV2_SA_SESSION       *Ikev2SaSession;
  UINT8                  Value;
  EFI_STATUS             Status;
  IKE_EXCHANGE_INTERFACE *Exchange;
  UINT8                  IkeVersion;

  Exchange = NULL;

  //
  // If the IKEv1 is supported, first deal with the Ikev1Estatblished list.
  //

  //
  // If IKEv2 SAs are under establishing, delete it directly.
  //
  if (!IsListEmpty (&Private->Ikev2SessionList)) {
    NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->Ikev2SessionList) {
      Ikev2SaSession = IKEV2_SA_SESSION_BY_SESSION (Entry);
      RemoveEntryList (Entry);
      Ikev2SaSessionFree (Ikev2SaSession);
    }
  }

  //
  // If there is no existing established IKE SA, set the Ipsec DisableFlag to TRUE
  // and turn off the IsIPsecDisabling flag.
  //
  if (IsListEmpty (&Private->Ikev2EstablishedList) && IsDisableIpsec) {
    Value = IPSEC_STATUS_DISABLED;
    Status = gRT->SetVariable (
               IPSECCONFIG_STATUS_NAME,
               &gEfiIpSecConfigProtocolGuid,
               EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
               sizeof (Value),
               &Value
               );
    if (!EFI_ERROR (Status)) {
      Private->IpSec.DisabledFlag = TRUE;
      Private->IsIPsecDisabling   = FALSE;
      return ;
    }
  }

  //
  // Delete established IKEv2 SAs.
  //
  if (!IsListEmpty (&Private->Ikev2EstablishedList)) {
    for (Entry = Private->Ikev2EstablishedList.ForwardLink; Entry != &Private->Ikev2EstablishedList;) {
      Ikev2SaSession = IKEV2_SA_SESSION_BY_SESSION (Entry);
      Entry = Entry->ForwardLink;

      Ikev2SaSession->SessionCommon.State = IkeStateSaDeleting;

      //
      // Call for Information Exchange.
      //
      IkeVersion = IkeGetVersionFromSession ((UINT8*)Ikev2SaSession);
      if (IkeVersion == 2) {
        Exchange = mIkeExchange[IkeVersion - 1];
        Exchange->NegotiateInfo((UINT8*)Ikev2SaSession, NULL);
      }
    }
  }

}



