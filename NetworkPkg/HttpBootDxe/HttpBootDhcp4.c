/** @file
  Functions implementation related with DHCPv4 for HTTP boot driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpBootDxe.h"

//
// This is a map from the interested DHCP4 option tags' index to the tag value.
//
UINT8  mInterestedDhcp4Tags[HTTP_BOOT_DHCP4_TAG_INDEX_MAX] = {
  DHCP4_TAG_BOOTFILE_LEN,
  DHCP4_TAG_OVERLOAD,
  DHCP4_TAG_MSG_TYPE,
  DHCP4_TAG_SERVER_ID,
  DHCP4_TAG_VENDOR_CLASS_ID,
  DHCP4_TAG_BOOTFILE,
  DHCP4_TAG_DNS_SERVER
};

//
// There are 4 times retries with the value of 4, 8, 16 and 32, refers to UEFI 2.5 spec.
//
UINT32  mHttpDhcpTimeout[4] = { 4, 8, 16, 32 };

/**
  Build the options buffer for the DHCPv4 request packet.

  @param[in]  Private             Pointer to HTTP boot driver private data.
  @param[out] OptList             Pointer to the option pointer array.
  @param[in]  Buffer              Pointer to the buffer to contain the option list.

  @return     Index               The count of the built-in options.

**/
UINT32
HttpBootBuildDhcp4Options (
  IN  HTTP_BOOT_PRIVATE_DATA   *Private,
  OUT EFI_DHCP4_PACKET_OPTION  **OptList,
  IN  UINT8                    *Buffer
  )
{
  HTTP_BOOT_DHCP4_OPTION_ENTRY  OptEnt;
  UINT16                        Value;
  UINT32                        Index;

  Index      = 0;
  OptList[0] = (EFI_DHCP4_PACKET_OPTION *)Buffer;

  //
  // Append parameter request list option.
  //
  OptList[Index]->OpCode    = DHCP4_TAG_PARA_LIST;
  OptList[Index]->Length    = 27;
  OptEnt.Para               = (HTTP_BOOT_DHCP4_OPTION_PARA *)OptList[Index]->Data;
  OptEnt.Para->ParaList[0]  = DHCP4_TAG_NETMASK;
  OptEnt.Para->ParaList[1]  = DHCP4_TAG_TIME_OFFSET;
  OptEnt.Para->ParaList[2]  = DHCP4_TAG_ROUTER;
  OptEnt.Para->ParaList[3]  = DHCP4_TAG_TIME_SERVER;
  OptEnt.Para->ParaList[4]  = DHCP4_TAG_NAME_SERVER;
  OptEnt.Para->ParaList[5]  = DHCP4_TAG_DNS_SERVER;
  OptEnt.Para->ParaList[6]  = DHCP4_TAG_HOSTNAME;
  OptEnt.Para->ParaList[7]  = DHCP4_TAG_BOOTFILE_LEN;
  OptEnt.Para->ParaList[8]  = DHCP4_TAG_DOMAINNAME;
  OptEnt.Para->ParaList[9]  = DHCP4_TAG_ROOTPATH;
  OptEnt.Para->ParaList[10] = DHCP4_TAG_EXTEND_PATH;
  OptEnt.Para->ParaList[11] = DHCP4_TAG_EMTU;
  OptEnt.Para->ParaList[12] = DHCP4_TAG_TTL;
  OptEnt.Para->ParaList[13] = DHCP4_TAG_BROADCAST;
  OptEnt.Para->ParaList[14] = DHCP4_TAG_NIS_DOMAIN;
  OptEnt.Para->ParaList[15] = DHCP4_TAG_NIS_SERVER;
  OptEnt.Para->ParaList[16] = DHCP4_TAG_NTP_SERVER;
  OptEnt.Para->ParaList[17] = DHCP4_TAG_VENDOR;
  OptEnt.Para->ParaList[18] = DHCP4_TAG_REQUEST_IP;
  OptEnt.Para->ParaList[19] = DHCP4_TAG_LEASE;
  OptEnt.Para->ParaList[20] = DHCP4_TAG_SERVER_ID;
  OptEnt.Para->ParaList[21] = DHCP4_TAG_T1;
  OptEnt.Para->ParaList[22] = DHCP4_TAG_T2;
  OptEnt.Para->ParaList[23] = DHCP4_TAG_VENDOR_CLASS_ID;
  OptEnt.Para->ParaList[25] = DHCP4_TAG_BOOTFILE;
  OptEnt.Para->ParaList[26] = DHCP4_TAG_UUID;
  Index++;
  OptList[Index] = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  //
  // Append UUID/Guid-based client identifier option
  //
  OptList[Index]->OpCode = DHCP4_TAG_UUID;
  OptList[Index]->Length = (UINT8)sizeof (HTTP_BOOT_DHCP4_OPTION_UUID);
  OptEnt.Uuid            = (HTTP_BOOT_DHCP4_OPTION_UUID *)OptList[Index]->Data;
  OptEnt.Uuid->Type      = 0;
  if (EFI_ERROR (NetLibGetSystemGuid ((EFI_GUID *)OptEnt.Uuid->Guid))) {
    //
    // Zero the Guid to indicate NOT programmable if failed to get system Guid.
    //
    ZeroMem (OptEnt.Uuid->Guid, sizeof (EFI_GUID));
  }

  Index++;
  OptList[Index] = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  //
  // Append client network device interface option
  //
  OptList[Index]->OpCode = DHCP4_TAG_UNDI;
  OptList[Index]->Length = (UINT8)sizeof (HTTP_BOOT_DHCP4_OPTION_UNDI);
  OptEnt.Undi            = (HTTP_BOOT_DHCP4_OPTION_UNDI *)OptList[Index]->Data;

  if (Private->Nii != NULL) {
    OptEnt.Undi->Type     = Private->Nii->Type;
    OptEnt.Undi->MajorVer = Private->Nii->MajorVer;
    OptEnt.Undi->MinorVer = Private->Nii->MinorVer;
  } else {
    OptEnt.Undi->Type     = DEFAULT_UNDI_TYPE;
    OptEnt.Undi->MajorVer = DEFAULT_UNDI_MAJOR;
    OptEnt.Undi->MinorVer = DEFAULT_UNDI_MINOR;
  }

  Index++;
  OptList[Index] = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  //
  // Append client system architecture option
  //
  OptList[Index]->OpCode = DHCP4_TAG_ARCH;
  OptList[Index]->Length = (UINT8)sizeof (HTTP_BOOT_DHCP4_OPTION_ARCH);
  OptEnt.Arch            = (HTTP_BOOT_DHCP4_OPTION_ARCH *)OptList[Index]->Data;
  Value                  = HTONS (EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE);
  CopyMem (&OptEnt.Arch->Type, &Value, sizeof (UINT16));
  Index++;
  OptList[Index] = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  //
  // Append vendor class identify option
  //
  OptList[Index]->OpCode = DHCP4_TAG_VENDOR_CLASS_ID;
  OptList[Index]->Length = (UINT8)sizeof (HTTP_BOOT_DHCP4_OPTION_CLID);
  OptEnt.Clid            = (HTTP_BOOT_DHCP4_OPTION_CLID *)OptList[Index]->Data;
  CopyMem (
    OptEnt.Clid,
    DEFAULT_CLASS_ID_DATA,
    sizeof (HTTP_BOOT_DHCP4_OPTION_CLID)
    );
  HttpBootUintnToAscDecWithFormat (
    EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE,
    OptEnt.Clid->ArchitectureType,
    sizeof (OptEnt.Clid->ArchitectureType)
    );

  if (Private->Nii != NULL) {
    CopyMem (OptEnt.Clid->InterfaceName, Private->Nii->StringId, sizeof (OptEnt.Clid->InterfaceName));
    HttpBootUintnToAscDecWithFormat (Private->Nii->MajorVer, OptEnt.Clid->UndiMajor, sizeof (OptEnt.Clid->UndiMajor));
    HttpBootUintnToAscDecWithFormat (Private->Nii->MinorVer, OptEnt.Clid->UndiMinor, sizeof (OptEnt.Clid->UndiMinor));
  }

  Index++;

  return Index;
}

