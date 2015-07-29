/** @file
  Functions implementation related with DHCPv4 for UefiPxeBc Driver.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

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

//
// There are 4 times retries with the value of 4, 8, 16 and 32, refers to PXE2.1 spec.
//
UINT32 mPxeDhcpTimeout[4] = {4, 8, 16, 32};


/**
  Parse a certain dhcp4 option by OptTag in Buffer, and return with start pointer.

  @param[in]  Buffer              Pointer to the option buffer.
  @param[in]  Length              Length of the option buffer.
  @param[in]  OptTag              Tag of the required option.

  @retval     NULL                Failed to find the required option.
  @retval     Others              The position of the required option.

**/
EFI_DHCP4_PACKET_OPTION *
PxeBcParseDhcp4Options (
  IN UINT8                      *Buffer,
  IN UINT32                     Length,
  IN UINT8                      OptTag
  )
{
  EFI_DHCP4_PACKET_OPTION       *Option;
  UINT32                        Offset;

  Option  = (EFI_DHCP4_PACKET_OPTION *) Buffer;
  Offset  = 0;

  while (Offset < Length && Option->OpCode != PXEBC_DHCP4_TAG_EOP) {

    if (Option->OpCode == OptTag) {
      //
      // Found the required option.
      //
      return Option;
    }

    //
    // Skip the current option to the next.
    //
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
  Parse the PXE vender options and extract the information from them.

  @param[in]  Dhcp4Option        Pointer to vendor options in buffer.
  @param[in]  VendorOption       Pointer to structure to store information in vendor options.

**/
VOID
PxeBcParseVendorOptions (
  IN EFI_DHCP4_PACKET_OPTION    *Dhcp4Option,
  IN PXEBC_VENDOR_OPTION        *VendorOption
  )
{
  UINT32                        *BitMap;
  UINT8                         VendorOptionLen;
  EFI_DHCP4_PACKET_OPTION       *PxeOption;
  UINT8                         Offset;

  BitMap          = VendorOption->BitMap;
  VendorOptionLen = Dhcp4Option->Length;
  PxeOption       = (EFI_DHCP4_PACKET_OPTION *) &Dhcp4Option->Data[0];
  Offset          = 0;

  ASSERT (PxeOption != NULL);

  while ((Offset < VendorOptionLen) && (PxeOption->OpCode != PXEBC_DHCP4_TAG_EOP)) {
    //
    // Parse all the interesting PXE vendor options one by one.
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

    default:
      //
      // Not interesting PXE vendor options.
      //
      break;
    }

    //
    // Set the bit map for the special PXE options.
    //
    SET_VENDOR_OPTION_BIT_MAP (BitMap, PxeOption->OpCode);

    //
    // Continue to the next option.
    //
    if (PxeOption->OpCode == PXEBC_DHCP4_TAG_PAD) {
      Offset++;
    } else {
      Offset = (UINT8) (Offset + PxeOption->Length + 2);
    }

    PxeOption = (EFI_DHCP4_PACKET_OPTION *) (Dhcp4Option->Data + Offset);
  }
}


/**
  Build the options buffer for the DHCPv4 request packet.

  @param[in]  Private             Pointer to PxeBc private data.
  @param[out] OptList             Pointer to the option pointer array.
  @param[in]  Buffer              Pointer to the buffer to contain the option list.
  @param[in]  NeedMsgType         If TRUE, it is necessary to include the Msg type option.
                                  Otherwise, it is not necessary.

  @return     Index               The count of the built-in options.

**/
UINT32
PxeBcBuildDhcp4Options (
  IN  PXEBC_PRIVATE_DATA       *Private,
  OUT EFI_DHCP4_PACKET_OPTION  **OptList,
  IN  UINT8                    *Buffer,
  IN  BOOLEAN                  NeedMsgType
  )
{
  UINT32                       Index;
  PXEBC_DHCP4_OPTION_ENTRY     OptEnt;
  UINT16                       Value;

  Index      = 0;
  OptList[0] = (EFI_DHCP4_PACKET_OPTION *) Buffer;

  if (NeedMsgType) {
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
    Value                   = NTOHS (PXEBC_DHCP4_PACKET_MAX_SIZE - 8);
    CopyMem (&OptEnt.MaxMesgSize->Size, &Value, sizeof (UINT16));
    Index++;
    OptList[Index]          = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);
  }

  //
  // Append parameter request list option.
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
    // Zero the Guid to indicate NOT programable if failed to get system Guid.
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
  OptList[Index]->OpCode  = PXEBC_PXE_DHCP4_TAG_ARCH;
  OptList[Index]->Length  = (UINT8) sizeof (PXEBC_DHCP4_OPTION_ARCH);
  OptEnt.Arch             = (PXEBC_DHCP4_OPTION_ARCH *) OptList[Index]->Data;
  Value                   = HTONS (EFI_PXE_CLIENT_SYSTEM_ARCHITECTURE);
  CopyMem (&OptEnt.Arch->Type, &Value, sizeof (UINT16));
  Index++;
  OptList[Index]          = GET_NEXT_DHCP_OPTION (OptList[Index - 1]);

  //
  // Append vendor class identify option
  //
  OptList[Index]->OpCode  = PXEBC_DHCP4_TAG_CLASS_ID;
  OptList[Index]->Length  = (UINT8) sizeof (PXEBC_DHCP4_OPTION_CLID);
  OptEnt.Clid             = (PXEBC_DHCP4_OPTION_CLID *) OptList[Index]->Data;
  CopyMem (
    OptEnt.Clid,
    DEFAULT_CLASS_ID_DATA,
    sizeof (PXEBC_DHCP4_OPTION_CLID)
    );
  PxeBcUintnToAscDecWithFormat (
    EFI_PXE_CLIENT_SYSTEM_ARCHITECTURE,
    OptEnt.Clid->ArchitectureType,
    sizeof (OptEnt.Clid->ArchitectureType)
    );

  if (Private->Nii != NULL) {
    CopyMem (OptEnt.Clid->InterfaceName, Private->Nii->StringId, sizeof (OptEnt.Clid->InterfaceName));
    PxeBcUintnToAscDecWithFormat (Private->Nii->MajorVer, OptEnt.Clid->UndiMajor, sizeof (OptEnt.Clid->UndiMajor));
    PxeBcUintnToAscDecWithFormat (Private->Nii->MinorVer, OptEnt.Clid->UndiMinor, sizeof (OptEnt.Clid->UndiMinor));
  }

  Index++;

  return Index;
}


