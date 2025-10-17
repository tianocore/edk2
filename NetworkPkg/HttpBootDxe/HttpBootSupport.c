/** @file
  Support functions implementation for UEFI HTTP boot driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 - 2020 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpBootDxe.h"

/**
  Get the Nic handle using any child handle in the IPv4 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv4.

  @return NicHandle               The pointer to the Nic handle.
  @return NULL                    Can't find the Nic handle.

**/
EFI_HANDLE
HttpBootGetNicByIp4Children (
  IN EFI_HANDLE  ControllerHandle
  )
{
  EFI_HANDLE  NicHandle;

  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiHttpProtocolGuid);
  if (NicHandle == NULL) {
    NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiDhcp4ProtocolGuid);
    if (NicHandle == NULL) {
      return NULL;
    }
  }

  return NicHandle;
}

/**
  Get the Nic handle using any child handle in the IPv6 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv6.

  @return NicHandle               The pointer to the Nic handle.
  @return NULL                    Can't find the Nic handle.

**/
EFI_HANDLE
HttpBootGetNicByIp6Children (
  IN EFI_HANDLE  ControllerHandle
  )
{
  EFI_HANDLE  NicHandle;

  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiHttpProtocolGuid);
  if (NicHandle == NULL) {
    NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiDhcp6ProtocolGuid);
    if (NicHandle == NULL) {
      return NULL;
    }
  }

  return NicHandle;
}

/**
  This function is to convert UINTN to ASCII string with the required formatting.

  @param[in]  Number         Numeric value to be converted.
  @param[in]  Buffer         The pointer to the buffer for ASCII string.
  @param[in]  Length         The length of the required format.

**/
VOID
HttpBootUintnToAscDecWithFormat (
  IN UINTN  Number,
  IN UINT8  *Buffer,
  IN INTN   Length
  )
{
  UINTN  Remainder;

  for ( ; Length > 0; Length--) {
    Remainder          = Number % 10;
    Number            /= 10;
    Buffer[Length - 1] = (UINT8)('0' + Remainder);
  }
}

/**
  This function is to display the IPv4 address.

  @param[in]  Ip        The pointer to the IPv4 address.

**/
VOID
HttpBootShowIp4Addr (
  IN EFI_IPv4_ADDRESS  *Ip
  )
{
  UINTN  Index;

  for (Index = 0; Index < 4; Index++) {
    AsciiPrint ("%d", Ip->Addr[Index]);
    if (Index < 3) {
      AsciiPrint (".");
    }
  }
}

/**
  This function is to display the IPv6 address.

  @param[in]  Ip        The pointer to the IPv6 address.

**/
VOID
HttpBootShowIp6Addr (
  IN EFI_IPv6_ADDRESS  *Ip
  )
{
  UINTN  Index;

  for (Index = 0; Index < 16; Index++) {
    if (Ip->Addr[Index] != 0) {
      AsciiPrint ("%x", Ip->Addr[Index]);
    }

    Index++;
    if (Index > 15) {
      return;
    }

    if (((Ip->Addr[Index] & 0xf0) == 0) && (Ip->Addr[Index - 1] != 0)) {
      AsciiPrint ("0");
    }

    AsciiPrint ("%x", Ip->Addr[Index]);
    if (Index < 15) {
      AsciiPrint (":");
    }
  }
}

