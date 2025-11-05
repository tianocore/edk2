/** @file
  RedfishHttpOperation handles HTTP operations.

  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishHttpOperation.h"
#include "RedfishHttpData.h"

/**
  This function copies all headers in SrcHeaders to DstHeaders.
  It's call responsibility to release returned DstHeaders.

  @param[in]  SrcHeaders      Source headers.
  @param[in]  SrcHeaderCount  Number of header in source headers.
  @param[out] DstHeaders      Destination headers.
  @param[out] DstHeaderCount  Number of header in designation headers.

  @retval     EFI_SUCCESS     Headers are copied successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
CopyHttpHeaders (
  IN  EFI_HTTP_HEADER  *SrcHeaders,
  IN  UINTN            SrcHeaderCount,
  OUT EFI_HTTP_HEADER  **DstHeaders,
  OUT UINTN            *DstHeaderCount
  )
{
  UINTN  Index;

  if ((SrcHeaders == NULL) || (SrcHeaderCount == 0) || (DstHeaders == NULL) || (DstHeaderCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *DstHeaderCount = 0;
  *DstHeaders     = AllocateZeroPool (sizeof (EFI_HTTP_HEADER) * SrcHeaderCount);
  if (*DstHeaders == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *DstHeaderCount = SrcHeaderCount;
  for (Index = 0; Index < SrcHeaderCount; Index++) {
    (*DstHeaders)[Index].FieldName = ASCII_STR_DUPLICATE (SrcHeaders[Index].FieldName);
    if ((*DstHeaders)[Index].FieldName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    (*DstHeaders)[Index].FieldValue = ASCII_STR_DUPLICATE (SrcHeaders[Index].FieldValue);
    if ((*DstHeaders)[Index].FieldValue == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function free resources in Request. Request is no longer available
  after this function returns successfully.

  @param[in]  Request      HTTP request to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
ReleaseRedfishRequest (
  IN  REDFISH_REQUEST  *Request
  )
{
  if (Request == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Request->Headers != NULL) && (Request->HeaderCount > 0)) {
    HttpFreeHeaderFields (Request->Headers, Request->HeaderCount);
    Request->Headers     = NULL;
    Request->HeaderCount = 0;
  }

  if (Request->Content != NULL) {
    FreePool (Request->Content);
    Request->Content = NULL;
  }

  if (Request->ContentType != NULL) {
    FreePool (Request->ContentType);
    Request->ContentType = NULL;
  }

  Request->ContentLength = 0;

  return EFI_SUCCESS;
}

/**
  This function free resources in given Response.

  @param[in]  Response     HTTP response to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
ReleaseRedfishResponse (
  IN  REDFISH_RESPONSE  *Response
  )
{
  if (Response == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Response->Headers != NULL) && (Response->HeaderCount > 0)) {
    HttpFreeHeaderFields (Response->Headers, Response->HeaderCount);
    Response->Headers     = NULL;
    Response->HeaderCount = 0;
  }

  if (Response->Payload != NULL) {
    ReleaseRedfishPayload (Response->Payload);
    Response->Payload = NULL;
  }

  if (Response->StatusCode != NULL) {
    FreePool (Response->StatusCode);
    Response->StatusCode = NULL;
  }

  return EFI_SUCCESS;
}

/**
  This function free resources in given HTTP message.

  @param[in]  HttpMessage     HTTP message to be released.
  @param[in]  IsRequest       TRUE if this is request type of HTTP message.
                              FALSE if this is response type of HTTP message.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
ReleaseHttpMessage (
  IN  EFI_HTTP_MESSAGE  *HttpMessage,
  IN  BOOLEAN           IsRequest
  )
{
  if (HttpMessage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsRequest) {
    if (HttpMessage->Data.Request != NULL) {
      if (HttpMessage->Data.Request->Url != NULL) {
        FreePool (HttpMessage->Data.Request->Url);
      }

      FreePool (HttpMessage->Data.Request);
      HttpMessage->Data.Request = NULL;
    }
  } else {
    if (HttpMessage->Data.Response != NULL) {
      FreePool (HttpMessage->Data.Response);
      HttpMessage->Data.Response = NULL;
    }
  }

  if (HttpMessage->Body != NULL) {
    FreePool (HttpMessage->Body);
    HttpMessage->Body = NULL;
  }

  if (HttpMessage->Headers != NULL) {
    HttpFreeHeaderFields (HttpMessage->Headers, HttpMessage->HeaderCount);
    HttpMessage->Headers     = NULL;
    HttpMessage->HeaderCount = 0;
  }

  return EFI_SUCCESS;
}

/**
  This function build Redfish message for sending data to Redfish service.
  It's call responsibility to properly release returned HTTP message by
  calling ReleaseHttpMessage.

  @param[in]   ServicePrivate    Pointer to Redfish service private data.
  @param[in]   Uri               Redfish service URI.
  @param[in]   Method            HTTP method.
  @param[in]   Request           Additional data to send to Redfish service.
                                 This is optional.
  @param[in]   ContentEncoding   Content encoding method to compress HTTP context.
                                 This is optional. When ContentEncoding is NULL,
                                 No compress method will be performed.

  @retval     EFI_HTTP_MESSAGE *   Pointer to newly created HTTP message.
  @retval     NULL                 Error occurred.

**/
EFI_HTTP_MESSAGE *
BuildRequestMessage (
  IN REDFISH_SERVICE_PRIVATE  *ServicePrivate,
  IN EFI_STRING               Uri,
  IN EFI_HTTP_METHOD          Method,
  IN REDFISH_REQUEST          *Request OPTIONAL,
  IN CHAR8                    *ContentEncoding OPTIONAL
  )
{
  EFI_STATUS             Status;
  EFI_STRING             Url;
  UINTN                  UrlSize;
  UINTN                  Index;
  EFI_HTTP_MESSAGE       *RequestMsg;
  EFI_HTTP_REQUEST_DATA  *RequestData;
  UINTN                  HeaderCount;
  UINTN                  HeaderIndex;
  EFI_HTTP_HEADER        *Headers;
  CHAR8                  ContentLengthStr[REDFISH_CONTENT_LENGTH_SIZE];
  VOID                   *Content;
  UINTN                  ContentLength;
  BOOLEAN                HasContent;
  BOOLEAN                DoContentEncoding;

  RequestMsg        = NULL;
  RequestData       = NULL;
  Url               = NULL;
  UrlSize           = 0;
  Content           = NULL;
  ContentLength     = 0;
  HeaderCount       = REDFISH_COMMON_HEADER_SIZE;
  HeaderIndex       = 0;
  Headers           = NULL;
  HasContent        = FALSE;
  DoContentEncoding = FALSE;

  if ((ServicePrivate == NULL) || (IS_EMPTY_STRING (Uri))) {
    return NULL;
  }

  if (Method >= HttpMethodMax) {
    return NULL;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: %s\n", __func__, Uri));

  //
  // Build full URL for HTTP query.
  //
  UrlSize = (AsciiStrLen (ServicePrivate->Host) + StrLen (Uri) + 1) * sizeof (CHAR16);
  Url     = AllocateZeroPool (UrlSize);
  if (Url == NULL) {
    return NULL;
  }

  UnicodeSPrint (Url, UrlSize, L"%a%s", ServicePrivate->Host, Uri);
  DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: Url: %s\n", __func__, Url));

  //
  // Step 1: build the HTTP headers.
  //
  if (!IS_EMPTY_STRING (ServicePrivate->SessionToken) || !IS_EMPTY_STRING (ServicePrivate->BasicAuth)) {
    HeaderCount++;
  }

  if ((Request != NULL) && (Request->HeaderCount > 0)) {
    HeaderCount += Request->HeaderCount;
  }

  //
  // Check and see if we will do content encoding or not
  //
  if (!IS_EMPTY_STRING (ContentEncoding)) {
    if (AsciiStrCmp (ContentEncoding, REDFISH_HTTP_CONTENT_ENCODING_NONE) != 0) {
      DoContentEncoding = TRUE;
    }
  }

  if ((Request != NULL) && !IS_EMPTY_STRING (Request->Content)) {
    HeaderCount += 2;
    HasContent   = TRUE;
    if (DoContentEncoding) {
      HeaderCount += 1;
    }
  }

  Headers = AllocateZeroPool (HeaderCount * sizeof (EFI_HTTP_HEADER));
  if (Headers == NULL) {
    goto ON_ERROR;
  }

  if (!IS_EMPTY_STRING (ServicePrivate->SessionToken)) {
    Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_X_AUTH_TOKEN, ServicePrivate->SessionToken);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  } else if (!IS_EMPTY_STRING (ServicePrivate->BasicAuth)) {
    Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_AUTHORIZATION, ServicePrivate->BasicAuth);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  if (Request != NULL) {
    for (Index = 0; Index < Request->HeaderCount; Index++) {
      Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], Request->Headers[Index].FieldName, Request->Headers[Index].FieldValue);
      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    }
  }

  Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_HOST, ServicePrivate->HostName);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], REDFISH_HTTP_HEADER_ODATA_VERSION_STR, REDFISH_HTTP_HEADER_ODATA_VERSION_VALUE);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_ACCEPT, HTTP_CONTENT_TYPE_APP_JSON);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_USER_AGENT, REDFISH_HTTP_HEADER_USER_AGENT_VALUE);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], REDFISH_HTTP_HEADER_CONNECTION_STR, REDFISH_HTTP_HEADER_CONNECTION_VALUE);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Handle content header
  //
  if (HasContent) {
    if (Request->ContentType == NULL) {
      Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_APP_JSON);
      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    } else {
      Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_CONTENT_TYPE, Request->ContentType);
      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    }

    if (Request->ContentLength == 0) {
      Request->ContentLength =  AsciiStrLen (Request->Content);
    }

    AsciiSPrint (
      ContentLengthStr,
      sizeof (ContentLengthStr),
      "%lu",
      (UINT64)Request->ContentLength
      );
    Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_CONTENT_LENGTH, ContentLengthStr);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Encoding
    //
    if (DoContentEncoding) {
      //
      // We currently only support gzip Content-Encoding.
      //
      Status =  RedfishContentEncode (
                  ContentEncoding,
                  Request->Content,
                  Request->ContentLength,
                  &Content,
                  &ContentLength
                  );
      if (Status == EFI_INVALID_PARAMETER) {
        DEBUG ((DEBUG_ERROR, "%a: Error to encode content.\n", __func__));
        goto ON_ERROR;
      } else if (Status == EFI_UNSUPPORTED) {
        DoContentEncoding = FALSE;
        DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: No content coding for %a! Use raw data instead.\n", __func__, ContentEncoding));
        Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_CONTENT_ENCODING, HTTP_CONTENT_ENCODING_IDENTITY);
        if (EFI_ERROR (Status)) {
          goto ON_ERROR;
        }
      } else {
        Status = HttpSetFieldNameAndValue (&Headers[HeaderIndex++], HTTP_HEADER_CONTENT_ENCODING, HTTP_CONTENT_ENCODING_GZIP);
        if (EFI_ERROR (Status)) {
          goto ON_ERROR;
        }
      }
    }

    //
    // When the content is from caller, we use our own copy so that we properly release it later.
    //
    if (!DoContentEncoding) {
      Content = AllocateCopyPool (Request->ContentLength, Request->Content);
      if (Content == NULL) {
        goto ON_ERROR;
      }

      ContentLength = Request->ContentLength;
    }
  }

  //
  // Step 2: build the rest of HTTP request info.
  //
  RequestData = AllocateZeroPool (sizeof (EFI_HTTP_REQUEST_DATA));
  if (RequestData == NULL) {
    goto ON_ERROR;
  }

  RequestData->Method = Method;
  RequestData->Url    = Url;

  //
  // Step 3: fill in EFI_HTTP_MESSAGE
  //
  RequestMsg = AllocateZeroPool (sizeof (EFI_HTTP_MESSAGE));
  if (RequestMsg == NULL) {
    goto ON_ERROR;
  }

  ASSERT (HeaderIndex == HeaderCount);
  RequestMsg->Data.Request = RequestData;
  RequestMsg->HeaderCount  = HeaderIndex;
  RequestMsg->Headers      = Headers;

  if (HasContent) {
    RequestMsg->BodyLength = ContentLength;
    RequestMsg->Body       = Content;
  }

  return RequestMsg;

