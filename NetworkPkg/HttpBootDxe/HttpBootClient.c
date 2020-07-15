/** @file
  Implementation of the boot file download function.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpBootDxe.h"

/**
  Update the device path node to include the boot resource information.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              Device patch successfully updated.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.
  @retval Others                   Unexpected error happened.

**/
EFI_STATUS
HttpBootUpdateDevicePath (
  IN   HTTP_BOOT_PRIVATE_DATA   *Private
  )
{
  EFI_DEV_PATH               *Node;
  EFI_DEVICE_PATH_PROTOCOL   *TmpIpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *TmpDnsDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *NewDevicePath;
  UINTN                      Length;
  EFI_STATUS                 Status;

  TmpIpDevicePath  = NULL;
  TmpDnsDevicePath = NULL;

  //
  // Update the IP node with DHCP assigned information.
  //
  if (!Private->UsingIpv6) {
    Node = AllocateZeroPool (sizeof (IPv4_DEVICE_PATH));
    if (Node == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Node->Ipv4.Header.Type    = MESSAGING_DEVICE_PATH;
    Node->Ipv4.Header.SubType = MSG_IPv4_DP;
    SetDevicePathNodeLength (Node, sizeof (IPv4_DEVICE_PATH));
    CopyMem (&Node->Ipv4.LocalIpAddress, &Private->StationIp, sizeof (EFI_IPv4_ADDRESS));
    Node->Ipv4.RemotePort      = Private->Port;
    Node->Ipv4.Protocol        = EFI_IP_PROTO_TCP;
    Node->Ipv4.StaticIpAddress = FALSE;
    CopyMem (&Node->Ipv4.GatewayIpAddress, &Private->GatewayIp, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Node->Ipv4.SubnetMask, &Private->SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  } else {
    Node = AllocateZeroPool (sizeof (IPv6_DEVICE_PATH));
    if (Node == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Node->Ipv6.Header.Type     = MESSAGING_DEVICE_PATH;
    Node->Ipv6.Header.SubType  = MSG_IPv6_DP;
    SetDevicePathNodeLength (Node, sizeof (IPv6_DEVICE_PATH));
    Node->Ipv6.PrefixLength    = IP6_PREFIX_LENGTH;
    Node->Ipv6.RemotePort      = Private->Port;
    Node->Ipv6.Protocol        = EFI_IP_PROTO_TCP;
    Node->Ipv6.IpAddressOrigin = 0;
    CopyMem (&Node->Ipv6.LocalIpAddress, &Private->StationIp.v6, sizeof (EFI_IPv6_ADDRESS));
    CopyMem (&Node->Ipv6.RemoteIpAddress, &Private->ServerIp.v6, sizeof (EFI_IPv6_ADDRESS));
    CopyMem (&Node->Ipv6.GatewayIpAddress, &Private->GatewayIp.v6, sizeof (EFI_IPv6_ADDRESS));
  }

  TmpIpDevicePath = AppendDevicePathNode (Private->ParentDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
  FreePool (Node);
  if (TmpIpDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Update the DNS node with DNS server IP list if existed.
  //
  if (Private->DnsServerIp != NULL) {
    Length = sizeof (EFI_DEVICE_PATH_PROTOCOL) + sizeof (Node->Dns.IsIPv6) + Private->DnsServerCount * sizeof (EFI_IP_ADDRESS);
    Node = AllocatePool (Length);
    if (Node == NULL) {
      FreePool (TmpIpDevicePath);
      return EFI_OUT_OF_RESOURCES;
    }
    Node->DevPath.Type    = MESSAGING_DEVICE_PATH;
    Node->DevPath.SubType = MSG_DNS_DP;
    SetDevicePathNodeLength (Node, Length);
    Node->Dns.IsIPv6 = Private->UsingIpv6 ? 0x01 : 0x00;
    CopyMem ((UINT8*) Node + sizeof (EFI_DEVICE_PATH_PROTOCOL) + sizeof (Node->Dns.IsIPv6), Private->DnsServerIp, Private->DnsServerCount * sizeof (EFI_IP_ADDRESS));

    TmpDnsDevicePath = AppendDevicePathNode (TmpIpDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
    FreePool (Node);
    FreePool (TmpIpDevicePath);
    TmpIpDevicePath = NULL;
    if (TmpDnsDevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Update the URI node with the boot file URI.
  //
  Length = sizeof (EFI_DEVICE_PATH_PROTOCOL) + AsciiStrSize (Private->BootFileUri);
  Node = AllocatePool (Length);
  if (Node == NULL) {
    if (TmpIpDevicePath != NULL) {
      FreePool (TmpIpDevicePath);
    }
    if (TmpDnsDevicePath != NULL) {
      FreePool (TmpDnsDevicePath);
    }
    return EFI_OUT_OF_RESOURCES;
  }
  Node->DevPath.Type    = MESSAGING_DEVICE_PATH;
  Node->DevPath.SubType = MSG_URI_DP;
  SetDevicePathNodeLength (Node, Length);
  CopyMem ((UINT8*) Node + sizeof (EFI_DEVICE_PATH_PROTOCOL), Private->BootFileUri, AsciiStrSize (Private->BootFileUri));

  if (TmpDnsDevicePath != NULL) {
    NewDevicePath = AppendDevicePathNode (TmpDnsDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
    FreePool (TmpDnsDevicePath);
  } else {
    ASSERT (TmpIpDevicePath != NULL);
    NewDevicePath = AppendDevicePathNode (TmpIpDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
    FreePool (TmpIpDevicePath);
  }
  FreePool (Node);
  if (NewDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (!Private->UsingIpv6) {
    //
    // Reinstall the device path protocol of the child handle.
    //
    Status = gBS->ReinstallProtocolInterface (
                    Private->Ip4Nic->Controller,
                    &gEfiDevicePathProtocolGuid,
                    Private->Ip4Nic->DevicePath,
                    NewDevicePath
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    FreePool (Private->Ip4Nic->DevicePath);
    Private->Ip4Nic->DevicePath = NewDevicePath;
  } else {
    //
    // Reinstall the device path protocol of the child handle.
    //
    Status = gBS->ReinstallProtocolInterface (
                    Private->Ip6Nic->Controller,
                    &gEfiDevicePathProtocolGuid,
                    Private->Ip6Nic->DevicePath,
                    NewDevicePath
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    FreePool (Private->Ip6Nic->DevicePath);
    Private->Ip6Nic->DevicePath = NewDevicePath;
  }

  return EFI_SUCCESS;
}

/**
  Parse the boot file URI information from the selected Dhcp4 offer packet.

  @param[in]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully parsed out all the boot information.
  @retval Others               Failed to parse out the boot information.

**/
EFI_STATUS
HttpBootDhcp4ExtractUriInfo (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private
  )
{
  HTTP_BOOT_DHCP4_PACKET_CACHE    *SelectOffer;
  HTTP_BOOT_DHCP4_PACKET_CACHE    *HttpOffer;
  UINT32                          SelectIndex;
  UINT32                          ProxyIndex;
  UINT32                          DnsServerIndex;
  EFI_DHCP4_PACKET_OPTION         *Option;
  EFI_STATUS                      Status;

  ASSERT (Private != NULL);
  ASSERT (Private->SelectIndex != 0);
  SelectIndex = Private->SelectIndex - 1;
  ASSERT (SelectIndex < HTTP_BOOT_OFFER_MAX_NUM);

  DnsServerIndex = 0;

  Status = EFI_SUCCESS;

  //
  // SelectOffer contains the IP address configuration and name server configuration.
  // HttpOffer contains the boot file URL.
  //
  SelectOffer = &Private->OfferBuffer[SelectIndex].Dhcp4;
  if (Private->FilePathUri == NULL) {
    //
    // In Corporate environment, we need a HttpOffer.
    //
    if ((SelectOffer->OfferType == HttpOfferTypeDhcpIpUri) ||
        (SelectOffer->OfferType == HttpOfferTypeDhcpIpUriDns) ||
        (SelectOffer->OfferType == HttpOfferTypeDhcpNameUriDns)) {
      HttpOffer = SelectOffer;
    } else {
      ASSERT (Private->SelectProxyType != HttpOfferTypeMax);
      ProxyIndex = Private->OfferIndex[Private->SelectProxyType][0];
      HttpOffer = &Private->OfferBuffer[ProxyIndex].Dhcp4;
    }
    Private->BootFileUriParser = HttpOffer->UriParser;
    Private->BootFileUri = (CHAR8*) HttpOffer->OptList[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE]->Data;
  } else {
    //
    // In Home environment the BootFileUri comes from the FilePath.
    //
    Private->BootFileUriParser = Private->FilePathUriParser;
    Private->BootFileUri = Private->FilePathUri;
  }

  //
  // Check the URI scheme.
  //
  Status = HttpBootCheckUriScheme (Private->BootFileUri);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "HttpBootDhcp4ExtractUriInfo: %r.\n", Status));
    if (Status == EFI_INVALID_PARAMETER) {
      AsciiPrint ("\n  Error: Invalid URI address.\n");
    } else if (Status == EFI_ACCESS_DENIED) {
      AsciiPrint ("\n  Error: Access forbidden, only HTTPS connection is allowed.\n");
    }
    return Status;
  }

  if ((SelectOffer->OfferType == HttpOfferTypeDhcpNameUriDns) ||
      (SelectOffer->OfferType == HttpOfferTypeDhcpDns) ||
      (SelectOffer->OfferType == HttpOfferTypeDhcpIpUriDns)) {
    Option = SelectOffer->OptList[HTTP_BOOT_DHCP4_TAG_INDEX_DNS_SERVER];
    ASSERT (Option != NULL);

    //
    // Record the Dns Server address list.
    //
    Private->DnsServerCount = (Option->Length) / sizeof (EFI_IPv4_ADDRESS);

    Private->DnsServerIp = AllocateZeroPool (Private->DnsServerCount * sizeof (EFI_IP_ADDRESS));
    if (Private->DnsServerIp == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (DnsServerIndex = 0; DnsServerIndex < Private->DnsServerCount; DnsServerIndex++) {
      CopyMem (&(Private->DnsServerIp[DnsServerIndex].v4), &(((EFI_IPv4_ADDRESS *) Option->Data)[DnsServerIndex]), sizeof (EFI_IPv4_ADDRESS));
    }

    //
    // Configure the default DNS server if server assigned.
    //
    Status = HttpBootRegisterIp4Dns (
               Private,
               Option->Length,
               Option->Data
               );
    if (EFI_ERROR (Status)) {
      FreePool (Private->DnsServerIp);
      Private->DnsServerIp = NULL;
      return Status;
    }
  }

  //
  // Extract the port from URL, and use default HTTP port 80 if not provided.
  //
  Status = HttpUrlGetPort (
             Private->BootFileUri,
             Private->BootFileUriParser,
             &Private->Port
             );
  if (EFI_ERROR (Status) || Private->Port == 0) {
    Private->Port = 80;
  }

  //
  // All boot informations are valid here.
  //

  //
  // Update the device path to include the boot resource information.
  //
  Status = HttpBootUpdateDevicePath (Private);
  if (EFI_ERROR (Status) && Private->DnsServerIp != NULL) {
    FreePool (Private->DnsServerIp);
    Private->DnsServerIp = NULL;
  }

  return Status;
}

/**
  Parse the boot file URI information from the selected Dhcp6 offer packet.

  @param[in]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully parsed out all the boot information.
  @retval Others               Failed to parse out the boot information.

**/
EFI_STATUS
HttpBootDhcp6ExtractUriInfo (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private
  )
{
  HTTP_BOOT_DHCP6_PACKET_CACHE    *SelectOffer;
  HTTP_BOOT_DHCP6_PACKET_CACHE    *HttpOffer;
  UINT32                          SelectIndex;
  UINT32                          ProxyIndex;
  UINT32                          DnsServerIndex;
  EFI_DHCP6_PACKET_OPTION         *Option;
  EFI_IPv6_ADDRESS                IpAddr;
  CHAR8                           *HostName;
  UINTN                           HostNameSize;
  CHAR16                          *HostNameStr;
  EFI_STATUS                      Status;

  ASSERT (Private != NULL);
  ASSERT (Private->SelectIndex != 0);
  SelectIndex = Private->SelectIndex - 1;
  ASSERT (SelectIndex < HTTP_BOOT_OFFER_MAX_NUM);

  DnsServerIndex = 0;

  Status   = EFI_SUCCESS;
  HostName = NULL;
  //
  // SelectOffer contains the IP address configuration and name server configuration.
  // HttpOffer contains the boot file URL.
  //
  SelectOffer = &Private->OfferBuffer[SelectIndex].Dhcp6;
  if (Private->FilePathUri == NULL) {
    //
    // In Corporate environment, we need a HttpOffer.
    //
    if ((SelectOffer->OfferType == HttpOfferTypeDhcpIpUri) ||
        (SelectOffer->OfferType == HttpOfferTypeDhcpIpUriDns) ||
        (SelectOffer->OfferType == HttpOfferTypeDhcpNameUriDns)) {
      HttpOffer = SelectOffer;
    } else {
      ASSERT (Private->SelectProxyType != HttpOfferTypeMax);
      ProxyIndex = Private->OfferIndex[Private->SelectProxyType][0];
      HttpOffer = &Private->OfferBuffer[ProxyIndex].Dhcp6;
    }
    Private->BootFileUriParser = HttpOffer->UriParser;
    Private->BootFileUri = (CHAR8*) HttpOffer->OptList[HTTP_BOOT_DHCP6_IDX_BOOT_FILE_URL]->Data;
  } else {
    //
    // In Home environment the BootFileUri comes from the FilePath.
    //
    Private->BootFileUriParser = Private->FilePathUriParser;
    Private->BootFileUri = Private->FilePathUri;
  }

  //
  // Check the URI scheme.
  //
  Status = HttpBootCheckUriScheme (Private->BootFileUri);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "HttpBootDhcp6ExtractUriInfo: %r.\n", Status));
    if (Status == EFI_INVALID_PARAMETER) {
      AsciiPrint ("\n  Error: Invalid URI address.\n");
    } else if (Status == EFI_ACCESS_DENIED) {
      AsciiPrint ("\n  Error: Access forbidden, only HTTPS connection is allowed.\n");
    }
    return Status;
  }

  //
  //  Set the Local station address to IP layer.
  //
  Status = HttpBootSetIp6Address (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register the IPv6 gateway address to the network device.
  //
  Status = HttpBootSetIp6Gateway (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((SelectOffer->OfferType == HttpOfferTypeDhcpNameUriDns) ||
      (SelectOffer->OfferType == HttpOfferTypeDhcpDns) ||
      (SelectOffer->OfferType == HttpOfferTypeDhcpIpUriDns)) {
    Option = SelectOffer->OptList[HTTP_BOOT_DHCP6_IDX_DNS_SERVER];
    ASSERT (Option != NULL);

    //
    // Record the Dns Server address list.
    //
    Private->DnsServerCount = HTONS (Option->OpLen) / sizeof (EFI_IPv6_ADDRESS);

    Private->DnsServerIp = AllocateZeroPool (Private->DnsServerCount * sizeof (EFI_IP_ADDRESS));
    if (Private->DnsServerIp == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (DnsServerIndex = 0; DnsServerIndex < Private->DnsServerCount; DnsServerIndex++) {
      CopyMem (&(Private->DnsServerIp[DnsServerIndex].v6), &(((EFI_IPv6_ADDRESS *) Option->Data)[DnsServerIndex]), sizeof (EFI_IPv6_ADDRESS));
    }

    //
    // Configure the default DNS server if server assigned.
    //
    Status = HttpBootSetIp6Dns (
               Private,
               HTONS (Option->OpLen),
               Option->Data
               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  }

  //
  // Extract the HTTP server Ip from URL. This is used to Check route table
  // whether can send message to HTTP Server Ip through the GateWay.
  //
  Status = HttpUrlGetIp6 (
             Private->BootFileUri,
             Private->BootFileUriParser,
             &IpAddr
             );

  if (EFI_ERROR (Status)) {
    //
    // The Http server address is expressed by Name Ip, so perform DNS resolution
    //
    Status = HttpUrlGetHostName (
               Private->BootFileUri,
               Private->BootFileUriParser,
               &HostName
               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    HostNameSize = AsciiStrSize (HostName);
    HostNameStr = AllocateZeroPool (HostNameSize * sizeof (CHAR16));
    if (HostNameStr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    AsciiStrToUnicodeStrS (HostName, HostNameStr, HostNameSize);

    if (HostName != NULL) {
      FreePool (HostName);
    }

    Status = HttpBootDns (Private, HostNameStr, &IpAddr);
    FreePool (HostNameStr);
    if (EFI_ERROR (Status)) {
      AsciiPrint ("\n  Error: Could not retrieve the host address from DNS server.\n");
      goto Error;
    }
  }

  CopyMem (&Private->ServerIp.v6, &IpAddr, sizeof (EFI_IPv6_ADDRESS));

  //
  // Extract the port from URL, and use default HTTP port 80 if not provided.
  //
  Status = HttpUrlGetPort (
             Private->BootFileUri,
             Private->BootFileUriParser,
             &Private->Port
             );
  if (EFI_ERROR (Status) || Private->Port == 0) {
    Private->Port = 80;
  }

  //
  // All boot informations are valid here.
  //

  //
  // Update the device path to include the boot resource information.
  //
  Status = HttpBootUpdateDevicePath (Private);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  return Status;

Error:
  if (Private->DnsServerIp != NULL) {
    FreePool (Private->DnsServerIp);
    Private->DnsServerIp = NULL;
  }

  return Status;
}


/**
  Discover all the boot information for boot file.

  @param[in, out]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully obtained all the boot information .
  @retval Others               Failed to retrieve the boot information.

**/
EFI_STATUS
HttpBootDiscoverBootInfo (
  IN OUT HTTP_BOOT_PRIVATE_DATA   *Private
  )
{
  EFI_STATUS              Status;

  //
  // Start D.O.R.A/S.A.R.R exchange to acquire station ip address and
  // other Http boot information.
  //
  Status = HttpBootDhcp (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!Private->UsingIpv6) {
    Status = HttpBootDhcp4ExtractUriInfo (Private);
  } else {
    Status = HttpBootDhcp6ExtractUriInfo (Private);
  }

  return Status;
}

/**
  HttpIo Callback function which will be invoked when specified HTTP_IO_CALLBACK_EVENT happened.

  @param[in]    EventType      Indicate the Event type that occurs in the current callback.
  @param[in]    Message        HTTP message which will be send to, or just received from HTTP server.
  @param[in]    Context        The Callback Context pointer.

  @retval EFI_SUCCESS          Tells the HttpIo to continue the HTTP process.
  @retval Others               Tells the HttpIo to abort the current HTTP process.
**/
EFI_STATUS
EFIAPI
HttpBootHttpIoCallback (
  IN  HTTP_IO_CALLBACK_EVENT    EventType,
  IN  EFI_HTTP_MESSAGE          *Message,
  IN  VOID                      *Context
  )
{
  HTTP_BOOT_PRIVATE_DATA       *Private;
  EFI_STATUS                   Status;
  Private = (HTTP_BOOT_PRIVATE_DATA *) Context;
  if (Private->HttpBootCallback != NULL) {
    Status = Private->HttpBootCallback->Callback (
               Private->HttpBootCallback,
               EventType == HttpIoRequest ? HttpBootHttpRequest : HttpBootHttpResponse,
               EventType == HttpIoRequest ? FALSE : TRUE,
               sizeof (EFI_HTTP_MESSAGE),
               (VOID *) Message
               );
    return Status;
  }
  return EFI_SUCCESS;
}

/**
  Create a HttpIo instance for the file download.

  @param[in]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully created.
  @retval Others               Failed to create HttpIo.

**/
EFI_STATUS
HttpBootCreateHttpIo (
  IN     HTTP_BOOT_PRIVATE_DATA       *Private
  )
{
  HTTP_IO_CONFIG_DATA          ConfigData;
  EFI_STATUS                   Status;
  EFI_HANDLE                   ImageHandle;

  ASSERT (Private != NULL);

  ZeroMem (&ConfigData, sizeof (HTTP_IO_CONFIG_DATA));
  if (!Private->UsingIpv6) {
    ConfigData.Config4.HttpVersion    = HttpVersion11;
    ConfigData.Config4.RequestTimeOut = HTTP_BOOT_REQUEST_TIMEOUT;
    IP4_COPY_ADDRESS (&ConfigData.Config4.LocalIp, &Private->StationIp.v4);
    IP4_COPY_ADDRESS (&ConfigData.Config4.SubnetMask, &Private->SubnetMask.v4);
    ImageHandle = Private->Ip4Nic->ImageHandle;
  } else {
    ConfigData.Config6.HttpVersion    = HttpVersion11;
    ConfigData.Config6.RequestTimeOut = HTTP_BOOT_REQUEST_TIMEOUT;
    IP6_COPY_ADDRESS (&ConfigData.Config6.LocalIp, &Private->StationIp.v6);
    ImageHandle = Private->Ip6Nic->ImageHandle;
  }

  Status = HttpIoCreateIo (
             ImageHandle,
             Private->Controller,
             Private->UsingIpv6 ? IP_VERSION_6 : IP_VERSION_4,
             &ConfigData,
             HttpBootHttpIoCallback,
             (VOID *) Private,
             &Private->HttpIo
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->HttpCreated = TRUE;
  return EFI_SUCCESS;
}

/**
  Release all the resource of a cache item.

  @param[in]          Cache         The pointer to the cache item.

**/
VOID
HttpBootFreeCache (
  IN  HTTP_BOOT_CACHE_CONTENT    *Cache
  )
{
  UINTN                       Index;
  LIST_ENTRY                  *Entry;
  LIST_ENTRY                  *NextEntry;
  HTTP_BOOT_ENTITY_DATA       *EntityData;

  if (Cache != NULL) {
    //
    // Free the request data
    //
    if (Cache->RequestData != NULL) {
      if (Cache->RequestData->Url != NULL) {
        FreePool (Cache->RequestData->Url);
      }
      FreePool (Cache->RequestData);
    }

    //
    // Free the response header
    //
    if (Cache->ResponseData != NULL) {
      if (Cache->ResponseData->Headers != NULL) {
        for (Index = 0; Index < Cache->ResponseData->HeaderCount; Index++) {
          FreePool (Cache->ResponseData->Headers[Index].FieldName);
          FreePool (Cache->ResponseData->Headers[Index].FieldValue);
        }
        FreePool (Cache->ResponseData->Headers);
      }
    }

    //
    // Free the response body
    //
    NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Cache->EntityDataList) {
      EntityData = NET_LIST_USER_STRUCT (Entry, HTTP_BOOT_ENTITY_DATA, Link);
      if (EntityData->Block != NULL) {
        FreePool (EntityData->Block);
      }
      RemoveEntryList (&EntityData->Link);
      FreePool (EntityData);
    }

    FreePool (Cache);
  }
}

/**
  Clean up all cached data.

  @param[in]          Private         The pointer to the driver's private data.

**/
VOID
HttpBootFreeCacheList (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private
  )
{
  LIST_ENTRY                  *Entry;
  LIST_ENTRY                  *NextEntry;
  HTTP_BOOT_CACHE_CONTENT     *Cache;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->CacheList) {
    Cache = NET_LIST_USER_STRUCT (Entry, HTTP_BOOT_CACHE_CONTENT, Link);
    RemoveEntryList (&Cache->Link);
    HttpBootFreeCache (Cache);
  }
}

/**
  Get the file content from cached data.

  @param[in]          Private         The pointer to the driver's private data.
  @param[in]          Uri             Uri of the file to be retrieved from cache.
  @param[in, out]     BufferSize      On input the size of Buffer in bytes. On output with a return
                                      code of EFI_SUCCESS, the amount of data transferred to
                                      Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                      the size of Buffer required to retrieve the requested file.
  @param[out]         Buffer          The memory buffer to transfer the file to. IF Buffer is NULL,
                                      then the size of the requested file is returned in
                                      BufferSize.
  @param[out]         ImageType       The image type of the downloaded file.

  @retval EFI_SUCCESS          Successfully created.
  @retval Others               Failed to create HttpIo.

**/
EFI_STATUS
HttpBootGetFileFromCache (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private,
  IN     CHAR16                   *Uri,
  IN OUT UINTN                    *BufferSize,
     OUT UINT8                    *Buffer,
     OUT HTTP_BOOT_IMAGE_TYPE     *ImageType
  )
{
  LIST_ENTRY                  *Entry;
  LIST_ENTRY                  *Entry2;
  HTTP_BOOT_CACHE_CONTENT     *Cache;
  HTTP_BOOT_ENTITY_DATA       *EntityData;
  UINTN                       CopyedSize;

  if (Uri == NULL || BufferSize == NULL || Buffer == NULL || ImageType == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NET_LIST_FOR_EACH (Entry, &Private->CacheList) {
    Cache = NET_LIST_USER_STRUCT (Entry, HTTP_BOOT_CACHE_CONTENT, Link);
    //
    // Compare the URI to see whether we already have a cache for this file.
    //
    if ((Cache->RequestData != NULL) &&
        (Cache->RequestData->Url != NULL) &&
        (StrCmp (Uri, Cache->RequestData->Url) == 0)) {
      //
      // Hit in cache, record image type.
      //
      *ImageType  = Cache->ImageType;

      //
      // Check buffer size.
      //
      if (*BufferSize < Cache->EntityLength) {
        *BufferSize = Cache->EntityLength;
        return EFI_BUFFER_TOO_SMALL;
      }

      //
      // Fill data to buffer.
      //
      CopyedSize = 0;
      NET_LIST_FOR_EACH (Entry2, &Cache->EntityDataList) {
        EntityData = NET_LIST_USER_STRUCT (Entry2, HTTP_BOOT_ENTITY_DATA, Link);
        if (*BufferSize > CopyedSize) {
          CopyMem (
            Buffer + CopyedSize,
            EntityData->DataStart,
            MIN (EntityData->DataLength, *BufferSize - CopyedSize)
            );
          CopyedSize += MIN (EntityData->DataLength, *BufferSize - CopyedSize);
        }
      }
      *BufferSize = CopyedSize;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  A callback function to intercept events during message parser.

  This function will be invoked during HttpParseMessageBody() with various events type. An error
  return status of the callback function will cause the HttpParseMessageBody() aborted.

  @param[in]    EventType          Event type of this callback call.
  @param[in]    Data               A pointer to data buffer.
  @param[in]    Length             Length in bytes of the Data.
  @param[in]    Context            Callback context set by HttpInitMsgParser().

  @retval EFI_SUCCESS              Continue to parser the message body.
  @retval Others                   Abort the parse.

**/
EFI_STATUS
EFIAPI
HttpBootGetBootFileCallback (
  IN HTTP_BODY_PARSE_EVENT      EventType,
  IN CHAR8                      *Data,
  IN UINTN                      Length,
  IN VOID                       *Context
  )
{
  HTTP_BOOT_CALLBACK_DATA      *CallbackData;
  HTTP_BOOT_ENTITY_DATA        *NewEntityData;
  EFI_STATUS                   Status;
  EFI_HTTP_BOOT_CALLBACK_PROTOCOL   *HttpBootCallback;

  //
  // We only care about the entity data.
  //
  if (EventType != BodyParseEventOnData) {
    return EFI_SUCCESS;
  }

  CallbackData = (HTTP_BOOT_CALLBACK_DATA *) Context;
  HttpBootCallback = CallbackData->Private->HttpBootCallback;
  if (HttpBootCallback != NULL) {
    Status = HttpBootCallback->Callback (
               HttpBootCallback,
               HttpBootHttpEntityBody,
               TRUE,
               (UINT32)Length,
               Data
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // Copy data if caller has provided a buffer.
  //
  if (CallbackData->BufferSize > CallbackData->CopyedSize) {
    CopyMem (
      CallbackData->Buffer + CallbackData->CopyedSize,
      Data,
      MIN (Length, CallbackData->BufferSize - CallbackData->CopyedSize)
      );
    CallbackData->CopyedSize += MIN (Length, CallbackData->BufferSize - CallbackData->CopyedSize);
  }

  //
  // The caller doesn't provide a buffer, save the block into cache list.
  //
  if (CallbackData->Cache != NULL) {
    NewEntityData = AllocatePool (sizeof (HTTP_BOOT_ENTITY_DATA));
    if (NewEntityData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    if (CallbackData->NewBlock) {
      NewEntityData->Block = CallbackData->Block;
      CallbackData->Block = NULL;
    }
    NewEntityData->DataLength = Length;
    NewEntityData->DataStart  = (UINT8*) Data;
    InsertTailList (&CallbackData->Cache->EntityDataList, &NewEntityData->Link);
  }
  return EFI_SUCCESS;
}

/**
  This function download the boot file by using UEFI HTTP protocol.

  @param[in]       Private         The pointer to the driver's private data.
  @param[in]       HeaderOnly      Only request the response header, it could save a lot of time if
                                   the caller only want to know the size of the requested file.
  @param[in, out]  BufferSize      On input the size of Buffer in bytes. On output with a return
                                   code of EFI_SUCCESS, the amount of data transferred to
                                   Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                   the size of Buffer required to retrieve the requested file.
  @param[out]      Buffer          The memory buffer to transfer the file to. IF Buffer is NULL,
                                   then the size of the requested file is returned in
                                   BufferSize.
  @param[out]      ImageType       The image type of the downloaded file.

  @retval EFI_SUCCESS              The file was loaded.
  @retval EFI_INVALID_PARAMETER    BufferSize is NULL or Buffer Size is not NULL but Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources
  @retval EFI_BUFFER_TOO_SMALL     The BufferSize is too small to read the current directory entry.
                                   BufferSize has been updated with the size needed to complete
                                   the request.
  @retval Others                   Unexpected error happened.

**/
EFI_STATUS
HttpBootGetBootFile (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private,
  IN     BOOLEAN                  HeaderOnly,
  IN OUT UINTN                    *BufferSize,
     OUT UINT8                    *Buffer,
     OUT HTTP_BOOT_IMAGE_TYPE     *ImageType
  )
{
  EFI_STATUS                 Status;
  EFI_HTTP_STATUS_CODE       StatusCode;
  CHAR8                      *HostName;
  EFI_HTTP_REQUEST_DATA      *RequestData;
  HTTP_IO_RESPONSE_DATA      *ResponseData;
  HTTP_IO_RESPONSE_DATA      ResponseBody;
  HTTP_IO                    *HttpIo;
  HTTP_IO_HEADER             *HttpIoHeader;
  VOID                       *Parser;
  HTTP_BOOT_CALLBACK_DATA    Context;
  UINTN                      ContentLength;
  HTTP_BOOT_CACHE_CONTENT    *Cache;
  UINT8                      *Block;
  UINTN                      UrlSize;
  CHAR16                     *Url;
  BOOLEAN                    IdentityMode;
  UINTN                      ReceivedSize;

  ASSERT (Private != NULL);
  ASSERT (Private->HttpCreated);

  if (BufferSize == NULL || ImageType == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*BufferSize != 0 && Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First, check whether we already cached the requested Uri.
  //
  UrlSize = AsciiStrSize (Private->BootFileUri);
  Url = AllocatePool (UrlSize * sizeof (CHAR16));
  if (Url == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  AsciiStrToUnicodeStrS (Private->BootFileUri, Url, UrlSize);
  if (!HeaderOnly && Buffer != NULL) {
    Status = HttpBootGetFileFromCache (Private, Url, BufferSize, Buffer, ImageType);
    if (Status != EFI_NOT_FOUND) {
      FreePool (Url);
      return Status;
    }
  }

  //
  // Not found in cache, try to download it through HTTP.
  //

  //
  // 1. Create a temp cache item for the requested URI if caller doesn't provide buffer.
  //
  Cache = NULL;
  if ((!HeaderOnly) && (*BufferSize == 0)) {
    Cache = AllocateZeroPool (sizeof (HTTP_BOOT_CACHE_CONTENT));
    if (Cache == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ERROR_1;
    }
    Cache->ImageType = ImageTypeMax;
    InitializeListHead (&Cache->EntityDataList);
  }

  //
  // 2. Send HTTP request message.
  //

  //
  // 2.1 Build HTTP header for the request, 3 header is needed to download a boot file:
  //       Host
  //       Accept
  //       User-Agent
  //
  HttpIoHeader = HttpBootCreateHeader (3);
  if (HttpIoHeader == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR_2;
  }

  //
  // Add HTTP header field 1: Host
  //
  HostName = NULL;
  Status = HttpUrlGetHostName (
             Private->BootFileUri,
             Private->BootFileUriParser,
             &HostName
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_3;
  }
  Status = HttpBootSetHeader (
             HttpIoHeader,
             HTTP_HEADER_HOST,
             HostName
             );
  FreePool (HostName);
  if (EFI_ERROR (Status)) {
    goto ERROR_3;
  }

  //
  // Add HTTP header field 2: Accept
  //
  Status = HttpBootSetHeader (
             HttpIoHeader,
             HTTP_HEADER_ACCEPT,
             "*/*"
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_3;
  }

  //
  // Add HTTP header field 3: User-Agent
  //
  Status = HttpBootSetHeader (
             HttpIoHeader,
             HTTP_HEADER_USER_AGENT,
             HTTP_USER_AGENT_EFI_HTTP_BOOT
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_3;
  }

  //
  // 2.2 Build the rest of HTTP request info.
  //
  RequestData = AllocatePool (sizeof (EFI_HTTP_REQUEST_DATA));
  if (RequestData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR_3;
  }
  RequestData->Method = HeaderOnly ? HttpMethodHead : HttpMethodGet;
  RequestData->Url = Url;

  //
  // 2.3 Record the request info in a temp cache item.
  //
  if (Cache != NULL) {
    Cache->RequestData = RequestData;
  }

  //
  // 2.4 Send out the request to HTTP server.
  //
  HttpIo = &Private->HttpIo;
  Status = HttpIoSendRequest (
             HttpIo,
             RequestData,
             HttpIoHeader->HeaderCount,
             HttpIoHeader->Headers,
             0,
             NULL
            );
  if (EFI_ERROR (Status)) {
    goto ERROR_4;
  }

  //
  // 3. Receive HTTP response message.
  //

  //
  // 3.1 First step, use zero BodyLength to only receive the response headers.
  //
  ResponseData = AllocateZeroPool (sizeof(HTTP_IO_RESPONSE_DATA));
  if (ResponseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR_4;
  }
  Status = HttpIoRecvResponse (
             &Private->HttpIo,
             TRUE,
             ResponseData
             );
  if (EFI_ERROR (Status) || EFI_ERROR (ResponseData->Status)) {
    if (EFI_ERROR (ResponseData->Status)) {
      StatusCode = HttpIo->RspToken.Message->Data.Response->StatusCode;
      HttpBootPrintErrorMessage (StatusCode);
      Status = ResponseData->Status;
    }
    goto ERROR_5;
  }

  //
  // Check the image type according to server's response.
  //
  Status = HttpBootCheckImageType (
             Private->BootFileUri,
             Private->BootFileUriParser,
             ResponseData->HeaderCount,
             ResponseData->Headers,
             ImageType
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_5;
  }

  //
  // 3.2 Cache the response header.
  //
  if (Cache != NULL) {
    Cache->ResponseData = ResponseData;
    Cache->ImageType = *ImageType;
  }

  //
  // 3.3 Init a message-body parser from the header information.
  //
  Parser = NULL;
  Context.NewBlock   = FALSE;
  Context.Block      = NULL;
  Context.CopyedSize = 0;
  Context.Buffer     = Buffer;
  Context.BufferSize = *BufferSize;
  Context.Cache      = Cache;
  Context.Private    = Private;
  Status = HttpInitMsgParser (
             HeaderOnly ? HttpMethodHead : HttpMethodGet,
             ResponseData->Response.StatusCode,
             ResponseData->HeaderCount,
             ResponseData->Headers,
             HttpBootGetBootFileCallback,
             (VOID*) &Context,
             &Parser
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_6;
  }

  //
  // 3.4 Continue to receive and parse message-body if needed.
  //
  Block = NULL;
  if (!HeaderOnly) {
    //
    // 3.4.1, check whether we are in identity transfer-coding.
    //
    ContentLength = 0;
    Status = HttpGetEntityLength (Parser, &ContentLength);
    if (!EFI_ERROR (Status)) {
      IdentityMode = TRUE;
    } else {
      IdentityMode = FALSE;
    }

    //
    // 3.4.2, start the message-body download, the identity and chunked transfer-coding
    // is handled in different path here.
    //
    ZeroMem (&ResponseBody, sizeof (HTTP_IO_RESPONSE_DATA));
    if (IdentityMode) {
      //
      // In identity transfer-coding there is no need to parse the message body,
      // just download the message body to the user provided buffer directly.
      //
      ReceivedSize = 0;
      while (ReceivedSize < ContentLength) {
        ResponseBody.Body       = (CHAR8*) Buffer + ReceivedSize;
        ResponseBody.BodyLength = *BufferSize - ReceivedSize;
        Status = HttpIoRecvResponse (
                   &Private->HttpIo,
                   FALSE,
                   &ResponseBody
                   );
        if (EFI_ERROR (Status) || EFI_ERROR (ResponseBody.Status)) {
          if (EFI_ERROR (ResponseBody.Status)) {
            Status = ResponseBody.Status;
          }
          goto ERROR_6;
        }
        ReceivedSize += ResponseBody.BodyLength;
        if (Private->HttpBootCallback != NULL) {
          Status = Private->HttpBootCallback->Callback (
                     Private->HttpBootCallback,
                     HttpBootHttpEntityBody,
                     TRUE,
                     (UINT32)ResponseBody.BodyLength,
                     ResponseBody.Body
                     );
          if (EFI_ERROR (Status)) {
            goto ERROR_6;
          }
        }
      }
    } else {
      //
      // In "chunked" transfer-coding mode, so we need to parse the received
      // data to get the real entity content.
      //
      Block = NULL;
      while (!HttpIsMessageComplete (Parser)) {
        //
        // Allocate a buffer in Block to hold the message-body.
        // If caller provides a buffer, this Block will be reused in every HttpIoRecvResponse().
        // Otherwise a buffer, the buffer in Block will be cached and we should allocate a new before
        // every HttpIoRecvResponse().
        //
        if (Block == NULL || Context.BufferSize == 0) {
          Block = AllocatePool (HTTP_BOOT_BLOCK_SIZE);
          if (Block == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto ERROR_6;
          }
          Context.NewBlock = TRUE;
          Context.Block = Block;
        } else {
          Context.NewBlock = FALSE;
        }

        ResponseBody.Body       = (CHAR8*) Block;
        ResponseBody.BodyLength = HTTP_BOOT_BLOCK_SIZE;
        Status = HttpIoRecvResponse (
                   &Private->HttpIo,
                   FALSE,
                   &ResponseBody
                   );
        if (EFI_ERROR (Status) || EFI_ERROR (ResponseBody.Status)) {
          if (EFI_ERROR (ResponseBody.Status)) {
            Status = ResponseBody.Status;
          }
          goto ERROR_6;
        }

        //
        // Parse the new received block of the message-body, the block will be saved in cache.
        //
        Status = HttpParseMessageBody (
                   Parser,
                   ResponseBody.BodyLength,
                   ResponseBody.Body
                   );
        if (EFI_ERROR (Status)) {
          goto ERROR_6;
        }
      }
    }
  }

  //
  // 3.5 Message-body receive & parse is completed, we should be able to get the file size now.
  //
  Status = HttpGetEntityLength (Parser, &ContentLength);
  if (EFI_ERROR (Status)) {
    goto ERROR_6;
  }

  if (*BufferSize < ContentLength) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    Status = EFI_SUCCESS;
  }
  *BufferSize = ContentLength;

  //
  // 4. Save the cache item to driver's cache list and return.
  //
  if (Cache != NULL) {
    Cache->EntityLength = ContentLength;
    InsertTailList (&Private->CacheList, &Cache->Link);
  }

  if (Parser != NULL) {
    HttpFreeMsgParser (Parser);
  }

  return Status;

ERROR_6:
  if (Parser != NULL) {
    HttpFreeMsgParser (Parser);
  }
  if (Context.Block != NULL) {
    FreePool (Context.Block);
  }
  HttpBootFreeCache (Cache);

ERROR_5:
  if (ResponseData != NULL) {
    FreePool (ResponseData);
  }
ERROR_4:
  if (RequestData != NULL) {
    FreePool (RequestData);
  }
ERROR_3:
  HttpBootFreeHeader (HttpIoHeader);
ERROR_2:
  if (Cache != NULL) {
    FreePool (Cache);
  }
ERROR_1:
  if (Url != NULL) {
    FreePool (Url);
  }

  return Status;
}