/**
  Parse a certain dhcp4 option by OptTag in Buffer, and return with start pointer.

  @param[in]  Buffer              Pointer to the option buffer.
  @param[in]  Length              Length of the option buffer.
  @param[in]  OptTag              Tag of the required option.

  @retval     NULL                Failed to find the required option.
  @retval     Others              The position of the required option.

**/
EFI_DHCP4_PACKET_OPTION *
HttpBootParseDhcp4Options (
  IN UINT8   *Buffer,
  IN UINT32  Length,
  IN UINT8   OptTag
  )
{
  EFI_DHCP4_PACKET_OPTION  *Option;
  UINT32                   Offset;

  Option = (EFI_DHCP4_PACKET_OPTION *)Buffer;
  Offset = 0;

  while (Offset < Length && Option->OpCode != DHCP4_TAG_EOP) {
    if (Option->OpCode == OptTag) {
      //
      // Found the required option.
      //
      return Option;
    }

    //
    // Skip the current option to the next.
    //
    if (Option->OpCode == DHCP4_TAG_PAD) {
      Offset++;
    } else {
      Offset += Option->Length + 2;
    }

    Option = (EFI_DHCP4_PACKET_OPTION *)(Buffer + Offset);
  }

  return NULL;
}

/**
  Cache the DHCPv4 packet.

  @param[in]  Dst          Pointer to the cache buffer for DHCPv4 packet.
  @param[in]  Src          Pointer to the DHCPv4 packet to be cached.

  @retval     EFI_SUCCESS                Packet is copied.
  @retval     EFI_BUFFER_TOO_SMALL       Cache buffer is not big enough to hold the packet.

**/
EFI_STATUS
HttpBootCacheDhcp4Packet (
  IN EFI_DHCP4_PACKET  *Dst,
  IN EFI_DHCP4_PACKET  *Src
  )
{
  if (Dst->Size < Src->Length) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (&Dst->Dhcp4, &Src->Dhcp4, Src->Length);
  Dst->Length = Src->Length;

  return EFI_SUCCESS;
}

