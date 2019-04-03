/** @file
  Functions implementation related with DHCPv6 for UefiPxeBc Driver.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PxeBcImpl.h"

//
// Well-known multi-cast address defined in section-24.1 of rfc-3315
//
//   ALL_DHCP_Relay_Agents_and_Servers address: FF02::1:2
//
EFI_IPv6_ADDRESS   mAllDhcpRelayAndServersAddress = {{0xFF, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2}};

/**
  Parse out a DHCPv6 option by OptTag, and find the position in buffer.

  @param[in]  Buffer        The pointer to the option buffer.
  @param[in]  Length        Length of the option buffer.
  @param[in]  OptTag        The required option tag.

  @retval     NULL          Failed to parse the required option.
  @retval     Others        The postion of the required option in buffer.

**/
EFI_DHCP6_PACKET_OPTION *
PxeBcParseDhcp6Options (
  IN UINT8                       *Buffer,
  IN UINT32                      Length,
  IN UINT16                      OptTag
  )
{
  EFI_DHCP6_PACKET_OPTION        *Option;
  UINT32                         Offset;

  Option  = (EFI_DHCP6_PACKET_OPTION *) Buffer;
  Offset  = 0;

  //
  // OpLen and OpCode here are both stored in network order.
  //
  while (Offset < Length) {

    if (NTOHS (Option->OpCode) == OptTag) {

      return Option;
    }

    Offset += (NTOHS(Option->OpLen) + 4);
    Option  = (EFI_DHCP6_PACKET_OPTION *) (Buffer + Offset);
  }

  return NULL;
}


/**
  Build the options buffer for the DHCPv6 request packet.

  @param[in]  Private             The pointer to PxeBc private data.
  @param[out] OptList             The pointer to the option pointer array.
  @param[in]  Buffer              The pointer to the buffer to contain the option list.

  @return     Index               The count of the built-in options.

**/
UINT32
PxeBcBuildDhcp6Options (
  IN  PXEBC_PRIVATE_DATA           *Private,
  OUT EFI_DHCP6_PACKET_OPTION      **OptList,
  IN  UINT8                        *Buffer
  )
{
  PXEBC_DHCP6_OPTION_ENTRY         OptEnt;
  UINT32                           Index;
  UINT16                           Value;

  Index       = 0;
  OptList[0]  = (EFI_DHCP6_PACKET_OPTION *) Buffer;

  //
  // Append client option request option
  //
  OptList[Index]->OpCode     = HTONS (DHCP6_OPT_ORO);
  OptList[Index]->OpLen      = HTONS (8);
  OptEnt.Oro                 = (PXEBC_DHCP6_OPTION_ORO *) OptList[Index]->Data;
  OptEnt.Oro->OpCode[0]      = HTONS(DHCP6_OPT_BOOT_FILE_URL);
  OptEnt.Oro->OpCode[1]      = HTONS(DHCP6_OPT_BOOT_FILE_PARAM);
  OptEnt.Oro->OpCode[2]      = HTONS(DHCP6_OPT_DNS_SERVERS);
  OptEnt.Oro->OpCode[3]      = HTONS(DHCP6_OPT_VENDOR_CLASS);
  Index++;
  OptList[Index]             = GET_NEXT_DHCP6_OPTION (OptList[Index - 1]);

  //
  // Append client network device interface option
  //
  OptList[Index]->OpCode     = HTONS (DHCP6_OPT_UNDI);
  OptList[Index]->OpLen      = HTONS ((UINT16)3);
  OptEnt.Undi                = (PXEBC_DHCP6_OPTION_UNDI *) OptList[Index]->Data;

  if (Private->Nii != NULL) {
    OptEnt.Undi->Type        = Private->Nii->Type;
    OptEnt.Undi->MajorVer    = Private->Nii->MajorVer;
    OptEnt.Undi->MinorVer    = Private->Nii->MinorVer;
  } else {
    OptEnt.Undi->Type        = DEFAULT_UNDI_TYPE;
    OptEnt.Undi->MajorVer    = DEFAULT_UNDI_MAJOR;
    OptEnt.Undi->MinorVer    = DEFAULT_UNDI_MINOR;
  }

  Index++;
  OptList[Index]             = GET_NEXT_DHCP6_OPTION (OptList[Index - 1]);

  //
  // Append client system architecture option
  //
  OptList[Index]->OpCode     = HTONS (DHCP6_OPT_ARCH);
  OptList[Index]->OpLen      = HTONS ((UINT16) sizeof (PXEBC_DHCP6_OPTION_ARCH));
  OptEnt.Arch                = (PXEBC_DHCP6_OPTION_ARCH *) OptList[Index]->Data;
  Value                      = HTONS (EFI_PXE_CLIENT_SYSTEM_ARCHITECTURE);
  CopyMem (&OptEnt.Arch->Type, &Value, sizeof (UINT16));
  Index++;
  OptList[Index]             = GET_NEXT_DHCP6_OPTION (OptList[Index - 1]);

  //
  // Append vendor class option to store the PXE class identifier.
  //
  OptList[Index]->OpCode       = HTONS (DHCP6_OPT_VENDOR_CLASS);
  OptList[Index]->OpLen        = HTONS ((UINT16) sizeof (PXEBC_DHCP6_OPTION_VENDOR_CLASS));
  OptEnt.VendorClass           = (PXEBC_DHCP6_OPTION_VENDOR_CLASS *) OptList[Index]->Data;
  OptEnt.VendorClass->Vendor   = HTONL (PXEBC_DHCP6_ENTERPRISE_NUM);
  OptEnt.VendorClass->ClassLen = HTONS ((UINT16) sizeof (PXEBC_CLASS_ID));
  CopyMem (
    &OptEnt.VendorClass->ClassId,
    DEFAULT_CLASS_ID_DATA,
    sizeof (PXEBC_CLASS_ID)
    );
  PxeBcUintnToAscDecWithFormat (
    EFI_PXE_CLIENT_SYSTEM_ARCHITECTURE,
    OptEnt.VendorClass->ClassId.ArchitectureType,
    sizeof (OptEnt.VendorClass->ClassId.ArchitectureType)
    );

  if (Private->Nii != NULL) {
    CopyMem (
      OptEnt.VendorClass->ClassId.InterfaceName,
      Private->Nii->StringId,
      sizeof (OptEnt.VendorClass->ClassId.InterfaceName)
      );
    PxeBcUintnToAscDecWithFormat (
      Private->Nii->MajorVer,
      OptEnt.VendorClass->ClassId.UndiMajor,
      sizeof (OptEnt.VendorClass->ClassId.UndiMajor)
      );
    PxeBcUintnToAscDecWithFormat (
      Private->Nii->MinorVer,
      OptEnt.VendorClass->ClassId.UndiMinor,
      sizeof (OptEnt.VendorClass->ClassId.UndiMinor)
      );
  }

  Index++;

  return Index;
}


/**
  Cache the DHCPv6 packet.

  @param[in]  Dst          The pointer to the cache buffer for DHCPv6 packet.
  @param[in]  Src          The pointer to the DHCPv6 packet to be cached.

  @retval     EFI_SUCCESS                Packet is copied.
  @retval     EFI_BUFFER_TOO_SMALL       Cache buffer is not big enough to hold the packet.

**/
EFI_STATUS
PxeBcCacheDhcp6Packet (
  IN EFI_DHCP6_PACKET          *Dst,
  IN EFI_DHCP6_PACKET          *Src
  )
{
  if (Dst->Size < Src->Length) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (&Dst->Dhcp6, &Src->Dhcp6, Src->Length);
  Dst->Length = Src->Length;

  return EFI_SUCCESS;
}