/**
  Create a template DHCPv4 packet as a seed.

  @param[out] Seed           Pointer to the seed packet.
  @param[in]  Udp4           Pointer to EFI_UDP4_PROTOCOL.

**/
VOID
PxeBcSeedDhcp4Packet (
  OUT EFI_DHCP4_PACKET       *Seed,
  IN  EFI_UDP4_PROTOCOL      *Udp4
  )
{
  EFI_SIMPLE_NETWORK_MODE    Mode;
  EFI_DHCP4_HEADER           *Header;

  //
  // Get IfType and HwAddressSize from SNP mode data.
  //
  Udp4->GetModeData (Udp4, NULL, NULL, NULL, &Mode);

  Seed->Size            = sizeof (EFI_DHCP4_PACKET);
  Seed->Length          = sizeof (Seed->Dhcp4);
  Header                = &Seed->Dhcp4.Header;
  ZeroMem (Header, sizeof (EFI_DHCP4_HEADER));
  Header->OpCode        = PXEBC_DHCP4_OPCODE_REQUEST;
  Header->HwType        = Mode.IfType;
  Header->HwAddrLen     = (UINT8) Mode.HwAddressSize;
  CopyMem (Header->ClientHwAddr, &Mode.CurrentAddress, Header->HwAddrLen);

  Seed->Dhcp4.Magik     = PXEBC_DHCP4_MAGIC;
  Seed->Dhcp4.Option[0] = PXEBC_DHCP4_TAG_EOP;
}


/**
  Cache the DHCPv4 packet.

  @param[in]  Dst          Pointer to the cache buffer for DHCPv4 packet.
  @param[in]  Src          Pointer to the DHCPv4 packet to be cached.

**/
VOID
PxeBcCacheDhcp4Packet (
  IN EFI_DHCP4_PACKET     *Dst,
  IN EFI_DHCP4_PACKET     *Src
  )
{
  ASSERT (Dst->Size >= Src->Length);

  CopyMem (&Dst->Dhcp4, &Src->Dhcp4, Src->Length);
  Dst->Length = Src->Length;
}


/**
  Parse the cached DHCPv4 packet, including all the options.

  @param[in]  Cache4           Pointer to cached DHCPv4 packet.

  @retval     EFI_SUCCESS      Parsed the DHCPv4 packet successfully.
  @retval     EFI_DEVICE_ERROR Failed to parse and invalid packet.

**/
EFI_STATUS
PxeBcParseDhcp4Packet (
  IN PXEBC_DHCP4_PACKET_CACHE    *Cache4
  )
{
  EFI_DHCP4_PACKET               *Offer;
  EFI_DHCP4_PACKET_OPTION        **Options;
  EFI_DHCP4_PACKET_OPTION        *Option;
  PXEBC_OFFER_TYPE               OfferType;
  UINTN                          Index;
  BOOLEAN                        IsProxyOffer;
  BOOLEAN                        IsPxeOffer;
  UINT8                          *Ptr8;

  IsProxyOffer = FALSE;
  IsPxeOffer   = FALSE;

  ZeroMem (Cache4->OptList, sizeof (Cache4->OptList));
  ZeroMem (&Cache4->VendorOpt, sizeof (Cache4->VendorOpt));

  Offer   = &Cache4->Packet.Offer;
  Options = Cache4->OptList;

  //
  // Parse DHCPv4 options in this offer, and store the pointers.
  // First, try to parse DHCPv4 options from the DHCP optional parameters field.
  //
  for (Index = 0; Index < PXEBC_DHCP4_TAG_INDEX_MAX; Index++) {
    Options[Index] = PxeBcParseDhcp4Options (
                       Offer->Dhcp4.Option,
                       GET_OPTION_BUFFER_LEN (Offer),
                       mInterestedDhcp4Tags[Index]
                       );
  }
  //
  // Second, Check if bootfilename and serverhostname is overloaded to carry DHCP options refers to rfc-2132. 
  // If yes, try to parse options from the BootFileName field, then ServerName field.
  //
  Option = Options[PXEBC_DHCP4_TAG_INDEX_OVERLOAD];
  if (Option != NULL) {
    if ((Option->Data[0] & PXEBC_DHCP4_OVERLOAD_FILE) != 0) {
      for (Index = 0; Index < PXEBC_DHCP4_TAG_INDEX_MAX; Index++) {
        if (Options[Index] == NULL) {
          Options[Index] = PxeBcParseDhcp4Options (
                             (UINT8 *) Offer->Dhcp4.Header.BootFileName,
                             sizeof (Offer->Dhcp4.Header.BootFileName),
                             mInterestedDhcp4Tags[Index]
                             );
        }
      }
    }
    if ((Option->Data[0] & PXEBC_DHCP4_OVERLOAD_SERVER_NAME) != 0) {
      for (Index = 0; Index < PXEBC_DHCP4_TAG_INDEX_MAX; Index++) {
        if (Options[Index] == NULL) {
          Options[Index] = PxeBcParseDhcp4Options (
                             (UINT8 *) Offer->Dhcp4.Header.ServerName,
                             sizeof (Offer->Dhcp4.Header.ServerName),
                             mInterestedDhcp4Tags[Index]
                             );
        }
      }
    }
  }

  //
  // The offer with zero "yiaddr" is a proxy offer.
  //
  if (Offer->Dhcp4.Header.YourAddr.Addr[0] == 0) {
    IsProxyOffer = TRUE;
  }

  //
  // The offer with "PXEClient" is a PXE offer.
  //
  Option = Options[PXEBC_DHCP4_TAG_INDEX_CLASS_ID];
  if ((Option != NULL) && (Option->Length >= 9) &&
      (CompareMem (Option->Data, DEFAULT_CLASS_ID_DATA, 9) == 0)) {
    IsPxeOffer = TRUE;
  }

  //
  // Parse PXE vendor options in this offer, and store the contents/pointers.
  //
  Option = Options[PXEBC_DHCP4_TAG_INDEX_VENDOR];
  if (IsPxeOffer && Option != NULL) {
    PxeBcParseVendorOptions (Option, &Cache4->VendorOpt);
  }

  //
  // Parse PXE boot file name:
  // According to PXE spec, boot file name should be read from DHCP option 67 (bootfile name) if present.
  // Otherwise, read from boot file field in DHCP header.
  //
  if (Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL) {
    //
    // RFC 2132, Section 9.5 does not strictly state Bootfile name (option 67) is null
    // terminated string. So force to append null terminated character at the end of string.
    //
    Ptr8 =  (UINT8*)&Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE]->Data[0];
    Ptr8 += Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE]->Length;
    if (*(Ptr8 - 1) != '\0') {
      *Ptr8 = '\0';
    }
  } else if (Offer->Dhcp4.Header.BootFileName[0] != 0) {
    //
    // If the bootfile is not present and bootfilename is present in DHCPv4 packet, just parse it.
    // Do not count dhcp option header here, or else will destroy the serverhostname.
    //
    Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] = (EFI_DHCP4_PACKET_OPTION *)
                                                (&Offer->Dhcp4.Header.BootFileName[0] -
                                                OFFSET_OF (EFI_DHCP4_PACKET_OPTION, Data[0]));

  }

  //
  // Determine offer type of the DHCPv4 packet.
  //
  Option = Options[PXEBC_DHCP4_TAG_INDEX_MSG_TYPE];
  if (Option == NULL || Option->Data[0] == 0) {
    //
    // It's a Bootp offer.
    //
    OfferType = PxeOfferTypeBootp;

    Option    = Cache4->OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE];
    if (Option == NULL) {
      //
      // If the Bootp offer without bootfilename, discard it.
      //
      return EFI_DEVICE_ERROR;
    }
  } else {

    if (IS_VALID_DISCOVER_VENDOR_OPTION (Cache4->VendorOpt.BitMap)) {
      //
      // It's a PXE10 offer with PXEClient and discover vendor option.
      //
      OfferType = IsProxyOffer ? PxeOfferTypeProxyPxe10 : PxeOfferTypeDhcpPxe10;
    } else if (IS_VALID_MTFTP_VENDOR_OPTION (Cache4->VendorOpt.BitMap)) {
      //
      // It's a WFM11a offer with PXEClient and mtftp vendor option.
      // But multi-cast download is not supported currently, so discard it.
      //
      return EFI_DEVICE_ERROR;
    } else if (IsPxeOffer) {
      //
      // It's a BINL offer only with PXEClient.
      //
      OfferType = IsProxyOffer ? PxeOfferTypeProxyBinl : PxeOfferTypeDhcpBinl;
    } else {
      //
      // It's a DHCPv4 only offer, which is a pure DHCPv4 offer packet.
      //
      OfferType = PxeOfferTypeDhcpOnly;
    }
  }

  Cache4->OfferType = OfferType;

  return EFI_SUCCESS;
}