/**
  Parse the cached DHCPv4 packet, including all the options.

  @param[in]  Cache4           Pointer to cached DHCPv4 packet.

  @retval     EFI_SUCCESS      Parsed the DHCPv4 packet successfully.
  @retval     EFI_DEVICE_ERROR Failed to parse an invalid packet.

**/
EFI_STATUS
HttpBootParseDhcp4Packet (
  IN HTTP_BOOT_DHCP4_PACKET_CACHE  *Cache4
  )
{
  EFI_DHCP4_PACKET         *Offer;
  EFI_DHCP4_PACKET_OPTION  **Options;
  UINTN                    Index;
  EFI_DHCP4_PACKET_OPTION  *Option;
  BOOLEAN                  IsProxyOffer;
  BOOLEAN                  IsHttpOffer;
  BOOLEAN                  IsDnsOffer;
  BOOLEAN                  IpExpressedUri;
  UINT8                    *Ptr8;
  EFI_STATUS               Status;
  HTTP_BOOT_OFFER_TYPE     OfferType;
  EFI_IPv4_ADDRESS         IpAddr;
  BOOLEAN                  FileFieldOverloaded;

  IsDnsOffer          = FALSE;
  IpExpressedUri      = FALSE;
  IsProxyOffer        = FALSE;
  IsHttpOffer         = FALSE;
  FileFieldOverloaded = FALSE;

  ZeroMem (Cache4->OptList, sizeof (Cache4->OptList));

  Offer   = &Cache4->Packet.Offer;
  Options = Cache4->OptList;

  //
  // Parse DHCPv4 options in this offer, and store the pointers.
  // First, try to parse DHCPv4 options from the DHCP optional parameters field.
  //
  for (Index = 0; Index < HTTP_BOOT_DHCP4_TAG_INDEX_MAX; Index++) {
    Options[Index] = HttpBootParseDhcp4Options (
                       Offer->Dhcp4.Option,
                       GET_OPTION_BUFFER_LEN (Offer),
                       mInterestedDhcp4Tags[Index]
                       );
  }

  //
  // Second, Check if bootfilename and serverhostname is overloaded to carry DHCP options refers to rfc-2132.
  // If yes, try to parse options from the BootFileName field, then ServerName field.
  //
  Option = Options[HTTP_BOOT_DHCP4_TAG_INDEX_OVERLOAD];
  if (Option != NULL) {
    if ((Option->Data[0] & HTTP_BOOT_DHCP4_OVERLOAD_FILE) != 0) {
      FileFieldOverloaded = TRUE;
      for (Index = 0; Index < HTTP_BOOT_DHCP4_TAG_INDEX_MAX; Index++) {
        if (Options[Index] == NULL) {
          Options[Index] = HttpBootParseDhcp4Options (
                             (UINT8 *)Offer->Dhcp4.Header.BootFileName,
                             sizeof (Offer->Dhcp4.Header.BootFileName),
                             mInterestedDhcp4Tags[Index]
                             );
        }
      }
    }

    if ((Option->Data[0] & HTTP_BOOT_DHCP4_OVERLOAD_SERVER_NAME) != 0) {
      for (Index = 0; Index < HTTP_BOOT_DHCP4_TAG_INDEX_MAX; Index++) {
        if (Options[Index] == NULL) {
          Options[Index] = HttpBootParseDhcp4Options (
                             (UINT8 *)Offer->Dhcp4.Header.ServerName,
                             sizeof (Offer->Dhcp4.Header.ServerName),
                             mInterestedDhcp4Tags[Index]
                             );
        }
      }
    }
  }

  //
  // The offer with "yiaddr" is a proxy offer.
  //
  if (Offer->Dhcp4.Header.YourAddr.Addr[0] == 0) {
    IsProxyOffer = TRUE;
  }

  //
  // The offer with "HTTPClient" is a Http offer.
  //
  Option = Options[HTTP_BOOT_DHCP4_TAG_INDEX_CLASS_ID];
  if ((Option != NULL) && (Option->Length >= 10) &&
      (CompareMem (Option->Data, DEFAULT_CLASS_ID_DATA, 10) == 0))
  {
    IsHttpOffer = TRUE;
  }

  //
  // The offer with Domain Server is a DNS offer.
  //
  Option = Options[HTTP_BOOT_DHCP4_TAG_INDEX_DNS_SERVER];
  if (Option != NULL) {
    IsDnsOffer = TRUE;
  }

  //
  // Parse boot file name:
  // Boot URI information is provided thru 'file' field in DHCP Header or option 67.
  // According to RFC 2132, boot file name should be read from DHCP option 67 (bootfile name) if present.
  // Otherwise, read from boot file field in DHCP header.
  //
  if (Options[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE] != NULL) {
    //
    // RFC 2132, Section 9.5 does not strictly state Bootfile name (option 67) is null
    // terminated string. So force to append null terminated character at the end of string.
    //
    Ptr8  =  (UINT8 *)&Options[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE]->Data[0];
    Ptr8 += Options[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE]->Length;
    if (*(Ptr8 - 1) != '\0') {
      *Ptr8 = '\0';
    }
  } else if (!FileFieldOverloaded && (Offer->Dhcp4.Header.BootFileName[0] != 0)) {
    //
    // If the bootfile is not present and bootfilename is present in DHCPv4 packet, just parse it.
    // Do not count dhcp option header here, or else will destroy the serverhostname.
    //
    Options[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE] = (EFI_DHCP4_PACKET_OPTION *)
                                                  (&Offer->Dhcp4.Header.BootFileName[0] -
                                                   OFFSET_OF (EFI_DHCP4_PACKET_OPTION, Data[0]));
  }

  //
  // Http offer must have a boot URI.
  //
  if (IsHttpOffer && (Options[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE] == NULL)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Try to retrieve the IP of HTTP server from URI.
  //
  if (IsHttpOffer) {
    Status = HttpParseUrl (
               (CHAR8 *)Options[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE]->Data,
               (UINT32)AsciiStrLen ((CHAR8 *)Options[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE]->Data),
               FALSE,
               &Cache4->UriParser
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Status = HttpUrlGetIp4 (
               (CHAR8 *)Options[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE]->Data,
               Cache4->UriParser,
               &IpAddr
               );
    IpExpressedUri = !EFI_ERROR (Status);
  }

  //
  // Determine offer type of the DHCPv4 packet.
  //
  if (IsHttpOffer) {
    if (IpExpressedUri) {
      if (IsProxyOffer) {
        OfferType = HttpOfferTypeProxyIpUri;
      } else {
        OfferType = IsDnsOffer ? HttpOfferTypeDhcpIpUriDns : HttpOfferTypeDhcpIpUri;
      }
    } else {
      if (!IsProxyOffer) {
        OfferType = IsDnsOffer ? HttpOfferTypeDhcpNameUriDns : HttpOfferTypeDhcpNameUri;
      } else {
        OfferType = HttpOfferTypeProxyNameUri;
      }
    }
  } else {
    if (!IsProxyOffer) {
      OfferType = IsDnsOffer ? HttpOfferTypeDhcpDns : HttpOfferTypeDhcpOnly;
    } else {
      if (Cache4->UriParser != NULL) {
        FreePool (Cache4->UriParser);
      }

      return EFI_DEVICE_ERROR;
    }
  }

  Cache4->OfferType = OfferType;
  return EFI_SUCCESS;
}

/**
  Cache all the received DHCPv4 offers, and set OfferIndex and OfferCount.

  @param[in]  Private               Pointer to HTTP boot driver private data.
  @param[in]  RcvdOffer             Pointer to the received offer packet.

  @retval     EFI_SUCCESS      Cache and parse the packet successfully.
  @retval     Others           Operation failed.
**/
EFI_STATUS
HttpBootCacheDhcp4Offer (
  IN HTTP_BOOT_PRIVATE_DATA  *Private,
  IN EFI_DHCP4_PACKET        *RcvdOffer
  )
{
  HTTP_BOOT_DHCP4_PACKET_CACHE  *Cache4;
  EFI_DHCP4_PACKET              *Offer;
  HTTP_BOOT_OFFER_TYPE          OfferType;
  EFI_STATUS                    Status;

  ASSERT (Private->OfferNum < HTTP_BOOT_OFFER_MAX_NUM);
  Cache4 = &Private->OfferBuffer[Private->OfferNum].Dhcp4;
  Offer  = &Cache4->Packet.Offer;

  //
  // Cache the content of DHCPv4 packet firstly.
  //
  Status = HttpBootCacheDhcp4Packet (Offer, RcvdOffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Validate the DHCPv4 packet, and parse the options and offer type.
  //
  if (EFI_ERROR (HttpBootParseDhcp4Packet (Cache4))) {
    return EFI_ABORTED;
  }

  //
  // Determine whether cache the current offer by type, and record OfferIndex and OfferCount.
  //
  OfferType = Cache4->OfferType;
  ASSERT (OfferType < HttpOfferTypeMax);
  ASSERT (Private->OfferCount[OfferType] < HTTP_BOOT_OFFER_MAX_NUM);
  Private->OfferIndex[OfferType][Private->OfferCount[OfferType]] = Private->OfferNum;
  Private->OfferCount[OfferType]++;
  Private->OfferNum++;

  return EFI_SUCCESS;
}

/**
  Select an DHCPv4 or DHCP6 offer, and record SelectIndex and SelectProxyType.

  @param[in]  Private             Pointer to HTTP boot driver private data.

**/
VOID
HttpBootSelectDhcpOffer (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  Private->SelectIndex     = 0;
  Private->SelectProxyType = HttpOfferTypeMax;

  if (Private->FilePathUri != NULL) {
    //
    // We are in home environment, the URI is already specified.
    // Just need to choose a DHCP offer.
    // The offer with DNS server address takes priority here.
    //
    if (Private->OfferCount[HttpOfferTypeDhcpDns] > 0) {
      Private->SelectIndex = Private->OfferIndex[HttpOfferTypeDhcpDns][0] + 1;
    } else if (Private->OfferCount[HttpOfferTypeDhcpIpUriDns] > 0) {
      Private->SelectIndex = Private->OfferIndex[HttpOfferTypeDhcpIpUriDns][0] + 1;
    } else if (Private->OfferCount[HttpOfferTypeDhcpNameUriDns] > 0) {
      Private->SelectIndex = Private->OfferIndex[HttpOfferTypeDhcpNameUriDns][0] + 1;
    } else if (Private->OfferCount[HttpOfferTypeDhcpOnly] > 0) {
      Private->SelectIndex = Private->OfferIndex[HttpOfferTypeDhcpOnly][0] + 1;
    } else if (Private->OfferCount[HttpOfferTypeDhcpIpUri] > 0) {
      Private->SelectIndex = Private->OfferIndex[HttpOfferTypeDhcpIpUri][0] + 1;
    }
  } else {
    //
    // We are in corporate environment.
    //
    // Priority1: HttpOfferTypeDhcpIpUri or HttpOfferTypeDhcpIpUriDns
    // Priority2: HttpOfferTypeDhcpNameUriDns
    // Priority3: HttpOfferTypeDhcpOnly + HttpOfferTypeProxyIpUri
    // Priority4: HttpOfferTypeDhcpDns  + HttpOfferTypeProxyIpUri
    // Priority5: HttpOfferTypeDhcpDns  + HttpOfferTypeProxyNameUri
    // Priority6: HttpOfferTypeDhcpDns  + HttpOfferTypeDhcpNameUri
    //
    if (Private->OfferCount[HttpOfferTypeDhcpIpUri] > 0) {
      Private->SelectIndex = Private->OfferIndex[HttpOfferTypeDhcpIpUri][0] + 1;
    } else if (Private->OfferCount[HttpOfferTypeDhcpIpUriDns] > 0) {
      Private->SelectIndex = Private->OfferIndex[HttpOfferTypeDhcpIpUriDns][0] + 1;
    } else if (Private->OfferCount[HttpOfferTypeDhcpNameUriDns] > 0) {
      Private->SelectIndex = Private->OfferIndex[HttpOfferTypeDhcpNameUriDns][0] + 1;
    } else if ((Private->OfferCount[HttpOfferTypeDhcpOnly] > 0) &&
               (Private->OfferCount[HttpOfferTypeProxyIpUri] > 0))
    {
      Private->SelectIndex     = Private->OfferIndex[HttpOfferTypeDhcpOnly][0] + 1;
      Private->SelectProxyType = HttpOfferTypeProxyIpUri;
    } else if ((Private->OfferCount[HttpOfferTypeDhcpDns] > 0) &&
               (Private->OfferCount[HttpOfferTypeProxyIpUri] > 0))
    {
      Private->SelectIndex     = Private->OfferIndex[HttpOfferTypeDhcpDns][0] + 1;
      Private->SelectProxyType = HttpOfferTypeProxyIpUri;
    } else if ((Private->OfferCount[HttpOfferTypeDhcpDns] > 0) &&
               (Private->OfferCount[HttpOfferTypeProxyNameUri] > 0))
    {
      Private->SelectIndex     = Private->OfferIndex[HttpOfferTypeDhcpDns][0] + 1;
      Private->SelectProxyType = HttpOfferTypeProxyNameUri;
    } else if ((Private->OfferCount[HttpOfferTypeDhcpDns] > 0) &&
               (Private->OfferCount[HttpOfferTypeDhcpNameUri] > 0))
    {
      Private->SelectIndex     = Private->OfferIndex[HttpOfferTypeDhcpDns][0] + 1;
      Private->SelectProxyType = HttpOfferTypeDhcpNameUri;
    }
  }
}

/**
  EFI_DHCP4_CALLBACK is provided by the consumer of the EFI DHCPv4 Protocol driver
  to intercept events that occurred in the configuration process.

  @param[in]  This              Pointer to the EFI DHCPv4 Protocol.
  @param[in]  Context           Pointer to the context set by EFI_DHCP4_PROTOCOL.Configure().
  @param[in]  CurrentState      The current operational state of the EFI DHCPv4 Protocol driver.
  @param[in]  Dhcp4Event        The event that occurs in the current state, which usually means a
                                state transition.
  @param[in]  Packet            The DHCPv4 packet that is going to be sent or already received.
  @param[out] NewPacket         The packet that is used to replace the above Packet.

  @retval EFI_SUCCESS           Tells the EFI DHCPv4 Protocol driver to continue the DHCP process.
  @retval EFI_NOT_READY         Only used in the Dhcp4Selecting state. The EFI DHCPv4 Protocol
                                driver will continue to wait for more DHCPOFFER packets until the
                                retry timeout expires.
  @retval EFI_ABORTED           Tells the EFI DHCPv4 Protocol driver to abort the current process
                                and return to the Dhcp4Init or Dhcp4InitReboot state.

**/
EFI_STATUS
EFIAPI
HttpBootDhcp4CallBack (
  IN  EFI_DHCP4_PROTOCOL  *This,
  IN  VOID                *Context,
  IN  EFI_DHCP4_STATE     CurrentState,
  IN  EFI_DHCP4_EVENT     Dhcp4Event,
  IN  EFI_DHCP4_PACKET    *Packet            OPTIONAL,
  OUT EFI_DHCP4_PACKET    **NewPacket        OPTIONAL
  )
{
  HTTP_BOOT_PRIVATE_DATA   *Private;
  EFI_DHCP4_PACKET_OPTION  *MaxMsgSize;
  UINT16                   Value;
  EFI_STATUS               Status;
  BOOLEAN                  Received;

  if ((Dhcp4Event != Dhcp4SendDiscover) &&
      (Dhcp4Event != Dhcp4RcvdOffer) &&
      (Dhcp4Event != Dhcp4SendRequest) &&
      (Dhcp4Event != Dhcp4RcvdAck) &&
      (Dhcp4Event != Dhcp4SelectOffer))
  {
    return EFI_SUCCESS;
  }

  Private = (HTTP_BOOT_PRIVATE_DATA *)Context;

  //
  // Override the Maximum DHCP Message Size.
  //
  MaxMsgSize = HttpBootParseDhcp4Options (
                 Packet->Dhcp4.Option,
                 GET_OPTION_BUFFER_LEN (Packet),
                 DHCP4_TAG_MAXMSG
                 );
  if (MaxMsgSize != NULL) {
    Value = HTONS (HTTP_BOOT_DHCP4_PACKET_MAX_SIZE);
    CopyMem (MaxMsgSize->Data, &Value, sizeof (Value));
  }

  //
  // Callback to user if any packets sent or received.
  //
  if ((Private->HttpBootCallback != NULL) && (Dhcp4Event != Dhcp4SelectOffer)) {
    Received = (BOOLEAN)(Dhcp4Event == Dhcp4RcvdOffer || Dhcp4Event == Dhcp4RcvdAck);
    Status   = Private->HttpBootCallback->Callback (
                                            Private->HttpBootCallback,
                                            HttpBootDhcp4,
                                            Received,
                                            Packet->Length,
                                            &Packet->Dhcp4
                                            );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }

  Status = EFI_SUCCESS;
  switch (Dhcp4Event) {
    case Dhcp4RcvdOffer:
      Status = EFI_NOT_READY;
      if (Packet->Length > HTTP_BOOT_DHCP4_PACKET_MAX_SIZE) {
        //
        // Ignore the incoming packets which exceed the maximum length.
        //
        break;
      }

      if (Private->OfferNum < HTTP_BOOT_OFFER_MAX_NUM) {
        //
        // Cache the DHCPv4 offers to OfferBuffer[] for select later, and record
        // the OfferIndex and OfferCount.
        // If error happens, just ignore this packet and continue to wait more offer.
        //
        HttpBootCacheDhcp4Offer (Private, Packet);
      }

      break;

    case Dhcp4SelectOffer:
      //
      // Select offer according to the priority in UEFI spec, and record the SelectIndex
      // and SelectProxyType.
      //
      HttpBootSelectDhcpOffer (Private);

      if (Private->SelectIndex == 0) {
        Status = EFI_ABORTED;
      } else {
        *NewPacket = &Private->OfferBuffer[Private->SelectIndex - 1].Dhcp4.Packet.Offer;
      }

      break;

    default:
      break;
  }

  return Status;
}

/**
  This function will register the IPv4 gateway address to the network device.

  @param[in]  Private             The pointer to HTTP_BOOT_PRIVATE_DATA.

  @retval     EFI_SUCCESS         The new IP configuration has been configured successfully.
  @retval     Others              Failed to configure the address.

**/
EFI_STATUS
HttpBootRegisterIp4Gateway (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                Status;
  EFI_IP4_CONFIG2_PROTOCOL  *Ip4Config2;

  ASSERT (!Private->UsingIpv6);

  Ip4Config2 = Private->Ip4Config2;

  //
  // Configure the gateway if valid.
  //
  if (!EFI_IP4_EQUAL (&Private->GatewayIp, &mZeroIp4Addr)) {
    Status = Ip4Config2->SetData (
                           Ip4Config2,
                           Ip4Config2DataTypeGateway,
                           sizeof (EFI_IPv4_ADDRESS),
                           &Private->GatewayIp
                           );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function will register the default DNS addresses to the network device.

  @param[in]  Private             The pointer to HTTP_BOOT_PRIVATE_DATA.
  @param[in]  DataLength          Size of the buffer pointed to by DnsServerData in bytes.
  @param[in]  DnsServerData       Point a list of DNS server address in an array
                                  of EFI_IPv4_ADDRESS instances.

  @retval     EFI_SUCCESS         The DNS configuration has been configured successfully.
  @retval     Others              Failed to configure the address.

**/
EFI_STATUS
HttpBootRegisterIp4Dns (
  IN HTTP_BOOT_PRIVATE_DATA  *Private,
  IN UINTN                   DataLength,
  IN VOID                    *DnsServerData
  )
{
  EFI_IP4_CONFIG2_PROTOCOL  *Ip4Config2;

  ASSERT (!Private->UsingIpv6);

  Ip4Config2 = Private->Ip4Config2;

  return Ip4Config2->SetData (
                       Ip4Config2,
                       Ip4Config2DataTypeDnsServer,
                       DataLength,
                       DnsServerData
                       );
}

/**
  This function will switch the IP4 configuration policy to Static.

  @param[in]  Private             Pointer to HTTP boot driver private data.

  @retval     EFI_SUCCESS         The policy is already configured to static.
  @retval     Others              Other error as indicated..

**/
EFI_STATUS
HttpBootSetIp4Policy (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  EFI_IP4_CONFIG2_POLICY    Policy;
  EFI_STATUS                Status;
  EFI_IP4_CONFIG2_PROTOCOL  *Ip4Config2;
  UINTN                     DataSize;

  Ip4Config2 = Private->Ip4Config2;

  DataSize = sizeof (EFI_IP4_CONFIG2_POLICY);
  Status   = Ip4Config2->GetData (
                           Ip4Config2,
                           Ip4Config2DataTypePolicy,
                           &DataSize,
                           &Policy
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Policy != Ip4Config2PolicyStatic) {
    Policy = Ip4Config2PolicyStatic;
    Status = Ip4Config2->SetData (
                           Ip4Config2,
                           Ip4Config2DataTypePolicy,
                           sizeof (EFI_IP4_CONFIG2_POLICY),
                           &Policy
                           );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Start the D.O.R.A DHCPv4 process to acquire the IPv4 address and other Http boot information.

  @param[in]  Private           Pointer to HTTP boot driver private data.

  @retval EFI_SUCCESS           The D.O.R.A process successfully finished.
  @retval Others                Failed to finish the D.O.R.A process.

**/
EFI_STATUS
HttpBootDhcp4Dora (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  EFI_DHCP4_PROTOCOL       *Dhcp4;
  UINT32                   OptCount;
  EFI_DHCP4_PACKET_OPTION  *OptList[HTTP_BOOT_DHCP4_OPTION_MAX_NUM];
  UINT8                    Buffer[HTTP_BOOT_DHCP4_OPTION_MAX_SIZE];
  EFI_DHCP4_CONFIG_DATA    Config;
  EFI_STATUS               Status;
  EFI_DHCP4_MODE_DATA      Mode;

  Dhcp4 = Private->Dhcp4;
  ASSERT (Dhcp4 != NULL);

  Status = HttpBootSetIp4Policy (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Build option list for the request packet.
  //
  OptCount = HttpBootBuildDhcp4Options (Private, OptList, Buffer);
  ASSERT (OptCount > 0);

  ZeroMem (&Config, sizeof (Config));
  Config.OptionCount      = OptCount;
  Config.OptionList       = OptList;
  Config.Dhcp4Callback    = HttpBootDhcp4CallBack;
  Config.CallbackContext  = Private;
  Config.DiscoverTryCount = HTTP_BOOT_DHCP_RETRIES;
  Config.DiscoverTimeout  = mHttpDhcpTimeout;

  //
  // Configure the DHCPv4 instance for HTTP boot.
  //
  Status = Dhcp4->Configure (Dhcp4, &Config);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Initialize the record fields for DHCPv4 offer in private data.
  //
  Private->OfferNum = 0;
  ZeroMem (Private->OfferCount, sizeof (Private->OfferCount));
  ZeroMem (Private->OfferIndex, sizeof (Private->OfferIndex));

  //
  // Start DHCPv4 D.O.R.A. process to acquire IPv4 address.
  //
  Status = Dhcp4->Start (Dhcp4, NULL);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Get the acquired IPv4 address and store them.
  //
  Status = Dhcp4->GetModeData (Dhcp4, &Mode);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  ASSERT (Mode.State == Dhcp4Bound);
  CopyMem (&Private->StationIp, &Mode.ClientAddress, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Private->SubnetMask, &Mode.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Private->GatewayIp, &Mode.RouterAddress, sizeof (EFI_IPv4_ADDRESS));

  Status = HttpBootRegisterIp4Gateway (Private);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  AsciiPrint ("\n  Station IP address is ");
  HttpBootShowIp4Addr (&Private->StationIp.v4);
  AsciiPrint ("\n");

ON_EXIT:
  if (EFI_ERROR (Status)) {
    Dhcp4->Stop (Dhcp4);
    Dhcp4->Configure (Dhcp4, NULL);
  } else {
    ZeroMem (&Config, sizeof (EFI_DHCP4_CONFIG_DATA));
    Dhcp4->Configure (Dhcp4, &Config);
  }

  return Status;
}
