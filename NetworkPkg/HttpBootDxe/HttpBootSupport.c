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
  Notify the callback function when an event is triggered.

  @param[in]  Event           The triggered event.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
HttpIoCommonNotify (
  IN EFI_EVENT           Event,
  IN VOID                *Context
  )
{
  *((BOOLEAN *) Context) = TRUE;
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
    ASSERT (FALSE);
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
                  HttpIoCommonNotify,
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
                  HttpIoCommonNotify,
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
  // Poll the network until transmit finish.
  //
  while (!HttpIo->IsRxDone) {
    Http->Poll (Http);
  }

  //
  // Store the received data into the wrapper.
  //
  Status = HttpIo->ReqToken.Status;
  if (!EFI_ERROR (Status)) {
    ResponseData->HeaderCount = HttpIo->RspToken.Message->HeaderCount;
    ResponseData->Headers     = HttpIo->RspToken.Message->Headers;
    ResponseData->BodyLength  = HttpIo->RspToken.Message->BodyLength;
  }

  return Status;
}