/**
  Retrieve the boot server address using the EFI_DNS6_PROTOCOL.

  @param[in]  Private             Pointer to PxeBc private data.
  @param[in]  HostName            Pointer to buffer containing hostname.
  @param[out] IpAddress           On output, pointer to buffer containing IPv6 address.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
PxeBcDns6 (
  IN PXEBC_PRIVATE_DATA           *Private,
  IN     CHAR16                   *HostName,
     OUT EFI_IPv6_ADDRESS         *IpAddress
  )
{
  EFI_STATUS                      Status;
  EFI_DNS6_PROTOCOL               *Dns6;
  EFI_DNS6_CONFIG_DATA            Dns6ConfigData;
  EFI_DNS6_COMPLETION_TOKEN       Token;
  EFI_HANDLE                      Dns6Handle;
  EFI_IPv6_ADDRESS                *DnsServerList;
  BOOLEAN                         IsDone;

  Dns6                = NULL;
  Dns6Handle          = NULL;
  DnsServerList       = Private->DnsServer;
  ZeroMem (&Token, sizeof (EFI_DNS6_COMPLETION_TOKEN));

  //
  // Create a DNSv6 child instance and get the protocol.
  //
  Status = NetLibCreateServiceChild (
             Private->Controller,
             Private->Image,
             &gEfiDns6ServiceBindingProtocolGuid,
             &Dns6Handle
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  Dns6Handle,
                  &gEfiDns6ProtocolGuid,
                  (VOID **) &Dns6,
                  Private->Image,
                  Private->Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Configure DNS6 instance for the DNS server address and protocol.
  //
  ZeroMem (&Dns6ConfigData, sizeof (EFI_DNS6_CONFIG_DATA));
  Dns6ConfigData.DnsServerCount = 1;
  Dns6ConfigData.DnsServerList  = DnsServerList;
  Dns6ConfigData.EnableDnsCache = TRUE;
  Dns6ConfigData.Protocol       = EFI_IP_PROTO_UDP;
  IP6_COPY_ADDRESS (&Dns6ConfigData.StationIp, &Private->TmpStationIp.v6);
  Status = Dns6->Configure (
                   Dns6,
                   &Dns6ConfigData
                   );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Token.Status = EFI_NOT_READY;
  IsDone       = FALSE;
  //
  // Create event to set the  IsDone flag when name resolution is finished.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PxeBcCommonNotify,
                  &IsDone,
                  &Token.Event
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Start asynchronous name resolution.
  //
  Status = Dns6->HostNameToIp (Dns6, HostName, &Token);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  while (!IsDone) {
    Dns6->Poll (Dns6);
  }

  //
  // Name resolution is done, check result.
  //
  Status = Token.Status;
  if (!EFI_ERROR (Status)) {
    if (Token.RspData.H2AData == NULL) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }
    if (Token.RspData.H2AData->IpCount == 0 || Token.RspData.H2AData->IpList == NULL) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }
    //
    // We just return the first IPv6 address from DNS protocol.
    //
    IP6_COPY_ADDRESS (IpAddress, Token.RspData.H2AData->IpList);
    Status = EFI_SUCCESS;
  }

Exit:
  FreePool (HostName);

  if (Token.Event != NULL) {
    gBS->CloseEvent (Token.Event);
  }
  if (Token.RspData.H2AData != NULL) {
    if (Token.RspData.H2AData->IpList != NULL) {
      FreePool (Token.RspData.H2AData->IpList);
    }
    FreePool (Token.RspData.H2AData);
  }

  if (Dns6 != NULL) {
    Dns6->Configure (Dns6, NULL);

    gBS->CloseProtocol (
           Dns6Handle,
           &gEfiDns6ProtocolGuid,
           Private->Image,
           Private->Controller
           );
  }

  if (Dns6Handle != NULL) {
    NetLibDestroyServiceChild (
      Private->Controller,
      Private->Image,
      &gEfiDns6ServiceBindingProtocolGuid,
      Dns6Handle
      );
  }

  if (DnsServerList != NULL) {
    FreePool (DnsServerList);
  }

  return Status;
}

/**
  Parse the Boot File URL option.

  @param[in]      Private      Pointer to PxeBc private data.
  @param[out]     FileName     The pointer to the boot file name.
  @param[in, out] SrvAddr      The pointer to the boot server address.
  @param[in]      BootFile     The pointer to the boot file URL option data.
  @param[in]      Length       The length of the boot file URL option data.

  @retval EFI_ABORTED     User cancel operation.
  @retval EFI_SUCCESS     Selected the boot menu successfully.
  @retval EFI_NOT_READY   Read the input key from the keybroad has not finish.

**/
EFI_STATUS
PxeBcExtractBootFileUrl (
  IN PXEBC_PRIVATE_DATA      *Private,
     OUT UINT8               **FileName,
  IN OUT EFI_IPv6_ADDRESS    *SrvAddr,
  IN     CHAR8               *BootFile,
  IN     UINT16              Length
  )
{
  UINT16                     PrefixLen;
  CHAR8                      *BootFileNamePtr;
  CHAR8                      *BootFileName;
  UINT16                     BootFileNameLen;
  CHAR8                      *TmpStr;
  CHAR8                      TmpChar;
  CHAR8                      *ServerAddressOption;
  CHAR8                      *ServerAddress;
  CHAR8                      *ModeStr;
  CHAR16                     *HostName;
  BOOLEAN                    IpExpressedUrl;
  UINTN                      Len;
  EFI_STATUS                 Status;

  IpExpressedUrl = TRUE;
  //
  // The format of the Boot File URL option is:
  //
  //  0                   1                   2                   3
  //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  // |       OPT_BOOTFILE_URL        |            option-len         |
  // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  // |                                                               |
  // .                  bootfile-url  (variable length)              .
  // |                                                               |
  // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  //
  // Based upon RFC 5970 and UEFI 2.6, bootfile-url format can be
  // tftp://[SERVER_ADDRESS]/BOOTFILE_NAME or tftp://domain_name/BOOTFILE_NAME
  // As an example where the BOOTFILE_NAME is the EFI loader and
  // SERVER_ADDRESS is the ASCII encoding of an IPV6 address.
  //
  PrefixLen = (UINT16) AsciiStrLen (PXEBC_DHCP6_BOOT_FILE_URL_PREFIX);

  if (Length <= PrefixLen ||
      CompareMem (BootFile, PXEBC_DHCP6_BOOT_FILE_URL_PREFIX, PrefixLen) != 0) {
    return EFI_NOT_FOUND;
  }

  BootFile = BootFile + PrefixLen;
  Length   = (UINT16) (Length - PrefixLen);

  TmpStr = (CHAR8 *) AllocateZeroPool (Length + 1);
  if (TmpStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TmpStr, BootFile, Length);
  TmpStr[Length] = '\0';

  //
  // Get the part of SERVER_ADDRESS string.
  //
  ServerAddressOption = TmpStr;
  if (*ServerAddressOption == PXEBC_ADDR_START_DELIMITER) {
    ServerAddressOption ++;
    ServerAddress = ServerAddressOption;
    while (*ServerAddress != '\0' && *ServerAddress != PXEBC_ADDR_END_DELIMITER) {
      ServerAddress++;
    }

    if (*ServerAddress != PXEBC_ADDR_END_DELIMITER) {
      FreePool (TmpStr);
      return EFI_INVALID_PARAMETER;
    }

    *ServerAddress = '\0';

    //
    // Convert the string of server address to Ipv6 address format and store it.
    //
    Status = NetLibAsciiStrToIp6 (ServerAddressOption, SrvAddr);
    if (EFI_ERROR (Status)) {
      FreePool (TmpStr);
      return Status;
    }

  } else {
    IpExpressedUrl = FALSE;
    ServerAddress = ServerAddressOption;
    while (*ServerAddress != '\0' && *ServerAddress != PXEBC_TFTP_URL_SEPARATOR) {
      ServerAddress++;
    }

    if (*ServerAddress != PXEBC_TFTP_URL_SEPARATOR) {
      FreePool (TmpStr);
      return EFI_INVALID_PARAMETER;
    }
    *ServerAddress = '\0';

    Len = AsciiStrSize (ServerAddressOption);
    HostName = AllocateZeroPool (Len * sizeof (CHAR16));
    if (HostName == NULL) {
      FreePool (TmpStr);
      return EFI_OUT_OF_RESOURCES;
    }
    AsciiStrToUnicodeStrS (
      ServerAddressOption,
      HostName,
      Len
      );

    //
    // Perform DNS resolution.
    //
    Status = PxeBcDns6 (Private,HostName, SrvAddr);
    if (EFI_ERROR (Status)) {
      FreePool (TmpStr);
      return Status;
    }
  }

  //
  // Get the part of BOOTFILE_NAME string.
  //
  BootFileNamePtr = (CHAR8*)((UINTN)ServerAddress + 1);
  if (IpExpressedUrl) {
    if (*BootFileNamePtr != PXEBC_TFTP_URL_SEPARATOR) {
      FreePool (TmpStr);
      return EFI_INVALID_PARAMETER;
    }
    ++BootFileNamePtr;
  }

  BootFileNameLen = (UINT16)(Length - (UINT16) ((UINTN)BootFileNamePtr - (UINTN)TmpStr) + 1);
  if (BootFileNameLen != 0 || FileName != NULL) {
    //
    // Remove trailing mode=octet if present and ignore.  All other modes are
    // invalid for netboot6, so reject them.
    //
    ModeStr = AsciiStrStr (BootFileNamePtr, ";mode=octet");
    if (ModeStr != NULL && *(ModeStr + AsciiStrLen (";mode=octet")) == '\0') {
      *ModeStr = '\0';
    } else if (AsciiStrStr (BootFileNamePtr, ";mode=") != NULL) {
      FreePool (TmpStr);
      return EFI_INVALID_PARAMETER;
    }

    //
    // Extract boot file name from URL.
    //
    BootFileName = (CHAR8 *) AllocateZeroPool (BootFileNameLen);
    if (BootFileName == NULL) {
      FreePool (TmpStr);
      return EFI_OUT_OF_RESOURCES;
    }
    *FileName = (UINT8*) BootFileName;

    //
    // Decode percent-encoding in boot file name.
    //
    while (*BootFileNamePtr != '\0') {
      if (*BootFileNamePtr == '%') {
        TmpChar = *(BootFileNamePtr+ 3);
        *(BootFileNamePtr+ 3) = '\0';
        *BootFileName = (UINT8) AsciiStrHexToUintn ((CHAR8*)(BootFileNamePtr + 1));
        BootFileName++;
        *(BootFileNamePtr+ 3) = TmpChar;
        BootFileNamePtr += 3;
      } else {
        *BootFileName = *BootFileNamePtr;
        BootFileName++;
        BootFileNamePtr++;
      }
    }
    *BootFileName = '\0';
  }

  FreePool (TmpStr);

  return EFI_SUCCESS;
}