ON_ERROR:

  if (Headers != NULL) {
    HttpFreeHeaderFields (Headers, HeaderIndex);
  }

  if (RequestData != NULL) {
    FreePool (RequestData);
  }

  if (RequestMsg != NULL) {
    FreePool (RequestMsg);
  }

  if (Url != NULL) {
    FreePool (Url);
  }

  return NULL;
}

/**
  This function parse response message from Redfish service, and
  build Redfish response for caller. It's call responsibility to
  properly release Redfish response by calling ReleaseRedfishResponse.

  @param[in]   ServicePrivate   Pointer to Redfish service private data.
  @param[in]   ResponseMsg      Response message from Redfish service.
  @param[out]  RedfishResponse  Redfish response data.

  @retval     EFI_SUCCESS     Redfish response is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
ParseResponseMessage (
  IN  REDFISH_SERVICE_PRIVATE  *ServicePrivate,
  IN  EFI_HTTP_MESSAGE         *ResponseMsg,
  OUT REDFISH_RESPONSE         *RedfishResponse
  )
{
  EFI_STATUS        Status;
  EDKII_JSON_VALUE  JsonData;
  EFI_HTTP_HEADER   *ContentEncodedHeader;
  EFI_HTTP_HEADER   *ContentTypeHeader;
  VOID              *DecodedBody;
  UINTN             DecodedLength;

  if ((ServicePrivate == NULL) || (ResponseMsg == NULL) || (RedfishResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a\n", __func__));

  //
  // Initialization
  //
  JsonData                     = NULL;
  RedfishResponse->HeaderCount = 0;
  RedfishResponse->Headers     = NULL;
  RedfishResponse->Payload     = NULL;
  RedfishResponse->StatusCode  = NULL;
  DecodedBody                  = NULL;
  DecodedLength                = 0;

  //
  // Return the HTTP StatusCode.
  //
  if (ResponseMsg->Data.Response != NULL) {
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: status: %d\n", __func__, ResponseMsg->Data.Response->StatusCode));
    RedfishResponse->StatusCode = AllocateCopyPool (sizeof (EFI_HTTP_STATUS_CODE), &ResponseMsg->Data.Response->StatusCode);
    if (RedfishResponse->StatusCode == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to create status code.\n", __func__));
    }
  }

  //
  // Return the HTTP headers.
  //
  if (ResponseMsg->Headers != NULL) {
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: header count: %d\n", __func__, ResponseMsg->HeaderCount));
    Status = CopyHttpHeaders (
               ResponseMsg->Headers,
               ResponseMsg->HeaderCount,
               &RedfishResponse->Headers,
               &RedfishResponse->HeaderCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to copy HTTP headers: %r\n", __func__, Status));
    }
  }

  //
  // Return the HTTP body.
  //
  if ((ResponseMsg->BodyLength != 0) && (ResponseMsg->Body != NULL)) {
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: body length: %d\n", __func__, ResponseMsg->BodyLength));

    //
    // We expect to see JSON body
    //
    ContentTypeHeader = HttpFindHeader (RedfishResponse->HeaderCount, RedfishResponse->Headers, HTTP_HEADER_CONTENT_TYPE);
    if (ContentTypeHeader != NULL) {
      if (AsciiStrCmp (ContentTypeHeader->FieldValue, HTTP_CONTENT_TYPE_APP_JSON) != 0) {
        DEBUG ((DEBUG_WARN, "%a: body is not in %a format\n", __func__, HTTP_CONTENT_TYPE_APP_JSON));
      }
    }

    //
    // Check if data is encoded.
    //
    ContentEncodedHeader = HttpFindHeader (RedfishResponse->HeaderCount, RedfishResponse->Headers, HTTP_HEADER_CONTENT_ENCODING);
    if (ContentEncodedHeader != NULL) {
      //
      // The content is encoded.
      //
      Status = RedfishContentDecode (
                 ContentEncodedHeader->FieldValue,
                 ResponseMsg->Body,
                 ResponseMsg->BodyLength,
                 &DecodedBody,
                 &DecodedLength
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to decompress the response content: %r decoding method: %a\n.", __func__, Status, ContentEncodedHeader->FieldValue));
        goto ON_ERROR;
      }

      JsonData = JsonLoadBuffer (DecodedBody, DecodedLength, 0, NULL);
      FreePool (DecodedBody);
    } else {
      JsonData = JsonLoadBuffer (ResponseMsg->Body, ResponseMsg->BodyLength, 0, NULL);
    }

    if (!JsonValueIsNull (JsonData)) {
      RedfishResponse->Payload = CreateRedfishPayload (ServicePrivate, JsonData);
      if (RedfishResponse->Payload == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to create payload\n.", __func__));
      }

      JsonValueFree (JsonData);
    } else {
      DEBUG ((DEBUG_ERROR, "%a: No payload available\n", __func__));
    }
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (RedfishResponse != NULL) {
    ReleaseRedfishResponse (RedfishResponse);
  }

  return Status;
}

/**
  This function send Redfish request to Redfish service by calling
  Rest Ex protocol.

  @param[in]   Service       Pointer to Redfish service.
  @param[in]   Uri           Uri of Redfish service.
  @param[in]   Method        HTTP method.
  @param[in]   Request     Request data. This is optional.
  @param[out]  Response    Redfish response data.

  @retval     EFI_SUCCESS     Request is sent and received successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
HttpSendReceive (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  EFI_HTTP_METHOD   Method,
  IN  REDFISH_REQUEST   *Request  OPTIONAL,
  OUT REDFISH_RESPONSE  *Response
  )
{
  EFI_STATUS               Status;
  EFI_STATUS               RestExStatus;
  EFI_HTTP_MESSAGE         *RequestMsg;
  EFI_HTTP_MESSAGE         ResponseMsg;
  REDFISH_SERVICE_PRIVATE  *ServicePrivate;
  EFI_HTTP_HEADER          *XAuthTokenHeader;
  CHAR8                    *HttpContentEncoding;

  if ((Service == NULL) || IS_EMPTY_STRING (Uri) || (Response == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: Method: 0x%x %s\n", __func__, Method, Uri));

  ServicePrivate = (REDFISH_SERVICE_PRIVATE *)Service;
  if (ServicePrivate->Signature != REDFISH_HTTP_SERVICE_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a: signature check failure\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ResponseMsg, sizeof (ResponseMsg));
  HttpContentEncoding = (CHAR8 *)PcdGetPtr (PcdRedfishServiceContentEncoding);

  RequestMsg = BuildRequestMessage (Service, Uri, Method, Request, HttpContentEncoding);
  if (RequestMsg == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: cannot build request message for %s\n", __func__, Uri));
    return EFI_PROTOCOL_ERROR;
  }

  //
  // call RESTEx to get response from REST service.
  //
  RestExStatus = ServicePrivate->RestEx->SendReceive (ServicePrivate->RestEx, RequestMsg, &ResponseMsg);
  if (EFI_ERROR (RestExStatus)) {
    DEBUG ((DEBUG_ERROR, "%a: %s SendReceive failure: %r\n", __func__, Uri, RestExStatus));
  }

  //
  // Return status code, headers and payload to caller as much as possible even when RestEx returns failure.
  //
  Status = ParseResponseMessage (ServicePrivate, &ResponseMsg, Response);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %s parse response failure: %r\n", __func__, Uri, Status));
  } else {
    //
    // Capture session token in header
    //
    if ((Method == HttpMethodPost) &&
        (Response->StatusCode != NULL) &&
        ((*Response->StatusCode == HTTP_STATUS_200_OK) || (*Response->StatusCode == HTTP_STATUS_204_NO_CONTENT)))
    {
      XAuthTokenHeader = HttpFindHeader (ResponseMsg.HeaderCount, ResponseMsg.Headers, HTTP_HEADER_X_AUTH_TOKEN);
      if (XAuthTokenHeader != NULL) {
        Status = UpdateSessionToken (ServicePrivate, XAuthTokenHeader->FieldValue);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: update session token failure: %r\n", __func__, Status));
        }
      }
    }
  }

  //
  // Release resources
  //
  if (RequestMsg != NULL) {
    ReleaseHttpMessage (RequestMsg, TRUE);
    FreePool (RequestMsg);
  }

  ReleaseHttpMessage (&ResponseMsg, FALSE);

  return RestExStatus;
}
