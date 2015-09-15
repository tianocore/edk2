/** @file
  Implementation of the boot file download function.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HttpBootDxe.h"

/**
  Update the IP and URL device path node to include the boot resource information.

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
  EFI_DEVICE_PATH_PROTOCOL   *TmpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *NewDevicePath;
  UINTN                      Length;
  EFI_STATUS                 Status;

  TmpDevicePath = NULL;
  
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
    
    TmpDevicePath = AppendDevicePathNode (Private->ParentDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
    FreePool (Node);
    if (TmpDevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    ASSERT (FALSE);
  }

  //
  // Update the URI node with the boot file URI.
  //
  Length = sizeof (EFI_DEVICE_PATH_PROTOCOL) + AsciiStrSize (Private->BootFileUri);
  Node = AllocatePool (Length);
  if (Node == NULL) {
    FreePool (TmpDevicePath);
    return EFI_OUT_OF_RESOURCES;
  }
  Node->DevPath.Type    = MESSAGING_DEVICE_PATH;
  Node->DevPath.SubType = MSG_URI_DP;
  SetDevicePathNodeLength (Node, Length);
  CopyMem ((UINT8*) Node + sizeof (EFI_DEVICE_PATH_PROTOCOL), Private->BootFileUri, AsciiStrSize (Private->BootFileUri));
  
  NewDevicePath = AppendDevicePathNode (TmpDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
  FreePool (Node);
  FreePool (TmpDevicePath);
  if (NewDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Reinstall the device path protocol of the child handle.
  //
  Status = gBS->ReinstallProtocolInterface (
                  Private->ChildHandle,
                  &gEfiDevicePathProtocolGuid,
                  Private->DevicePath,
                  NewDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  FreePool (Private->DevicePath);
  Private->DevicePath = NewDevicePath;
  return EFI_SUCCESS;
}

/**
  Parse the boot file URI information from the selected Dhcp4 offer packet.

  @param[in]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully parsed out all the boot information.
  @retval Others               Failed to parse out the boot information.

**/
EFI_STATUS
HttpBootExtractUriInfo (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private
  )
{
  HTTP_BOOT_DHCP4_PACKET_CACHE    *SelectOffer;
  HTTP_BOOT_DHCP4_PACKET_CACHE    *HttpOffer;
  UINT32                          SelectIndex;
  UINT32                          ProxyIndex;
  EFI_DHCP4_PACKET_OPTION         *Option;
  EFI_STATUS                      Status;

  ASSERT (Private != NULL);
  ASSERT (Private->SelectIndex != 0);
  SelectIndex = Private->SelectIndex - 1;
  ASSERT (SelectIndex < HTTP_BOOT_OFFER_MAX_NUM);

  Status = EFI_SUCCESS;

  //
  // SelectOffer contains the IP address configuration and name server configuration.
  // HttpOffer contains the boot file URL.
  //
  SelectOffer = &Private->OfferBuffer[SelectIndex].Dhcp4;
  if ((SelectOffer->OfferType == HttpOfferTypeDhcpIpUri) || (SelectOffer->OfferType == HttpOfferTypeDhcpNameUriDns)) {
    HttpOffer = SelectOffer;
  } else {
    ASSERT (Private->SelectProxyType != HttpOfferTypeMax);
    ProxyIndex = Private->OfferIndex[Private->SelectProxyType][0];
    HttpOffer = &Private->OfferBuffer[ProxyIndex].Dhcp4;
  }

  //
  // Configure the default DNS server if server assigned.
  //
  if ((SelectOffer->OfferType == HttpOfferTypeDhcpNameUriDns) || (SelectOffer->OfferType == HttpOfferTypeDhcpDns)) {
    Option = SelectOffer->OptList[HTTP_BOOT_DHCP4_TAG_INDEX_DNS_SERVER];
    ASSERT (Option != NULL);
    Status = HttpBootRegisterIp4Dns (
               Private,
               Option->Length,
               Option->Data
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Extract the port from URL, and use default HTTP port 80 if not provided.
  //
  Status = HttpUrlGetPort (
             (CHAR8*) HttpOffer->OptList[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE]->Data,
             HttpOffer->UriParser,
             &Private->Port
             );
  if (EFI_ERROR (Status) || Private->Port == 0) {
    Private->Port = 80;
  }
  
  //
  // Record the URI of boot file from the selected HTTP offer.
  //
  Private->BootFileUriParser = HttpOffer->UriParser;
  Private->BootFileUri = (CHAR8*) HttpOffer->OptList[HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE]->Data;

  
  //
  // All boot informations are valid here.
  //
  AsciiPrint ("\n  URI: %a", Private->BootFileUri);

  //
  // Update the device path to include the IP and boot URI information.
  //
  Status = HttpBootUpdateDevicePath (Private);

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
    Status = HttpBootExtractUriInfo (Private);
  } else {
    ASSERT (FALSE);
  }

  return Status;
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

  ASSERT (Private != NULL);

  ZeroMem (&ConfigData, sizeof (HTTP_IO_CONFIG_DATA));
  if (!Private->UsingIpv6) {
    ConfigData.Config4.HttpVersion = HttpVersion11;
    ConfigData.Config4.RequestTimeOut = HTTP_BOOT_REQUEST_TIMEOUT;
    IP4_COPY_ADDRESS (&ConfigData.Config4.LocalIp, &Private->StationIp.v4);
    IP4_COPY_ADDRESS (&ConfigData.Config4.SubnetMask, &Private->SubnetMask.v4);
  } else {
    ASSERT (FALSE);
  }

  Status = HttpIoCreateIo (
             Private->Image,
             Private->Controller,
             Private->UsingIpv6 ? IP_VERSION_6 : IP_VERSION_4,
             &ConfigData,
             &Private->HttpIo
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->HttpCreated = TRUE;
  return EFI_SUCCESS;
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

  @retval EFI_SUCCESS          Successfully created.
  @retval Others               Failed to create HttpIo.

**/
EFI_STATUS
HttpBootGetFileFromCache (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private,
  IN     CHAR16                   *Uri,
  IN OUT UINTN                    *BufferSize,
     OUT UINT8                    *Buffer
  )
{
  LIST_ENTRY                  *Entry;
  LIST_ENTRY                  *Entry2;
  HTTP_BOOT_CACHE_CONTENT     *Cache;
  HTTP_BOOT_ENTITY_DATA       *EntityData;
  UINTN                       CopyedSize;
  
  if (Uri == NULL || BufferSize == 0 || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NET_LIST_FOR_EACH (Entry, &Private->CacheList) {
    Cache = NET_LIST_USER_STRUCT (Entry, HTTP_BOOT_CACHE_CONTENT, Link);
    //
    // Compare the URI to see whether we already have a cache for this file.
    //
    if ((Cache->RequestData != NULL) &&
        (Cache->RequestData->Url != NULL) &&
        (StrCmp (Uri, Cache->RequestData->Url) == 0)) 
    {
      //
      // Hit cache, check buffer size.
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

  //
  // We only care about the entity data.
  //
  if (EventType != BodyParseEventOnData) {
    return EFI_SUCCESS;
  }

  CallbackData = (HTTP_BOOT_CALLBACK_DATA *) Context;
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
     OUT UINT8                    *Buffer
  )
{
  EFI_STATUS                 Status;
  CHAR8                      *HostName;
  EFI_HTTP_REQUEST_DATA      *RequestData;
  HTTP_IO_RESOPNSE_DATA      *ResponseData;
  HTTP_IO_RESOPNSE_DATA      ResponseBody;
  HTTP_IO                    *HttpIo;
  HTTP_IO_HEADER             *HttpIoHeader;
  VOID                       *Parser;
  HTTP_BOOT_CALLBACK_DATA    Context;
  UINTN                      ContentLength;
  HTTP_BOOT_CACHE_CONTENT    *Cache;
  UINT8                      *Block;
  CHAR16                     *Url;
  
  ASSERT (Private != NULL);
  ASSERT (Private->HttpCreated);

  if (BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*BufferSize != 0 && Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First, check whether we already cached the requested Uri.
  //
  Url = AllocatePool ((AsciiStrLen (Private->BootFileUri) + 1) * sizeof (CHAR16));
  if (Url == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  AsciiStrToUnicodeStr (Private->BootFileUri, Url);
  if (!HeaderOnly) {
    Status = HttpBootGetFileFromCache (Private, Url, BufferSize, Buffer);
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
             HTTP_FIELD_NAME_HOST,
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
             HTTP_FIELD_NAME_ACCEPT,
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
             HTTP_FIELD_NAME_USER_AGENT,
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
  if (RequestData->Url == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR_4;
  }
  AsciiStrToUnicodeStr (Private->BootFileUri, RequestData->Url);

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
  ResponseData = AllocateZeroPool (sizeof(HTTP_IO_RESOPNSE_DATA));
  if (ResponseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR_4;
  }
  Status = HttpIoRecvResponse (
             &Private->HttpIo,
             TRUE,
             ResponseData
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_5;
  }

  //
  // 3.2 Cache the response header.
  //
  if (Cache != NULL) {
    Cache->ResponseData = ResponseData;
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
  Status = HttpInitMsgParser (
             HeaderOnly? HttpMethodHead : HttpMethodGet,
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
    ZeroMem (&ResponseBody, sizeof (HTTP_IO_RESOPNSE_DATA));
    while (!HttpIsMessageComplete (Parser)) {
      //
      // Allocate a block to hold the message-body, if caller doesn't provide
      // a buffer, the block will be cached and we will allocate a new one here.
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
      if (EFI_ERROR (Status)) {
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
  
  //
  // 3.5 Message-body receive & parse is completed, get the file size.
  //
  Status = HttpGetEntityLength (Parser, &ContentLength);
  if (EFI_ERROR (Status)) {
    goto ERROR_6;
  }

  if (*BufferSize < ContentLength) {
    Status = EFI_BUFFER_TOO_SMALL;
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

  return EFI_SUCCESS;
  
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