/**
  This function is to display the HTTP error status.

  @param[in]      StatusCode      The status code value in HTTP message.

**/
VOID
HttpBootPrintErrorMessage (
  EFI_HTTP_STATUS_CODE  StatusCode
  )
{
  AsciiPrint ("\n");

  switch (StatusCode) {
    case HTTP_STATUS_300_MULTIPLE_CHOICES:
      AsciiPrint ("\n  Redirection: 300 Multiple Choices");
      break;

    case HTTP_STATUS_301_MOVED_PERMANENTLY:
      AsciiPrint ("\n  Redirection: 301 Moved Permanently");
      break;

    case HTTP_STATUS_302_FOUND:
      AsciiPrint ("\n  Redirection: 302 Found");
      break;

    case HTTP_STATUS_303_SEE_OTHER:
      AsciiPrint ("\n  Redirection: 303 See Other");
      break;

    case HTTP_STATUS_304_NOT_MODIFIED:
      AsciiPrint ("\n  Redirection: 304 Not Modified");
      break;

    case HTTP_STATUS_305_USE_PROXY:
      AsciiPrint ("\n  Redirection: 305 Use Proxy");
      break;

    case HTTP_STATUS_307_TEMPORARY_REDIRECT:
      AsciiPrint ("\n  Redirection: 307 Temporary Redirect");
      break;

    case HTTP_STATUS_308_PERMANENT_REDIRECT:
      AsciiPrint ("\n  Redirection: 308 Permanent Redirect");
      break;

    case HTTP_STATUS_400_BAD_REQUEST:
      AsciiPrint ("\n  Client Error: 400 Bad Request");
      break;

    case HTTP_STATUS_401_UNAUTHORIZED:
      AsciiPrint ("\n  Client Error: 401 Unauthorized");
      break;

    case HTTP_STATUS_402_PAYMENT_REQUIRED:
      AsciiPrint ("\n  Client Error: 402 Payment Required");
      break;

    case HTTP_STATUS_403_FORBIDDEN:
      AsciiPrint ("\n  Client Error: 403 Forbidden");
      break;

    case HTTP_STATUS_404_NOT_FOUND:
      AsciiPrint ("\n  Client Error: 404 Not Found");
      break;

    case HTTP_STATUS_405_METHOD_NOT_ALLOWED:
      AsciiPrint ("\n  Client Error: 405 Method Not Allowed");
      break;

    case HTTP_STATUS_406_NOT_ACCEPTABLE:
      AsciiPrint ("\n  Client Error: 406 Not Acceptable");
      break;

    case HTTP_STATUS_407_PROXY_AUTHENTICATION_REQUIRED:
      AsciiPrint ("\n  Client Error: 407 Proxy Authentication Required");
      break;

    case HTTP_STATUS_408_REQUEST_TIME_OUT:
      AsciiPrint ("\n  Client Error: 408 Request Timeout");
      break;

    case HTTP_STATUS_409_CONFLICT:
      AsciiPrint ("\n  Client Error: 409 Conflict");
      break;

    case HTTP_STATUS_410_GONE:
      AsciiPrint ("\n  Client Error: 410 Gone");
      break;

    case HTTP_STATUS_411_LENGTH_REQUIRED:
      AsciiPrint ("\n  Client Error: 411 Length Required");
      break;

    case HTTP_STATUS_412_PRECONDITION_FAILED:
      AsciiPrint ("\n  Client Error: 412 Precondition Failed");
      break;

    case HTTP_STATUS_413_REQUEST_ENTITY_TOO_LARGE:
      AsciiPrint ("\n  Client Error: 413 Request Entity Too Large");
      break;

    case HTTP_STATUS_414_REQUEST_URI_TOO_LARGE:
      AsciiPrint ("\n  Client Error: 414 Request URI Too Long");
      break;

    case HTTP_STATUS_415_UNSUPPORTED_MEDIA_TYPE:
      AsciiPrint ("\n  Client Error: 415 Unsupported Media Type");
      break;

    case HTTP_STATUS_416_REQUESTED_RANGE_NOT_SATISFIED:
      AsciiPrint ("\n  Client Error: 416 Requested Range Not Satisfiable");
      break;

    case HTTP_STATUS_417_EXPECTATION_FAILED:
      AsciiPrint ("\n  Client Error: 417 Expectation Failed");
      break;

    case HTTP_STATUS_500_INTERNAL_SERVER_ERROR:
      AsciiPrint ("\n  Server Error: 500 Internal Server Error");
      break;

    case HTTP_STATUS_501_NOT_IMPLEMENTED:
      AsciiPrint ("\n  Server Error: 501 Not Implemented");
      break;

    case HTTP_STATUS_502_BAD_GATEWAY:
      AsciiPrint ("\n  Server Error: 502 Bad Gateway");
      break;

    case HTTP_STATUS_503_SERVICE_UNAVAILABLE:
      AsciiPrint ("\n  Server Error: 503 Service Unavailable");
      break;

    case HTTP_STATUS_504_GATEWAY_TIME_OUT:
      AsciiPrint ("\n  Server Error: 504 Gateway Timeout");
      break;

    case HTTP_STATUS_505_HTTP_VERSION_NOT_SUPPORTED:
      AsciiPrint ("\n  Server Error: 505 HTTP Version Not Supported");
      break;

    default:;
  }
}