/**
  Parse the Boot File Parameter option.

  @param[in]  BootFilePara      The pointer to boot file parameter option data.
  @param[out] BootFileSize      The pointer to the parsed boot file size.

  @retval EFI_SUCCESS     Successfully obtained the boot file size from parameter option.
  @retval EFI_NOT_FOUND   Failed to extract the boot file size from parameter option.

**/
EFI_STATUS
PxeBcExtractBootFileParam (
  IN  CHAR8                  *BootFilePara,
  OUT UINT16                 *BootFileSize
  )
{
  UINT16                     Length;
  UINT8                      Index;
  UINT8                      Digit;
  UINT32                     Size;

  CopyMem (&Length, BootFilePara, sizeof (UINT16));
  Length = NTOHS (Length);

  //
  // The BootFile Size should be 1~5 byte ASCII strings
  //
  if (Length < 1 || Length > 5) {
    return EFI_NOT_FOUND;
  }

  //
  // Extract the value of BootFile Size.
  //
  BootFilePara = BootFilePara + sizeof (UINT16);
  Size         = 0;
  for (Index = 0; Index < Length; Index++) {
    if (EFI_ERROR (PxeBcUniHexToUint8 (&Digit, *(BootFilePara + Index)))) {
      return EFI_NOT_FOUND;
    }

    Size = (Size + Digit) * 10;
  }

  Size = Size / 10;
  if (Size > PXEBC_DHCP6_MAX_BOOT_FILE_SIZE) {
    return EFI_NOT_FOUND;
  }

  *BootFileSize = (UINT16) Size;
  return EFI_SUCCESS;
}


/**
  Parse the cached DHCPv6 packet, including all the options.

  @param[in]  Cache6           The pointer to a cached DHCPv6 packet.

  @retval     EFI_SUCCESS      Parsed the DHCPv6 packet successfully.
  @retval     EFI_DEVICE_ERROR Failed to parse and invalid the packet.

**/
EFI_STATUS
PxeBcParseDhcp6Packet (
  IN PXEBC_DHCP6_PACKET_CACHE  *Cache6
  )
{
  EFI_DHCP6_PACKET             *Offer;
  EFI_DHCP6_PACKET_OPTION      **Options;
  EFI_DHCP6_PACKET_OPTION      *Option;
  PXEBC_OFFER_TYPE             OfferType;
  BOOLEAN                      IsProxyOffer;
  BOOLEAN                      IsPxeOffer;
  UINT32                       Offset;
  UINT32                       Length;
  UINT32                       EnterpriseNum;

  IsProxyOffer = TRUE;
  IsPxeOffer   = FALSE;
  Offer        = &Cache6->Packet.Offer;
  Options      = Cache6->OptList;

  ZeroMem (Cache6->OptList, sizeof (Cache6->OptList));

  Option  = (EFI_DHCP6_PACKET_OPTION *) (Offer->Dhcp6.Option);
  Offset  = 0;
  Length  = GET_DHCP6_OPTION_SIZE (Offer);

  //
  // OpLen and OpCode here are both stored in network order, since they are from original packet.
  //
  while (Offset < Length) {

    if (NTOHS (Option->OpCode) == DHCP6_OPT_IA_NA) {
      Options[PXEBC_DHCP6_IDX_IA_NA] = Option;
    } else if (NTOHS (Option->OpCode) == DHCP6_OPT_BOOT_FILE_URL) {
      //
      // The server sends this option to inform the client about an URL to a boot file.
      //
      Options[PXEBC_DHCP6_IDX_BOOT_FILE_URL] = Option;
    } else if (NTOHS (Option->OpCode) == DHCP6_OPT_BOOT_FILE_PARAM) {
      Options[PXEBC_DHCP6_IDX_BOOT_FILE_PARAM] = Option;
    } else if (NTOHS (Option->OpCode) == DHCP6_OPT_VENDOR_CLASS) {
      Options[PXEBC_DHCP6_IDX_VENDOR_CLASS] = Option;
    } else if (NTOHS (Option->OpCode) == DHCP6_OPT_DNS_SERVERS) {
      Options[PXEBC_DHCP6_IDX_DNS_SERVER] = Option;
    }

    Offset += (NTOHS (Option->OpLen) + 4);
    Option  = (EFI_DHCP6_PACKET_OPTION *) (Offer->Dhcp6.Option + Offset);
  }

  //
  // The offer with assigned client address is NOT a proxy offer.
  // An ia_na option, embeded with valid ia_addr option and a status_code of success.
  //
  Option = Options[PXEBC_DHCP6_IDX_IA_NA];
  if (Option != NULL) {
    Option = PxeBcParseDhcp6Options (
               Option->Data + 12,
               NTOHS (Option->OpLen),
               DHCP6_OPT_STATUS_CODE
               );
    if ((Option != NULL && Option->Data[0] == 0) || (Option == NULL)) {
      IsProxyOffer = FALSE;
    }
  }

  //
  // The offer with "PXEClient" is a pxe offer.
  //
  Option        = Options[PXEBC_DHCP6_IDX_VENDOR_CLASS];
  EnterpriseNum = HTONL(PXEBC_DHCP6_ENTERPRISE_NUM);

  if (Option != NULL &&
      NTOHS(Option->OpLen) >= 13 &&
      CompareMem (Option->Data, &EnterpriseNum, sizeof (UINT32)) == 0 &&
      CompareMem (&Option->Data[6], DEFAULT_CLASS_ID_DATA, 9) == 0) {
    IsPxeOffer = TRUE;
  }

  //
  // Determine offer type of the dhcp6 packet.
  //
  if (IsPxeOffer) {
    //
    // It's a binl offer only with PXEClient.
    //
    OfferType = IsProxyOffer ? PxeOfferTypeProxyBinl : PxeOfferTypeDhcpBinl;
  } else {
    //
    // It's a dhcp only offer, which is a pure dhcp6 offer packet.
    //
    OfferType = PxeOfferTypeDhcpOnly;
  }

  Cache6->OfferType = OfferType;

  return EFI_SUCCESS;
}


/**
  Cache the DHCPv6 ack packet, and parse it on demand.

  @param[in]  Private             The pointer to PxeBc private data.
  @param[in]  Ack                 The pointer to the DHCPv6 ack packet.
  @param[in]  Verified            If TRUE, parse the ACK packet and store info into mode data.

  @retval     EFI_SUCCESS                Cache and parse the packet successfully.
  @retval     EFI_BUFFER_TOO_SMALL       Cache buffer is not big enough to hold the packet.

**/
EFI_STATUS
PxeBcCopyDhcp6Ack (
  IN PXEBC_PRIVATE_DATA   *Private,
  IN EFI_DHCP6_PACKET     *Ack,
  IN BOOLEAN              Verified
  )
{
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_STATUS              Status;

  Mode = Private->PxeBc.Mode;

  Status = PxeBcCacheDhcp6Packet (&Private->DhcpAck.Dhcp6.Packet.Ack, Ack);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Verified) {
    //
    // Parse the ack packet and store it into mode data if needed.
    //
    PxeBcParseDhcp6Packet (&Private->DhcpAck.Dhcp6);
    CopyMem (&Mode->DhcpAck.Dhcpv6, &Ack->Dhcp6, Ack->Length);
    Mode->DhcpAckReceived = TRUE;
  }

  return EFI_SUCCESS;
}


/**
  Cache the DHCPv6 proxy offer packet according to the received order.

  @param[in]  Private               The pointer to PxeBc private data.
  @param[in]  OfferIndex            The received order of offer packets.

  @retval     EFI_SUCCESS                Cache and parse the packet successfully.
  @retval     EFI_BUFFER_TOO_SMALL       Cache buffer is not big enough to hold the packet.

**/
EFI_STATUS
PxeBcCopyDhcp6Proxy (
  IN PXEBC_PRIVATE_DATA     *Private,
  IN UINT32                 OfferIndex
  )
{
  EFI_PXE_BASE_CODE_MODE    *Mode;
  EFI_DHCP6_PACKET          *Offer;
  EFI_STATUS              Status;

  ASSERT (OfferIndex < Private->OfferNum);
  ASSERT (OfferIndex < PXEBC_OFFER_MAX_NUM);

  Mode  = Private->PxeBc.Mode;
  Offer = &Private->OfferBuffer[OfferIndex].Dhcp6.Packet.Offer;

  //
  // Cache the proxy offer packet and parse it.
  //
  Status = PxeBcCacheDhcp6Packet (&Private->ProxyOffer.Dhcp6.Packet.Offer, Offer);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  PxeBcParseDhcp6Packet (&Private->ProxyOffer.Dhcp6);

  //
  // Store this packet into mode data.
  //
  CopyMem (&Mode->ProxyOffer.Dhcpv6, &Offer->Dhcp6, Offer->Length);
  Mode->ProxyOfferReceived = TRUE;

  return EFI_SUCCESS;
}

/**
  Seek the address of the first byte of the option header.

  @param[in]  Buf           The pointer to the buffer.
  @param[in]  SeekLen       The length to seek.
  @param[in]  OptType       The option type.

  @retval     NULL          If it failed to seek the option.
  @retval     others        The position to the option.

**/
UINT8 *
PxeBcDhcp6SeekOption (
  IN UINT8           *Buf,
  IN UINT32          SeekLen,
  IN UINT16          OptType
  )
{
  UINT8              *Cursor;
  UINT8              *Option;
  UINT16             DataLen;
  UINT16             OpCode;

  Option = NULL;
  Cursor = Buf;

  while (Cursor < Buf + SeekLen) {
    OpCode = ReadUnaligned16 ((UINT16 *) Cursor);
    if (OpCode == HTONS (OptType)) {
      Option = Cursor;
      break;
    }
    DataLen = NTOHS (ReadUnaligned16 ((UINT16 *) (Cursor + 2)));
    Cursor += (DataLen + 4);
  }

  return Option;
}


