/** @file
  Support for PxeBc dhcp functions.

Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "PxeBcImpl.h"

//
// This is a map from the interested DHCP4 option tags' index to the tag value.
//
UINT8 mInterestedDhcp4Tags[PXEBC_DHCP4_TAG_INDEX_MAX] = {
  PXEBC_DHCP4_TAG_BOOTFILE_LEN,
  PXEBC_DHCP4_TAG_VENDOR,
  PXEBC_DHCP4_TAG_OVERLOAD,
  PXEBC_DHCP4_TAG_MSG_TYPE,
  PXEBC_DHCP4_TAG_SERVER_ID,
  PXEBC_DHCP4_TAG_CLASS_ID,
  PXEBC_DHCP4_TAG_BOOTFILE
};


/**
  This function initialize the DHCP4 message instance.

  This function will pad each item of dhcp4 message packet.

  @param  Seed    Pointer to the message instance of the DHCP4 packet.
  @param  Udp4    Pointer to the EFI_UDP4_PROTOCOL instance.

**/
VOID
PxeBcInitSeedPacket (
  IN EFI_DHCP4_PACKET  *Seed,
  IN EFI_UDP4_PROTOCOL *Udp4
  )
{
  EFI_SIMPLE_NETWORK_MODE Mode;
  EFI_DHCP4_HEADER        *Header;

  Udp4->GetModeData (Udp4, NULL, NULL, NULL, &Mode);

  Seed->Size    = sizeof (EFI_DHCP4_PACKET);
  Seed->Length  = sizeof (Seed->Dhcp4);

  Header        = &Seed->Dhcp4.Header;

  ZeroMem (Header, sizeof (EFI_DHCP4_HEADER));
  Header->OpCode    = PXEBC_DHCP4_OPCODE_REQUEST;
  Header->HwType    = Mode.IfType;
  Header->HwAddrLen = (UINT8) Mode.HwAddressSize;
  CopyMem (Header->ClientHwAddr, &Mode.CurrentAddress, Header->HwAddrLen);

  Seed->Dhcp4.Magik     = PXEBC_DHCP4_MAGIC;
  Seed->Dhcp4.Option[0] = PXEBC_DHCP4_TAG_EOP;
}


/**
  Copy the DCHP4 packet from srouce to destination.

  @param  Dst   Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  Src   Pointer to the EFI_DHCP4_PROTOCOL instance.

**/
VOID
PxeBcCopyEfiDhcp4Packet (
  IN EFI_DHCP4_PACKET  *Dst,
  IN EFI_DHCP4_PACKET  *Src
  )
{
  ASSERT (Dst->Size >= Src->Length);

  CopyMem (&Dst->Dhcp4, &Src->Dhcp4, Src->Length);
  Dst->Length = Src->Length;
}


/**
  Copy the dhcp4 packet to the PxeBc private data and parse the dhcp4 packet.

  @param  Private       Pointer to PxeBc private data.
  @param  OfferIndex    Index of cached packets as complements of pxe mode data,
                        the index is maximum offer number.

**/
VOID
PxeBcCopyProxyOffer (
  IN PXEBC_PRIVATE_DATA  *Private,
  IN UINT32              OfferIndex
  )
{
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_DHCP4_PACKET        *Offer;

  ASSERT (OfferIndex < Private->NumOffers);
  ASSERT (OfferIndex < PXEBC_MAX_OFFER_NUM);

  Mode  = Private->PxeBc.Mode;
  Offer = &Private->Dhcp4Offers[OfferIndex].Packet.Offer;

  PxeBcCopyEfiDhcp4Packet (&Private->ProxyOffer.Packet.Offer, Offer);
  CopyMem (&Mode->ProxyOffer, &Offer->Dhcp4, Offer->Length);
  Mode->ProxyOfferReceived = TRUE;

  PxeBcParseCachedDhcpPacket (&Private->ProxyOffer);
}


/**
  Parse the cached dhcp packet.

  @param  CachedPacket  Pointer to cached dhcp packet.

  @retval TRUE          Succeed to parse and validation.
  @retval FALSE         Fail to parse or validation.

**/
BOOLEAN
PxeBcParseCachedDhcpPacket (
  IN PXEBC_CACHED_DHCP4_PACKET  *CachedPacket
  )
{
  EFI_DHCP4_PACKET        *Offer;
  EFI_DHCP4_PACKET_OPTION **Options;
  EFI_DHCP4_PACKET_OPTION *Option;
  UINT8                   OfferType;
  UINTN                   Index;
  UINT8                   *Ptr8;

  CachedPacket->IsPxeOffer = FALSE;
  ZeroMem (CachedPacket->Dhcp4Option, sizeof (CachedPacket->Dhcp4Option));
  ZeroMem (&CachedPacket->PxeVendorOption, sizeof (CachedPacket->PxeVendorOption));

  Offer   = &CachedPacket->Packet.Offer;
  Options = CachedPacket->Dhcp4Option;

  //
  // Parse interested dhcp options and store their pointers in CachedPacket->Dhcp4Option.
  //
  for (Index = 0; Index < PXEBC_DHCP4_TAG_INDEX_MAX; Index++) {
    Options[Index] = PxeBcParseExtendOptions (
                      Offer->Dhcp4.Option,
                      GET_OPTION_BUFFER_LEN (Offer),
                      mInterestedDhcp4Tags[Index]
                      );
  }

  //
  // Check whether is an offer with PXEClient or not.
  //
  Option = Options[PXEBC_DHCP4_TAG_INDEX_CLASS_ID];
  if ((Option != NULL) && (Option->Length >= 9) &&
    (CompareMem (Option->Data, DEFAULT_CLASS_ID_DATA, 9) == 0)) {

    CachedPacket->IsPxeOffer = TRUE;
  }

  //
  // Parse pxe vendor options and store their content/pointers in CachedPacket->PxeVendorOption.
  //
  Option = Options[PXEBC_DHCP4_TAG_INDEX_VENDOR];
  if (CachedPacket->IsPxeOffer && (Option != NULL)) {

    if (!PxeBcParseVendorOptions (Option, &CachedPacket->PxeVendorOption)) {
      return FALSE;
    }
  }

  //
  // Check whether bootfilename/serverhostname overloaded (See details in dhcp spec).
  // If overloaded, parse this buffer as nested dhcp options, or just parse bootfilename/
  // serverhostname option.
  //
  Option = Options[PXEBC_DHCP4_TAG_INDEX_OVERLOAD];
  if ((Option != NULL) && ((Option->Data[0] & PXEBC_DHCP4_OVERLOAD_FILE) != 0)) {

    Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] = PxeBcParseExtendOptions (
                                                (UINT8 *) Offer->Dhcp4.Header.BootFileName,
                                                sizeof (Offer->Dhcp4.Header.BootFileName),
                                                PXEBC_DHCP4_TAG_BOOTFILE
                                                );
    //
    // RFC 2132, Section 9.5 does not strictly state Bootfile name (option 67) is null 
    // terminated string. So force to append null terminated character at the end of string.
    //
    if (Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL) {
      Ptr8 =  (UINT8*)&Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE]->Data[0];
      Ptr8 += Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE]->Length;
      *Ptr8 =  '\0';
    }

  } else if ((Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] == NULL) &&
            (Offer->Dhcp4.Header.BootFileName[0] != 0)) {
    //
    // If the bootfile is not present and bootfilename is present in dhcp packet, just parse it.
    // And do not count dhcp option header, or else will destory the serverhostname.
    //
    Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] = (EFI_DHCP4_PACKET_OPTION *) (&Offer->Dhcp4.Header.BootFileName[0] -
                                            OFFSET_OF (EFI_DHCP4_PACKET_OPTION, Data[0]));

  }

  //
  // Determine offer type of the dhcp packet.
  //
  Option = Options[PXEBC_DHCP4_TAG_INDEX_MSG_TYPE];
  if ((Option == NULL) || (Option->Data[0] == 0)) {
    //
    // It's a bootp offer
    //
    Option = CachedPacket->Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_BOOTFILE];
    if (Option == NULL) {
      //
      // bootp offer without bootfilename, discard it.
      //
      return FALSE;
    }

    OfferType = DHCP4_PACKET_TYPE_BOOTP;

  } else {

    if (IS_VALID_DISCOVER_VENDOR_OPTION (CachedPacket->PxeVendorOption.BitMap)) {
      //
      // It's a pxe10 offer with PXEClient and discover vendor option.
      //
      OfferType = DHCP4_PACKET_TYPE_PXE10;
    } else if (IS_VALID_MTFTP_VENDOR_OPTION (CachedPacket->PxeVendorOption.BitMap)) {
      //
      // It's a wfm11a offer with PXEClient and mtftp vendor option, and
      // return false since mtftp not supported currently.
      //
      return FALSE;
    } else {
      //
      // If the binl offer with only PXEClient.
      //
      OfferType = (UINT8) ((CachedPacket->IsPxeOffer) ? DHCP4_PACKET_TYPE_BINL : DHCP4_PACKET_TYPE_DHCP_ONLY);
    }
  }

  CachedPacket->OfferType = OfferType;

  return TRUE;
}