/**
  Cache the DHCPv4 ack packet, and parse it on demand.

  @param[in]  Private             Pointer to PxeBc private data.
  @param[in]  Ack                 Pointer to the DHCPv4 ack packet.
  @param[in]  Verified            If TRUE, parse the ACK packet and store info into mode data.

**/
VOID
PxeBcCopyDhcp4Ack (
  IN PXEBC_PRIVATE_DATA   *Private,
  IN EFI_DHCP4_PACKET     *Ack,
  IN BOOLEAN              Verified
  )
{
  EFI_PXE_BASE_CODE_MODE  *Mode;

  Mode = Private->PxeBc.Mode;

  PxeBcCacheDhcp4Packet (&Private->DhcpAck.Dhcp4.Packet.Ack, Ack);

  if (Verified) {
    //
    // Parse the ack packet and store it into mode data if needed.
    //
    PxeBcParseDhcp4Packet (&Private->DhcpAck.Dhcp4);
    CopyMem (&Mode->DhcpAck.Dhcpv4, &Ack->Dhcp4, Ack->Length);
    Mode->DhcpAckReceived = TRUE;
  }
}


/**
  Cache the DHCPv4 proxy offer packet according to the received order.

  @param[in]  Private               Pointer to PxeBc private data.
  @param[in]  OfferIndex            The received order of offer packets.

**/
VOID
PxeBcCopyProxyOffer (
  IN PXEBC_PRIVATE_DATA   *Private,
  IN UINT32               OfferIndex
  )
{
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_DHCP4_PACKET        *Offer;

  ASSERT (OfferIndex < Private->OfferNum);
  ASSERT (OfferIndex < PXEBC_OFFER_MAX_NUM);

  Mode  = Private->PxeBc.Mode;
  Offer = &Private->OfferBuffer[OfferIndex].Dhcp4.Packet.Offer;

  //
  // Cache the proxy offer packet and parse it.
  //
  PxeBcCacheDhcp4Packet (&Private->ProxyOffer.Dhcp4.Packet.Offer, Offer);
  PxeBcParseDhcp4Packet (&Private->ProxyOffer.Dhcp4);

  //
  // Store this packet into mode data.
  //
  CopyMem (&Mode->ProxyOffer.Dhcpv4, &Offer->Dhcp4, Offer->Length);
  Mode->ProxyOfferReceived = TRUE;
}


