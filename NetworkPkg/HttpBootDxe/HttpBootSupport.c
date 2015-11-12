/** @file
  Support functions implementation for UEFI HTTP boot driver.

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
  Get the Nic handle using any child handle in the IPv4 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv4.

  @return NicHandle               The pointer to the Nic handle.
  @return NULL                    Can't find the Nic handle.

**/
EFI_HANDLE
HttpBootGetNicByIp4Children (
  IN EFI_HANDLE                 ControllerHandle
  )
{
  EFI_HANDLE                    NicHandle;

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
  IN EFI_HANDLE                 ControllerHandle
  )
{
  EFI_HANDLE                    NicHandle;
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
  IN UINTN                       Number,
  IN UINT8                       *Buffer,
  IN INTN                        Length
  )
{
  UINTN                          Remainder;

  while (Length > 0) {
    Length--;
    Remainder      = Number % 10;
    Number        /= 10;
    Buffer[Length] = (UINT8) ('0' + Remainder);
  }
}

/**
  This function is to display the IPv4 address.

  @param[in]  Ip        The pointer to the IPv4 address.

**/
VOID
HttpBootShowIp4Addr (
  IN EFI_IPv4_ADDRESS   *Ip
  )
{
  UINTN                 Index;

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
  IN EFI_IPv6_ADDRESS   *Ip
  )
{
  UINTN                 Index;

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
  EFI_HTTP_STATUS_CODE            StatusCode
  )
{
  AsciiPrint ("\n");

  switch (StatusCode) {
  case HTTP_STATUS_300_MULTIPLE_CHIOCES:
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

  default: ;
  
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
  IN EFI_EVENT           Event,
  IN VOID                *Context
  )
{
  *((BOOLEAN *) Context) = TRUE;
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
  IN     HTTP_BOOT_PRIVATE_DATA   *Private,
  IN     CHAR16                   *HostName,
     OUT EFI_IPv6_ADDRESS         *IpAddress 
  )
{
  EFI_STATUS                      Status;
  EFI_DNS6_PROTOCOL               *Dns6;
  EFI_DNS6_CONFIG_DATA            Dns6ConfigData;
  EFI_DNS6_COMPLETION_TOKEN       Token;
  EFI_HANDLE                      Dns6Handle;
  EFI_IP6_CONFIG_PROTOCOL         *Ip6Config;
  EFI_IPv6_ADDRESS                *DnsServerList;
  UINTN                           DnsServerListCount;
  UINTN                           DataSize;
  BOOLEAN                         IsDone;  
  
  DnsServerList       = NULL;
  DnsServerListCount  = 0;
  Dns6                = NULL;
  Dns6Handle          = NULL;
  ZeroMem (&Token, sizeof (EFI_DNS6_COMPLETION_TOKEN));
  
  //
  // Get DNS server list from EFI IPv6 Configuration protocol.
  //
  Status = gBS->HandleProtocol (Private->Controller, &gEfiIp6ConfigProtocolGuid, (VOID **) &Ip6Config);
  if (!EFI_ERROR (Status)) {
    //
    // Get the required size.
    //
    DataSize = 0;
    Status = Ip6Config->GetData (Ip6Config, Ip6ConfigDataTypeDnsServer, &DataSize, NULL);
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
  Dns6ConfigData.DnsServerCount = (UINT32)DnsServerListCount;
  Dns6ConfigData.DnsServerList  = DnsServerList;
  Dns6ConfigData.EnableDnsCache = TRUE;
  Dns6ConfigData.Protocol       = EFI_IP_PROTO_UDP;
  IP6_COPY_ADDRESS (&Dns6ConfigData.StationIp,&Private->StationIp.v6);
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
  Create a HTTP_IO_HEADER to hold the HTTP header items.

  @param[in]  MaxHeaderCount         The maximun number of HTTP header in this holder.

  @return    A pointer of the HTTP header holder or NULL if failed.
  
**/
HTTP_IO_HEADER *
HttpBootCreateHeader (
  UINTN                     MaxHeaderCount
  )
{
  HTTP_IO_HEADER        *HttpIoHeader;

  if (MaxHeaderCount == 0) {
    return NULL;
  }

  HttpIoHeader = AllocateZeroPool (sizeof (HTTP_IO_HEADER) + MaxHeaderCount * sizeof (EFI_HTTP_HEADER));
  if (HttpIoHeader == NULL) {
    return NULL;
  }

  HttpIoHeader->MaxHeaderCount = MaxHeaderCount;
  HttpIoHeader->Headers = (EFI_HTTP_HEADER *) (HttpIoHeader + 1);

  return HttpIoHeader;
}

/**
  Destroy the HTTP_IO_HEADER and release the resouces. 

  @param[in]  HttpIoHeader       Point to the HTTP header holder to be destroyed.

**/
VOID
HttpBootFreeHeader (
  IN  HTTP_IO_HEADER       *HttpIoHeader
  )
{
  UINTN      Index;
  
  if (HttpIoHeader != NULL) {
    if (HttpIoHeader->HeaderCount != 0) {
      for (Index = 0; Index < HttpIoHeader->HeaderCount; Index++) {
        FreePool (HttpIoHeader->Headers[Index].FieldName);
        FreePool (HttpIoHeader->Headers[Index].FieldValue);
      }
    }
    FreePool (HttpIoHeader);
  }
}

/**
  Find a specified header field according to the field name.

  @param[in]   HeaderCount      Number of HTTP header structures in Headers list. 
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[in]   FieldName        Null terminated string which describes a field name. 

  @return    Pointer to the found header or NULL.

**/
EFI_HTTP_HEADER *
HttpBootFindHeader (
  IN  UINTN                HeaderCount,
  IN  EFI_HTTP_HEADER      *Headers,
  IN  CHAR8                *FieldName
  )
{
  UINTN                 Index;
  
  if (HeaderCount == 0 || Headers == NULL || FieldName == NULL) {
    return NULL;
  }

  for (Index = 0; Index < HeaderCount; Index++){
    //
    // Field names are case-insensitive (RFC 2616).
    //
    if (AsciiStriCmp (Headers[Index].FieldName, FieldName) == 0) {
      return &Headers[Index];
    }
  }
  return NULL;
}

/**
  Set or update a HTTP header with the field name and corresponding value.

  @param[in]  HttpIoHeader       Point to the HTTP header holder.
  @param[in]  FieldName          Null terminated string which describes a field name. 
  @param[in]  FieldValue         Null terminated string which describes the corresponding field value.

  @retval  EFI_SUCCESS           The HTTP header has been set or updated.
  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES  Insufficient resource to complete the operation.
  @retval  Other                 Unexpected error happened.
  
**/
EFI_STATUS
HttpBootSetHeader (
  IN  HTTP_IO_HEADER       *HttpIoHeader,
  IN  CHAR8                *FieldName,
  IN  CHAR8                *FieldValue
  )
{
  EFI_HTTP_HEADER       *Header;
  UINTN                 StrSize;
  CHAR8                 *NewFieldValue;
  
  if (HttpIoHeader == NULL || FieldName == NULL || FieldValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Header = HttpBootFindHeader (HttpIoHeader->HeaderCount, HttpIoHeader->Headers, FieldName);
  if (Header == NULL) {
    //
    // Add a new header.
    //
    if (HttpIoHeader->HeaderCount >= HttpIoHeader->MaxHeaderCount) {
      return EFI_OUT_OF_RESOURCES;
    }
    Header = &HttpIoHeader->Headers[HttpIoHeader->HeaderCount];

    StrSize = AsciiStrSize (FieldName);
    Header->FieldName = AllocatePool (StrSize);
    if (Header->FieldName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (Header->FieldName, FieldName, StrSize);
    Header->FieldName[StrSize -1] = '\0';

    StrSize = AsciiStrSize (FieldValue);
    Header->FieldValue = AllocatePool (StrSize);
    if (Header->FieldValue == NULL) {
      FreePool (Header->FieldName);
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (Header->FieldValue, FieldValue, StrSize);
    Header->FieldValue[StrSize -1] = '\0';

    HttpIoHeader->HeaderCount++;
  } else {
    //
    // Update an existing one.
    //
    StrSize = AsciiStrSize (FieldValue);
    NewFieldValue = AllocatePool (StrSize);
    if (NewFieldValue == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (NewFieldValue, FieldValue, StrSize);
    NewFieldValue[StrSize -1] = '\0';
    
    if (Header->FieldValue != NULL) {
      FreePool (Header->FieldValue);
    }
    Header->FieldValue = NewFieldValue;
  }

  return EFI_SUCCESS;
}

/**
  Create a HTTP_IO to access the HTTP service. It will create and configure
  a HTTP child handle.

  @param[in]  Image          The handle of the driver image.
  @param[in]  Controller     The handle of the controller.
  @param[in]  IpVersion      IP_VERSION_4 or IP_VERSION_6.
  @param[in]  ConfigData     The HTTP_IO configuration data.
  @param[out] HttpIo         The HTTP_IO.
  
  @retval EFI_SUCCESS            The HTTP_IO is created and configured.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Failed to create the HTTP_IO or configure it.

**/
EFI_STATUS
HttpIoCreateIo (
  IN EFI_HANDLE             Image,
  IN EFI_HANDLE             Controller,
  IN UINT8                  IpVersion,
  IN HTTP_IO_CONFIG_DATA    *ConfigData,
  OUT HTTP_IO               *HttpIo
  )
{
  EFI_STATUS                Status;
  EFI_HTTP_CONFIG_DATA      HttpConfigData;
  EFI_HTTPv4_ACCESS_POINT   Http4AccessPoint;
  EFI_HTTPv6_ACCESS_POINT   Http6AccessPoint;
  EFI_HTTP_PROTOCOL         *Http;
  EFI_EVENT                 Event;
  
  if ((Image == NULL) || (Controller == NULL) || (ConfigData == NULL) || (HttpIo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IpVersion != IP_VERSION_4 && IpVersion != IP_VERSION_6) {
    return EFI_UNSUPPORTED;
  }

  ZeroMem (HttpIo, sizeof (HTTP_IO));
  
  //
  // Create the HTTP child instance and get the HTTP protocol.
  //  
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             &gEfiHttpServiceBindingProtocolGuid,
             &HttpIo->Handle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  HttpIo->Handle,
                  &gEfiHttpProtocolGuid,
                  (VOID **) &Http,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) || (Http == NULL)) {
    goto ON_ERROR;
  }

  //
  // Init the configuration data and configure the HTTP child.
  //
  HttpIo->Image       = Image;
  HttpIo->Controller  = Controller;
  HttpIo->IpVersion   = IpVersion;
  HttpIo->Http        = Http;

  ZeroMem (&HttpConfigData, sizeof (EFI_HTTP_CONFIG_DATA));
  HttpConfigData.HttpVersion        = HttpVersion11;
  HttpConfigData.TimeOutMillisec    = ConfigData->Config4.RequestTimeOut;
  if (HttpIo->IpVersion == IP_VERSION_4) {
    HttpConfigData.LocalAddressIsIPv6 = FALSE;
    
    Http4AccessPoint.UseDefaultAddress = ConfigData->Config4.UseDefaultAddress;
    Http4AccessPoint.LocalPort         = ConfigData->Config4.LocalPort;
    IP4_COPY_ADDRESS (&Http4AccessPoint.LocalAddress, &ConfigData->Config4.LocalIp);
    IP4_COPY_ADDRESS (&Http4AccessPoint.LocalSubnet, &ConfigData->Config4.SubnetMask);
    HttpConfigData.AccessPoint.IPv4Node = &Http4AccessPoint;   
  } else {
    HttpConfigData.LocalAddressIsIPv6 = TRUE;
    Http6AccessPoint.LocalPort        = ConfigData->Config6.LocalPort;
    IP6_COPY_ADDRESS (&Http6AccessPoint.LocalAddress, &ConfigData->Config6.LocalIp);
    HttpConfigData.AccessPoint.IPv6Node = &Http6AccessPoint;
  }
  
  Status = Http->Configure (Http, &HttpConfigData);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Create events for variuos asynchronous operations.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpBootCommonNotify,
                  &HttpIo->IsTxDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  HttpIo->ReqToken.Event = Event;
  HttpIo->ReqToken.Message = &HttpIo->ReqMessage;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpBootCommonNotify,
                  &HttpIo->IsRxDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  HttpIo->RspToken.Event = Event;
  HttpIo->RspToken.Message = &HttpIo->RspMessage;

  return EFI_SUCCESS;
  
ON_ERROR:
  HttpIoDestroyIo (HttpIo);

  return Status;
}

/**
  Destroy the HTTP_IO and release the resouces. 

  @param[in]  HttpIo          The HTTP_IO which wraps the HTTP service to be destroyed.

**/
VOID
HttpIoDestroyIo (
  IN HTTP_IO                *HttpIo
  )
{
  EFI_HTTP_PROTOCOL         *Http;
  EFI_EVENT                 Event;

  if (HttpIo == NULL) {
    return;
  }

  Event = HttpIo->ReqToken.Event;
  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  Event = HttpIo->RspToken.Event;
  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }
  
  Http = HttpIo->Http;
  if (Http != NULL) {
    Http->Configure (Http, NULL);
    gBS->CloseProtocol (
           HttpIo->Handle,
           &gEfiHttpProtocolGuid,
           HttpIo->Image,
           HttpIo->Controller
           );
  }

  NetLibDestroyServiceChild (
    HttpIo->Controller,
    HttpIo->Image,
    &gEfiHttpServiceBindingProtocolGuid,
    HttpIo->Handle
    );
}

/**
  Synchronously send a HTTP REQUEST message to the server.
  
  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   Request          A pointer to storage such data as URL and HTTP method.
  @param[in]   HeaderCount      Number of HTTP header structures in Headers list. 
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[in]   BodyLength       Length in bytes of the HTTP body.
  @param[in]   Body             Body associated with the HTTP request. 
  
  @retval EFI_SUCCESS            The HTTP request is trasmitted.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoSendRequest (
  IN  HTTP_IO                *HttpIo,
  IN  EFI_HTTP_REQUEST_DATA  *Request,
  IN  UINTN                  HeaderCount,
  IN  EFI_HTTP_HEADER        *Headers,
  IN  UINTN                  BodyLength,
  IN  VOID                   *Body
  )
{
  EFI_STATUS                 Status;
  EFI_HTTP_PROTOCOL          *Http;

  if (HttpIo == NULL || HttpIo->Http == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpIo->ReqToken.Status  = EFI_NOT_READY;
  HttpIo->ReqToken.Message->Data.Request = Request;
  HttpIo->ReqToken.Message->HeaderCount  = HeaderCount;
  HttpIo->ReqToken.Message->Headers      = Headers;
  HttpIo->ReqToken.Message->BodyLength   = BodyLength;
  HttpIo->ReqToken.Message->Body         = Body;

  //
  // Queue the request token to HTTP instances.
  //
  Http = HttpIo->Http;
  HttpIo->IsTxDone = FALSE;
  Status = Http->Request (
                   Http,
                   &HttpIo->ReqToken
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Poll the network until transmit finish.
  //
  while (!HttpIo->IsTxDone) {
    Http->Poll (Http);
  }

  return HttpIo->ReqToken.Status;
}

/**
  Synchronously receive a HTTP RESPONSE message from the server.
  
  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   RecvMsgHeader    TRUE to receive a new HTTP response (from message header).
                                FALSE to continue receive the previous response message.
  @param[out]  ResponseData     Point to a wrapper of the received response data.
  
  @retval EFI_SUCCESS            The HTTP resopnse is received.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoRecvResponse (
  IN      HTTP_IO                  *HttpIo,
  IN      BOOLEAN                  RecvMsgHeader,
     OUT  HTTP_IO_RESOPNSE_DATA    *ResponseData
  )
{
  EFI_STATUS                 Status;
  EFI_HTTP_PROTOCOL          *Http;
  EFI_HTTP_STATUS_CODE       StatusCode;

  if (HttpIo == NULL || HttpIo->Http == NULL || ResponseData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Queue the response token to HTTP instances.
  //
  HttpIo->RspToken.Status  = EFI_NOT_READY;
  if (RecvMsgHeader) {
    HttpIo->RspToken.Message->Data.Response = &ResponseData->Response;
  } else {
    HttpIo->RspToken.Message->Data.Response = NULL;
  }
  HttpIo->RspToken.Message->HeaderCount   = 0;
  HttpIo->RspToken.Message->Headers       = NULL;
  HttpIo->RspToken.Message->BodyLength    = ResponseData->BodyLength;
  HttpIo->RspToken.Message->Body          = ResponseData->Body;

  Http = HttpIo->Http;
  HttpIo->IsRxDone = FALSE;
  Status = Http->Response (
                   Http,
                   &HttpIo->RspToken
                   );
  
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Poll the network until receive finish.
  //
  while (!HttpIo->IsRxDone) {
    Http->Poll (Http);
  }

  //
  // Store the received data into the wrapper.
  //
  Status = HttpIo->RspToken.Status;
  if (!EFI_ERROR (Status)) {
    ResponseData->HeaderCount = HttpIo->RspToken.Message->HeaderCount;
    ResponseData->Headers     = HttpIo->RspToken.Message->Headers;
    ResponseData->BodyLength  = HttpIo->RspToken.Message->BodyLength;
  }
  
  if (RecvMsgHeader) {
    StatusCode = HttpIo->RspToken.Message->Data.Response->StatusCode;
    HttpBootPrintErrorMessage (StatusCode);
  }

  return Status;
}