/**
  Offer dhcp service with a BINL dhcp offer.

  @param  Private   Pointer to PxeBc private data.
  @param  Index     Index of cached packets as complements of pxe mode data,
                    the index is maximum offer number.

  @retval TRUE      Offer the service successfully under priority BINL.
  @retval FALSE     Boot Service failed, parse cached dhcp packet failed or this
                    BINL ack cannot find options set or bootfile name specified.

**/
BOOLEAN
PxeBcTryBinl (
  IN PXEBC_PRIVATE_DATA  *Private,
  IN UINT32              Index
  )
{
  EFI_DHCP4_PACKET          *Offer;
  EFI_IP_ADDRESS            ServerIp;
  EFI_STATUS                Status;
  PXEBC_CACHED_DHCP4_PACKET *CachedPacket;
  EFI_DHCP4_PACKET          *Reply;

  ASSERT (Index < PXEBC_MAX_OFFER_NUM);
  ASSERT (Private->Dhcp4Offers[Index].OfferType == DHCP4_PACKET_TYPE_BINL);

  Offer = &Private->Dhcp4Offers[Index].Packet.Offer;

  //
  // Use siaddr(next server) in DHCPOFFER packet header, if zero, use option 54(server identifier)
  // in DHCPOFFER packet.
  // (It does not comply with PXE Spec, Ver2.1)
  //
  if (EFI_IP4_EQUAL (&Offer->Dhcp4.Header.ServerAddr.Addr, &mZeroIp4Addr)) {
    CopyMem (
      &ServerIp.Addr[0],
      Private->Dhcp4Offers[Index].Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_SERVER_ID]->Data,
      sizeof (EFI_IPv4_ADDRESS)
      );
  } else {
    CopyMem (
      &ServerIp.Addr[0],
      &Offer->Dhcp4.Header.ServerAddr,
      sizeof (EFI_IPv4_ADDRESS)
      );
  }
  if (ServerIp.Addr[0] == 0) {
    return FALSE;
  }

  CachedPacket = &Private->ProxyOffer;
  Reply        = &CachedPacket->Packet.Offer;

  Status = PxeBcDiscvBootService (
            Private,
            0,
            NULL,
            FALSE,
            &ServerIp,
            0,
            NULL,
            FALSE,
            Reply
            );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (!PxeBcParseCachedDhcpPacket (CachedPacket)) {
    return FALSE;
  }

  if ((CachedPacket->OfferType != DHCP4_PACKET_TYPE_PXE10) &&
      (CachedPacket->Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] == NULL)) {
    //
    // This BINL ack doesn't have discovery options set or bootfile name
    // specified.
    //
    return FALSE;
  }

  Private->PxeBc.Mode->ProxyOfferReceived = TRUE;
  CopyMem (&Private->PxeBc.Mode->ProxyOffer, &Reply->Dhcp4, Reply->Length);

  return TRUE;
}


/**
  Offer dhcp service for each proxy with a BINL dhcp offer.

  @param  Private     Pointer to PxeBc private data
  @param  OfferIndex  Pointer to the index of cached packets as complements of
                      pxe mode data, the index is maximum offer number.

  @return If there is no service needed offer return FALSE, otherwise TRUE.

**/
BOOLEAN
PxeBcTryBinlProxy (
  IN  PXEBC_PRIVATE_DATA  *Private,
  OUT UINT32              *OfferIndex
  )
{
  UINT32  Index;

  for (Index = 0; Index < Private->ProxyIndex[DHCP4_PACKET_TYPE_BINL]; Index++) {

    *OfferIndex = Private->BinlIndex[Index];
    //
    // Try this BINL proxy offer
    //
    if (PxeBcTryBinl (Private, *OfferIndex)) {
      return TRUE;
    }
  }

  return FALSE;
}