/**
  Retry to request bootfile name by the BINL offer.

  @param[in]  Private              Pointer to PxeBc private data.
  @param[in]  Index                The received order of offer packets.

  @retval     EFI_SUCCESS          Successfully retried to request bootfile name.
  @retval     EFI_DEVICE_ERROR     Failed to retry bootfile name.

**/
EFI_STATUS
PxeBcRetryBinlOffer (
  IN PXEBC_PRIVATE_DATA     *Private,
  IN UINT32                 Index
  )
{
  EFI_DHCP4_PACKET          *Offer;
  EFI_IP_ADDRESS            ServerIp;
  EFI_STATUS                Status;
  PXEBC_DHCP4_PACKET_CACHE  *Cache4;
  EFI_DHCP4_PACKET          *Reply;

  ASSERT (Index < PXEBC_OFFER_MAX_NUM);
  ASSERT (Private->OfferBuffer[Index].Dhcp4.OfferType == PxeOfferTypeDhcpBinl ||
          Private->OfferBuffer[Index].Dhcp4.OfferType == PxeOfferTypeProxyBinl);

  Offer = &Private->OfferBuffer[Index].Dhcp4.Packet.Offer;

  //
  // Prefer to siaddr in header as next server address. If it's zero, then use option 54.
  //
  if (Offer->Dhcp4.Header.ServerAddr.Addr[0] == 0) {
    CopyMem (
      &ServerIp.Addr[0],
      Private->OfferBuffer[Index].Dhcp4.OptList[PXEBC_DHCP4_TAG_INDEX_SERVER_ID]->Data,
      sizeof (EFI_IPv4_ADDRESS)
      );
  } else {
    CopyMem (
      &ServerIp.Addr[0],
      &Offer->Dhcp4.Header.ServerAddr,
      sizeof (EFI_IPv4_ADDRESS)
      );
  }

  Private->IsDoDiscover = FALSE;
  Cache4                = &Private->ProxyOffer.Dhcp4;
  Reply                 = &Cache4->Packet.Offer;

  //
  // Send another request packet for bootfile name.
  //
  Status = PxeBcDhcp4Discover (
             Private,
             0,
             NULL,
             FALSE,
             &ServerIp,
             0,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Parse the reply for the last request packet.
  //
  Status = PxeBcParseDhcp4Packet (Cache4);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Cache4->OfferType != PxeOfferTypeProxyPxe10 &&
      Cache4->OfferType != PxeOfferTypeProxyWfm11a &&
      Cache4->OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] == NULL) {
    //
    // This BINL ack doesn't have discovery option set or multicast option set
    // or bootfile name specified.
    //
    return EFI_DEVICE_ERROR;
  }

  //
  // Store the reply into mode data.
  //
  Private->PxeBc.Mode->ProxyOfferReceived = TRUE;
  CopyMem (&Private->PxeBc.Mode->ProxyOffer.Dhcpv4, &Reply->Dhcp4, Reply->Length);

  return EFI_SUCCESS;
}


/**
  Cache all the received DHCPv4 offers, and set OfferIndex and OfferCount.

  @param[in]  Private               Pointer to PxeBc private data.
  @param[in]  RcvdOffer             Pointer to the received offer packet.

**/
VOID
PxeBcCacheDhcp4Offer (
  IN PXEBC_PRIVATE_DATA     *Private,
  IN EFI_DHCP4_PACKET       *RcvdOffer
  )
{
  PXEBC_DHCP4_PACKET_CACHE  *Cache4;
  EFI_DHCP4_PACKET          *Offer;
  PXEBC_OFFER_TYPE          OfferType;

  ASSERT (Private->OfferNum < PXEBC_OFFER_MAX_NUM);
  Cache4 = &Private->OfferBuffer[Private->OfferNum].Dhcp4;
  Offer  = &Cache4->Packet.Offer;

  //
  // Cache the content of DHCPv4 packet firstly.
  //
  PxeBcCacheDhcp4Packet (Offer, RcvdOffer);

  //
  // Validate the DHCPv4 packet, and parse the options and offer type.
  //
  if (EFI_ERROR (PxeBcParseDhcp4Packet (Cache4))) {
    return;
  }

  //
  // Determine whether cache the current offer by type, and record OfferIndex and OfferCount.
  //
  OfferType = Cache4->OfferType;
  ASSERT (OfferType < PxeOfferTypeMax);

  if (OfferType == PxeOfferTypeBootp) {
    //
    // It's a Bootp offer, only cache the first one, and discard the others.
    //
    if (Private->OfferCount[OfferType] == 0) {
      Private->OfferIndex[OfferType][0] = Private->OfferNum;
      Private->OfferCount[OfferType]    = 1;
    } else {
      return;
    }
  } else {
    ASSERT (Private->OfferCount[OfferType] < PXEBC_OFFER_MAX_NUM);
    if (IS_PROXY_DHCP_OFFER (Offer)) {
      //
      // It's a proxy offer without yiaddr, including PXE10, WFM11a or BINL offer.
      //
      Private->IsProxyRecved = TRUE;

      if (OfferType == PxeOfferTypeProxyBinl) {
        //
        // Cache all proxy BINL offers.
        //
        Private->OfferIndex[OfferType][Private->OfferCount[OfferType]] = Private->OfferNum;
        Private->OfferCount[OfferType]++;
      } else if (Private->OfferCount[OfferType] > 0) {
        //
        // Only cache the first PXE10/WFM11a offer, and discard the others.
        //
        Private->OfferIndex[OfferType][0] = Private->OfferNum;
        Private->OfferCount[OfferType]    = 1;
      } else {
        return ;
      }
    } else {
      //
      // It's a DHCPv4 offer with yiaddr, and cache them all.
      //
      Private->OfferIndex[OfferType][Private->OfferCount[OfferType]] = Private->OfferNum;
      Private->OfferCount[OfferType]++;
    }
  }

  Private->OfferNum++;
}