/**
  Build and send out the request packet for the bootfile, and parse the reply.

  @param[in]  Private               The pointer to PxeBc private data.
  @param[in]  Index                 PxeBc option boot item type.

  @retval     EFI_SUCCESS           Successfully discovered the boot file.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate resources.
  @retval     EFI_NOT_FOUND         Can't get the PXE reply packet.
  @retval     Others                Failed to discover the boot file.

**/
EFI_STATUS
PxeBcRequestBootService (
  IN  PXEBC_PRIVATE_DATA              *Private,
  IN  UINT32                          Index
  )
{
  EFI_PXE_BASE_CODE_UDP_PORT          SrcPort;
  EFI_PXE_BASE_CODE_UDP_PORT          DestPort;
  EFI_PXE_BASE_CODE_PROTOCOL          *PxeBc;
  EFI_PXE_BASE_CODE_DHCPV6_PACKET     *Discover;
  UINTN                               DiscoverLen;
  EFI_DHCP6_PACKET                    *Request;
  UINTN                               RequestLen;
  EFI_DHCP6_PACKET                    *Reply;
  UINT8                               *RequestOpt;
  UINT8                               *DiscoverOpt;
  UINTN                               ReadSize;
  UINT16                              OpFlags;
  UINT16                              OpCode;
  UINT16                              OpLen;
  EFI_STATUS                          Status;
  EFI_DHCP6_PACKET                    *IndexOffer;
  UINT8                               *Option;

  PxeBc       = &Private->PxeBc;
  Request     = Private->Dhcp6Request;
  IndexOffer  = &Private->OfferBuffer[Index].Dhcp6.Packet.Offer;
  SrcPort     = PXEBC_BS_DISCOVER_PORT;
  DestPort    = PXEBC_BS_DISCOVER_PORT;
  OpFlags     = 0;

  if (Request == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Discover = AllocateZeroPool (sizeof (EFI_PXE_BASE_CODE_DHCPV6_PACKET));
  if (Discover == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Build the request packet by the cached request packet before.
  //
  Discover->TransactionId = IndexOffer->Dhcp6.Header.TransactionId;
  Discover->MessageType   = Request->Dhcp6.Header.MessageType;
  RequestOpt              = Request->Dhcp6.Option;
  DiscoverOpt             = Discover->DhcpOptions;
  DiscoverLen             = sizeof (EFI_DHCP6_HEADER);
  RequestLen              = DiscoverLen;

  //
  // Find Server ID Option from ProxyOffer.
  //
  if (Private->OfferBuffer[Index].Dhcp6.OfferType == PxeOfferTypeProxyBinl) {
    Option = PxeBcDhcp6SeekOption (
               IndexOffer->Dhcp6.Option,
               IndexOffer->Length - 4,
               DHCP6_OPT_SERVER_ID
               );
    if (Option == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // Add Server ID Option.
    //
    OpLen = NTOHS (((EFI_DHCP6_PACKET_OPTION *) Option)->OpLen);
    CopyMem (DiscoverOpt, Option, OpLen + 4);
    DiscoverOpt += (OpLen + 4);
    DiscoverLen += (OpLen + 4);
  }

  while (RequestLen < Request->Length) {
    OpCode = NTOHS (((EFI_DHCP6_PACKET_OPTION *) RequestOpt)->OpCode);
    OpLen  = NTOHS (((EFI_DHCP6_PACKET_OPTION *) RequestOpt)->OpLen);
    if (OpCode != EFI_DHCP6_IA_TYPE_NA &&
        OpCode != EFI_DHCP6_IA_TYPE_TA &&
        OpCode != DHCP6_OPT_SERVER_ID
        ) {
      //
      // Copy all the options except IA option and Server ID
      //
      CopyMem (DiscoverOpt, RequestOpt, OpLen + 4);
      DiscoverOpt += (OpLen + 4);
      DiscoverLen += (OpLen + 4);
    }
    RequestOpt += (OpLen + 4);
    RequestLen += (OpLen + 4);
  }

  //
  // Update Elapsed option in the package
  //
  Option = PxeBcDhcp6SeekOption (
             Discover->DhcpOptions,
             (UINT32)(RequestLen - 4),
             DHCP6_OPT_ELAPSED_TIME
             );
  if (Option != NULL) {
    CalcElapsedTime (Private);
    WriteUnaligned16 ((UINT16*)(Option + 4), HTONS((UINT16) Private->ElapsedTime));
  }

  Status = PxeBc->UdpWrite (
                    PxeBc,
                    OpFlags,
                    &Private->ServerIp,
                    &DestPort,
                    NULL,
                    &Private->StationIp,
                    &SrcPort,
                    NULL,
                    NULL,
                    &DiscoverLen,
                    (VOID *) Discover
                    );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Cache the right PXE reply packet here, set valid flag later.
  // Especially for PXE discover packet, store it into mode data here.
  //
  Reply = &Private->ProxyOffer.Dhcp6.Packet.Offer;
  ReadSize = (UINTN) Reply->Size;

  //
  // Start Udp6Read instance
  //
  Status = Private->Udp6Read->Configure (Private->Udp6Read, &Private->Udp6CfgData);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = PxeBc->UdpRead (
                    PxeBc,
                    EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP | EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP,
                    NULL,
                    &SrcPort,
                    &Private->ServerIp,
                    &DestPort,
                    NULL,
                    NULL,
                    &ReadSize,
                    (VOID *) &Reply->Dhcp6
                    );
  //
  // Stop Udp6Read instance
  //
  Private->Udp6Read->Configure (Private->Udp6Read, NULL);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Update length
  //
  Reply->Length = (UINT32) ReadSize;

  return EFI_SUCCESS;

ON_ERROR:
  if (Discover != NULL) {
    FreePool (Discover);
  }

  return Status;
}


/**
  Retry to request bootfile name by the BINL offer.

  @param[in]  Private              The pointer to PxeBc private data.
  @param[in]  Index                The received order of offer packets.

  @retval     EFI_SUCCESS          Successfully retried a request for the bootfile name.
  @retval     EFI_DEVICE_ERROR     Failed to retry the bootfile name.

**/
EFI_STATUS
PxeBcRetryDhcp6Binl (
  IN PXEBC_PRIVATE_DATA  *Private,
  IN UINT32              Index
  )
{
  EFI_PXE_BASE_CODE_MODE    *Mode;
  PXEBC_DHCP6_PACKET_CACHE  *Offer;
  PXEBC_DHCP6_PACKET_CACHE  *Cache6;
  EFI_STATUS                Status;

  ASSERT (Index < PXEBC_OFFER_MAX_NUM);
  ASSERT (Private->OfferBuffer[Index].Dhcp6.OfferType == PxeOfferTypeDhcpBinl ||
          Private->OfferBuffer[Index].Dhcp6.OfferType == PxeOfferTypeProxyBinl);

  Mode                  = Private->PxeBc.Mode;
  Private->IsDoDiscover = FALSE;
  Offer                 = &Private->OfferBuffer[Index].Dhcp6;
  if (Offer->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL] == NULL) {
    //
    // There is no BootFileUrl option in dhcp6 offer, so use servers multi-cast address instead.
    //
    CopyMem (
      &Private->ServerIp.v6,
      &mAllDhcpRelayAndServersAddress,
      sizeof (EFI_IPv6_ADDRESS)
      );
  } else {
    ASSERT (Offer->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL] != NULL);
    //
    // Parse out the next server address from the last offer, and store it
    //
    Status = PxeBcExtractBootFileUrl (
               Private,
               &Private->BootFileName,
               &Private->ServerIp.v6,
               (CHAR8 *) (Offer->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL]->Data),
               NTOHS (Offer->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL]->OpLen)
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Retry Dhcp6Binl again for the bootfile, and the reply cached into Private->ProxyOffer.
  //
  Status = PxeBcRequestBootService (Private, Index);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Cache6 = &Private->ProxyOffer.Dhcp6;
  Status = PxeBcParseDhcp6Packet (Cache6);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Cache6->OfferType != PxeOfferTypeProxyPxe10 &&
      Cache6->OfferType != PxeOfferTypeProxyWfm11a &&
      Cache6->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL] == NULL) {
    //
    // This BINL ack doesn't have discovery option set or multicast option set
    // or bootfile name specified.
    //
    return EFI_DEVICE_ERROR;
  }

  Mode->ProxyOfferReceived = TRUE;
  CopyMem (
    &Mode->ProxyOffer.Dhcpv6,
    &Cache6->Packet.Offer.Dhcp6,
    Cache6->Packet.Offer.Length
    );

  return EFI_SUCCESS;
}


