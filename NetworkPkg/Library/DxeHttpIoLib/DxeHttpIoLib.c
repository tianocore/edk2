/** @file
  Http IO Helper Library.

  (C) Copyright 2020 Hewlett-Packard Development Company, L.P.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Protocol/Http.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HttpIoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Notify the callback function when an event is triggered.

  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
HttpIoNotifyDpc (
  IN VOID  *Context
  )
{
  *((BOOLEAN *)Context) = TRUE;
}

/**
  Request HttpIoNotifyDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                 The event signaled.
  @param[in]  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
HttpIoNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Request HttpIoNotifyDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, HttpIoNotifyDpc, Context);
}

/**
  Destroy the HTTP_IO and release the resources.

  @param[in]  HttpIo          The HTTP_IO which wraps the HTTP service to be destroyed.

**/
VOID
HttpIoDestroyIo (
  IN HTTP_IO  *HttpIo
  )
{
  EFI_HTTP_PROTOCOL  *Http;
  EFI_EVENT          Event;

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

  Event = HttpIo->TimeoutEvent;
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
  Create a HTTP_IO to access the HTTP service. It will create and configure
  a HTTP child handle.

  @param[in]  Image          The handle of the driver image.
  @param[in]  Controller     The handle of the controller.
  @param[in]  IpVersion      IP_VERSION_4 or IP_VERSION_6.
  @param[in]  ConfigData     The HTTP_IO configuration data ,
                             NULL means not to configure the HTTP child.
  @param[in]  Callback       Callback function which will be invoked when specified
                             HTTP_IO_CALLBACK_EVENT happened.
  @param[in]  Context        The Context data which will be passed to the Callback function.
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
  IN EFI_HANDLE           Image,
  IN EFI_HANDLE           Controller,
  IN UINT8                IpVersion,
  IN HTTP_IO_CONFIG_DATA  *ConfigData  OPTIONAL,
  IN HTTP_IO_CALLBACK     Callback,
  IN VOID                 *Context,
  OUT HTTP_IO             *HttpIo
  )
{
  EFI_STATUS               Status;
  EFI_HTTP_CONFIG_DATA     HttpConfigData;
  EFI_HTTPv4_ACCESS_POINT  Http4AccessPoint;
  EFI_HTTPv6_ACCESS_POINT  Http6AccessPoint;
  EFI_HTTP_PROTOCOL        *Http;
  EFI_EVENT                Event;

  if ((Image == NULL) || (Controller == NULL) || (HttpIo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((IpVersion != IP_VERSION_4) && (IpVersion != IP_VERSION_6)) {
    return EFI_UNSUPPORTED;
  }

  ZeroMem (HttpIo, sizeof (HTTP_IO));
  ZeroMem (&HttpConfigData, sizeof (EFI_HTTP_CONFIG_DATA));

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
                  (VOID **)&Http,
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
  HttpIo->Image      = Image;
  HttpIo->Controller = Controller;
  HttpIo->IpVersion  = IpVersion;
  HttpIo->Http       = Http;
  HttpIo->Callback   = Callback;
  HttpIo->Context    = Context;
  HttpIo->Timeout    = PcdGet32 (PcdHttpIoTimeout);

  if (ConfigData != NULL) {
    if (HttpIo->IpVersion == IP_VERSION_4) {
      HttpConfigData.LocalAddressIsIPv6 = FALSE;
      HttpConfigData.HttpVersion        = ConfigData->Config4.HttpVersion;
      HttpConfigData.TimeOutMillisec    = ConfigData->Config4.RequestTimeOut;

      Http4AccessPoint.UseDefaultAddress = ConfigData->Config4.UseDefaultAddress;
      Http4AccessPoint.LocalPort         = ConfigData->Config4.LocalPort;
      IP4_COPY_ADDRESS (&Http4AccessPoint.LocalAddress, &ConfigData->Config4.LocalIp);
      IP4_COPY_ADDRESS (&Http4AccessPoint.LocalSubnet, &ConfigData->Config4.SubnetMask);
      HttpConfigData.AccessPoint.IPv4Node = &Http4AccessPoint;
    } else {
      HttpConfigData.LocalAddressIsIPv6 = TRUE;
      HttpConfigData.HttpVersion        = ConfigData->Config6.HttpVersion;
      HttpConfigData.TimeOutMillisec    = ConfigData->Config6.RequestTimeOut;

      Http6AccessPoint.LocalPort = ConfigData->Config6.LocalPort;
      IP6_COPY_ADDRESS (&Http6AccessPoint.LocalAddress, &ConfigData->Config6.LocalIp);
      HttpConfigData.AccessPoint.IPv6Node = &Http6AccessPoint;
    }

    Status = Http->Configure (Http, &HttpConfigData);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  //
  // Create events for variuos asynchronous operations.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpIoNotify,
                  &HttpIo->IsTxDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  HttpIo->ReqToken.Event   = Event;
  HttpIo->ReqToken.Message = &HttpIo->ReqMessage;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpIoNotify,
                  &HttpIo->IsRxDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  HttpIo->RspToken.Event   = Event;
  HttpIo->RspToken.Message = &HttpIo->RspMessage;

  //
  // Create TimeoutEvent for response
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  HttpIo->TimeoutEvent = Event;
  return EFI_SUCCESS;

ON_ERROR:
  HttpIoDestroyIo (HttpIo);

  return Status;
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
  EFI_STATUS         Status;
  EFI_HTTP_PROTOCOL  *Http;

  if ((HttpIo == NULL) || (HttpIo->Http == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HttpIo->ReqToken.Status                = EFI_NOT_READY;
  HttpIo->ReqToken.Message->Data.Request = Request;
  HttpIo->ReqToken.Message->HeaderCount  = HeaderCount;
  HttpIo->ReqToken.Message->Headers      = Headers;
  HttpIo->ReqToken.Message->BodyLength   = BodyLength;
  HttpIo->ReqToken.Message->Body         = Body;

  if (HttpIo->Callback != NULL) {
    Status = HttpIo->Callback (
                       HttpIoRequest,
                       HttpIo->ReqToken.Message,
                       HttpIo->Context
                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Queue the request token to HTTP instances.
  //
  Http             = HttpIo->Http;
  HttpIo->IsTxDone = FALSE;
  Status           = Http->Request (
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

  @retval EFI_SUCCESS            The HTTP response is received.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoRecvResponse (
  IN      HTTP_IO                *HttpIo,
  IN      BOOLEAN                RecvMsgHeader,
  OUT     HTTP_IO_RESPONSE_DATA  *ResponseData
  )
{
  EFI_STATUS         Status;
  EFI_HTTP_PROTOCOL  *Http;

  if ((HttpIo == NULL) || (HttpIo->Http == NULL) || (ResponseData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Queue the response token to HTTP instances.
  //
  HttpIo->RspToken.Status = EFI_NOT_READY;
  if (RecvMsgHeader) {
    HttpIo->RspToken.Message->Data.Response = &ResponseData->Response;
  } else {
    HttpIo->RspToken.Message->Data.Response = NULL;
  }

  HttpIo->RspToken.Message->HeaderCount = 0;
  HttpIo->RspToken.Message->Headers     = NULL;
  HttpIo->RspToken.Message->BodyLength  = ResponseData->BodyLength;
  HttpIo->RspToken.Message->Body        = ResponseData->Body;

  Http             = HttpIo->Http;
  HttpIo->IsRxDone = FALSE;

  //
  // Start the timer, and wait Timeout seconds to receive the header packet.
  //
  Status = gBS->SetTimer (HttpIo->TimeoutEvent, TimerRelative, HttpIo->Timeout * TICKS_PER_MS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Http->Response (
                   Http,
                   &HttpIo->RspToken
                   );

  if (EFI_ERROR (Status)) {
    //
    // Remove timeout timer from the event list.
    //
    gBS->SetTimer (HttpIo->TimeoutEvent, TimerCancel, 0);
    return Status;
  }

  //
  // Poll the network until receive finish.
  //
  while (!HttpIo->IsRxDone && EFI_ERROR (gBS->CheckEvent (HttpIo->TimeoutEvent))) {
    Http->Poll (Http);
  }

  //
  // Remove timeout timer from the event list.
  //
  gBS->SetTimer (HttpIo->TimeoutEvent, TimerCancel, 0);

  if (!HttpIo->IsRxDone) {
    //
    // Timeout occurs, cancel the response token.
    //
    Http->Cancel (Http, &HttpIo->RspToken);

    Status = EFI_TIMEOUT;

    return Status;
  } else {
    HttpIo->IsRxDone = FALSE;
  }

  if ((HttpIo->Callback != NULL) &&
      ((HttpIo->RspToken.Status == EFI_SUCCESS) || (HttpIo->RspToken.Status == EFI_HTTP_ERROR)))
  {
    Status = HttpIo->Callback (
                       HttpIoResponse,
                       HttpIo->RspToken.Message,
                       HttpIo->Context
                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Store the received data into the wrapper.
  //
  ResponseData->Status      = HttpIo->RspToken.Status;
  ResponseData->HeaderCount = HttpIo->RspToken.Message->HeaderCount;
  ResponseData->Headers     = HttpIo->RspToken.Message->Headers;
  ResponseData->BodyLength  = HttpIo->RspToken.Message->BodyLength;

  return Status;
}

/**
  Get the value of the content length if there is a "Content-Length" header.

  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.
  @param[out]   ContentLength      Pointer to save the value of the content length.

  @retval EFI_SUCCESS              Successfully get the content length.
  @retval EFI_NOT_FOUND            No "Content-Length" header in the Headers.

**/
EFI_STATUS
HttpIoGetContentLength (
  IN     UINTN            HeaderCount,
  IN     EFI_HTTP_HEADER  *Headers,
  OUT    UINTN            *ContentLength
  )
{
  EFI_HTTP_HEADER  *Header;

  Header = HttpFindHeader (HeaderCount, Headers, HTTP_HEADER_CONTENT_LENGTH);
  if (Header == NULL) {
    return EFI_NOT_FOUND;
  }

  return AsciiStrDecimalToUintnS (Header->FieldValue, (CHAR8 **)NULL, ContentLength);
}

/**
  Send HTTP request in chunks.

  @param[in]   HttpIo             The HttpIo wrapping the HTTP service.
  @param[in]   SendChunkProcess   Pointer to current chunk process status.
  @param[in]   RequestMessage     Request to send.

  @retval EFI_SUCCESS             Successfully to send chunk data according to SendChunkProcess.
  @retval Other                   Other errors.

**/
EFI_STATUS
HttpIoSendChunkedTransfer (
  IN  HTTP_IO                     *HttpIo,
  IN  HTTP_IO_SEND_CHUNK_PROCESS  *SendChunkProcess,
  IN  EFI_HTTP_MESSAGE            *RequestMessage
  )
{
  EFI_STATUS             Status;
  EFI_HTTP_HEADER        *NewHeaders;
  EFI_HTTP_HEADER        *ContentLengthHeader;
  UINTN                  AddNewHeader;
  UINTN                  HeaderCount;
  CHAR8                  *MessageBody;
  UINTN                  MessageBodyLength;
  UINTN                  ChunkLength;
  CHAR8                  ChunkLengthStr[HTTP_IO_CHUNK_SIZE_STRING_LEN];
  EFI_HTTP_REQUEST_DATA  *SentRequestData;

  AddNewHeader        = 0;
  NewHeaders          = NULL;
  MessageBody         = NULL;
  ContentLengthHeader = NULL;
  MessageBodyLength   = 0;

  switch (*SendChunkProcess) {
    case HttpIoSendChunkHeaderZeroContent:
      ContentLengthHeader = HttpFindHeader (RequestMessage->HeaderCount, RequestMessage->Headers, HTTP_HEADER_CONTENT_LENGTH);
      if (ContentLengthHeader == NULL) {
        AddNewHeader = 1;
      }

      NewHeaders = AllocateZeroPool ((RequestMessage->HeaderCount + AddNewHeader) * sizeof (EFI_HTTP_HEADER));
      CopyMem ((VOID *)NewHeaders, (VOID *)RequestMessage->Headers, RequestMessage->HeaderCount * sizeof (EFI_HTTP_HEADER));
      if (AddNewHeader == 0) {
        //
        // Override content-length to Transfer-Encoding.
        //
        ContentLengthHeader             = HttpFindHeader (RequestMessage->HeaderCount, NewHeaders, HTTP_HEADER_CONTENT_LENGTH);
        ContentLengthHeader->FieldName  = NULL;
        ContentLengthHeader->FieldValue = NULL;
      } else {
        ContentLengthHeader = NewHeaders + RequestMessage->HeaderCount;
      }

      HttpSetFieldNameAndValue (ContentLengthHeader, HTTP_HEADER_TRANSFER_ENCODING, HTTP_HEADER_TRANSFER_ENCODING_CHUNKED);
      HeaderCount       = RequestMessage->HeaderCount + AddNewHeader;
      MessageBodyLength = 0;
      MessageBody       = NULL;
      SentRequestData   = RequestMessage->Data.Request;
      break;

    case HttpIoSendChunkContent:
      HeaderCount     = 0;
      NewHeaders      = NULL;
      SentRequestData = NULL;
      if (RequestMessage->BodyLength > HTTP_IO_MAX_SEND_PAYLOAD) {
        MessageBodyLength = HTTP_IO_MAX_SEND_PAYLOAD;
      } else {
        MessageBodyLength = RequestMessage->BodyLength;
      }

      AsciiSPrint (
        ChunkLengthStr,
        HTTP_IO_CHUNK_SIZE_STRING_LEN,
        "%x%c%c",
        MessageBodyLength,
        CHUNKED_TRANSFER_CODING_CR,
        CHUNKED_TRANSFER_CODING_LF
        );
      ChunkLength = AsciiStrLen (ChunkLengthStr);
      MessageBody = AllocatePool (ChunkLength + MessageBodyLength + 2);
      if (MessageBody == NULL) {
        DEBUG ((DEBUG_ERROR, "Not enough memory for chunk transfer\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Build up the chunk transfer paylaod.
      //
      CopyMem (MessageBody, ChunkLengthStr, ChunkLength);
      CopyMem (MessageBody + ChunkLength, RequestMessage->Body, MessageBodyLength);
      *(MessageBody + ChunkLength + MessageBodyLength)     = CHUNKED_TRANSFER_CODING_CR;
      *(MessageBody + ChunkLength + MessageBodyLength + 1) = CHUNKED_TRANSFER_CODING_LF;
      //
      // Change variables for the next chunk trasnfer.
      //
      RequestMessage->BodyLength -= MessageBodyLength;
      RequestMessage->Body        = (VOID *)((CHAR8 *)RequestMessage->Body + MessageBodyLength);
      MessageBodyLength          += (ChunkLength + 2);
      if (RequestMessage->BodyLength == 0) {
        *SendChunkProcess = HttpIoSendChunkEndChunk;
      }

      break;

    case HttpIoSendChunkEndChunk:
      HeaderCount     = 0;
      NewHeaders      = NULL;
      SentRequestData = NULL;
      AsciiSPrint (
        ChunkLengthStr,
        HTTP_IO_CHUNK_SIZE_STRING_LEN,
        "0%c%c%c%c",
        CHUNKED_TRANSFER_CODING_CR,
        CHUNKED_TRANSFER_CODING_LF,
        CHUNKED_TRANSFER_CODING_CR,
        CHUNKED_TRANSFER_CODING_LF
        );
      MessageBody = AllocatePool (AsciiStrLen (ChunkLengthStr));
      if (MessageBody == NULL) {
        DEBUG ((DEBUG_ERROR, "Not enough memory for the end chunk transfer\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (MessageBody, ChunkLengthStr, AsciiStrLen (ChunkLengthStr));
      MessageBodyLength = AsciiStrLen (ChunkLengthStr);
      *SendChunkProcess = HttpIoSendChunkFinish;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  Status = HttpIoSendRequest (
             HttpIo,
             SentRequestData,
             HeaderCount,
             NewHeaders,
             MessageBodyLength,
             MessageBody
             );
  if (ContentLengthHeader != NULL) {
    if (ContentLengthHeader->FieldName != NULL) {
      FreePool (ContentLengthHeader->FieldName);
    }

    if (ContentLengthHeader->FieldValue != NULL) {
      FreePool (ContentLengthHeader->FieldValue);
    }
  }

  if (NewHeaders != NULL) {
    FreePool (NewHeaders);
  }

  if (MessageBody != NULL) {
    FreePool (MessageBody);
  }

  return Status;
}

/**
  Synchronously receive a HTTP RESPONSE message from the server.

  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   HeaderCount      Number of headers in Headers.
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[out]  ChunkListHead    A pointer to receive list head
                                of chunked data. Caller has to
                                release memory of ChunkListHead
                                and all list entries.
  @param[out]  ContentLength    Total content length

  @retval EFI_SUCCESS            The HTTP chunked transfer is received.
  @retval EFI_NOT_FOUND          No chunked transfer coding header found.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_INVALID_PARAMETER  Improper parameters.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoGetChunkedTransferContent (
  IN     HTTP_IO          *HttpIo,
  IN     UINTN            HeaderCount,
  IN     EFI_HTTP_HEADER  *Headers,
  OUT    LIST_ENTRY       **ChunkListHead,
  OUT    UINTN            *ContentLength
  )
{
  EFI_HTTP_HEADER        *Header;
  CHAR8                  ChunkSizeAscii[256];
  EFI_STATUS             Status;
  UINTN                  Index;
  HTTP_IO_RESPONSE_DATA  ResponseData;
  UINTN                  TotalLength;
  UINTN                  MaxTotalLength;
  LIST_ENTRY             *HttpChunks;
  HTTP_IO_CHUNKS         *ThisChunk;
  LIST_ENTRY             *ThisListEntry;

  if ((ChunkListHead == NULL) || (ContentLength == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *ContentLength = 0;
  Header         = HttpFindHeader (HeaderCount, Headers, HTTP_HEADER_TRANSFER_ENCODING);
  if (Header == NULL) {
    return EFI_NOT_FOUND;
  }

  if (AsciiStrCmp (Header->FieldValue, HTTP_HEADER_TRANSFER_ENCODING_CHUNKED) != 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Loop to get all chunks.
  //
  TotalLength    = 0;
  MaxTotalLength = PcdGet32 (PcdMaxHttpChunkTransfer);
  HttpChunks     = (LIST_ENTRY *)AllocateZeroPool (sizeof (LIST_ENTRY));
  if (HttpChunks == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitDeleteChunks;
  }

  InitializeListHead (HttpChunks);
  DEBUG ((DEBUG_INFO, "     Chunked transfer\n"));
  while (TRUE) {
    ZeroMem ((VOID *)&ResponseData, sizeof (HTTP_IO_RESPONSE_DATA));
    ResponseData.BodyLength = HTTP_IO_CHUNKED_TRANSFER_CODING_DATA_LENGTH;
    ResponseData.Body       = ChunkSizeAscii;
    Status                  = HttpIoRecvResponse (
                                HttpIo,
                                FALSE,
                                &ResponseData
                                );
    if (EFI_ERROR (Status)) {
      goto ExitDeleteChunks;
    }

    //
    // Decoding Chunked Transfer Coding.
    // Only decode chunk-size and last chunk.
    //
    DEBUG ((DEBUG_INFO, "     Chunk HTTP Response StatusCode - %d\n", ResponseData.Response.StatusCode));
    //
    // Break if this is last chunk.
    //
    if (ChunkSizeAscii[0] == CHUNKED_TRANSFER_CODING_LAST_CHUNK) {
      //
      // Check if this is a valid Last-Chunk.
      //
      if ((ChunkSizeAscii[1] != CHUNKED_TRANSFER_CODING_CR) ||
          (ChunkSizeAscii[2] != CHUNKED_TRANSFER_CODING_LF)
          )
      {
        DEBUG ((DEBUG_ERROR, "     This is an invalid Last-chunk\n"));
        Status = EFI_INVALID_PARAMETER;
        goto ExitDeleteChunks;
      }

      Status = EFI_SUCCESS;
      DEBUG ((DEBUG_INFO, "     Last-chunk\n"));
      ThisChunk = (HTTP_IO_CHUNKS *)AllocateZeroPool (sizeof (HTTP_IO_CHUNKS));
      if (ThisChunk == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ExitDeleteChunks;
      }

      InitializeListHead (&ThisChunk->NextChunk);
      ThisChunk->Length = ResponseData.BodyLength - 1 - 2; // Minus sizeof '0' and CRLF.
      ThisChunk->Data   = (CHAR8 *)AllocatePool (ThisChunk->Length);
      if (ThisChunk->Data == NULL) {
        FreePool ((UINT8 *)ThisChunk);
        Status = EFI_OUT_OF_RESOURCES;
        goto ExitDeleteChunks;
      }

      CopyMem ((UINT8 *)ThisChunk->Data, (UINT8 *)ResponseData.Body + 1, ThisChunk->Length);
      TotalLength += ThisChunk->Length;
      InsertTailList (HttpChunks, &ThisChunk->NextChunk);
      break;
    }

    //
    // Get the chunk length
    //
    Index = 0;
    while ((ChunkSizeAscii[Index] != CHUNKED_TRANSFER_CODING_EXTENSION_SEPARATOR) &&
           (ChunkSizeAscii[Index] != (CHAR8)CHUNKED_TRANSFER_CODING_CR) &&
           (Index != HTTP_IO_CHUNKED_TRANSFER_CODING_DATA_LENGTH))
    {
      Index++;
    }

    if (Index == HTTP_IO_CHUNKED_TRANSFER_CODING_DATA_LENGTH) {
      Status = EFI_NOT_FOUND;
      goto ExitDeleteChunks;
    }

    ChunkSizeAscii[Index] = 0;
    AsciiStrHexToUintnS (ChunkSizeAscii, NULL, ContentLength);
    DEBUG ((DEBUG_INFO, "     Length of this chunk %d\n", *ContentLength));
    //
    // Receive the data;
    //
    ThisChunk = (HTTP_IO_CHUNKS *)AllocateZeroPool (sizeof (HTTP_IO_CHUNKS));
    if (ThisChunk == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ExitDeleteChunks;
    }

    ResponseData.BodyLength = *ContentLength;
    ResponseData.Body       = (CHAR8 *)AllocatePool (*ContentLength);
    if (ResponseData.Body == NULL) {
      FreePool (ThisChunk);
      Status = EFI_OUT_OF_RESOURCES;
      goto ExitDeleteChunks;
    }

    InitializeListHead (&ThisChunk->NextChunk);
    ThisChunk->Length = *ContentLength;
    ThisChunk->Data   = ResponseData.Body;
    InsertTailList (HttpChunks, &ThisChunk->NextChunk);
    Status = HttpIoRecvResponse (
               HttpIo,
               FALSE,
               &ResponseData
               );
    if (EFI_ERROR (Status)) {
      goto ExitDeleteChunks;
    }

    //
    // Read CRLF
    //
    ZeroMem ((VOID *)&ResponseData, sizeof (HTTP_IO_RESPONSE_DATA));
    ResponseData.BodyLength = 2;
    ResponseData.Body       = ChunkSizeAscii;
    Status                  = HttpIoRecvResponse (
                                HttpIo,
                                FALSE,
                                &ResponseData
                                );
    if (EFI_ERROR (Status)) {
      goto ExitDeleteChunks;
    }

    //
    // Verify the end of chunk payload.
    //
    if ((ChunkSizeAscii[0] != CHUNKED_TRANSFER_CODING_CR) ||
        (ChunkSizeAscii[1] != CHUNKED_TRANSFER_CODING_LF)
        )
    {
      DEBUG ((DEBUG_ERROR, "     This is an invalid End-of-chunk notation.\n"));
      goto ExitDeleteChunks;
    }

    TotalLength += *ContentLength;
    if (TotalLength > MaxTotalLength) {
      DEBUG ((DEBUG_ERROR, "     Total chunk transfer payload exceeds the size defined by PcdMaxHttpChunkTransfer.\n"));
      goto ExitDeleteChunks;
    }
  }

  *ContentLength = TotalLength;
  *ChunkListHead = HttpChunks;
  DEBUG ((DEBUG_INFO, "     Total of lengh of chunks :%d\n", TotalLength));
  return EFI_SUCCESS;

ExitDeleteChunks:
  if (HttpChunks != NULL) {
    while (!IsListEmpty (HttpChunks)) {
      ThisListEntry = GetFirstNode (HttpChunks);
      RemoveEntryList (ThisListEntry);
      ThisChunk = (HTTP_IO_CHUNKS *)ThisListEntry;
      if (ThisChunk->Data != NULL) {
        FreePool (ThisChunk->Data);
      }

      FreePool (ThisListEntry);
    }

    FreePool (HttpChunks);
  }

  return Status;
}