/**
  Select an DHCPv4 offer, and record SelectIndex and SelectProxyType.

  @param[in]  Private             Pointer to PxeBc private data.

**/
VOID
PxeBcSelectDhcp4Offer (
  IN PXEBC_PRIVATE_DATA       *Private
  )
{
  UINT32                      Index;
  UINT32                      OfferIndex;
  EFI_DHCP4_PACKET            *Offer;

  Private->SelectIndex = 0;

  if (Private->IsOfferSorted) {
    //
    // Select offer by default policy.
    //
    if (Private->OfferCount[PxeOfferTypeDhcpPxe10] > 0) {
      //
      // 1. DhcpPxe10 offer
      //
      Private->SelectIndex = Private->OfferIndex[PxeOfferTypeDhcpPxe10][0] + 1;

    } else if (Private->OfferCount[PxeOfferTypeDhcpWfm11a] > 0) {
      //
      // 2. DhcpWfm11a offer
      //
      Private->SelectIndex = Private->OfferIndex[PxeOfferTypeDhcpWfm11a][0] + 1;

    } else if (Private->OfferCount[PxeOfferTypeDhcpOnly] > 0 &&
               Private->OfferCount[PxeOfferTypeProxyPxe10] > 0) {
      //
      // 3. DhcpOnly offer and ProxyPxe10 offer.
      //
      Private->SelectIndex     = Private->OfferIndex[PxeOfferTypeDhcpOnly][0] + 1;
      Private->SelectProxyType = PxeOfferTypeProxyPxe10;

    } else if (Private->OfferCount[PxeOfferTypeDhcpOnly] > 0 &&
               Private->OfferCount[PxeOfferTypeProxyWfm11a] > 0) {
      //
      // 4. DhcpOnly offer and ProxyWfm11a offer.
      //
      Private->SelectIndex     = Private->OfferIndex[PxeOfferTypeDhcpOnly][0] + 1;
      Private->SelectProxyType = PxeOfferTypeProxyWfm11a;

    } else if (Private->OfferCount[PxeOfferTypeDhcpBinl] > 0) {
      //
      // 5. DhcpBinl offer.
      //
      Private->SelectIndex = Private->OfferIndex[PxeOfferTypeDhcpBinl][0] + 1;

    } else if (Private->OfferCount[PxeOfferTypeDhcpOnly] > 0 &&
               Private->OfferCount[PxeOfferTypeProxyBinl] > 0) {
      //
      // 6. DhcpOnly offer and ProxyBinl offer.
      //
      Private->SelectIndex     = Private->OfferIndex[PxeOfferTypeDhcpOnly][0] + 1;
      Private->SelectProxyType = PxeOfferTypeProxyBinl;

    } else {
      //
      // 7. DhcpOnly offer with bootfilename.
      //
      for (Index = 0; Index < Private->OfferCount[PxeOfferTypeDhcpOnly]; Index++) {
        OfferIndex = Private->OfferIndex[PxeOfferTypeDhcpOnly][Index];
        if (Private->OfferBuffer[OfferIndex].Dhcp4.OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL) {
          Private->SelectIndex = OfferIndex + 1;
          break;
        }
      }
      //
      // 8. Bootp offer with bootfilename.
      //
      OfferIndex = Private->OfferIndex[PxeOfferTypeBootp][0];
      if (Private->SelectIndex == 0 &&
          Private->OfferCount[PxeOfferTypeBootp] > 0 &&
          Private->OfferBuffer[OfferIndex].Dhcp4.OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL) {
        Private->SelectIndex = OfferIndex + 1;
      }
    }
  } else {
    //
    // Select offer by received order.
    //
    for (Index = 0; Index < Private->OfferNum; Index++) {

      Offer = &Private->OfferBuffer[Index].Dhcp4.Packet.Offer;

      if (IS_PROXY_DHCP_OFFER (Offer)) {
        //
        // Skip proxy offers
        //
        continue;
      }

      if (!Private->IsProxyRecved &&
          Private->OfferBuffer[Index].Dhcp4.OfferType == PxeOfferTypeDhcpOnly &&
          Private->OfferBuffer[Index].Dhcp4.OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] == NULL) {
        //
        // Skip if DhcpOnly offer without any other proxy offers or bootfilename.
        //
        continue;
      }

      //
      // Record the index of the select offer.
      //
      Private->SelectIndex = Index + 1;
      break;
    }
  }
}