/**
  Cache all the received DHCPv6 offers, and set OfferIndex and OfferCount.

  @param[in]  Private               The pointer to PXEBC_PRIVATE_DATA.
  @param[in]  RcvdOffer             The pointer to the received offer packet.

  @retval     EFI_SUCCESS      Cache and parse the packet successfully.
  @retval     Others           Operation failed.
**/
EFI_STATUS
PxeBcCacheDhcp6Offer (
  IN PXEBC_PRIVATE_DATA     *Private,
  IN EFI_DHCP6_PACKET       *RcvdOffer
  )
{
  PXEBC_DHCP6_PACKET_CACHE  *Cache6;
  EFI_DHCP6_PACKET          *Offer;
  PXEBC_OFFER_TYPE          OfferType;
  EFI_STATUS                Status;

  Cache6 = &Private->OfferBuffer[Private->OfferNum].Dhcp6;
  Offer  = &Cache6->Packet.Offer;

  //
  // Cache the content of DHCPv6 packet firstly.
  //
  Status = PxeBcCacheDhcp6Packet (Offer, RcvdOffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Validate the DHCPv6 packet, and parse the options and offer type.
  //
  if (EFI_ERROR (PxeBcParseDhcp6Packet (Cache6))) {
    return EFI_ABORTED;
  }

  //
  // Determine whether cache the current offer by type, and record OfferIndex and OfferCount.
  //
  OfferType = Cache6->OfferType;
  ASSERT (OfferType < PxeOfferTypeMax);
  ASSERT (Private->OfferCount[OfferType] < PXEBC_OFFER_MAX_NUM);

  if (IS_PROXY_OFFER (OfferType)) {
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
    } else if ((OfferType == PxeOfferTypeProxyPxe10 || OfferType == PxeOfferTypeProxyWfm11a) &&
                 Private->OfferCount[OfferType] < 1) {
      //
      // Only cache the first PXE10/WFM11a offer, and discard the others.
      //
      Private->OfferIndex[OfferType][0] = Private->OfferNum;
      Private->OfferCount[OfferType]    = 1;
    } else {
      return EFI_ABORTED;
    }
  } else {
    //
    // It's a DHCPv6 offer with yiaddr, and cache them all.
    //
    Private->OfferIndex[OfferType][Private->OfferCount[OfferType]] = Private->OfferNum;
    Private->OfferCount[OfferType]++;
  }

  Private->OfferNum++;

  return EFI_SUCCESS;
}


/**
  Select an DHCPv6 offer, and record SelectIndex and SelectProxyType.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

**/
VOID
PxeBcSelectDhcp6Offer (
  IN PXEBC_PRIVATE_DATA     *Private
  )
{
  UINT32                Index;
  UINT32                OfferIndex;
  PXEBC_OFFER_TYPE      OfferType;

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
        if (Private->OfferBuffer[OfferIndex].Dhcp6.OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL] != NULL) {
          Private->SelectIndex = OfferIndex + 1;
          break;
        }
      }
    }
  } else {
    //
    // Select offer by received order.
    //
    for (Index = 0; Index < Private->OfferNum; Index++) {

      OfferType = Private->OfferBuffer[Index].Dhcp6.OfferType;

      if (IS_PROXY_OFFER (OfferType)) {
        //
        // Skip proxy offers
        //
        continue;
      }

      if (!Private->IsProxyRecved &&
          OfferType == PxeOfferTypeDhcpOnly &&
          Private->OfferBuffer[Index].Dhcp6.OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL] == NULL) {
        //
        // Skip if DhcpOnly offer without any other proxy offers or bootfilename.
        //
        continue;
      }

      Private->SelectIndex = Index + 1;
      break;
    }
  }
}