/**
  This function is to check the selected proxy offer (include BINL dhcp offer and
  DHCP_ONLY offer ) and set the flag and copy the DHCP packets to the Pxe base code
  mode structure.

  @param  Private          Pointer to PxeBc private data.

  @retval EFI_SUCCESS      Operational successful.
  @retval EFI_NO_RESPONSE  Offer dhcp service failed.

**/
EFI_STATUS
PxeBcCheckSelectedOffer (
  IN PXEBC_PRIVATE_DATA  *Private
  )
{
  PXEBC_CACHED_DHCP4_PACKET *SelectedOffer;
  EFI_DHCP4_PACKET_OPTION   **Options;
  UINT32                    Index;
  EFI_DHCP4_PACKET          *Offer;
  UINT32                    ProxyOfferIndex;
  EFI_STATUS                Status;
  EFI_PXE_BASE_CODE_MODE    *Mode;
  EFI_DHCP4_PACKET          *Ack;

  ASSERT (Private->SelectedOffer != 0);

  Status        = EFI_SUCCESS;
  SelectedOffer = &Private->Dhcp4Offers[Private->SelectedOffer - 1];
  Options       = SelectedOffer->Dhcp4Option;

  if (SelectedOffer->OfferType == DHCP4_PACKET_TYPE_BINL) {
    //
    // The addresses are acquired from a BINL dhcp offer, try BINL to get
    // the bootfile name
    //
    if (!PxeBcTryBinl (Private, Private->SelectedOffer - 1)) {
      Status = EFI_NO_RESPONSE;
    }
  } else if (SelectedOffer->OfferType == DHCP4_PACKET_TYPE_DHCP_ONLY) {
    //
    // The selected offer to finish the D.O.R.A. is a DHCP only offer, we need
    // try proxy offers if there are some, othewise the bootfile name must be
    // set in this DHCP only offer.
    //
    if (Private->GotProxyOffer) {
      //
      // Get rid of the compiler warning.
      //
      ProxyOfferIndex = 0;
      if (Private->SortOffers) {
        //
        // The offers are sorted before selecting, the proxy offer type must be
        // already determined.
        //
        ASSERT (Private->ProxyIndex[Private->ProxyOfferType] > 0);

        if (Private->ProxyOfferType == DHCP4_PACKET_TYPE_BINL) {
          //
          // We buffer all received BINL proxy offers, try them all one by one
          //
          if (!PxeBcTryBinlProxy (Private, &ProxyOfferIndex)) {
            Status = EFI_NO_RESPONSE;
          }
        } else {
          //
          // For other types, only one proxy offer is buffered.
          //
          ProxyOfferIndex = Private->ProxyIndex[Private->ProxyOfferType] - 1;
        }
      } else {
        //
        // The proxy offer type is not determined, choose proxy offer in the
        // received order.
        //
        Status = EFI_NO_RESPONSE;

        for (Index = 0; Index < Private->NumOffers; Index++) {

          Offer = &Private->Dhcp4Offers[Index].Packet.Offer;
          if (!IS_PROXY_DHCP_OFFER (Offer)) {
            //
            // Skip non proxy dhcp offers.
            //
            continue;
          }

          if (Private->Dhcp4Offers[Index].OfferType == DHCP4_PACKET_TYPE_BINL) {
            //
            // Try BINL
            //
            if (!PxeBcTryBinl (Private, Index)) {
              //
              // Failed, skip to the next offer
              //
              continue;
            }
          }

          Private->ProxyOfferType = Private->Dhcp4Offers[Index].OfferType;
          ProxyOfferIndex         = Index;
          Status                  = EFI_SUCCESS;
          break;
        }
      }

      if (!EFI_ERROR (Status) && (Private->ProxyOfferType != DHCP4_PACKET_TYPE_BINL)) {
        //
        // Copy the proxy offer to Mode and set the flag
        //
        PxeBcCopyProxyOffer (Private, ProxyOfferIndex);
      }
    } else {
      //
      // No proxy offer is received, the bootfile name MUST be set.
      //
      ASSERT (Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL);
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // Everything is OK, set the flag and copy the DHCP packets.
    //
    Mode  = Private->PxeBc.Mode;
    Offer = &SelectedOffer->Packet.Offer;

    //
    // The discover packet is already copied, just set flag here.
    //
    Mode->DhcpDiscoverValid = TRUE;

    Ack                     = &Private->Dhcp4Ack.Packet.Ack;
    if (SelectedOffer->OfferType == DHCP4_PACKET_TYPE_BOOTP) {
      //
      // Other type of ACK is already cached. Bootp is special that we should
      // use the bootp reply as the ACK and put it into the DHCP_ONLY buffer.
      //
      PxeBcCopyEfiDhcp4Packet (&Private->Dhcp4Ack.Packet.Ack, Offer);
    }

    PxeBcParseCachedDhcpPacket (&Private->Dhcp4Ack);

    Mode->DhcpAckReceived = TRUE;

    //
    // Copy the dhcp ack.
    //
    CopyMem (&Mode->DhcpAck, &Ack->Dhcp4, Ack->Length);
  }

  return Status;
}


/**
  Cache the Dhcp4 packet offer, Parse and validate each option of the packet.

  @param  Private    Pointer to PxeBc private data.
  @param  RcvdOffer  Pointer to the received Dhcp proxy offer packet.

**/
VOID
PxeBcCacheDhcpOffer (
  IN PXEBC_PRIVATE_DATA  *Private,
  IN EFI_DHCP4_PACKET    *RcvdOffer
  )
{
  PXEBC_CACHED_DHCP4_PACKET *CachedOffer;
  EFI_DHCP4_PACKET          *Offer;
  UINT8                     OfferType;

  CachedOffer = &Private->Dhcp4Offers[Private->NumOffers];
  Offer       = &CachedOffer->Packet.Offer;

  //
  // Cache the orignal dhcp packet
  //
  PxeBcCopyEfiDhcp4Packet (Offer, RcvdOffer);

  //
  // Parse and validate the options (including dhcp option and vendor option)
  //
  if (!PxeBcParseCachedDhcpPacket (CachedOffer)) {
    return ;
  }

  OfferType = CachedOffer->OfferType;
  if (OfferType >= DHCP4_PACKET_TYPE_MAX) {
    return ;
  }

  if (OfferType == DHCP4_PACKET_TYPE_BOOTP) {

    if (Private->BootpIndex != 0) {
      //
      // Only cache the first bootp offer, discard others.
      //
      return ;
    } else {
      //
      // Take as a dhcp only offer, but record index specifically.
      //
      Private->BootpIndex = Private->NumOffers + 1;
    }
  } else {

    if (IS_PROXY_DHCP_OFFER (Offer)) {
      //
      // It's a proxy dhcp offer with no your address, including pxe10, wfm11a or binl offer.
      //
      Private->GotProxyOffer = TRUE;

      if (OfferType == DHCP4_PACKET_TYPE_BINL) {
        //
        // Cache all binl offers.
        //
        Private->BinlIndex[Private->ProxyIndex[DHCP4_PACKET_TYPE_BINL]] = Private->NumOffers;
        Private->ProxyIndex[DHCP4_PACKET_TYPE_BINL]++;
      } else if (Private->ProxyIndex[OfferType] != 0) {
        //
        // Only cache the first pxe10/wfm11a offers each, discard the others.
        //
        return ;
      } else {
        //
        // Record index of the proxy dhcp offer with type other than binl.
        //
        Private->ProxyIndex[OfferType] = Private->NumOffers + 1;
      }
    } else {
      //
      // It's a dhcp offer with your address.
      //
      ASSERT (Private->ServerCount[OfferType] < PXEBC_MAX_OFFER_NUM);
      Private->OfferIndex[OfferType][Private->ServerCount[OfferType]] = Private->NumOffers;
      Private->ServerCount[OfferType]++;
    }
  }

  //
  // Count the accepted offers.
  //
  Private->NumOffers++;
}


/**
  Select the specified proxy offer, such as BINL, DHCP_ONLY and so on.
  If the proxy does not exist, try offers with bootfile.

  @param  Private   Pointer to PxeBc private data.

**/
VOID
PxeBcSelectOffer (
  IN PXEBC_PRIVATE_DATA  *Private
  )
{
  UINT32            Index;
  UINT32            OfferIndex;
  EFI_DHCP4_PACKET  *Offer;

  Private->SelectedOffer = 0;

  if (Private->SortOffers) {
    //
    // Select offer according to the priority
    //
    if (Private->ServerCount[DHCP4_PACKET_TYPE_PXE10] > 0) {
      //
      // DHCP with PXE10
      //
      Private->SelectedOffer = Private->OfferIndex[DHCP4_PACKET_TYPE_PXE10][0] + 1;

    } else if (Private->ServerCount[DHCP4_PACKET_TYPE_WFM11A] > 0) {
      //
      // DHCP with WfM
      //
      Private->SelectedOffer = Private->OfferIndex[DHCP4_PACKET_TYPE_WFM11A][0] + 1;

    } else if ((Private->ProxyIndex[DHCP4_PACKET_TYPE_PXE10] > 0) &&
             (Private->ServerCount[DHCP4_PACKET_TYPE_DHCP_ONLY] > 0)
            ) {
      //
      // DHCP only and proxy DHCP with PXE10
      //
      Private->SelectedOffer = Private->OfferIndex[DHCP4_PACKET_TYPE_DHCP_ONLY][0] + 1;
      Private->ProxyOfferType     = DHCP4_PACKET_TYPE_PXE10;

    } else if ((Private->ProxyIndex[DHCP4_PACKET_TYPE_WFM11A] > 0) &&
             (Private->ServerCount[DHCP4_PACKET_TYPE_DHCP_ONLY] > 0)
            ) {
      //
      // DHCP only and proxy DHCP with WfM
      //
      Private->SelectedOffer = Private->OfferIndex[DHCP4_PACKET_TYPE_DHCP_ONLY][0] + 1;
      Private->ProxyOfferType     = DHCP4_PACKET_TYPE_WFM11A;

    } else if (Private->ServerCount[DHCP4_PACKET_TYPE_BINL] > 0) {
      //
      // DHCP with BINL
      //
      Private->SelectedOffer = Private->OfferIndex[DHCP4_PACKET_TYPE_BINL][0] + 1;

    } else if ((Private->ProxyIndex[DHCP4_PACKET_TYPE_BINL] > 0) &&
             (Private->ServerCount[DHCP4_PACKET_TYPE_DHCP_ONLY] > 0)
            ) {
      //
      // DHCP only and proxy DHCP with BINL
      //
      Private->SelectedOffer = Private->OfferIndex[DHCP4_PACKET_TYPE_DHCP_ONLY][0] + 1;
      Private->ProxyOfferType     = DHCP4_PACKET_TYPE_BINL;

    } else {
      //
      // Try offers with bootfile
      //
      for (Index = 0; Index < Private->ServerCount[DHCP4_PACKET_TYPE_DHCP_ONLY]; Index++) {
        //
        // Select the first DHCP only offer with bootfile
        //
        OfferIndex = Private->OfferIndex[DHCP4_PACKET_TYPE_DHCP_ONLY][Index];
        if (Private->Dhcp4Offers[OfferIndex].Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL) {
          Private->SelectedOffer = OfferIndex + 1;
          break;
        }
      }

      if (Private->SelectedOffer == 0) {
        //
        // Select the Bootp reply with bootfile if any
        //
        Private->SelectedOffer = Private->BootpIndex;
      }
    }
  } else {
    //
    // Try the offers in the received order.
    //
    for (Index = 0; Index < Private->NumOffers; Index++) {

      Offer = &Private->Dhcp4Offers[Index].Packet.Offer;

      if (IS_PROXY_DHCP_OFFER (Offer)) {
        //
        // Skip proxy offers
        //
        continue;
      }

      if ((Private->Dhcp4Offers[Index].OfferType == DHCP4_PACKET_TYPE_DHCP_ONLY) &&
          ((!Private->GotProxyOffer) && (Private->Dhcp4Offers[Index].Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] == NULL))) {
        //
        // DHCP only offer but no proxy offer received and no bootfile option in this offer
        //
        continue;
      }

      Private->SelectedOffer = Index + 1;
      break;
    }
  }
}