/**
  Notify the callback function when an event is triggered.

  @param[in]  Event           The triggered event.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
HttpBootCommonNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  *((BOOLEAN *)Context) = TRUE;
}

/**
  Retrieve the host address using the EFI_DNS6_PROTOCOL.

  @param[in]  Private             The pointer to the driver's private data.
  @param[in]  HostName            Pointer to buffer containing hostname.
  @param[out] IpAddress           On output, pointer to buffer containing IPv6 address.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
HttpBootDns (
  IN     HTTP_BOOT_PRIVATE_DATA  *Private,
  IN     CHAR16                  *HostName,
  OUT EFI_IPv6_ADDRESS           *IpAddress
  )
{
  EFI_STATUS                 Status;
  EFI_DNS6_PROTOCOL          *Dns6;
  EFI_DNS6_CONFIG_DATA       Dns6ConfigData;
  EFI_DNS6_COMPLETION_TOKEN  Token;
  EFI_HANDLE                 Dns6Handle;
  EFI_IP6_CONFIG_PROTOCOL    *Ip6Config;
  EFI_IPv6_ADDRESS           *DnsServerList;
  UINTN                      DnsServerListCount;
  UINTN                      DataSize;
  BOOLEAN                    IsDone;

  DnsServerList      = NULL;
  DnsServerListCount = 0;
  Dns6               = NULL;
  Dns6Handle         = NULL;
  ZeroMem (&Token, sizeof (EFI_DNS6_COMPLETION_TOKEN));

  //
  // Get DNS server list from EFI IPv6 Configuration protocol.
  //
  Status = gBS->HandleProtocol (Private->Controller, &gEfiIp6ConfigProtocolGuid, (VOID **)&Ip6Config);
  if (!EFI_ERROR (Status)) {
    //
    // Get the required size.
    //
    DataSize = 0;
    Status   = Ip6Config->GetData (Ip6Config, Ip6ConfigDataTypeDnsServer, &DataSize, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      DnsServerList = AllocatePool (DataSize);
      if (DnsServerList == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = Ip6Config->GetData (Ip6Config, Ip6ConfigDataTypeDnsServer, &DataSize, DnsServerList);
      if (EFI_ERROR (Status)) {
        FreePool (DnsServerList);
        DnsServerList = NULL;
      } else {
        DnsServerListCount = DataSize / sizeof (EFI_IPv6_ADDRESS);
      }
    }
  }

  //
  // Create a DNSv6 child instance and get the protocol.
  //
  Status = NetLibCreateServiceChild (
             Private->Controller,
             Private->Ip6Nic->ImageHandle,
             &gEfiDns6ServiceBindingProtocolGuid,
             &Dns6Handle
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  Dns6Handle,
                  &gEfiDns6ProtocolGuid,
                  (VOID **)&Dns6,
                  Private->Ip6Nic->ImageHandle,
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
  Dns6ConfigData.DnsServerCount = (UINT32)DnsServerListCount;
  Dns6ConfigData.DnsServerList  = DnsServerList;
  Dns6ConfigData.EnableDnsCache = TRUE;
  Dns6ConfigData.Protocol       = EFI_IP_PROTO_UDP;
  IP6_COPY_ADDRESS (&Dns6ConfigData.StationIp, &Private->StationIp.v6);
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
                  HttpBootCommonNotify,
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

    if ((Token.RspData.H2AData->IpCount == 0) || (Token.RspData.H2AData->IpList == NULL)) {
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
           Private->Ip6Nic->ImageHandle,
           Private->Controller
           );
  }

  if (Dns6Handle != NULL) {
    NetLibDestroyServiceChild (
      Private->Controller,
      Private->Ip6Nic->ImageHandle,
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
  This function checks the HTTP(S) URI scheme.

  @param[in]    Uri              The pointer to the URI string.

  @retval EFI_SUCCESS            The URI scheme is valid.
  @retval EFI_INVALID_PARAMETER  The URI scheme is not HTTP or HTTPS.
  @retval EFI_ACCESS_DENIED      HTTP is disabled and the URI is HTTP.

**/
EFI_STATUS
HttpBootCheckUriScheme (
  IN      CHAR8  *Uri
  )
{
  UINTN       Index;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // Convert the scheme to all lower case.
  //
  for (Index = 0; Index < AsciiStrLen (Uri); Index++) {
    if (Uri[Index] == ':') {
      break;
    }

    if ((Uri[Index] >= 'A') && (Uri[Index] <= 'Z')) {
      Uri[Index] -= (CHAR8)('A' - 'a');
    }
  }

  //
  // Return EFI_INVALID_PARAMETER if the URI is not HTTP or HTTPS.
  //
  if ((AsciiStrnCmp (Uri, "http://", 7) != 0) && (AsciiStrnCmp (Uri, "https://", 8) != 0)) {
    DEBUG ((DEBUG_ERROR, "HttpBootCheckUriScheme: Invalid Uri.\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // HTTP is disabled, return EFI_ACCESS_DENIED if the URI is HTTP.
  //
  if (!PcdGetBool (PcdAllowHttpConnections) && (AsciiStrnCmp (Uri, "http://", 7) == 0)) {
    DEBUG ((DEBUG_ERROR, "HttpBootCheckUriScheme: HTTP is disabled.\n"));
    return EFI_ACCESS_DENIED;
  }

  return Status;
}

/**
  Get the URI address string from the URI device path node.

  Caller need to free the buffer in the Uri pointer.

  @param[in]   Node               Pointer to the URI device path node.
  @param[out]  Uri                URI string extracted from the device path.

  @retval EFI_SUCCESS            The URI string is returned.
  @retval EFI_INVALID_PARAMETER  Parameters are NULL or invalid URI node.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.

**/
EFI_STATUS
HttpBootUriFromDevicePath (
  IN    URI_DEVICE_PATH  *Node,
  OUT   CHAR8            **Uri
  )
{
  UINTN  UriStrLength;

  if ((Node == NULL) || (Uri == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  UriStrLength = DevicePathNodeLength (Node) - sizeof (EFI_DEVICE_PATH_PROTOCOL);

  if (UriStrLength == 0) {
    // Invalid URI, return.
    return EFI_INVALID_PARAMETER;
  }

  *Uri = AllocatePool (UriStrLength + 1);
  if (*Uri == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (*Uri, Node->Uri, UriStrLength);
  (*Uri)[UriStrLength] = '\0';

  return EFI_SUCCESS;
}

/**
  Get the URI address string from the input device path.

  Caller needs to free the buffers returned by this function.

  @param[in]   FilePath           Pointer to the device path which contains a URI device path node.
  @param[out]  ProxyUriAddress    The proxy URI address string extract from the device path (if it exists)
  @param[out]  EndPointUriAddress The endpoint URI address string for the endpoint host.

  @retval EFI_SUCCESS            The URI string is returned.
  @retval EFI_INVALID_PARAMETER  Parameters are NULL or device path is invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.

**/
EFI_STATUS
HttpBootParseFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT CHAR8                     **ProxyUriAddress,
  OUT CHAR8                     **EndPointUriAddress
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *Node[2];
  EFI_DEVICE_PATH_PROTOCOL  *TempNode;
  BOOLEAN                   NodeIsUri[2];
  UINTN                     Index;

  if ((FilePath == NULL) ||
      (ProxyUriAddress == NULL) ||
      (EndPointUriAddress == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *ProxyUriAddress    = NULL;
  *EndPointUriAddress = NULL;
  ZeroMem (Node, sizeof (Node));

  // Obtain last 2 device path nodes.
  // Looking for sequences:
  // 1) //....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv6(...)[/Dns(...)]/Uri(ProxyServer)/Uri(EndPointServer/FilePath)
  // 2) //....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv6(...)[/Dns(...)]/Uri(EndPointServer/FilePath)
  //
  // Expected:
  // Node[1] - Uri(EndPointServer/FilePath)
  // Node[0] - Either Uri(EndPointServer/FilePath) or other.
  TempNode = FilePath;

  while (!IsDevicePathEnd (TempNode)) {
    Node[0]  = Node[1];
    Node[1]  = TempNode;
    TempNode = NextDevicePathNode (TempNode);
  }

  // Verify if device path nodes are of type MESSAGING + URI.
  for (Index = 0; Index < 2; Index++) {
    if (Node[Index] == NULL) {
      NodeIsUri[Index] = FALSE;
    } else {
      NodeIsUri[Index] = ((DevicePathType (Node[Index]) == MESSAGING_DEVICE_PATH) &&
                          (DevicePathSubType (Node[Index]) == MSG_URI_DP));
    }
  }

  // If exists, obtain endpoint URI string.
  if (NodeIsUri[1]) {
    Status = HttpBootUriFromDevicePath (
               (URI_DEVICE_PATH *)Node[1],
               EndPointUriAddress
               );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    // If exists, obtain proxy URI string.
    if (NodeIsUri[0]) {
      Status = HttpBootUriFromDevicePath (
                 (URI_DEVICE_PATH *)Node[0],
                 ProxyUriAddress
                 );

      if (EFI_ERROR (Status)) {
        goto ErrorExit;
      }
    }
  }

  return EFI_SUCCESS;

ErrorExit:
  ASSERT (*EndPointUriAddress != NULL);
  FreePool (*EndPointUriAddress);
  *EndPointUriAddress = NULL;

  return Status;
}

/**
  This function returns the image type according to server replied HTTP message
  and also the image's URI info.

  @param[in]    Uri              The pointer to the image's URI string.
  @param[in]    UriParser        URI Parse result returned by NetHttpParseUrl().
  @param[in]    HeaderCount      Number of HTTP header structures in Headers list.
  @param[in]    Headers          Array containing list of HTTP headers.
  @param[out]   ImageType        The image type of the downloaded file.

  @retval EFI_SUCCESS            The image type is returned in ImageType.
  @retval EFI_INVALID_PARAMETER  ImageType, Uri or UriParser is NULL.
  @retval EFI_INVALID_PARAMETER  HeaderCount is not zero, and Headers is NULL.
  @retval EFI_NOT_FOUND          Failed to identify the image type.
  @retval Others                 Unexpected error happened.

**/
EFI_STATUS
HttpBootCheckImageType (
  IN      CHAR8              *Uri,
  IN      VOID               *UriParser,
  IN      UINTN              HeaderCount,
  IN      EFI_HTTP_HEADER    *Headers,
  OUT  HTTP_BOOT_IMAGE_TYPE  *ImageType
  )
{
  EFI_STATUS       Status;
  EFI_HTTP_HEADER  *Header;
  CHAR8            *FilePath;
  CHAR8            *FilePost;

  FilePath = NULL;  // MU_CHANGE - CodeQL Change - conditionallyuninitializedvariable

  if ((Uri == NULL) || (UriParser == NULL) || (ImageType == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((HeaderCount != 0) && (Headers == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the image type by the HTTP Content-Type header field first.
  //   "application/efi"         -> EFI Image
  //   "application/vnd.efi-iso" -> CD/DVD Image
  //   "application/vnd.efi-img" -> Virtual Disk Image
  //
  Header = HttpFindHeader (HeaderCount, Headers, HTTP_HEADER_CONTENT_TYPE);
  if (Header != NULL) {
    if (AsciiStriCmp (Header->FieldValue, HTTP_CONTENT_TYPE_APP_EFI) == 0) {
      *ImageType = ImageTypeEfi;
      return EFI_SUCCESS;
    } else if (AsciiStriCmp (Header->FieldValue, HTTP_CONTENT_TYPE_APP_ISO) == 0) {
      *ImageType = ImageTypeVirtualCd;
      return EFI_SUCCESS;
    } else if (AsciiStriCmp (Header->FieldValue, HTTP_CONTENT_TYPE_APP_IMG) == 0) {
      *ImageType = ImageTypeVirtualDisk;
      return EFI_SUCCESS;
    }
  }

  //
  // Determine the image type by file extension:
  //   *.efi -> EFI Image
  //   *.iso -> CD/DVD Image
  //   *.img -> Virtual Disk Image
  //
  Status = HttpUrlGetPath (
             Uri,
             UriParser,
             &FilePath
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FilePost = FilePath + AsciiStrLen (FilePath) - 4;
  if (AsciiStriCmp (FilePost, ".efi") == 0) {
    *ImageType = ImageTypeEfi;
  } else if (AsciiStriCmp (FilePost, ".iso") == 0) {
    *ImageType = ImageTypeVirtualCd;
  } else if (AsciiStriCmp (FilePost, ".img") == 0) {
    *ImageType = ImageTypeVirtualDisk;
  } else {
    *ImageType = ImageTypeMax;
  }

  FreePool (FilePath);

  return (*ImageType < ImageTypeMax) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  This function register the RAM disk info to the system.

  @param[in]       Private         The pointer to the driver's private data.
  @param[in]       BufferSize      The size of Buffer in bytes.
  @param[in]       Buffer          The base address of the RAM disk.
  @param[in]       ImageType       The image type of the file in Buffer.

  @retval EFI_SUCCESS              The RAM disk has been registered.
  @retval EFI_NOT_FOUND            No RAM disk protocol instances were found.
  @retval EFI_UNSUPPORTED          The ImageType is not supported.
  @retval Others                   Unexpected error happened.

**/
EFI_STATUS
HttpBootRegisterRamDisk (
  IN  HTTP_BOOT_PRIVATE_DATA  *Private,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer,
  IN  HTTP_BOOT_IMAGE_TYPE    ImageType
  )
{
  EFI_RAM_DISK_PROTOCOL     *RamDisk;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_GUID                  *RamDiskType;

  ASSERT (Private != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (BufferSize != 0);

  Status = gBS->LocateProtocol (&gEfiRamDiskProtocolGuid, NULL, (VOID **)&RamDisk);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HTTP Boot: Couldn't find the RAM Disk protocol - %r\n", Status));
    return Status;
  }

  if (ImageType == ImageTypeVirtualCd) {
    RamDiskType = &gEfiVirtualCdGuid;
  } else if (ImageType == ImageTypeVirtualDisk) {
    RamDiskType = &gEfiVirtualDiskGuid;
  } else {
    return EFI_UNSUPPORTED;
  }

  Status = RamDisk->Register (
                      (UINTN)Buffer,
                      (UINT64)BufferSize,
                      RamDiskType,
                      Private->UsingIpv6 ? Private->Ip6Nic->DevicePath : Private->Ip4Nic->DevicePath,
                      &DevicePath
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HTTP Boot: Failed to register RAM Disk - %r\n", Status));
  }

  return Status;
}

/**
  Indicate if the HTTP status code indicates a redirection.

  @param[in]  StatusCode      HTTP status code from server.

  @return                     TRUE if it's redirection.

**/
BOOLEAN
HttpBootIsHttpRedirectStatusCode (
  IN   EFI_HTTP_STATUS_CODE  StatusCode
  )
{
  if ((StatusCode == HTTP_STATUS_301_MOVED_PERMANENTLY) ||
      (StatusCode == HTTP_STATUS_302_FOUND) ||
      (StatusCode == HTTP_STATUS_307_TEMPORARY_REDIRECT) ||
      (StatusCode == HTTP_STATUS_308_PERMANENT_REDIRECT))
  {
    return TRUE;
  }

  return FALSE;
}