/**
  Handle the DHCPv6 offer packet.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

  @retval     EFI_SUCCESS           Handled the DHCPv6 offer packet successfully.
  @retval     EFI_NO_RESPONSE       No response to the following request packet.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate resources.
  @retval     EFI_BUFFER_TOO_SMALL  Can't cache the offer pacet.

**/
EFI_STATUS
PxeBcHandleDhcp6Offer (
  IN PXEBC_PRIVATE_DATA            *Private
  )
{
  PXEBC_DHCP6_PACKET_CACHE         *Cache6;
  EFI_STATUS                       Status;
  PXEBC_OFFER_TYPE                 OfferType;
  UINT32                           ProxyIndex;
  UINT32                           SelectIndex;
  UINT32                           Index;

  ASSERT (Private->SelectIndex > 0);
  SelectIndex = (UINT32) (Private->SelectIndex - 1);
  ASSERT (SelectIndex < PXEBC_OFFER_MAX_NUM);
  Cache6      = &Private->OfferBuffer[SelectIndex].Dhcp6;
  Status      = EFI_SUCCESS;

  //
  // First try to cache DNS server address if DHCP6 offer provides.
  //
  if (Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER] != NULL) {
    Private->DnsServer = AllocateZeroPool (NTOHS (Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpLen));
    if (Private->DnsServer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (Private->DnsServer, Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->Data, sizeof (EFI_IPv6_ADDRESS));
  }

  if (Cache6->OfferType == PxeOfferTypeDhcpBinl) {
    //
    // DhcpBinl offer is selected, so need try to request bootfilename by this offer.
    //
    if (EFI_ERROR (PxeBcRetryDhcp6Binl (Private, SelectIndex))) {
      Status = EFI_NO_RESPONSE;
    }
  } else if (Cache6->OfferType == PxeOfferTypeDhcpOnly) {

    if (Private->IsProxyRecved) {
      //
      // DhcpOnly offer is selected, so need try to request bootfilename.
      //
      ProxyIndex = 0;
      if (Private->IsOfferSorted) {
        //
        // The proxy offer should be determined if select by default policy.
        // IsOfferSorted means all offers are labeled by OfferIndex.
        //
        ASSERT (Private->OfferCount[Private->SelectProxyType] > 0);

        if (Private->SelectProxyType == PxeOfferTypeProxyBinl) {
          //
          // Try all the cached ProxyBinl offer one by one to request bootfilename.
          //
          for (Index = 0; Index < Private->OfferCount[Private->SelectProxyType]; Index++) {

            ProxyIndex = Private->OfferIndex[Private->SelectProxyType][Index];
            if (!EFI_ERROR (PxeBcRetryDhcp6Binl (Private, ProxyIndex))) {
              break;
            }
          }
          if (Index == Private->OfferCount[Private->SelectProxyType]) {
            Status = EFI_NO_RESPONSE;
          }
        } else {
          //
          // For other proxy offers (pxe10 or wfm11a), only one is buffered.
          //
          ProxyIndex = Private->OfferIndex[Private->SelectProxyType][0];
        }
      } else {
        //
        // The proxy offer should not be determined if select by received order.
        //
        Status = EFI_NO_RESPONSE;

        for (Index = 0; Index < Private->OfferNum; Index++) {

          OfferType = Private->OfferBuffer[Index].Dhcp6.OfferType;

          if (!IS_PROXY_OFFER (OfferType)) {
            //
            // Skip non proxy dhcp offers.
            //
            continue;
          }

          if (OfferType == PxeOfferTypeProxyBinl) {
            //
            // Try all the cached ProxyBinl offer one by one to request bootfilename.
            //
            if (EFI_ERROR (PxeBcRetryDhcp6Binl (Private, Index))) {
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
        Status = PxeBcCopyDhcp6Proxy (Private, ProxyIndex);
      }
    } else {
      //
      //  Othewise, the bootfilename must be included in DhcpOnly offer.
      //
      ASSERT (Cache6->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL] != NULL);
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // All PXE boot information is ready by now.
    //
    Status = PxeBcCopyDhcp6Ack (Private, &Private->DhcpAck.Dhcp6.Packet.Ack, TRUE);
    Private->PxeBc.Mode->DhcpDiscoverValid = TRUE;
  }

  return Status;
}


/**
  Unregister the address by Ip6Config protocol.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

**/
VOID
PxeBcUnregisterIp6Address (
  IN PXEBC_PRIVATE_DATA           *Private
  )
{
  if (Private->Ip6Policy != PXEBC_IP6_POLICY_MAX) {
    //
    // PXE driver change the policy of IP6 driver, it's a chance to recover.
    // Keep the point and there is no enough requirements to do recovery.
    //
  }
}

/**
  Check whether IP driver could route the message which will be sent to ServerIp address.

  This function will check the IP6 route table every 1 seconds until specified timeout is expired, if a valid
  route is found in IP6 route table, the address will be filed in GatewayAddr and return.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.
  @param[in]  TimeOutInSecond     Timeout value in seconds.
  @param[out] GatewayAddr         Pointer to store the gateway IP address.

  @retval     EFI_SUCCESS         Found a valid gateway address successfully.
  @retval     EFI_TIMEOUT         The operation is time out.
  @retval     Other               Unexpect error happened.

**/
EFI_STATUS
PxeBcCheckRouteTable (
  IN  PXEBC_PRIVATE_DATA            *Private,
  IN  UINTN                         TimeOutInSecond,
  OUT EFI_IPv6_ADDRESS              *GatewayAddr
  )
{
  EFI_STATUS                       Status;
  EFI_IP6_PROTOCOL                 *Ip6;
  EFI_IP6_MODE_DATA                Ip6ModeData;
  UINTN                            Index;
  EFI_EVENT                        TimeOutEvt;
  UINTN                            RetryCount;
  BOOLEAN                          GatewayIsFound;

  ASSERT (GatewayAddr != NULL);
  ASSERT (Private != NULL);

  Ip6            = Private->Ip6;
  GatewayIsFound = FALSE;
  RetryCount     = 0;
  TimeOutEvt     = NULL;
  ZeroMem (GatewayAddr, sizeof (EFI_IPv6_ADDRESS));

  while (TRUE) {
    Status = Ip6->GetModeData (Ip6, &Ip6ModeData, NULL, NULL);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    //
    // Find out the gateway address which can route the message which send to ServerIp.
    //
    for (Index = 0; Index < Ip6ModeData.RouteCount; Index++) {
      if (NetIp6IsNetEqual (&Private->ServerIp.v6, &Ip6ModeData.RouteTable[Index].Destination, Ip6ModeData.RouteTable[Index].PrefixLength)) {
        IP6_COPY_ADDRESS (GatewayAddr, &Ip6ModeData.RouteTable[Index].Gateway);
        GatewayIsFound = TRUE;
        break;
      }
    }

    if (Ip6ModeData.AddressList != NULL) {
      FreePool (Ip6ModeData.AddressList);
    }
    if (Ip6ModeData.GroupTable != NULL) {
      FreePool (Ip6ModeData.GroupTable);
    }
    if (Ip6ModeData.RouteTable != NULL) {
      FreePool (Ip6ModeData.RouteTable);
    }
    if (Ip6ModeData.NeighborCache != NULL) {
      FreePool (Ip6ModeData.NeighborCache);
    }
    if (Ip6ModeData.PrefixTable != NULL) {
      FreePool (Ip6ModeData.PrefixTable);
    }
    if (Ip6ModeData.IcmpTypeList != NULL) {
      FreePool (Ip6ModeData.IcmpTypeList);
    }

    if (GatewayIsFound || RetryCount == TimeOutInSecond) {
      break;
    }

    RetryCount++;

    //
    // Delay 1 second then recheck it again.
    //
    if (TimeOutEvt == NULL) {
      Status = gBS->CreateEvent (
                      EVT_TIMER,
                      TPL_CALLBACK,
                      NULL,
                      NULL,
                      &TimeOutEvt
                      );
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }
    }

    Status = gBS->SetTimer (TimeOutEvt, TimerRelative, TICKS_PER_SECOND);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
    while (EFI_ERROR (gBS->CheckEvent (TimeOutEvt))) {
      Ip6->Poll (Ip6);
    }
  }

ON_EXIT:
  if (TimeOutEvt != NULL) {
    gBS->CloseEvent (TimeOutEvt);
  }

  if (GatewayIsFound) {
    Status = EFI_SUCCESS;
  } else if (RetryCount == TimeOutInSecond) {
    Status = EFI_TIMEOUT;
  }

  return Status;
}

/**
  Register the ready station address and gateway by Ip6Config protocol.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.
  @param[in]  Address             The pointer to the ready address.

  @retval     EFI_SUCCESS         Registered the address succesfully.
  @retval     Others              Failed to register the address.

**/
EFI_STATUS
PxeBcRegisterIp6Address (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_IPv6_ADDRESS              *Address
  )
{
  EFI_IP6_PROTOCOL                 *Ip6;
  EFI_IP6_CONFIG_PROTOCOL          *Ip6Cfg;
  EFI_IP6_CONFIG_POLICY            Policy;
  EFI_IP6_CONFIG_MANUAL_ADDRESS    CfgAddr;
  EFI_IPv6_ADDRESS                 GatewayAddr;
  UINTN                            DataSize;
  EFI_EVENT                        MappedEvt;
  EFI_STATUS                       Status;
  BOOLEAN                          NoGateway;
  EFI_IPv6_ADDRESS                 *Ip6Addr;
  UINTN                            Index;

  Status     = EFI_SUCCESS;
  MappedEvt  = NULL;
  Ip6Addr    = NULL;
  DataSize   = sizeof (EFI_IP6_CONFIG_POLICY);
  Ip6Cfg     = Private->Ip6Cfg;
  Ip6        = Private->Ip6;
  NoGateway  = FALSE;

  ZeroMem (&CfgAddr, sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS));
  CopyMem (&CfgAddr.Address, Address, sizeof (EFI_IPv6_ADDRESS));

  Status = Ip6->Configure (Ip6, &Private->Ip6CfgData);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Retrieve the gateway address from IP6 route table.
  //
  Status = PxeBcCheckRouteTable (Private, PXEBC_IP6_ROUTE_TABLE_TIMEOUT, &GatewayAddr);
  if (EFI_ERROR (Status)) {
    NoGateway = TRUE;
  }

  //
  // There is no channel between IP6 and PXE driver about address setting,
  // so it has to set the new address by Ip6ConfigProtocol manually.
  //
  Policy = Ip6ConfigPolicyManual;
  Status = Ip6Cfg->SetData (
                     Ip6Cfg,
                     Ip6ConfigDataTypePolicy,
                     sizeof(EFI_IP6_CONFIG_POLICY),
                     &Policy
                     );
  if (EFI_ERROR (Status)) {
    //
    // There is no need to recover later.
    //
    Private->Ip6Policy = PXEBC_IP6_POLICY_MAX;
    goto ON_EXIT;
  }

  //
  // Create a notify event to set address flag when DAD if IP6 driver succeeded.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PxeBcCommonNotify,
                  &Private->IsAddressOk,
                  &MappedEvt
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Private->IsAddressOk = FALSE;
  Status = Ip6Cfg->RegisterDataNotify (
                     Ip6Cfg,
                     Ip6ConfigDataTypeManualAddress,
                     MappedEvt
                     );
  if (EFI_ERROR(Status)) {
    goto ON_EXIT;
  }

  Status = Ip6Cfg->SetData (
                     Ip6Cfg,
                     Ip6ConfigDataTypeManualAddress,
                     sizeof(EFI_IP6_CONFIG_MANUAL_ADDRESS),
                     &CfgAddr
                     );
  if (EFI_ERROR(Status) && Status != EFI_NOT_READY) {
    goto ON_EXIT;
  } else if (Status == EFI_NOT_READY) {
    //
    // Poll the network until the asynchronous process is finished.
    //
    while (!Private->IsAddressOk) {
      Ip6->Poll (Ip6);
    }
    //
    // Check whether the IP6 address setting is successed.
    //
    DataSize = 0;
    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeManualAddress,
                       &DataSize,
                       NULL
                       );
    if (Status != EFI_BUFFER_TOO_SMALL || DataSize == 0) {
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
    }

    Ip6Addr = AllocatePool (DataSize);
    if (Ip6Addr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeManualAddress,
                       &DataSize,
                       (VOID*) Ip6Addr
                       );
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
    }

    for (Index = 0; Index < DataSize / sizeof (EFI_IPv6_ADDRESS); Index++) {
      if (CompareMem (Ip6Addr + Index, Address, sizeof (EFI_IPv6_ADDRESS)) == 0) {
        break;
      }
    }
    if (Index == DataSize / sizeof (EFI_IPv6_ADDRESS)) {
      Status = EFI_ABORTED;
      goto ON_EXIT;
    }
  }

  //
  // Set the default gateway address back if needed.
  //
  if (!NoGateway && !NetIp6IsUnspecifiedAddr (&GatewayAddr)) {
    Status = Ip6Cfg->SetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeGateway,
                       sizeof (EFI_IPv6_ADDRESS),
                       &GatewayAddr
                       );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

ON_EXIT:
  if (MappedEvt != NULL) {
    Ip6Cfg->UnregisterDataNotify (
              Ip6Cfg,
              Ip6ConfigDataTypeManualAddress,
              MappedEvt
              );
    gBS->CloseEvent (MappedEvt);
  }
  if (Ip6Addr != NULL) {
    FreePool (Ip6Addr);
  }
  return Status;
}

/**
  Set the IP6 policy to Automatic.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

  @retval     EFI_SUCCESS         Switch the IP policy succesfully.
  @retval     Others              Unexpect error happened.

**/
EFI_STATUS
PxeBcSetIp6Policy (
  IN PXEBC_PRIVATE_DATA            *Private
  )
{
  EFI_IP6_CONFIG_POLICY            Policy;
  EFI_STATUS                       Status;
  EFI_IP6_CONFIG_PROTOCOL          *Ip6Cfg;
  UINTN                            DataSize;

  Ip6Cfg      = Private->Ip6Cfg;
  DataSize    = sizeof (EFI_IP6_CONFIG_POLICY);

  //
  // Get and store the current policy of IP6 driver.
  //
  Status = Ip6Cfg->GetData (
                     Ip6Cfg,
                     Ip6ConfigDataTypePolicy,
                     &DataSize,
                     &Private->Ip6Policy
                     );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Private->Ip6Policy == Ip6ConfigPolicyManual) {
    Policy = Ip6ConfigPolicyAutomatic;
    Status = Ip6Cfg->SetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypePolicy,
                       sizeof(EFI_IP6_CONFIG_POLICY),
                       &Policy
                       );
    if (EFI_ERROR (Status)) {
      //
      // There is no need to recover later.
      //
      Private->Ip6Policy = PXEBC_IP6_POLICY_MAX;
    }
  }

  return Status;
}