/**
  Callback routine.

  EFI_DHCP4_CALLBACK is provided by the consumer of the EFI DHCPv4 Protocol driver
  to intercept events that occurred in the configuration process. This structure
  provides advanced control of each state transition of the DHCP process. The
  returned status code determines the behavior of the EFI DHCPv4 Protocol driver.
  There are three possible returned values, which are described in the following
  table.

  @param  This                  Pointer to the EFI DHCPv4 Protocol instance that is used to
                                configure this callback function.
  @param  Context               Pointer to the context that is initialized by
                                EFI_DHCP4_PROTOCOL.Configure().
  @param  CurrentState          The current operational state of the EFI DHCPv4 Protocol
                                driver.
  @param  Dhcp4Event            The event that occurs in the current state, which usually means a
                                state transition.
  @param  Packet                The DHCP packet that is going to be sent or already received.
  @param  NewPacket             The packet that is used to replace the above Packet.

  @retval EFI_SUCCESS           Tells the EFI DHCPv4 Protocol driver to continue the DHCP process.
  @retval EFI_NOT_READY         Only used in the Dhcp4Selecting state. The EFI DHCPv4 Protocol
                                driver will continue to wait for more DHCPOFFER packets until the retry
                                timeout expires.
  @retval EFI_ABORTED           Tells the EFI DHCPv4 Protocol driver to abort the current process and
                                return to the Dhcp4Init or Dhcp4InitReboot state.

**/
EFI_STATUS
EFIAPI
PxeBcDhcpCallBack (
  IN EFI_DHCP4_PROTOCOL                * This,
  IN VOID                              *Context,
  IN EFI_DHCP4_STATE                   CurrentState,
  IN EFI_DHCP4_EVENT                   Dhcp4Event,
  IN EFI_DHCP4_PACKET                  * Packet OPTIONAL,
  OUT EFI_DHCP4_PACKET                 **NewPacket OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA                  *Private;
  EFI_PXE_BASE_CODE_MODE              *Mode;
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL *Callback;
  EFI_DHCP4_PACKET_OPTION             *MaxMsgSize;
  UINT16                              Value;
  EFI_STATUS                          Status;
  BOOLEAN                             Received;
  EFI_DHCP4_HEADER                    *DhcpHeader;

  if ((Dhcp4Event != Dhcp4RcvdOffer) &&
      (Dhcp4Event != Dhcp4SelectOffer) &&
      (Dhcp4Event != Dhcp4SendDiscover) &&
      (Dhcp4Event != Dhcp4RcvdAck) &&
      (Dhcp4Event != Dhcp4SendRequest)) {
    return EFI_SUCCESS;
  }

  Private   = (PXEBC_PRIVATE_DATA *) Context;
  Mode      = Private->PxeBc.Mode;
  Callback  = Private->PxeBcCallback;

  //
  // Override the Maximum DHCP Message Size.
  //
  MaxMsgSize = PxeBcParseExtendOptions (
                Packet->Dhcp4.Option,
                GET_OPTION_BUFFER_LEN (Packet),
                PXEBC_DHCP4_TAG_MAXMSG
                );
  if (MaxMsgSize != NULL) {
    Value = HTONS (PXEBC_DHCP4_MAX_PACKET_SIZE);
    CopyMem (MaxMsgSize->Data, &Value, sizeof (Value));
  }

  if ((Dhcp4Event != Dhcp4SelectOffer) && (Callback != NULL)) {
    Received = (BOOLEAN) ((Dhcp4Event == Dhcp4RcvdOffer) || (Dhcp4Event == Dhcp4RcvdAck));
    Status = Callback->Callback (
                        Callback,
                        Private->Function,
                        Received,
                        Packet->Length,
                        (EFI_PXE_BASE_CODE_PACKET *) &Packet->Dhcp4
                        );
    if (Status != EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE) {
      return EFI_ABORTED;
    }
  }

  Status = EFI_SUCCESS;

  switch (Dhcp4Event) {

  case Dhcp4SendDiscover:
  case Dhcp4SendRequest:
    if (Mode->SendGUID) {
      //
      // send the system GUID instead of the MAC address as the hardware address
      // in the DHCP packet header.
      //
      DhcpHeader = &Packet->Dhcp4.Header;

      if (EFI_ERROR (NetLibGetSystemGuid ((EFI_GUID *) DhcpHeader->ClientHwAddr))) {
        //
        // GUID not yet set - send all 0xff's to show programable (via SetVariable)
        // SetMem(DHCPV4_OPTIONS_BUFFER.DhcpPlatformId.Guid, sizeof(EFI_GUID), 0xff);
        // GUID not yet set - send all 0's to show not programable
        //
        ZeroMem (DhcpHeader->ClientHwAddr, sizeof (EFI_GUID));
      }

      DhcpHeader->HwAddrLen = (UINT8) sizeof (EFI_GUID);
    }

    if (Dhcp4Event == Dhcp4SendDiscover) {
      //
      // Cache the dhcp discover packet, of which some information will be used later.
      //
      CopyMem (Mode->DhcpDiscover.Raw, &Packet->Dhcp4, Packet->Length);
    }

    break;

  case Dhcp4RcvdOffer:
    Status = EFI_NOT_READY;
    if (Private->NumOffers < PXEBC_MAX_OFFER_NUM) {
      //
      // Cache the dhcp offers in Private->Dhcp4Offers[]
      //
      PxeBcCacheDhcpOffer (Private, Packet);
    }

    break;

  case Dhcp4SelectOffer:
    //
    // Select an offer, if succeeded, Private->SelectedOffer points to
    // the index of the selected one.
    //
    PxeBcSelectOffer (Private);

    if (Private->SelectedOffer == 0) {
      Status = EFI_ABORTED;
    } else {
      *NewPacket = &Private->Dhcp4Offers[Private->SelectedOffer - 1].Packet.Offer;
    }

    break;

  case Dhcp4RcvdAck:
    //
    // Cache Ack
    //
    ASSERT (Private->SelectedOffer != 0);

    PxeBcCopyEfiDhcp4Packet (&Private->Dhcp4Ack.Packet.Ack, Packet);
    break;

  default:
    break;
  }

  return Status;
}