/**
  Handle the DHCPv4 offer packet.

  @param[in]  Private             Pointer to PxeBc private data.

  @retval     EFI_SUCCESS         Handled the DHCPv4 offer packet successfully.
  @retval     EFI_NO_RESPONSE     No response to the following request packet.
  @retval     EFI_NOT_FOUND       No boot filename received.

**/
EFI_STATUS
PxeBcHandleDhcp4Offer (
  IN PXEBC_PRIVATE_DATA     *Private
  )
{
  PXEBC_DHCP4_PACKET_CACHE  *Cache4;
  EFI_DHCP4_PACKET_OPTION   **Options;
  UINT32                    Index;
  EFI_DHCP4_PACKET          *Offer;
  PXEBC_OFFER_TYPE          OfferType;
  UINT32                    ProxyIndex;
  UINT32                    SelectIndex;
  EFI_STATUS                Status;
  EFI_PXE_BASE_CODE_MODE    *Mode;
  EFI_DHCP4_PACKET          *Ack;

  ASSERT (Private->SelectIndex > 0);
  SelectIndex = (UINT32) (Private->SelectIndex - 1);
  ASSERT (SelectIndex < PXEBC_OFFER_MAX_NUM);
  Cache4      = &Private->OfferBuffer[SelectIndex].Dhcp4;
  Options     = Cache4->OptList;
  Status      = EFI_SUCCESS;

  if (Cache4->OfferType == PxeOfferTypeDhcpBinl) {
    //
    // DhcpBinl offer is selected, so need try to request bootfilename by this offer.
    //
    if (EFI_ERROR (PxeBcRetryBinlOffer (Private, SelectIndex))) {
      Status = EFI_NO_RESPONSE;
    }
  } else if (Cache4->OfferType == PxeOfferTypeDhcpOnly) {

    if (Private->IsProxyRecved) {
      //
      // DhcpOnly offer is selected, so need try to request bootfile name.
      //
      ProxyIndex = 0;
      if (Private->IsOfferSorted) {
        //
        // The proxy offer should be determined if select by default policy.
        // IsOfferSorted means all offers are labeled by OfferIndex.
        //
        ASSERT (Private->SelectProxyType < PxeOfferTypeMax);
        ASSERT (Private->OfferCount[Private->SelectProxyType] > 0);

        if (Private->SelectProxyType == PxeOfferTypeProxyBinl) {
          //
          // Try all the cached ProxyBinl offer one by one to request bootfile name.
          //
          for (Index = 0; Index < Private->OfferCount[Private->SelectProxyType]; Index++) {
            ASSERT (Index < PXEBC_OFFER_MAX_NUM);
            ProxyIndex = Private->OfferIndex[Private->SelectProxyType][Index];
            if (!EFI_ERROR (PxeBcRetryBinlOffer (Private, ProxyIndex))) {
              break;
            }
          }
          if (Index == Private->OfferCount[Private->SelectProxyType]) {
            Status = EFI_NO_RESPONSE;
          }
        } else {
          //
          // For other proxy offers, only one is buffered.
          //
          ProxyIndex = Private->OfferIndex[Private->SelectProxyType][0];
        }
      } else {
        //
        // The proxy offer should not be determined if select by received order.
        //
        Status = EFI_NO_RESPONSE;

        for (Index = 0; Index < Private->OfferNum; Index++) {
          ASSERT (Index < PXEBC_OFFER_MAX_NUM);
          Offer     = &Private->OfferBuffer[Index].Dhcp4.Packet.Offer;
          OfferType = Private->OfferBuffer[Index].Dhcp4.OfferType;
          if (!IS_PROXY_DHCP_OFFER (Offer)) {
            //
            // Skip non proxy DHCPv4 offers.
            //
            continue;
          }

          if (OfferType == PxeOfferTypeProxyBinl) {
            //
            // Try all the cached ProxyBinl offer one by one to request bootfile name.
            //
            if (EFI_ERROR (PxeBcRetryBinlOffer (Private, Index))) {
              continue;
            }
          }

          Private->SelectProxyType = OfferType;
          ProxyIndex               = Index;
          Status                   = EFI_SUCCESS;
          break;
        }
      }

      if (!EFI_ERROR (Status) && Private->SelectProxyType != PxeOfferTypeProxyBinl) {
        //
        // Success to try to request by a ProxyPxe10 or ProxyWfm11a offer, copy and parse it.
        //
        PxeBcCopyProxyOffer (Private, ProxyIndex);
      }
    } else {
      //
      //  Othewise, the bootfile name must be included in DhcpOnly offer.
      //
      if (Options[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] == NULL) {
        Status = EFI_NOT_FOUND;
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // All PXE boot information is ready by now.
    //
    Mode  = Private->PxeBc.Mode;
    Offer = &Cache4->Packet.Offer;
    Ack   = &Private->DhcpAck.Dhcp4.Packet.Ack;
    if (Cache4->OfferType == PxeOfferTypeBootp) {
      //
      // Bootp is a special case that only 2 packets involved instead of 4. So the bootp's reply
      // should be taken as ack.
      //
      Ack = Offer;
    }

    PxeBcCopyDhcp4Ack (Private, Ack, TRUE);
    Mode->DhcpDiscoverValid = TRUE;
  }

  return Status;
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
PxeBcDhcp4CallBack (
  IN  EFI_DHCP4_PROTOCOL               *This,
  IN  VOID                             *Context,
  IN  EFI_DHCP4_STATE                  CurrentState,
  IN  EFI_DHCP4_EVENT                  Dhcp4Event,
  IN  EFI_DHCP4_PACKET                 *Packet            OPTIONAL,
  OUT EFI_DHCP4_PACKET                 **NewPacket        OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA                   *Private;
  EFI_PXE_BASE_CODE_MODE               *Mode;
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL  *Callback;
  EFI_DHCP4_PACKET_OPTION              *MaxMsgSize;
  UINT16                               Value;
  EFI_STATUS                           Status;
  BOOLEAN                              Received;

  if ((Dhcp4Event != Dhcp4RcvdOffer) &&
      (Dhcp4Event != Dhcp4SelectOffer) &&
      (Dhcp4Event != Dhcp4SendDiscover) &&
      (Dhcp4Event != Dhcp4RcvdAck)) {
    return EFI_SUCCESS;
  }

  Private   = (PXEBC_PRIVATE_DATA *) Context;
  Mode      = Private->PxeBc.Mode;
  Callback  = Private->PxeBcCallback;

  //
  // Override the Maximum DHCP Message Size.
  //
  MaxMsgSize = PxeBcParseDhcp4Options (
                 Packet->Dhcp4.Option,
                 GET_OPTION_BUFFER_LEN (Packet),
                 PXEBC_DHCP4_TAG_MAXMSG
                 );
  if (MaxMsgSize != NULL) {
    Value = HTONS (PXEBC_DHCP4_PACKET_MAX_SIZE - 8);
    CopyMem (MaxMsgSize->Data, &Value, sizeof (Value));
  }

  //
  // Callback to user if any packets sent or received.
  //
  if (Dhcp4Event != Dhcp4SelectOffer && Callback != NULL) {
    Received = (BOOLEAN) (Dhcp4Event == Dhcp4RcvdOffer || Dhcp4Event == Dhcp4RcvdAck);
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
    //
    // Cache the DHCPv4 discover packet to mode data directly.
    // It need to check SendGuid as well as Dhcp4SendRequest.
    //
    CopyMem (&Mode->DhcpDiscover.Dhcpv4, &Packet->Dhcp4, Packet->Length);

  case Dhcp4SendRequest:
    if (Mode->SendGUID) {
      //
      // Send the system Guid instead of the MAC address as the hardware address if required.
      //
      if (EFI_ERROR (NetLibGetSystemGuid ((EFI_GUID *) Packet->Dhcp4.Header.ClientHwAddr))) {
        //
        // Zero the Guid to indicate NOT programable if failed to get system Guid.
        //
        ZeroMem (Packet->Dhcp4.Header.ClientHwAddr, sizeof (EFI_GUID));
      }
      Packet->Dhcp4.Header.HwAddrLen = (UINT8) sizeof (EFI_GUID);
    }
    break;

  case Dhcp4RcvdOffer:
    Status = EFI_NOT_READY;
    if (Private->OfferNum < PXEBC_OFFER_MAX_NUM) {
      //
      // Cache the DHCPv4 offers to OfferBuffer[] for select later, and record
      // the OfferIndex and OfferCount.
      //
      PxeBcCacheDhcp4Offer (Private, Packet);
    }
    break;

  case Dhcp4SelectOffer:
    //
    // Select offer by the default policy or by order, and record the SelectIndex
    // and SelectProxyType.
    //
    PxeBcSelectDhcp4Offer (Private);

    if (Private->SelectIndex == 0) {
      Status = EFI_ABORTED;
    } else {
      *NewPacket = &Private->OfferBuffer[Private->SelectIndex - 1].Dhcp4.Packet.Offer;
    }
    break;

  case Dhcp4RcvdAck:
    //
    // Cache the DHCPv4 ack to Private->Dhcp4Ack, but it's not the final ack in mode data
    // without verification.
    //
    ASSERT (Private->SelectIndex != 0);

    PxeBcCopyDhcp4Ack (Private, Packet, FALSE);
    break;

  default:
    break;
  }

  return Status;
}


/**
  Build and send out the request packet for the bootfile, and parse the reply.

  @param[in]  Private               Pointer to PxeBc private data.
  @param[in]  Type                  PxeBc option boot item type.
  @param[in]  Layer                 Pointer to option boot item layer.
  @param[in]  UseBis                Use BIS or not.
  @param[in]  DestIp                Pointer to the server address.
  @param[in]  IpCount               The total count of the server address.
  @param[in]  SrvList               Pointer to EFI_PXE_BASE_CODE_SRVLIST.

  @retval     EFI_SUCCESS           Successfully discovered boot file.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate resource.
  @retval     EFI_NOT_FOUND         Can't get the PXE reply packet.
  @retval     Others                Failed to discover boot file.

**/
EFI_STATUS
PxeBcDhcp4Discover (
  IN  PXEBC_PRIVATE_DATA              *Private,
  IN  UINT16                          Type,
  IN  UINT16                          *Layer,
  IN  BOOLEAN                         UseBis,
  IN  EFI_IP_ADDRESS                  *DestIp,
  IN  UINT16                          IpCount,
  IN  EFI_PXE_BASE_CODE_SRVLIST       *SrvList
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
  UINT8                               Buffer[PXEBC_DHCP4_OPTION_MAX_SIZE];
  EFI_DHCP4_PACKET_OPTION             *OptList[PXEBC_DHCP4_OPTION_MAX_NUM];
  UINT32                              OptCount;
  EFI_DHCP4_PACKET_OPTION             *PxeOpt;
  PXEBC_OPTION_BOOT_ITEM              *PxeBootItem;
  UINT8                               VendorOptLen;
  UINT32                              Xid;

  Mode      = Private->PxeBc.Mode;
  Dhcp4     = Private->Dhcp4;
  Status    = EFI_SUCCESS;

  ZeroMem (&Token, sizeof (EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN));

  //
  // Use broadcast if destination address not specified.
  //
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

  //
  // Build all the options for the request packet.
  //
  OptCount = PxeBcBuildDhcp4Options (Private, OptList, Buffer, TRUE);

  if (Private->IsDoDiscover) {
    //
    // Add vendor option of PXE_BOOT_ITEM
    //
    VendorOptLen      = (UINT8) ((sizeof (EFI_DHCP4_PACKET_OPTION) - 1) * 2 + sizeof (PXEBC_OPTION_BOOT_ITEM) + 1);
    OptList[OptCount] = AllocateZeroPool (VendorOptLen);
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
    PxeOpt->Data[PxeOpt->Length]  = PXEBC_DHCP4_TAG_EOP;

    if (Layer != NULL) {
      PxeBootItem->Layer          = HTONS (*Layer);
    }

    OptCount++;
  }

  //
  // Build the request packet with seed packet and option list.
  //
  Status = Dhcp4->Build (
                    Dhcp4,
                    &Private->SeedPacket,
                    0,
                    NULL,
                    OptCount,
                    OptList,
                    &Token.Packet
                    );
  //
  // Free the vendor option of PXE_BOOT_ITEM.
  //
  if (Private->IsDoDiscover) {
    FreePool (OptList[OptCount - 1]);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Mode->SendGUID) {
    if (EFI_ERROR (NetLibGetSystemGuid ((EFI_GUID *) Token.Packet->Dhcp4.Header.ClientHwAddr))) {
      //
      // Zero the Guid to indicate NOT programable if failed to get system Guid.
      //
      ZeroMem (Token.Packet->Dhcp4.Header.ClientHwAddr, sizeof (EFI_GUID));
    }
    Token.Packet->Dhcp4.Header.HwAddrLen = (UINT8)  sizeof (EFI_GUID);
  }

  //
  // Set fields of the token for the request packet.
  //
  Xid                                 = NET_RANDOM (NetRandomInitSeed ());
  Token.Packet->Dhcp4.Header.Xid      = HTONL (Xid);
  Token.Packet->Dhcp4.Header.Reserved = HTONS ((UINT16) ((IsBCast) ? 0x8000 : 0x0));
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
  // Send out the request packet to discover the bootfile.
  //
  for (TryIndex = 1; TryIndex <= PXEBC_BOOT_REQUEST_RETRIES; TryIndex++) {

    Token.TimeoutValue                  = (UINT16) (PXEBC_BOOT_REQUEST_TIMEOUT * TryIndex);
    Token.Packet->Dhcp4.Header.Seconds  = (UINT16) (PXEBC_BOOT_REQUEST_TIMEOUT * (TryIndex - 1));

    Status = Dhcp4->TransmitReceive (Dhcp4, &Token);
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

    RepIndex  = 0;
    SrvIndex  = 0;
    Response  = Token.ResponseList;
    //
    // Find the right PXE Reply according to server address.
    //
    while (RepIndex < Token.ResponseCount) {

      while (SrvIndex < IpCount) {
        if (SrvList[SrvIndex].AcceptAnyResponse) {
          break;
        }
        if ((SrvList[SrvIndex].Type == Type) &&
            EFI_IP4_EQUAL (&Response->Dhcp4.Header.ServerAddr, &SrvList[SrvIndex].IpAddr)) {
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
      //
      // Cache the right PXE reply packet here, set valid flag later.
      // Especially for PXE discover packet, store it into mode data here.
      //
      if (Private->IsDoDiscover) {
        PxeBcCacheDhcp4Packet (&Private->PxeReply.Dhcp4.Packet.Ack, Response);
        CopyMem (&Mode->PxeDiscover, &Token.Packet->Dhcp4, Token.Packet->Length);
      } else {
        PxeBcCacheDhcp4Packet (&Private->ProxyOffer.Dhcp4.Packet.Offer, Response);
      }
    } else {
      //
      // Not found the right PXE reply packet.
      //
      Status = EFI_NOT_FOUND;
    }
    if (Token.ResponseList != NULL) {
      FreePool (Token.ResponseList);
    }
  }

  FreePool (Token.Packet);
  return Status;
}

/**
  Switch the Ip4 policy to static.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

  @retval     EFI_SUCCESS         The policy is already configured to static.
  @retval     Others              Other error as indicated..

**/
EFI_STATUS
PxeBcSetIp4Policy (   
  IN PXEBC_PRIVATE_DATA            *Private
  )
{
  EFI_STATUS                   Status;
  EFI_IP4_CONFIG2_PROTOCOL     *Ip4Config2;
  EFI_IP4_CONFIG2_POLICY       Policy;
  UINTN                        DataSize;

  Ip4Config2 = Private->Ip4Config2;
  DataSize = sizeof (EFI_IP4_CONFIG2_POLICY);
  Status = Ip4Config2->GetData (
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
    Status= Ip4Config2->SetData (
                          Ip4Config2,
                          Ip4Config2DataTypePolicy,
                          sizeof (EFI_IP4_CONFIG2_POLICY),
                          &Policy
                          );
    if (EFI_ERROR (Status)) {
      return Status;
    } 
  }

  return  EFI_SUCCESS;
}

/**
  Start the D.O.R.A DHCPv4 process to acquire the IPv4 address and other PXE boot information.

  @param[in]  Private           Pointer to PxeBc private data.
  @param[in]  Dhcp4             Pointer to the EFI_DHCP4_PROTOCOL

  @retval EFI_SUCCESS           The D.O.R.A process successfully finished.
  @retval Others                Failed to finish the D.O.R.A process.

**/
EFI_STATUS
PxeBcDhcp4Dora (
  IN PXEBC_PRIVATE_DATA        *Private,
  IN EFI_DHCP4_PROTOCOL        *Dhcp4
  )
{
  EFI_PXE_BASE_CODE_MODE       *PxeMode;
  EFI_DHCP4_CONFIG_DATA        Config;
  EFI_DHCP4_MODE_DATA          Mode;
  EFI_DHCP4_PACKET_OPTION      *OptList[PXEBC_DHCP4_OPTION_MAX_NUM];
  UINT8                        Buffer[PXEBC_DHCP4_OPTION_MAX_SIZE];
  UINT32                       OptCount;
  EFI_STATUS                   Status;

  ASSERT (Dhcp4 != NULL);

  Status   = EFI_SUCCESS;
  PxeMode  = Private->PxeBc.Mode;

  //
  // Build option list for the request packet.
  //
  OptCount = PxeBcBuildDhcp4Options (Private, OptList, Buffer, FALSE);
  ASSERT (OptCount> 0);

  ZeroMem (&Mode, sizeof (EFI_DHCP4_MODE_DATA));
  ZeroMem (&Config, sizeof (EFI_DHCP4_CONFIG_DATA));

  Config.OptionCount      = OptCount;
  Config.OptionList       = OptList;
  Config.Dhcp4Callback    = PxeBcDhcp4CallBack;
  Config.CallbackContext  = Private;
  Config.DiscoverTryCount = PXEBC_DHCP_RETRIES;
  Config.DiscoverTimeout  = mPxeDhcpTimeout;

  //
  // Configure the DHCPv4 instance for PXE boot.
  //
  Status = Dhcp4->Configure (Dhcp4, &Config);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Initialize the record fields for DHCPv4 offer in private data.
  //
  Private->IsProxyRecved = FALSE;
  Private->OfferNum      = 0;
  ZeroMem (Private->OfferCount, sizeof (Private->OfferCount));
  ZeroMem (Private->OfferIndex, sizeof (Private->OfferIndex));

  //
  // Start DHCPv4 D.O.R.A. process to acquire IPv4 address. This may 
  // have already been done, thus do not leave in error if the return
  // code is EFI_ALREADY_STARTED.
  //
  Status = Dhcp4->Start (Dhcp4, NULL);
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    if (Status == EFI_ICMP_ERROR) {
      PxeMode->IcmpErrorReceived = TRUE;
    }
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
  CopyMem (&PxeMode->StationIp, &Private->StationIp, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&PxeMode->SubnetMask, &Private->SubnetMask, sizeof (EFI_IPv4_ADDRESS));

  Status = PxeBcFlushStationIp (Private, &Private->StationIp, &Private->SubnetMask);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Check the selected offer whether BINL retry is needed.
  //
  Status = PxeBcHandleDhcp4Offer (Private);

  AsciiPrint ("\n  Station IP address is ");

  PxeBcShowIp4Addr (&Private->StationIp.v4);
  AsciiPrint ("\n");

ON_EXIT:
  if (EFI_ERROR (Status)) {
    Dhcp4->Stop (Dhcp4);
    Dhcp4->Configure (Dhcp4, NULL);
  } else {
    ZeroMem (&Config, sizeof (EFI_DHCP4_CONFIG_DATA));
    Dhcp4->Configure (Dhcp4, &Config);
    Private->IsAddressOk = TRUE;
  }

  return Status;
}