/**
  This function will register the station IP address and flush IP instance to start using the new IP address.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

  @retval     EFI_SUCCESS         The new IP address has been configured successfully.
  @retval     Others              Failed to configure the address.

**/
EFI_STATUS
PxeBcSetIp6Address (
  IN  PXEBC_PRIVATE_DATA              *Private
  )
{
  EFI_STATUS                  Status;
  EFI_DHCP6_PROTOCOL          *Dhcp6;

  Dhcp6 = Private->Dhcp6;

  CopyMem (&Private->StationIp.v6, &Private->TmpStationIp.v6, sizeof (EFI_IPv6_ADDRESS));
  CopyMem (&Private->PxeBc.Mode->StationIp.v6, &Private->StationIp.v6, sizeof (EFI_IPv6_ADDRESS));

  Status = PxeBcRegisterIp6Address (Private, &Private->StationIp.v6);
  if (EFI_ERROR (Status)) {
    Dhcp6->Stop (Dhcp6);
    return Status;
  }

  Status = PxeBcFlushStationIp (Private, &Private->StationIp, NULL);
  if (EFI_ERROR (Status)) {
    PxeBcUnregisterIp6Address (Private);
    Dhcp6->Stop (Dhcp6);
    return Status;
  }

  AsciiPrint ("\n  Station IP address is ");
  PxeBcShowIp6Addr (&Private->StationIp.v6);

  return EFI_SUCCESS;
}

/**
  EFI_DHCP6_CALLBACK is provided by the consumer of the EFI DHCPv6 Protocol driver
  to intercept events that occurred in the configuration process.

  @param[in]  This              The pointer to the EFI DHCPv6 Protocol.
  @param[in]  Context           The pointer to the context set by EFI_DHCP6_PROTOCOL.Configure().
  @param[in]  CurrentState      The current operational state of the EFI DHCPv Protocol driver.
  @param[in]  Dhcp6Event        The event that occurs in the current state, which usually means a
                                state transition.
  @param[in]  Packet            The DHCPv6 packet that is going to be sent or was already received.
  @param[out] NewPacket         The packet that is used to replace the Packet above.

  @retval EFI_SUCCESS           Told the EFI DHCPv6 Protocol driver to continue the DHCP process.
  @retval EFI_NOT_READY         Only used in the Dhcp6Selecting state. The EFI DHCPv6 Protocol
                                driver will continue to wait for more packets.
  @retval EFI_ABORTED           Told the EFI DHCPv6 Protocol driver to abort the current process.

**/
EFI_STATUS
EFIAPI
PxeBcDhcp6CallBack (
  IN  EFI_DHCP6_PROTOCOL           *This,
  IN  VOID                         *Context,
  IN  EFI_DHCP6_STATE              CurrentState,
  IN  EFI_DHCP6_EVENT              Dhcp6Event,
  IN  EFI_DHCP6_PACKET             *Packet,
  OUT EFI_DHCP6_PACKET             **NewPacket     OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA                  *Private;
  EFI_PXE_BASE_CODE_MODE              *Mode;
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL *Callback;
  EFI_DHCP6_PACKET                    *SelectAd;
  EFI_STATUS                          Status;
  BOOLEAN                             Received;

  if ((Dhcp6Event != Dhcp6RcvdAdvertise) &&
      (Dhcp6Event != Dhcp6SelectAdvertise) &&
      (Dhcp6Event != Dhcp6SendSolicit) &&
      (Dhcp6Event != Dhcp6SendRequest) &&
      (Dhcp6Event != Dhcp6RcvdReply)) {
    return EFI_SUCCESS;
  }

  ASSERT (Packet != NULL);

  Private   = (PXEBC_PRIVATE_DATA *) Context;
  Mode      = Private->PxeBc.Mode;
  Callback  = Private->PxeBcCallback;

  //
  // Callback to user when any traffic ocurred if has.
  //
  if (Dhcp6Event != Dhcp6SelectAdvertise && Callback != NULL) {
    Received = (BOOLEAN) (Dhcp6Event == Dhcp6RcvdAdvertise || Dhcp6Event == Dhcp6RcvdReply);
    Status = Callback->Callback (
                         Callback,
                         Private->Function,
                         Received,
                         Packet->Length,
                         (EFI_PXE_BASE_CODE_PACKET *) &Packet->Dhcp6
                         );
    if (Status != EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE) {
      return EFI_ABORTED;
    }
  }

  Status = EFI_SUCCESS;

  switch (Dhcp6Event) {

  case Dhcp6SendSolicit:
    if (Packet->Length > PXEBC_DHCP6_PACKET_MAX_SIZE) {
      //
      // If the to be sent packet exceeds the maximum length, abort the DHCP process.
      //
      Status = EFI_ABORTED;
      break;
    }

    //
    // Record the first Solicate msg time
    //
    if (Private->SolicitTimes == 0) {
      CalcElapsedTime (Private);
      Private->SolicitTimes++;
    }
    //
    // Cache the dhcp discover packet to mode data directly.
    //
    CopyMem (&Mode->DhcpDiscover.Dhcpv4, &Packet->Dhcp6, Packet->Length);
    break;

  case Dhcp6RcvdAdvertise:
    Status = EFI_NOT_READY;
    if (Packet->Length > PXEBC_DHCP6_PACKET_MAX_SIZE) {
      //
      // Ignore the incoming packets which exceed the maximum length.
      //
      break;
    }
    if (Private->OfferNum < PXEBC_OFFER_MAX_NUM) {
      //
      // Cache the dhcp offers to OfferBuffer[] for select later, and record
      // the OfferIndex and OfferCount.
      //
      PxeBcCacheDhcp6Offer (Private, Packet);
    }
    break;

  case Dhcp6SendRequest:
    if (Packet->Length > PXEBC_DHCP6_PACKET_MAX_SIZE) {
      //
      // If the to be sent packet exceeds the maximum length, abort the DHCP process.
      //
      Status = EFI_ABORTED;
      break;
    }

    //
    // Store the request packet as seed packet for discover.
    //
    if (Private->Dhcp6Request != NULL) {
      FreePool (Private->Dhcp6Request);
    }
    Private->Dhcp6Request = AllocateZeroPool (Packet->Size);
    if (Private->Dhcp6Request != NULL) {
      CopyMem (Private->Dhcp6Request, Packet, Packet->Size);
    }
    break;

  case Dhcp6SelectAdvertise:
    //
    // Select offer by the default policy or by order, and record the SelectIndex
    // and SelectProxyType.
    //
    PxeBcSelectDhcp6Offer (Private);

    if (Private->SelectIndex == 0) {
      Status = EFI_ABORTED;
    } else {
      ASSERT (NewPacket != NULL);
      SelectAd   = &Private->OfferBuffer[Private->SelectIndex - 1].Dhcp6.Packet.Offer;
      *NewPacket = AllocateZeroPool (SelectAd->Size);
      ASSERT (*NewPacket != NULL);
      if (*NewPacket == NULL) {
        return EFI_ABORTED;
      }
      CopyMem (*NewPacket, SelectAd, SelectAd->Size);
    }
    break;

  case Dhcp6RcvdReply:
    //
    // Cache the dhcp ack to Private->Dhcp6Ack, but it's not the final ack in mode data
    // without verification.
    //
    ASSERT (Private->SelectIndex != 0);
    Status = PxeBcCopyDhcp6Ack (Private, Packet, FALSE);
    if (EFI_ERROR (Status)) {
      Status = EFI_ABORTED;
    }
    break;

  default:
    ASSERT (0);
  }

  return Status;
}