/**
  Initialize the DHCP options and build the option list.

  @param  Private          Pointer to PxeBc private data.
  @param  OptList          Pointer to a DHCP option list.

  @param  IsDhcpDiscover   Discover dhcp option or not.

  @return The index item number of the option list.

**/
UINT32
PxeBcBuildDhcpOptions (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_DHCP4_PACKET_OPTION       **OptList,
  IN BOOLEAN                       IsDhcpDiscover
  )
{
  UINT32                    Index;
  PXEBC_DHCP4_OPTION_ENTRY  OptEnt;
  UINT16                    Value;

  Index       = 0;
  OptList[0]  = (EFI_DHCP4_PACKET_OPTION *) Private->OptionBuffer;

  if (!IsDhcpDiscover) {
    //
    // Append message type.
    //
    OptList[Index]->OpCode  = PXEBC_DHCP4_TAG_MSG_TYPE;
    OptList[Index]->Length  = 1;
    OptEnt.Mesg             = (PXEBC_DHCP4_OPTION_MESG *) OptList[Index]->Data;
    OptEnt.Mesg->Type       = PXEBC_DHCP4_MSG_TYPE_REQUEST;
    Index++;
    OptList[Index]          = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

    //
    // Append max message size.
    //
    OptList[Index]->OpCode  = PXEBC_DHCP4_TAG_MAXMSG;
    OptList[Index]->Length  = (UINT8) sizeof (PXEBC_DHCP4_OPTION_MAX_MESG_SIZE);
    OptEnt.MaxMesgSize      = (PXEBC_DHCP4_OPTION_MAX_MESG_SIZE *) OptList[Index]->Data;
    Value                   = NTOHS (PXEBC_DHCP4_MAX_PACKET_SIZE);
    CopyMem (&OptEnt.MaxMesgSize->Size, &Value, sizeof (UINT16));
    Index++;
    OptList[Index]          = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);
  }
  //
  // Parameter request list option.
  //
  OptList[Index]->OpCode    = PXEBC_DHCP4_TAG_PARA_LIST;
  OptList[Index]->Length    = 35;
  OptEnt.Para               = (PXEBC_DHCP4_OPTION_PARA *) OptList[Index]->Data;
  OptEnt.Para->ParaList[0]  = PXEBC_DHCP4_TAG_NETMASK;
  OptEnt.Para->ParaList[1]  = PXEBC_DHCP4_TAG_TIME_OFFSET;
  OptEnt.Para->ParaList[2]  = PXEBC_DHCP4_TAG_ROUTER;
  OptEnt.Para->ParaList[3]  = PXEBC_DHCP4_TAG_TIME_SERVER;
  OptEnt.Para->ParaList[4]  = PXEBC_DHCP4_TAG_NAME_SERVER;
  OptEnt.Para->ParaList[5]  = PXEBC_DHCP4_TAG_DNS_SERVER;
  OptEnt.Para->ParaList[6]  = PXEBC_DHCP4_TAG_HOSTNAME;
  OptEnt.Para->ParaList[7]  = PXEBC_DHCP4_TAG_BOOTFILE_LEN;
  OptEnt.Para->ParaList[8]  = PXEBC_DHCP4_TAG_DOMAINNAME;
  OptEnt.Para->ParaList[9]  = PXEBC_DHCP4_TAG_ROOTPATH;
  OptEnt.Para->ParaList[10] = PXEBC_DHCP4_TAG_EXTEND_PATH;
  OptEnt.Para->ParaList[11] = PXEBC_DHCP4_TAG_EMTU;
  OptEnt.Para->ParaList[12] = PXEBC_DHCP4_TAG_TTL;
  OptEnt.Para->ParaList[13] = PXEBC_DHCP4_TAG_BROADCAST;
  OptEnt.Para->ParaList[14] = PXEBC_DHCP4_TAG_NIS_DOMAIN;
  OptEnt.Para->ParaList[15] = PXEBC_DHCP4_TAG_NIS_SERVER;
  OptEnt.Para->ParaList[16] = PXEBC_DHCP4_TAG_NTP_SERVER;
  OptEnt.Para->ParaList[17] = PXEBC_DHCP4_TAG_VENDOR;
  OptEnt.Para->ParaList[18] = PXEBC_DHCP4_TAG_REQUEST_IP;
  OptEnt.Para->ParaList[19] = PXEBC_DHCP4_TAG_LEASE;
  OptEnt.Para->ParaList[20] = PXEBC_DHCP4_TAG_SERVER_ID;
  OptEnt.Para->ParaList[21] = PXEBC_DHCP4_TAG_T1;
  OptEnt.Para->ParaList[22] = PXEBC_DHCP4_TAG_T2;
  OptEnt.Para->ParaList[23] = PXEBC_DHCP4_TAG_CLASS_ID;
  OptEnt.Para->ParaList[24] = PXEBC_DHCP4_TAG_TFTP;
  OptEnt.Para->ParaList[25] = PXEBC_DHCP4_TAG_BOOTFILE;
  OptEnt.Para->ParaList[26] = PXEBC_PXE_DHCP4_TAG_UUID;
  OptEnt.Para->ParaList[27] = 0x80;
  OptEnt.Para->ParaList[28] = 0x81;
  OptEnt.Para->ParaList[29] = 0x82;
  OptEnt.Para->ParaList[30] = 0x83;
  OptEnt.Para->ParaList[31] = 0x84;
  OptEnt.Para->ParaList[32] = 0x85;
  OptEnt.Para->ParaList[33] = 0x86;
  OptEnt.Para->ParaList[34] = 0x87;
  Index++;
  OptList[Index]            = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  //
  // Append UUID/Guid-based client identifier option
  //
  OptList[Index]->OpCode  = PXEBC_PXE_DHCP4_TAG_UUID;
  OptList[Index]->Length  = (UINT8) sizeof (PXEBC_DHCP4_OPTION_UUID);
  OptEnt.Uuid             = (PXEBC_DHCP4_OPTION_UUID *) OptList[Index]->Data;
  OptEnt.Uuid->Type       = 0;
  Index++;
  OptList[Index]          = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  if (EFI_ERROR (NetLibGetSystemGuid ((EFI_GUID *) OptEnt.Uuid->Guid))) {
    //
    // GUID not yet set - send all 0xff's to show programable (via SetVariable)
    // SetMem(DHCPV4_OPTIONS_BUFFER.DhcpPlatformId.Guid, sizeof(EFI_GUID), 0xff);
    // GUID not yet set - send all 0's to show not programable
    //
    ZeroMem (OptEnt.Uuid->Guid, sizeof (EFI_GUID));
  }

  //
  // Append client network device interface option
  //
  OptList[Index]->OpCode  = PXEBC_PXE_DHCP4_TAG_UNDI;
  OptList[Index]->Length  = (UINT8) sizeof (PXEBC_DHCP4_OPTION_UNDI);
  OptEnt.Undi             = (PXEBC_DHCP4_OPTION_UNDI *) OptList[Index]->Data;
  if (Private->Nii != NULL) {
    OptEnt.Undi->Type       = Private->Nii->Type;
    OptEnt.Undi->MajorVer   = Private->Nii->MajorVer;
    OptEnt.Undi->MinorVer   = Private->Nii->MinorVer;
  } else {
    OptEnt.Undi->Type       = DEFAULT_UNDI_TYPE;
    OptEnt.Undi->MajorVer   = DEFAULT_UNDI_MAJOR;
    OptEnt.Undi->MinorVer   = DEFAULT_UNDI_MINOR;
  }

  Index++;
  OptList[Index] = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  //
  // Append client system architecture option
  //
  OptList[Index]->OpCode  = PXEBC_PXE_DHCP4_TAG_ARCH;
  OptList[Index]->Length  = (UINT8) sizeof (PXEBC_DHCP4_OPTION_ARCH);
  OptEnt.Arch             = (PXEBC_DHCP4_OPTION_ARCH *) OptList[Index]->Data;
  Value                   = HTONS (EFI_PXE_CLIENT_SYSTEM_ARCHITECTURE);
  CopyMem (&OptEnt.Arch->Type, &Value, sizeof (UINT16));
  Index++;
  OptList[Index]          = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  //
  // Append client system architecture option
  //
  OptList[Index]->OpCode  = PXEBC_DHCP4_TAG_CLASS_ID;
  OptList[Index]->Length  = (UINT8) sizeof (PXEBC_DHCP4_OPTION_CLID);
  OptEnt.Clid             = (PXEBC_DHCP4_OPTION_CLID *) OptList[Index]->Data;
  CopyMem (OptEnt.Clid, DEFAULT_CLASS_ID_DATA, sizeof (PXEBC_DHCP4_OPTION_CLID));
  CvtNum (EFI_PXE_CLIENT_SYSTEM_ARCHITECTURE, OptEnt.Clid->ArchitectureType, sizeof (OptEnt.Clid->ArchitectureType));

  if (Private->Nii != NULL) {
    //
    // If NII protocol exists, update DHCP option data
    //
    CopyMem (OptEnt.Clid->InterfaceName, Private->Nii->StringId, sizeof (OptEnt.Clid->InterfaceName));
    CvtNum (Private->Nii->MajorVer, OptEnt.Clid->UndiMajor, sizeof (OptEnt.Clid->UndiMajor));
    CvtNum (Private->Nii->MinorVer, OptEnt.Clid->UndiMinor, sizeof (OptEnt.Clid->UndiMinor));
  }

  Index++;

  return Index;
}