/**
  Build and send out the request packet for the bootfile, and parse the reply.

  @param[in]  Private               The pointer to PxeBc private data.
  @param[in]  Type                  PxeBc option boot item type.
  @param[in]  Layer                 The pointer to option boot item layer.
  @param[in]  UseBis                Use BIS or not.
  @param[in]  DestIp                The pointer to the server address.

  @retval     EFI_SUCCESS           Successfully discovered the boot file.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate resources.
  @retval     EFI_NOT_FOUND         Can't get the PXE reply packet.
  @retval     Others                Failed to discover the boot file.

**/
EFI_STATUS
PxeBcDhcp6Discover (
  IN  PXEBC_PRIVATE_DATA              *Private,
  IN  UINT16                          Type,
  IN  UINT16                          *Layer,
  IN  BOOLEAN                         UseBis,
  IN  EFI_IP_ADDRESS                  *DestIp
  )
{
  EFI_PXE_BASE_CODE_UDP_PORT          SrcPort;
  EFI_PXE_BASE_CODE_UDP_PORT          DestPort;
  EFI_PXE_BASE_CODE_MODE              *Mode;
  EFI_PXE_BASE_CODE_PROTOCOL          *PxeBc;
  EFI_PXE_BASE_CODE_DHCPV6_PACKET     *Discover;
  UINTN                               DiscoverLen;
  EFI_DHCP6_PACKET                    *Request;
  UINTN                               RequestLen;
  EFI_DHCP6_PACKET                    *Reply;
  UINT8                               *RequestOpt;
  UINT8                               *DiscoverOpt;
  UINTN                               ReadSize;
  UINT16                              OpCode;
  UINT16                              OpLen;
  UINT32                              Xid;
  EFI_STATUS                          Status;

  PxeBc       = &Private->PxeBc;
  Mode        = PxeBc->Mode;
  Request     = Private->Dhcp6Request;
  SrcPort     = PXEBC_BS_DISCOVER_PORT;
  DestPort    = PXEBC_BS_DISCOVER_PORT;

  if (!UseBis && Layer != NULL) {
    *Layer &= EFI_PXE_BASE_CODE_BOOT_LAYER_MASK;
  }

  if (Request == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Discover = AllocateZeroPool (sizeof (EFI_PXE_BASE_CODE_DHCPV6_PACKET));
  if (Discover == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Build the discover packet by the cached request packet before.
  //
  Xid                     = NET_RANDOM (NetRandomInitSeed ());
  Discover->TransactionId = HTONL (Xid);
  Discover->MessageType   = Request->Dhcp6.Header.MessageType;
  RequestOpt              = Request->Dhcp6.Option;
  DiscoverOpt             = Discover->DhcpOptions;
  DiscoverLen             = sizeof (EFI_DHCP6_HEADER);
  RequestLen              = DiscoverLen;

  while (RequestLen < Request->Length) {
    OpCode = NTOHS (((EFI_DHCP6_PACKET_OPTION *) RequestOpt)->OpCode);
    OpLen  = NTOHS (((EFI_DHCP6_PACKET_OPTION *) RequestOpt)->OpLen);
    if (OpCode != EFI_DHCP6_IA_TYPE_NA &&
        OpCode != EFI_DHCP6_IA_TYPE_TA) {
      //
      // Copy all the options except IA option.
      //
      CopyMem (DiscoverOpt, RequestOpt, OpLen + 4);
      DiscoverOpt += (OpLen + 4);
      DiscoverLen += (OpLen + 4);
    }
    RequestOpt += (OpLen + 4);
    RequestLen += (OpLen + 4);
  }

  Status = PxeBc->UdpWrite (
                    PxeBc,
                    0,
                    &Private->ServerIp,
                    &DestPort,
                    NULL,
                    &Private->StationIp,
                    &SrcPort,
                    NULL,
                    NULL,
                    &DiscoverLen,
                    (VOID *) Discover
                    );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Cache the right PXE reply packet here, set valid flag later.
  // Especially for PXE discover packet, store it into mode data here.
  //
  if (Private->IsDoDiscover) {
    CopyMem (&Mode->PxeDiscover.Dhcpv6, Discover, DiscoverLen);
    Reply = &Private->PxeReply.Dhcp6.Packet.Ack;
  } else {
    Reply = &Private->ProxyOffer.Dhcp6.Packet.Offer;
  }
  ReadSize = (UINTN) Reply->Size;

  //
  // Start Udp6Read instance
  //
  Status = Private->Udp6Read->Configure (Private->Udp6Read, &Private->Udp6CfgData);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = PxeBc->UdpRead (
                    PxeBc,
                    EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP,
                    NULL,
                    &SrcPort,
                    &Private->ServerIp,
                    &DestPort,
                    NULL,
                    NULL,
                    &ReadSize,
                    (VOID *) &Reply->Dhcp6
                    );
  //
  // Stop Udp6Read instance
  //
  Private->Udp6Read->Configure (Private->Udp6Read, NULL);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (Discover != NULL) {
    FreePool (Discover);
  }

  return Status;
}


/**
  Start the DHCPv6 S.A.R.R. process to acquire the IPv6 address and other PXE boot information.

  @param[in]  Private           The pointer to PxeBc private data.
  @param[in]  Dhcp6             The pointer to the EFI_DHCP6_PROTOCOL

  @retval EFI_SUCCESS           The S.A.R.R. process successfully finished.
  @retval Others                Failed to finish the S.A.R.R. process.

**/
EFI_STATUS
PxeBcDhcp6Sarr (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_DHCP6_PROTOCOL            *Dhcp6
  )
{
  EFI_PXE_BASE_CODE_MODE           *PxeMode;
  EFI_DHCP6_CONFIG_DATA            Config;
  EFI_DHCP6_MODE_DATA              Mode;
  EFI_DHCP6_RETRANSMISSION         *Retransmit;
  EFI_DHCP6_PACKET_OPTION          *OptList[PXEBC_DHCP6_OPTION_MAX_NUM];
  UINT8                            Buffer[PXEBC_DHCP6_OPTION_MAX_SIZE];
  UINT32                           OptCount;
  EFI_STATUS                       Status;
  EFI_IP6_CONFIG_PROTOCOL          *Ip6Cfg;
  EFI_STATUS                       TimerStatus;
  EFI_EVENT                        Timer;
  UINT64                           GetMappingTimeOut;
  UINTN                            DataSize;
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS    DadXmits;

  Status     = EFI_SUCCESS;
  PxeMode    = Private->PxeBc.Mode;
  Ip6Cfg     = Private->Ip6Cfg;
  Timer      = NULL;

  //
  // Build option list for the request packet.
  //
  OptCount   = PxeBcBuildDhcp6Options (Private, OptList, Buffer);
  ASSERT (OptCount> 0);

  Retransmit = AllocateZeroPool (sizeof (EFI_DHCP6_RETRANSMISSION));
  if (Retransmit == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (&Mode, sizeof (EFI_DHCP6_MODE_DATA));
  ZeroMem (&Config, sizeof (EFI_DHCP6_CONFIG_DATA));

  Config.OptionCount           = OptCount;
  Config.OptionList            = OptList;
  Config.Dhcp6Callback         = PxeBcDhcp6CallBack;
  Config.CallbackContext       = Private;
  Config.IaInfoEvent           = NULL;
  Config.RapidCommit           = FALSE;
  Config.ReconfigureAccept     = FALSE;
  Config.IaDescriptor.IaId     = Private->IaId;
  Config.IaDescriptor.Type     = EFI_DHCP6_IA_TYPE_NA;
  Config.SolicitRetransmission = Retransmit;
  Retransmit->Irt              = 4;
  Retransmit->Mrc              = 4;
  Retransmit->Mrt              = 32;
  Retransmit->Mrd              = 60;

  //
  // Configure the DHCPv6 instance for PXE boot.
  //
  Status = Dhcp6->Configure (Dhcp6, &Config);
  FreePool (Retransmit);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize the record fields for DHCPv6 offer in private data.
  //
  Private->IsProxyRecved = FALSE;
  Private->OfferNum      = 0;
  Private->SelectIndex   = 0;
  ZeroMem (Private->OfferCount, sizeof (Private->OfferCount));
  ZeroMem (Private->OfferIndex, sizeof (Private->OfferIndex));


  //
  // Start DHCPv6 S.A.R.R. process to acquire IPv6 address.
  //
  Status = Dhcp6->Start (Dhcp6);
  if (Status == EFI_NO_MAPPING) {
    //
    // IP6 Linklocal address is not available for use, so stop current Dhcp process
    // and wait for duplicate address detection to finish.
    //
    Dhcp6->Stop (Dhcp6);

    //
    // Get Duplicate Address Detection Transmits count.
    //
    DataSize = sizeof (EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS);
    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeDupAddrDetectTransmits,
                       &DataSize,
                       &DadXmits
                       );
    if (EFI_ERROR (Status)) {
      Dhcp6->Configure (Dhcp6, NULL);
      return Status;
    }

    Status = gBS->CreateEvent (EVT_TIMER, TPL_CALLBACK, NULL, NULL, &Timer);
    if (EFI_ERROR (Status)) {
      Dhcp6->Configure (Dhcp6, NULL);
      return Status;
    }

    GetMappingTimeOut = TICKS_PER_SECOND * DadXmits.DupAddrDetectTransmits + PXEBC_DAD_ADDITIONAL_DELAY;
    Status = gBS->SetTimer (Timer, TimerRelative, GetMappingTimeOut);
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (Timer);
      Dhcp6->Configure (Dhcp6, NULL);
      return Status;
    }

    do {

      TimerStatus = gBS->CheckEvent (Timer);
      if (!EFI_ERROR (TimerStatus)) {
        Status = Dhcp6->Start (Dhcp6);
      }
    } while (TimerStatus == EFI_NOT_READY);

    gBS->CloseEvent (Timer);
  }
  if (EFI_ERROR (Status)) {
    if (Status == EFI_ICMP_ERROR) {
      PxeMode->IcmpErrorReceived = TRUE;
    }
    Dhcp6->Configure (Dhcp6, NULL);
    return Status;
  }

  //
  // Get the acquired IPv6 address and store them.
  //
  Status = Dhcp6->GetModeData (Dhcp6, &Mode, NULL);
  if (EFI_ERROR (Status)) {
    Dhcp6->Stop (Dhcp6);
    return Status;
  }

  ASSERT ((Mode.Ia != NULL) && (Mode.Ia->State == Dhcp6Bound));
  //
  // DHCP6 doesn't have an option to specify the router address on the subnet, the only way to get the
  // router address in IP6 is the router discovery mechanism (the RS and RA, which only be handled when
  // the IP policy is Automatic). So we just hold the station IP address here and leave the IP policy as
  // Automatic, until we get the server IP address. This could let IP6 driver finish the router discovery
  // to find a valid router address.
  //
  CopyMem (&Private->TmpStationIp.v6, &Mode.Ia->IaAddress[0].IpAddress, sizeof (EFI_IPv6_ADDRESS));
  if (Mode.ClientId != NULL) {
    FreePool (Mode.ClientId);
  }
  if (Mode.Ia != NULL) {
    FreePool (Mode.Ia);
  }
  //
  // Check the selected offer whether BINL retry is needed.
  //
  Status = PxeBcHandleDhcp6Offer (Private);
  if (EFI_ERROR (Status)) {
    Dhcp6->Stop (Dhcp6);
    return Status;
  }

  return EFI_SUCCESS;
}