/**
  Discover the boot of service and initialize the vendor option if exists.

  @param  Private               Pointer to PxeBc private data.
  @param  Type                  PxeBc option boot item type
  @param  Layer                 PxeBc option boot item layer
  @param  UseBis                Use BIS or not
  @param  DestIp                Ip address for server
  @param  IpCount               The total count of the server ip address
  @param  SrvList               Server list
  @param  IsDiscv               Discover the vendor or not
  @param  Reply                 The dhcp4 packet of Pxe reply

  @retval EFI_SUCCESS           Operation succeeds.
  @retval EFI_OUT_OF_RESOURCES  Allocate memory pool failed.
  @retval EFI_NOT_FOUND         There is no vendor option exists.
  @retval EFI_TIMEOUT           Send Pxe Discover time out.

**/
EFI_STATUS
PxeBcDiscvBootService (
  IN PXEBC_PRIVATE_DATA                * Private,
  IN UINT16                            Type,
  IN UINT16                            *Layer,
  IN BOOLEAN                           UseBis,
  IN EFI_IP_ADDRESS                    * DestIp,
  IN UINT16                            IpCount,
  IN EFI_PXE_BASE_CODE_SRVLIST         * SrvList,
  IN BOOLEAN                           IsDiscv,
  OUT EFI_DHCP4_PACKET                 * Reply OPTIONAL
  )
{
  EFI_PXE_BASE_CODE_UDP_PORT          Sport;
  EFI_PXE_BASE_CODE_MODE              *Mode;
  EFI_DHCP4_PROTOCOL                  *Dhcp4;
  EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN    Token;
  BOOLEAN                             IsBCast;
  EFI_STATUS                          Status;
  UINT16                              RepIndex;
  UINT16                              SrvIndex;
  UINT16                              TryIndex;
  EFI_DHCP4_LISTEN_POINT              ListenPoint;
  EFI_DHCP4_PACKET                    *Response;
  EFI_DHCP4_PACKET_OPTION             *OptList[PXEBC_DHCP4_MAX_OPTION_NUM];
  UINT32                              OptCount;
  EFI_DHCP4_PACKET_OPTION             *PxeOpt;
  PXEBC_OPTION_BOOT_ITEM              *PxeBootItem;
  UINT8                               VendorOptLen;
  EFI_DHCP4_HEADER                    *DhcpHeader;
  UINT32                              Xid;

  Mode      = Private->PxeBc.Mode;
  Dhcp4     = Private->Dhcp4;
  Status    = EFI_SUCCESS;

  ZeroMem (&Token, sizeof (EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN));

  if (DestIp == NULL) {
    Sport   = PXEBC_DHCP4_S_PORT;
    IsBCast = TRUE;
  } else {
    Sport   = PXEBC_BS_DISCOVER_PORT;
    IsBCast = FALSE;
  }

  if (!UseBis && Layer != NULL) {
    *Layer &= EFI_PXE_BASE_CODE_BOOT_LAYER_MASK;
  }

  OptCount = PxeBcBuildDhcpOptions (Private, OptList, FALSE);

  if (IsDiscv) {
    ASSERT (Layer != NULL);
    //
    // Add vendor option of PXE_BOOT_ITEM
    //
    VendorOptLen = (UINT8) ((sizeof (EFI_DHCP4_PACKET_OPTION) - 1) * 2 + sizeof (PXEBC_OPTION_BOOT_ITEM) + 1);
    OptList[OptCount] = AllocatePool (VendorOptLen);
    if (OptList[OptCount] == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    OptList[OptCount]->OpCode     = PXEBC_DHCP4_TAG_VENDOR;
    OptList[OptCount]->Length     = (UINT8) (VendorOptLen - 2);
    PxeOpt                        = (EFI_DHCP4_PACKET_OPTION *) OptList[OptCount]->Data;
    PxeOpt->OpCode                = PXEBC_VENDOR_TAG_BOOT_ITEM;
    PxeOpt->Length                = (UINT8) sizeof (PXEBC_OPTION_BOOT_ITEM);
    PxeBootItem                   = (PXEBC_OPTION_BOOT_ITEM *) PxeOpt->Data;
    PxeBootItem->Type             = HTONS (Type);
    PxeBootItem->Layer            = HTONS (*Layer);
    PxeOpt->Data[PxeOpt->Length]  = PXEBC_DHCP4_TAG_EOP;

    OptCount++;
  }

  Status = Dhcp4->Build (Dhcp4, &Private->SeedPacket, 0, NULL, OptCount, OptList, &Token.Packet);

  if (IsDiscv) {
    FreePool (OptList[OptCount - 1]);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DhcpHeader = &Token.Packet->Dhcp4.Header;
  if (Mode->SendGUID) {
    if (EFI_ERROR (NetLibGetSystemGuid ((EFI_GUID *) DhcpHeader->ClientHwAddr))) {
      //
      // GUID not yet set - send all 0's to show not programable
      //
      ZeroMem (DhcpHeader->ClientHwAddr, sizeof (EFI_GUID));
    }

    DhcpHeader->HwAddrLen = (UINT8) sizeof (EFI_GUID);
  }

  Xid                                 = NET_RANDOM (NetRandomInitSeed ());
  Token.Packet->Dhcp4.Header.Xid      = HTONL(Xid);
  Token.Packet->Dhcp4.Header.Reserved = HTONS((UINT16) ((IsBCast) ? 0x8000 : 0));
  CopyMem (&Token.Packet->Dhcp4.Header.ClientAddr, &Private->StationIp, sizeof (EFI_IPv4_ADDRESS));

  Token.RemotePort = Sport;

  if (IsBCast) {
    SetMem (&Token.RemoteAddress, sizeof (EFI_IPv4_ADDRESS), 0xff);
  } else {
    CopyMem (&Token.RemoteAddress, DestIp, sizeof (EFI_IPv4_ADDRESS));
  }

  CopyMem (&Token.GatewayAddress, &Private->GatewayIp, sizeof (EFI_IPv4_ADDRESS));

  if (!IsBCast) {
    Token.ListenPointCount            = 1;
    Token.ListenPoints                = &ListenPoint;
    Token.ListenPoints[0].ListenPort  = PXEBC_BS_DISCOVER_PORT;
    CopyMem (&Token.ListenPoints[0].ListenAddress, &Private->StationIp, sizeof(EFI_IPv4_ADDRESS));
    CopyMem (&Token.ListenPoints[0].SubnetMask, &Private->SubnetMask, sizeof(EFI_IPv4_ADDRESS));
  }
  //
  // Send Pxe Discover
  //
  for (TryIndex = 1; TryIndex <= PXEBC_BOOT_REQUEST_RETRIES; TryIndex++) {

    Token.TimeoutValue                  = (UINT16) (PXEBC_BOOT_REQUEST_TIMEOUT * TryIndex);
    Token.Packet->Dhcp4.Header.Seconds  = (UINT16) (PXEBC_BOOT_REQUEST_TIMEOUT * (TryIndex - 1));

    Status              = Dhcp4->TransmitReceive (Dhcp4, &Token);

    if (Token.Status != EFI_TIMEOUT) {
      break;
    }
  }

  if (TryIndex > PXEBC_BOOT_REQUEST_RETRIES) {
    //
    // No server response our PXE request
    //
    Status = EFI_TIMEOUT;
  }

  if (!EFI_ERROR (Status)) {
    //
    // Find Pxe Reply
    //
    RepIndex  = 0;
    SrvIndex  = 0;
    Response  = Token.ResponseList;

    while (RepIndex < Token.ResponseCount) {

      while (SrvIndex < IpCount) {

        if (SrvList[SrvIndex].AcceptAnyResponse) {
          break;
        }

        if ((SrvList[SrvIndex].Type == Type) && EFI_IP4_EQUAL (&(Response->Dhcp4.Header.ServerAddr), &(Private->ServerIp))) {
          break;
        }

        SrvIndex++;
      }

      if ((IpCount != SrvIndex) || (IpCount == 0)) {
        break;
      }

      SrvIndex = 0;
      RepIndex++;

      Response = (EFI_DHCP4_PACKET *) ((UINT8 *) Response + Response->Size);
    }

    if (RepIndex < Token.ResponseCount) {

      if (Reply != NULL) {
        PxeBcCopyEfiDhcp4Packet (Reply, Response);
      }

      if (IsDiscv) {
        CopyMem (&(Mode->PxeDiscover), &(Token.Packet->Dhcp4), Token.Packet->Length);
        Mode->PxeDiscoverValid = TRUE;

        CopyMem (Mode->PxeReply.Raw, &Response->Dhcp4, Response->Length);
        Mode->PxeReplyReceived = TRUE;
      }
    } else {
      Status = EFI_NOT_FOUND;
    }

    //
    // free the responselist
    //
    if (Token.ResponseList != NULL) {
      FreePool (Token.ResponseList);
    }
  }
  //
  // Free the dhcp packet
  //
  FreePool (Token.Packet);

  return Status;
}


/**
  Parse interested dhcp options.

  @param  Buffer     Pointer to the dhcp options packet.
  @param  Length     The length of the dhcp options.
  @param  OptTag     The option OpCode.

  @return NULL if the buffer length is 0 and OpCode is not
          PXEBC_DHCP4_TAG_EOP, or the pointer to the buffer.

**/
EFI_DHCP4_PACKET_OPTION *
PxeBcParseExtendOptions (
  IN UINT8                         *Buffer,
  IN UINT32                        Length,
  IN UINT8                         OptTag
  )
{
  EFI_DHCP4_PACKET_OPTION *Option;
  UINT32                  Offset;

  Option  = (EFI_DHCP4_PACKET_OPTION *) Buffer;
  Offset  = 0;

  while (Offset < Length && Option->OpCode != PXEBC_DHCP4_TAG_EOP) {

    if (Option->OpCode == OptTag) {

      return Option;
    }

    if (Option->OpCode == PXEBC_DHCP4_TAG_PAD) {
      Offset++;
    } else {
      Offset += Option->Length + 2;
    }

    Option = (EFI_DHCP4_PACKET_OPTION *) (Buffer + Offset);
  }

  return NULL;
}


/**
  This function is to parse and check vendor options.

  @param  Dhcp4Option           Pointer to dhcp options
  @param  VendorOption          Pointer to vendor options

  @return TRUE if valid for vendor options, or FALSE.

**/
BOOLEAN
PxeBcParseVendorOptions (
  IN EFI_DHCP4_PACKET_OPTION       *Dhcp4Option,
  IN PXEBC_VENDOR_OPTION           *VendorOption
  )
{
  UINT32                  *BitMap;
  UINT8                   VendorOptionLen;
  EFI_DHCP4_PACKET_OPTION *PxeOption;
  UINT8                   Offset;

  BitMap          = VendorOption->BitMap;
  VendorOptionLen = Dhcp4Option->Length;
  PxeOption       = (EFI_DHCP4_PACKET_OPTION *) &Dhcp4Option->Data[0];
  Offset          = 0;

  while ((Offset < VendorOptionLen) && (PxeOption->OpCode != PXEBC_DHCP4_TAG_EOP)) {
    //
    // Parse every Vendor Option and set its BitMap
    //
    switch (PxeOption->OpCode) {

    case PXEBC_VENDOR_TAG_MTFTP_IP:

      CopyMem (&VendorOption->MtftpIp, PxeOption->Data, sizeof (EFI_IPv4_ADDRESS));
      break;

    case PXEBC_VENDOR_TAG_MTFTP_CPORT:

      CopyMem (&VendorOption->MtftpCPort, PxeOption->Data, sizeof (VendorOption->MtftpCPort));
      break;

    case PXEBC_VENDOR_TAG_MTFTP_SPORT:

      CopyMem (&VendorOption->MtftpSPort, PxeOption->Data, sizeof (VendorOption->MtftpSPort));
      break;

    case PXEBC_VENDOR_TAG_MTFTP_TIMEOUT:

      VendorOption->MtftpTimeout = *PxeOption->Data;
      break;

    case PXEBC_VENDOR_TAG_MTFTP_DELAY:

      VendorOption->MtftpDelay = *PxeOption->Data;
      break;

    case PXEBC_VENDOR_TAG_DISCOVER_CTRL:

      VendorOption->DiscoverCtrl = *PxeOption->Data;
      break;

    case PXEBC_VENDOR_TAG_DISCOVER_MCAST:

      CopyMem (&VendorOption->DiscoverMcastIp, PxeOption->Data, sizeof (EFI_IPv4_ADDRESS));
      break;

    case PXEBC_VENDOR_TAG_BOOT_SERVERS:

      VendorOption->BootSvrLen  = PxeOption->Length;
      VendorOption->BootSvr     = (PXEBC_BOOT_SVR_ENTRY *) PxeOption->Data;
      break;

    case PXEBC_VENDOR_TAG_BOOT_MENU:

      VendorOption->BootMenuLen = PxeOption->Length;
      VendorOption->BootMenu    = (PXEBC_BOOT_MENU_ENTRY *) PxeOption->Data;
      break;

    case PXEBC_VENDOR_TAG_MENU_PROMPT:

      VendorOption->MenuPromptLen = PxeOption->Length;
      VendorOption->MenuPrompt    = (PXEBC_MENU_PROMPT *) PxeOption->Data;
      break;

    case PXEBC_VENDOR_TAG_MCAST_ALLOC:

      CopyMem (&VendorOption->McastIpBase, PxeOption->Data, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&VendorOption->McastIpBlock, PxeOption->Data + 4, sizeof (VendorOption->McastIpBlock));
      CopyMem (&VendorOption->McastIpRange, PxeOption->Data + 6, sizeof (VendorOption->McastIpRange));
      break;

    case PXEBC_VENDOR_TAG_CREDENTIAL_TYPES:

      VendorOption->CredTypeLen = PxeOption->Length;
      VendorOption->CredType    = (UINT32 *) PxeOption->Data;
      break;

    case PXEBC_VENDOR_TAG_BOOT_ITEM:

      CopyMem (&VendorOption->BootSrvType, PxeOption->Data, sizeof (VendorOption->BootSrvType));
      CopyMem (&VendorOption->BootSrvLayer, PxeOption->Data + 2, sizeof (VendorOption->BootSrvLayer));
      break;
    }

    SET_VENDOR_OPTION_BIT_MAP (BitMap, PxeOption->OpCode);

    if (PxeOption->OpCode == PXEBC_DHCP4_TAG_PAD) {
      Offset++;
    } else {
      Offset = (UINT8) (Offset + PxeOption->Length + 2);
    }

    PxeOption = (EFI_DHCP4_PACKET_OPTION *) (Dhcp4Option->Data + Offset);
  }

  //
  // FixMe, return falas if invalid of any vendor option
  //

  return TRUE;
}


/**
  This function display boot item detail.

  If the length of the boot item string over 70 Char, just display 70 Char.

  @param  Str     Pointer to a string (boot item string).
  @param  Len     The length of string.

**/
VOID
PxeBcDisplayBootItem (
  IN UINT8                 *Str,
  IN UINT8                 Len
  )
{
  UINT8 Tmp;

  Len       = (UINT8) MIN (70, Len);
  Tmp       = Str[Len];
  Str[Len]  = 0;
  AsciiPrint ("%a \n", Str);
  Str[Len] = Tmp;
}


/**
  Choose the boot prompt.

  @param  Private              Pointer to PxeBc private data.

  @retval EFI_SUCCESS          Select boot prompt done.
  @retval EFI_TIMEOUT          Select boot prompt time out.
  @retval EFI_NOT_FOUND        The proxy offer is not Pxe10.
  @retval EFI_ABORTED          User cancel the operation.
  @retval EFI_NOT_READY        Read the input key from the keybroad has not finish.

**/
EFI_STATUS
PxeBcSelectBootPrompt (
  IN PXEBC_PRIVATE_DATA              *Private
  )
{
  PXEBC_CACHED_DHCP4_PACKET  *Packet;
  PXEBC_VENDOR_OPTION       *VendorOpt;
  EFI_EVENT                  TimeoutEvent;
  EFI_EVENT                  DescendEvent;
  EFI_INPUT_KEY              InputKey;
  EFI_STATUS                 Status;
  UINT8                      Timeout;
  UINT8                      *Prompt;
  UINT8                      PromptLen;
  INT32                      SecCol;
  INT32                      SecRow;

  TimeoutEvent  = NULL;
  DescendEvent  = NULL;

  if (Private->PxeBc.Mode->ProxyOfferReceived) {

    Packet  = &Private->ProxyOffer;
  } else {

    Packet  = &Private->Dhcp4Ack;
  }

  if (Packet->OfferType != DHCP4_PACKET_TYPE_PXE10) {
    return EFI_NOT_FOUND;
  }

  VendorOpt = &Packet->PxeVendorOption;

  if (!IS_VALID_BOOT_PROMPT (VendorOpt->BitMap)) {
    return EFI_SUCCESS;
  }

  Timeout   = VendorOpt->MenuPrompt->Timeout;
  Prompt    = VendorOpt->MenuPrompt->Prompt;
  PromptLen = (UINT8) (VendorOpt->MenuPromptLen - 1);

  if (Timeout == 0) {
    return EFI_SUCCESS;
  }

  if (Timeout == 255) {
    return EFI_TIMEOUT;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimeoutEvent
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (
                  TimeoutEvent,
                  TimerRelative,
                  Timeout * TICKS_PER_SECOND
                  );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &DescendEvent
                  );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = gBS->SetTimer (
                  DescendEvent,
                  TimerPeriodic,
                  TICKS_PER_SECOND
                  );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  SecCol = gST->ConOut->Mode->CursorColumn;
  SecRow = gST->ConOut->Mode->CursorRow;

  PxeBcDisplayBootItem (Prompt, PromptLen);

  gST->ConOut->SetCursorPosition (gST->ConOut, SecCol + PromptLen, SecRow);
  AsciiPrint ("(%d) ", Timeout--);

  while (EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {

    if (!EFI_ERROR (gBS->CheckEvent (DescendEvent))) {
      gST->ConOut->SetCursorPosition (gST->ConOut, SecCol + PromptLen, SecRow);
      AsciiPrint ("(%d) ", Timeout--);
    }

    if (gST->ConIn->ReadKeyStroke (gST->ConIn, &InputKey) == EFI_NOT_READY) {

      gBS->Stall (10 * TICKS_PER_MS);
      continue;
    }

    if (InputKey.ScanCode == 0) {

      switch (InputKey.UnicodeChar) {
      case CTRL ('c'):
        Status = EFI_ABORTED;
        break;

      case CTRL ('m'):
      case 'm':
      case 'M':
        Status = EFI_TIMEOUT;
        break;

      default:
        continue;
      }
    } else {

      switch (InputKey.ScanCode) {
      case SCAN_F8:
        Status = EFI_TIMEOUT;
        break;

      case SCAN_ESC:
        Status = EFI_ABORTED;
        break;

      default:
        continue;
      }
    }

    break;
  }

  gST->ConOut->SetCursorPosition (gST->ConOut, 0 , SecRow + 1);

ON_EXIT:

  if (DescendEvent != NULL) {
    gBS->CloseEvent (DescendEvent);
  }

  if (TimeoutEvent != NULL) {
    gBS->CloseEvent (TimeoutEvent);
  }

  return Status;
}


/**
  Select the boot menu.

  @param  Private         Pointer to PxeBc private data.
  @param  Type            The type of the menu.
  @param  UseDefaultItem  Use default item or not.

  @retval EFI_ABORTED     User cancel operation.
  @retval EFI_SUCCESS     Select the boot menu success.
  @retval EFI_NOT_READY   Read the input key from the keybroad has not finish.

**/
EFI_STATUS
PxeBcSelectBootMenu (
  IN  PXEBC_PRIVATE_DATA              *Private,
  OUT UINT16                          *Type,
  IN  BOOLEAN                         UseDefaultItem
  )
{
  PXEBC_CACHED_DHCP4_PACKET  *Packet;
  PXEBC_VENDOR_OPTION        *VendorOpt;
  EFI_INPUT_KEY              InputKey;
  UINT8                      MenuSize;
  UINT8                      MenuNum;
  INT32                      TopRow;
  UINT16                     Select;
  UINT16                     LastSelect;
  UINT8                      Index;
  BOOLEAN                    Finish;
  CHAR8                      Blank[70];
  PXEBC_BOOT_MENU_ENTRY      *MenuItem;
  PXEBC_BOOT_MENU_ENTRY      *MenuArray[PXEBC_MAX_MENU_NUM];

  Finish  = FALSE;
  Select  = 1;
  Index   = 0;
  *Type   = 0;

  if (Private->PxeBc.Mode->ProxyOfferReceived) {

    Packet  = &Private->ProxyOffer;
  } else {

    Packet  = &Private->Dhcp4Ack;
  }

  ASSERT (Packet->OfferType == DHCP4_PACKET_TYPE_PXE10);

  VendorOpt = &Packet->PxeVendorOption;

  if (!IS_VALID_BOOT_MENU (VendorOpt->BitMap)) {
    return EFI_SUCCESS;
  }

  SetMem (Blank, sizeof(Blank), ' ');

  MenuSize  = VendorOpt->BootMenuLen;
  MenuItem  = VendorOpt->BootMenu;

  if (MenuSize == 0) {
    return EFI_NOT_READY;
  }

  while (MenuSize > 0) {
    MenuArray[Index++]  = MenuItem;
    MenuSize          = (UINT8) (MenuSize - (MenuItem->DescLen + 3));
    MenuItem          = (PXEBC_BOOT_MENU_ENTRY *) ((UINT8 *) MenuItem + MenuItem->DescLen + 3);
    if (Index >= PXEBC_MAX_MENU_NUM) {
      break;
    }
  }

  if (UseDefaultItem) {
    *Type = MenuArray[0]->Type;
    *Type = NTOHS (*Type);
    return EFI_SUCCESS;
  }

  MenuNum = Index;

  for (Index = 0; Index < MenuNum; Index++) {
    PxeBcDisplayBootItem (MenuArray[Index]->DescStr, MenuArray[Index]->DescLen);
  }

  TopRow  = gST->ConOut->Mode->CursorRow - MenuNum;

  do {
    ASSERT (Select < PXEBC_MAX_MENU_NUM);
    //
    // highlight selected row
    //
    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + Select);
    Blank[MenuArray[Select]->DescLen] = 0;
    AsciiPrint ("%a\r", Blank);
    PxeBcDisplayBootItem (MenuArray[Select]->DescStr, MenuArray[Select]->DescLen);
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + MenuNum);
    LastSelect = Select;

    while (gST->ConIn->ReadKeyStroke (gST->ConIn, &InputKey) == EFI_NOT_READY) {
      gBS->Stall (10 * TICKS_PER_MS);
    }

    if (InputKey.ScanCode != 0) {
      switch (InputKey.UnicodeChar) {
      case CTRL ('c'):
        InputKey.ScanCode = SCAN_ESC;
        break;

      case CTRL ('j'):  /* linefeed */
      case CTRL ('m'):  /* return */
        Finish = TRUE;
        break;

      case CTRL ('i'):  /* tab */
      case ' ':
      case 'd':
      case 'D':
        InputKey.ScanCode = SCAN_DOWN;
        break;

      case CTRL ('h'):  /* backspace */
      case 'u':
      case 'U':
        InputKey.ScanCode = SCAN_UP;
        break;

      default:
        InputKey.ScanCode = 0;
      }
    }

    switch (InputKey.ScanCode) {
    case SCAN_LEFT:
    case SCAN_UP:
      if (Select > 0) {
        --Select;
      }

      break;

    case SCAN_DOWN:
    case SCAN_RIGHT:
      if (++Select == MenuNum) {
        --Select;
      }

      break;

    case SCAN_PAGE_UP:
    case SCAN_HOME:
      Select = 0;
      break;

    case SCAN_PAGE_DOWN:
    case SCAN_END:
      Select = (UINT16) (MenuNum - 1);
      break;

    case SCAN_ESC:
      return EFI_ABORTED;
    }

    /* unhighlight last selected row */
    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + LastSelect);
    Blank[MenuArray[LastSelect]->DescLen] = 0;
    AsciiPrint ("%a\r", Blank);
    PxeBcDisplayBootItem (MenuArray[LastSelect]->DescStr, MenuArray[LastSelect]->DescLen);
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + MenuNum);
  } while (!Finish);

   ASSERT (Select < PXEBC_MAX_MENU_NUM);

  //
  // Swap the byte order
  //
  CopyMem (Type, &MenuArray[Select]->Type, sizeof (UINT16));
  *Type = NTOHS (*Type);

  return EFI_SUCCESS;
}

