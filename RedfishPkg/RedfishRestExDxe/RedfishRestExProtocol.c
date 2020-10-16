/** @file
  Implementation of Redfish EFI_REST_EX_PROTOCOL interfaces.

   Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include "RedfishRestExInternal.h"

EFI_REST_EX_PROTOCOL  mRedfishRestExProtocol = {
  RedfishRestExSendReceive,
  RedfishRestExGetServiceTime,
  RedfishRestExGetService,
  RedfishRestExGetModeData,
  RedfishRestExConfigure,
  RedfishRestExAyncSendReceive,
  RedfishRestExEventService
};

/**
  Provides a simple HTTP-like interface to send and receive resources from a REST service.

  The SendReceive() function sends an HTTP request to this REST service, and returns a
  response when the data is retrieved from the service. RequestMessage contains the HTTP
  request to the REST resource identified by RequestMessage.Request.Url. The
  ResponseMessage is the returned HTTP response for that request, including any HTTP
  status.

  @param[in]  This                Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                  REST service.
  @param[in]  RequestMessage      Pointer to the HTTP request data for this resource
  @param[out] ResponseMessage     Pointer to the HTTP response data obtained for this requested.

  @retval EFI_SUCCESS             operation succeeded.
  @retval EFI_INVALID_PARAMETER   This, RequestMessage, or ResponseMessage are NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_ACCESS_DENIED       HTTP method is not allowed on this URL.
  @retval EFI_BAD_BUFFER_SIZE     The payload is to large to be handled on server side.
  @retval EFI_UNSUPPORTED         Unsupported HTTP response.

**/
EFI_STATUS
EFIAPI
RedfishRestExSendReceive (
  IN      EFI_REST_EX_PROTOCOL   *This,
  IN      EFI_HTTP_MESSAGE       *RequestMessage,
  OUT     EFI_HTTP_MESSAGE       *ResponseMessage
  )
{
  EFI_STATUS             Status;
  RESTEX_INSTANCE        *Instance;
  HTTP_IO_RESPONSE_DATA  *ResponseData;
  UINTN                  TotalReceivedSize;
  UINTN                  Index;
  LIST_ENTRY             *ChunkListLink;
  HTTP_IO_CHUNKS        *ThisChunk;
  BOOLEAN                CopyChunkData;
  BOOLEAN                MediaPresent;
  EFI_HTTP_HEADER        *PreservedRequestHeaders;
  BOOLEAN                ItsWrite;
  BOOLEAN                IsGetChunkedTransfer;
  HTTP_IO_SEND_CHUNK_PROCESS     SendChunkProcess;
  HTTP_IO_SEND_NON_CHUNK_PROCESS SendNonChunkProcess;
  EFI_HTTP_MESSAGE       ChunkTransferRequestMessage;

  Status            = EFI_SUCCESS;
  ResponseData      = NULL;
  IsGetChunkedTransfer = FALSE;
  SendChunkProcess = HttpIoSendChunkNone;
  SendNonChunkProcess = HttpIoSendNonChunkNone;

  //
  // Validate the parameters
  //
  if ((This == NULL) || (RequestMessage == NULL) || ResponseMessage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = RESTEX_INSTANCE_FROM_THIS (This);

  //
  // Check Media Status.
  //
  MediaPresent = TRUE;
  NetLibDetectMedia (Instance->Service->ControllerHandle, &MediaPresent);
  if (!MediaPresent) {
    DEBUG ((DEBUG_INFO, "RedfishRestExSendReceive(): No MediaPresent.\n"));
    return EFI_NO_MEDIA;
  }

  DEBUG ((DEBUG_INFO, "\nRedfishRestExSendReceive():\n"));
  DEBUG ((DEBUG_INFO, "*** Perform HTTP Request Method - %d, URL: %s\n", RequestMessage->Data.Request->Method, RequestMessage->Data.Request->Url));

  //
  // Add header "Expect" to server, only for URL write.
  //
  Status = RedfishHttpAddExpectation (This, RequestMessage, &PreservedRequestHeaders, &ItsWrite);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (ItsWrite == TRUE) {
    if (RequestMessage->BodyLength > HTTP_IO_MAX_SEND_PAYLOAD) {
      //
      // Send chunked transfer.
      //
      SendChunkProcess ++;
      CopyMem ((VOID *)&ChunkTransferRequestMessage, (VOID *)RequestMessage, sizeof (EFI_HTTP_MESSAGE));
    } else {
      SendNonChunkProcess ++;
    }
  }
ReSendRequest:;
  //
  // Send out the request to REST service.
  //
  if (ItsWrite == TRUE) {
    //
    // This is write to URI
    //
    if (SendChunkProcess > HttpIoSendChunkNone) {
      //
      // This is chunk transfer for writing large payload.
      // Send request header first and then handle the
      // following request message body using chunk transfer.
      //
      do {
        Status = HttpIoSendChunkedTransfer(
                   &(Instance->HttpIo),
                   &SendChunkProcess,
                   &ChunkTransferRequestMessage
                   );
        if (EFI_ERROR (Status)) {
          goto ON_EXIT;
        }
      } while (SendChunkProcess == HttpIoSendChunkContent || SendChunkProcess == HttpIoSendChunkEndChunk);
    } else {
      //
      // This is the non-chunk transfer, send request header first and then
      // handle the following request message body using chunk transfer.
      //
      Status = HttpIoSendRequest(
                 &(Instance->HttpIo),
                 (SendNonChunkProcess == HttpIoSendNonChunkContent)? NULL: RequestMessage->Data.Request,
                 (SendNonChunkProcess == HttpIoSendNonChunkContent)? 0: RequestMessage->HeaderCount,
                 (SendNonChunkProcess == HttpIoSendNonChunkContent)? NULL: RequestMessage->Headers,
                 (SendNonChunkProcess == HttpIoSendNonChunkHeaderZeroContent)? 0: RequestMessage->BodyLength,
                 (SendNonChunkProcess == HttpIoSendNonChunkHeaderZeroContent)? NULL: RequestMessage->Body
                 );
    }
  } else {
    //
    // This is read from URI.
    //
    Status = HttpIoSendRequest(
               &(Instance->HttpIo),
               RequestMessage->Data.Request,
               RequestMessage->HeaderCount,
               RequestMessage->Headers,
               RequestMessage->BodyLength,
               RequestMessage->Body
               );
  }
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // ResponseMessage->Data.Response is to indicate whether to receive the HTTP header or not.
  // ResponseMessage->BodyLength/ResponseMessage->Body are to indicate whether to receive the response body or not.
  // Clean the previous buffers and all of them will be allocated later according to the actual situation.
  //
  if (ResponseMessage->Data.Response != NULL) {
    FreePool(ResponseMessage->Data.Response);
    ResponseMessage->Data.Response = NULL;
  }

  ResponseMessage->BodyLength = 0;
  if (ResponseMessage->Body != NULL) {
    FreePool(ResponseMessage->Body);
    ResponseMessage->Body = NULL;
  }

  //
  // Use zero BodyLength to only receive the response headers.
  //
  ResponseData = AllocateZeroPool (sizeof(HTTP_IO_RESPONSE_DATA));
  if (ResponseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  DEBUG ((DEBUG_INFO, "Receiving HTTP response and headers...\n"));
  Status = RedfishCheckHttpReceiveStatus (
                Instance,
                HttpIoRecvResponse (
                  &(Instance->HttpIo),
                  TRUE,
                  ResponseData
                  )
             );
  if (Status == EFI_NOT_READY) {
     goto ReSendRequest;
  } else if (Status == EFI_DEVICE_ERROR) {
    goto ON_EXIT;
  }
  //
  // Restore the headers if it ever changed in RedfishHttpAddExpectation().
  //
  if (RequestMessage->Headers != PreservedRequestHeaders) {
    FreePool (RequestMessage->Headers);
    RequestMessage->Headers = PreservedRequestHeaders; // Restore headers before we adding "Expect".
    RequestMessage->HeaderCount --;                    // Minus one header count for "Expect".
  }

  DEBUG ((DEBUG_INFO, "HTTP Response StatusCode - %d:", ResponseData->Response.StatusCode));
  if (ResponseData->Response.StatusCode == HTTP_STATUS_200_OK) {
    DEBUG ((DEBUG_INFO, "HTTP_STATUS_200_OK\n"));

    if (SendChunkProcess == HttpIoSendChunkHeaderZeroContent) {
      DEBUG ((DEBUG_INFO, "This is chunk transfer, start to send all chunks.", ResponseData->Response.StatusCode));
      SendChunkProcess ++;
      goto ReSendRequest;
    }
  } else if (ResponseData->Response.StatusCode == HTTP_STATUS_413_REQUEST_ENTITY_TOO_LARGE) {
    DEBUG ((DEBUG_INFO, "HTTP_STATUS_413_REQUEST_ENTITY_TOO_LARGE\n"));

    Status = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  } else if (ResponseData->Response.StatusCode == HTTP_STATUS_405_METHOD_NOT_ALLOWED){
    DEBUG ((DEBUG_ERROR, "HTTP_STATUS_405_METHOD_NOT_ALLOWED\n"));

    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  } else if (ResponseData->Response.StatusCode == HTTP_STATUS_400_BAD_REQUEST) {
    DEBUG ((DEBUG_INFO, "HTTP_STATUS_400_BAD_REQUEST\n"));
    if (SendChunkProcess == HttpIoSendChunkHeaderZeroContent) {
      DEBUG ((DEBUG_INFO, "Bad request may caused by zero length chunk. Try to send all chunks...\n"));
      SendChunkProcess ++;
      goto ReSendRequest;
    }
  } else if (ResponseData->Response.StatusCode == HTTP_STATUS_100_CONTINUE) {
    DEBUG ((DEBUG_INFO, "HTTP_STATUS_100_CONTINUE\n"));
    if (SendChunkProcess == HttpIoSendChunkHeaderZeroContent) {
      //
      // We get HTTP_STATUS_100_CONTINUE to send the body using chunk transfer.
      //
      DEBUG ((DEBUG_INFO, "HTTP_STATUS_100_CONTINUE for chunk transfer...\n"));
      SendChunkProcess ++;
      goto ReSendRequest;
    }
    if (SendNonChunkProcess == HttpIoSendNonChunkHeaderZeroContent) {
      DEBUG ((DEBUG_INFO, "HTTP_STATUS_100_CONTINUE for non chunk transfer...\n"));
      SendNonChunkProcess ++;
      goto ReSendRequest;
    }
    //
    // It's the REST protocol's responsibility to handle the interim HTTP response (e.g. 100 Continue Informational),
    // and return the final response content to the caller.
    //
    if (ResponseData->Headers != NULL && ResponseData->HeaderCount != 0) {
      FreePool (ResponseData->Headers);
    }
    ZeroMem (ResponseData, sizeof(HTTP_IO_RESPONSE_DATA));
    Status = HttpIoRecvResponse (
               &(Instance->HttpIo),
               TRUE,
               ResponseData
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "This HTTP Status is not handled!\n"));
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Ready to return the StatusCode, Header info and BodyLength.
  //
  ResponseMessage->Data.Response = AllocateZeroPool (sizeof (EFI_HTTP_RESPONSE_DATA));
  if (ResponseMessage->Data.Response == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  ResponseMessage->Data.Response->StatusCode = ResponseData->Response.StatusCode;
  ResponseMessage->HeaderCount = ResponseData->HeaderCount;
  ResponseMessage->Headers = ResponseData->Headers;

  //
  // Get response message body.
  //
  if (ResponseMessage->HeaderCount > 0) {
    Status = HttpIoGetContentLength (ResponseMessage->HeaderCount, ResponseMessage->Headers, &ResponseMessage->BodyLength);
    if (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) {
      goto ON_EXIT;
    }

    if (Status == EFI_NOT_FOUND) {
      ASSERT (ResponseMessage->BodyLength == 0);
    }

    if (ResponseMessage->BodyLength == 0) {
      //
      // Check if Chunked Transfer Coding.
      //
      Status = HttpIoGetChunkedTransferContent (
                 &(Instance->HttpIo),
                 ResponseMessage->HeaderCount,
                 ResponseMessage->Headers,
                 &ChunkListLink,
                 &ResponseMessage->BodyLength
                 );
      if (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) {
        goto ON_EXIT;
      }
      if (Status == EFI_SUCCESS &&
          ChunkListLink != NULL &&
          !IsListEmpty(ChunkListLink) &&
          ResponseMessage->BodyLength != 0) {
        IsGetChunkedTransfer = TRUE;
        //
        // Copy data to Message body.
        //
        CopyChunkData = TRUE;
        ResponseMessage->Body = AllocateZeroPool (ResponseMessage->BodyLength);
        if (ResponseMessage->Body == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          CopyChunkData = FALSE;
        }
        Index = 0;
        while (!IsListEmpty(ChunkListLink)) {
          ThisChunk = (HTTP_IO_CHUNKS *)GetFirstNode (ChunkListLink);
          if (CopyChunkData) {
            CopyMem(((UINT8 *)ResponseMessage->Body + Index), (UINT8 *)ThisChunk->Data, ThisChunk->Length);
            Index += ThisChunk->Length;
          }
          RemoveEntryList (&ThisChunk->NextChunk);
          FreePool ((VOID *)ThisChunk->Data);
          FreePool ((VOID *)ThisChunk);
        };
        FreePool ((VOID *)ChunkListLink);
      }
    }
    Status = EFI_SUCCESS;
  }

  //
  // Ready to return the Body from REST service if have any.
  //
  if (ResponseMessage->BodyLength > 0 && !IsGetChunkedTransfer) {
    ResponseData->HeaderCount = 0;
    ResponseData->Headers = NULL;

    ResponseMessage->Body = AllocateZeroPool (ResponseMessage->BodyLength);
    if (ResponseMessage->Body == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    //
    // Only receive the Body.
    //
    TotalReceivedSize = 0;
    while (TotalReceivedSize < ResponseMessage->BodyLength) {
      ResponseData->BodyLength = ResponseMessage->BodyLength - TotalReceivedSize;
      ResponseData->Body = (CHAR8 *) ResponseMessage->Body + TotalReceivedSize;
      Status = HttpIoRecvResponse (
                 &(Instance->HttpIo),
                 FALSE,
                 ResponseData
                 );
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      TotalReceivedSize += ResponseData->BodyLength;
    }
    DEBUG ((DEBUG_INFO, "Total of lengh of Response :%d\n", TotalReceivedSize));
  }
  DEBUG ((DEBUG_INFO, "RedfishRestExSendReceive()- EFI_STATUS: %r\n", Status));

ON_EXIT:

  if (ResponseData != NULL) {
    FreePool (ResponseData);
  }

  if (EFI_ERROR (Status)) {
    if (ResponseMessage->Data.Response != NULL) {
      FreePool (ResponseMessage->Data.Response);
      ResponseMessage->Data.Response = NULL;
    }

    if (ResponseMessage->Body != NULL) {
      FreePool (ResponseMessage->Body);
      ResponseMessage->Body = NULL;
    }
  }
  return Status;
}

/**
  Obtain the current time from this REST service instance.

  The GetServiceTime() function is an optional interface to obtain the current time from
  this REST service instance. If this REST service does not support to retrieve the time,
  this function returns EFI_UNSUPPORTED. This function must returns EFI_UNSUPPORTED if
  EFI_REST_EX_SERVICE_TYPE returned in EFI_REST_EX_SERVICE_INFO from GetService() is
  EFI_REST_EX_SERVICE_UNSPECIFIC.

  @param[in]  This                Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                  REST service.
  @param[out] Time                A pointer to storage to receive a snapshot of the current time of
                                  the REST service.

  @retval EFI_SUCCESS             operation succeeded.
  @retval EFI_INVALID_PARAMETER   This or Time are NULL.
  @retval EFI_UNSUPPORTED         The RESTful service does not support returning the time.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_NOT_READY           The configuration of this instance is not set yet. Configure() must
                                  be executed and returns successfully prior to invoke this function.

**/
EFI_STATUS
EFIAPI
RedfishRestExGetServiceTime (
  IN      EFI_REST_EX_PROTOCOL   *This,
  OUT     EFI_TIME               *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function returns the information of REST service provided by this EFI REST EX driver instance.

  The information such as the type of REST service and the access mode of REST EX driver instance
  (In-band or Out-of-band) are described in EFI_REST_EX_SERVICE_INFO structure. For the vendor-specific
  REST service, vendor-specific REST service information is returned in VendorSpecifcData.
  REST EX driver designer is well know what REST service this REST EX driver instance intends to
  communicate with. The designer also well know this driver instance is used to talk to BMC through
  specific platform mechanism or talk to REST server through UEFI HTTP protocol. REST EX driver is
  responsible to fill up the correct information in EFI_REST_EX_SERVICE_INFO. EFI_REST_EX_SERVICE_INFO
  is referred by EFI REST clients to pickup the proper EFI REST EX driver instance to get and set resource.
  GetService() is a basic and mandatory function which must be able to use even Configure() is not invoked
  in previously.

  @param[in]  This                Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                  REST service.
  @param[out] RestExServiceInfo   Pointer to receive a pointer to EFI_REST_EX_SERVICE_INFO structure. The
                                  format of EFI_REST_EX_SERVICE_INFO is version controlled for the future
                                  extension. The version of EFI_REST_EX_SERVICE_INFO structure is returned
                                  in the header within this structure. EFI REST client refers to the correct
                                  format of structure according to the version number. The pointer to
                                  EFI_REST_EX_SERVICE_INFO is a memory block allocated by EFI REST EX driver
                                  instance. That is caller's responsibility to free this memory when this
                                  structure is no longer needed. Refer to Related Definitions below for the
                                  definitions of EFI_REST_EX_SERVICE_INFO structure.

  @retval EFI_SUCCESS             EFI_REST_EX_SERVICE_INFO is returned in RestExServiceInfo. This function
                                  is not supported in this REST EX Protocol driver instance.
  @retval EFI_UNSUPPORTED         This function is not supported in this REST EX Protocol driver instance.

**/
EFI_STATUS
EFIAPI
RedfishRestExGetService (
  IN   EFI_REST_EX_PROTOCOL      *This,
  OUT  EFI_REST_EX_SERVICE_INFO  **RestExServiceInfo
  )
{
  EFI_TPL                  OldTpl;
  RESTEX_INSTANCE          *Instance;
  EFI_REST_EX_SERVICE_INFO *ServiceInfo;

  ServiceInfo = NULL;

  if (This == NULL || RestExServiceInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl   = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = RESTEX_INSTANCE_FROM_THIS (This);

  ServiceInfo = AllocateZeroPool (sizeof (EFI_REST_EX_SERVICE_INFO));
  if (ServiceInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (ServiceInfo, &(Instance->Service->RestExServiceInfo), sizeof (EFI_REST_EX_SERVICE_INFO));

  *RestExServiceInfo = ServiceInfo;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**
  This function returns operational configuration of current EFI REST EX child instance.

  This function returns the current configuration of EFI REST EX child instance. The format of
  operational configuration depends on the implementation of EFI REST EX driver instance. For
  example, HTTP-aware EFI REST EX driver instance uses EFI HTTP protocol as the undying protocol
  to communicate with REST service. In this case, the type of configuration is
  EFI_REST_EX_CONFIG_TYPE_HTTP returned from GetService(). EFI_HTTP_CONFIG_DATA is used as EFI REST
  EX configuration format and returned to EFI REST client. User has to type cast RestExConfigData
  to EFI_HTTP_CONFIG_DATA. For those non HTTP-aware REST EX driver instances, the type of configuration
  is EFI_REST_EX_CONFIG_TYPE_UNSPECIFIC returned from GetService(). In this case, the format of
  returning data could be non industrial. Instead, the format of configuration data is system/platform
  specific definition such as BMC mechanism used in EFI REST EX driver instance. EFI REST client and
  EFI REST EX driver instance have to refer to the specific system /platform spec which is out of UEFI scope.

  @param[in]  This                This is the EFI_REST_EX_PROTOCOL instance.
  @param[out] RestExConfigData    Pointer to receive a pointer to EFI_REST_EX_CONFIG_DATA.
                                  The memory allocated for configuration data should be freed
                                  by caller. See Related Definitions for the details.

  @retval EFI_SUCCESS             EFI_REST_EX_CONFIG_DATA is returned in successfully.
  @retval EFI_UNSUPPORTED         This function is not supported in this REST EX Protocol driver instance.
  @retval EFI_NOT_READY           The configuration of this instance is not set yet. Configure() must be
                                  executed and returns successfully prior to invoke this function.

**/
EFI_STATUS
EFIAPI
RedfishRestExGetModeData (
  IN  EFI_REST_EX_PROTOCOL      *This,
  OUT EFI_REST_EX_CONFIG_DATA   *RestExConfigData
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function is used to configure EFI REST EX child instance.

  This function is used to configure the setting of underlying protocol of REST EX child
  instance. The type of configuration is according to the implementation of EFI REST EX
  driver instance. For example, HTTP-aware EFI REST EX driver instance uses EFI HTTP protocol
  as the undying protocol to communicate with REST service. The type of configuration is
  EFI_REST_EX_CONFIG_TYPE_HTTP and RestExConfigData is the same format with EFI_HTTP_CONFIG_DATA.
  Akin to HTTP configuration, REST EX child instance can be configure to use different HTTP
  local access point for the data transmission. Multiple REST clients may use different
  configuration of HTTP to distinguish themselves, such as to use the different TCP port.
  For those non HTTP-aware REST EX driver instance, the type of configuration is
  EFI_REST_EX_CONFIG_TYPE_UNSPECIFIC. RestExConfigData refers to the non industrial standard.
  Instead, the format of configuration data is system/platform specific definition such as BMC.
  In this case, EFI REST client and EFI REST EX driver instance have to refer to the specific
  system/platform spec which is out of the UEFI scope. Besides GetService()function, no other
  EFI REST EX functions can be executed by this instance until Configure()is executed and returns
  successfully. All other functions must returns EFI_NOT_READY if this instance is not configured
  yet. Set RestExConfigData to NULL means to put EFI REST EX child instance into the unconfigured
  state.

  @param[in]  This                This is the EFI_REST_EX_PROTOCOL instance.
  @param[in]  RestExConfigData    Pointer to EFI_REST_EX_CONFIG_DATA. See Related Definitions in
                                  GetModeData() protocol interface.

  @retval EFI_SUCCESS             EFI_REST_EX_CONFIG_DATA is set in successfully.
  @retval EFI_DEVICE_ERROR        Configuration for this REST EX child instance is failed with the given
                                  EFI_REST_EX_CONFIG_DATA.
  @retval EFI_UNSUPPORTED         This function is not supported in this REST EX Protocol driver instance.

**/
EFI_STATUS
EFIAPI
RedfishRestExConfigure (
  IN  EFI_REST_EX_PROTOCOL    *This,
  IN  EFI_REST_EX_CONFIG_DATA RestExConfigData
  )
{
  EFI_STATUS               Status;
  EFI_TPL                  OldTpl;
  RESTEX_INSTANCE          *Instance;

  EFI_HTTP_CONFIG_DATA     *HttpConfigData;

  Status         = EFI_SUCCESS;
  HttpConfigData = NULL;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl   = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = RESTEX_INSTANCE_FROM_THIS (This);

  if (RestExConfigData == NULL) {
    //
    // Set RestExConfigData to NULL means to put EFI REST EX child instance into the unconfigured state.
    //
    HttpIoDestroyIo (&(Instance->HttpIo));

    if (Instance->ConfigData != NULL) {
      if (((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData.AccessPoint.IPv4Node != NULL) {
        FreePool(((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData.AccessPoint.IPv4Node);
      }
      FreePool(Instance->ConfigData);
      Instance->ConfigData = NULL;
    }

    Instance->State = RESTEX_STATE_UNCONFIGED;
  } else {
    HttpConfigData = &((EFI_REST_EX_HTTP_CONFIG_DATA *)RestExConfigData)->HttpConfigData;
    Status = Instance->HttpIo.Http->Configure (Instance->HttpIo.Http, HttpConfigData);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
    Instance->HttpIo.Timeout = ((EFI_REST_EX_HTTP_CONFIG_DATA *)RestExConfigData)->SendReceiveTimeout;

    Instance->ConfigData = AllocateZeroPool (sizeof (EFI_REST_EX_HTTP_CONFIG_DATA));
    if (Instance->ConfigData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }
    CopyMem (Instance->ConfigData, RestExConfigData, sizeof (EFI_REST_EX_HTTP_CONFIG_DATA));
    if (HttpConfigData->LocalAddressIsIPv6 == TRUE) {
      ((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData.AccessPoint.IPv6Node = AllocateZeroPool (sizeof (EFI_HTTPv6_ACCESS_POINT));
      if (((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData.AccessPoint.IPv6Node == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }
      CopyMem (
        ((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData.AccessPoint.IPv6Node,
        HttpConfigData->AccessPoint.IPv6Node,
        sizeof (EFI_HTTPv6_ACCESS_POINT)
        );
    } else {
      ((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData.AccessPoint.IPv4Node = AllocateZeroPool (sizeof (EFI_HTTPv4_ACCESS_POINT));
      if (((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData.AccessPoint.IPv4Node == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }
      CopyMem (
        ((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData.AccessPoint.IPv4Node,
        HttpConfigData->AccessPoint.IPv4Node,
        sizeof (EFI_HTTPv4_ACCESS_POINT)
        );
    }
    Instance->State = RESTEX_STATE_CONFIGED;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  This function sends REST request to REST service and signal caller's event asynchronously when
  the final response is received by REST EX Protocol driver instance.

  The essential design of this function is to handle asynchronous send/receive implicitly according
  to REST service asynchronous request mechanism. Caller will get the notification once the response
  is returned from REST service.

  @param[in]  This                  This is the EFI_REST_EX_PROTOCOL instance.
  @param[in]  RequestMessage        This is the HTTP request message sent to REST service. Set RequestMessage
                                    to NULL to cancel the previous asynchronous request associated with the
                                    corresponding RestExToken. See descriptions for the details.
  @param[in]  RestExToken           REST EX token which REST EX Protocol instance uses to notify REST client
                                    the status of response of asynchronous REST request. See related definition
                                    of EFI_REST_EX_TOKEN.
  @param[in]  TimeOutInMilliSeconds The pointer to the timeout in milliseconds which REST EX Protocol driver
                                    instance refers as the duration to drop asynchronous REST request. NULL
                                    pointer means no timeout for this REST request. REST EX Protocol driver
                                    signals caller's event with EFI_STATUS set to EFI_TIMEOUT in RestExToken
                                    if REST EX Protocol can't get the response from REST service within
                                    TimeOutInMilliSeconds.

  @retval EFI_SUCCESS               Asynchronous REST request is established.
  @retval EFI_UNSUPPORTED           This REST EX Protocol driver instance doesn't support asynchronous request.
  @retval EFI_TIMEOUT               Asynchronous REST request is not established and timeout is expired.
  @retval EFI_ABORT                 Previous asynchronous REST request has been canceled.
  @retval EFI_DEVICE_ERROR          Otherwise, returns EFI_DEVICE_ERROR for other errors according to HTTP Status Code.
  @retval EFI_NOT_READY             The configuration of this instance is not set yet. Configure() must be executed
                                    and returns successfully prior to invoke this function.

**/
EFI_STATUS
EFIAPI
RedfishRestExAyncSendReceive (
  IN      EFI_REST_EX_PROTOCOL   *This,
  IN      EFI_HTTP_MESSAGE       *RequestMessage OPTIONAL,
  IN      EFI_REST_EX_TOKEN      *RestExToken,
  IN      UINTN                  *TimeOutInMilliSeconds OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function sends REST request to a REST Event service and signals caller's event
  token asynchronously when the URI resource change event is received by REST EX
  Protocol driver instance.

  The essential design of this function is to monitor event implicitly according to
  REST service event service mechanism. Caller will get the notification if certain
  resource is changed.

  @param[in]  This                  This is the EFI_REST_EX_PROTOCOL instance.
  @param[in]  RequestMessage        This is the HTTP request message sent to REST service. Set RequestMessage
                                    to NULL to cancel the previous event service associated with the corresponding
                                    RestExToken. See descriptions for the details.
  @param[in]  RestExToken           REST EX token which REST EX Protocol driver instance uses to notify REST client
                                    the URI resource which monitored by REST client has been changed. See the related
                                    definition of EFI_REST_EX_TOKEN in EFI_REST_EX_PROTOCOL.AsyncSendReceive().

  @retval EFI_SUCCESS               Asynchronous REST request is established.
  @retval EFI_UNSUPPORTED           This REST EX Protocol driver instance doesn't support asynchronous request.
  @retval EFI_ABORT                 Previous asynchronous REST request has been canceled or event subscription has been
                                    delete from service.
  @retval EFI_DEVICE_ERROR          Otherwise, returns EFI_DEVICE_ERROR for other errors according to HTTP Status Code.
  @retval EFI_NOT_READY             The configuration of this instance is not set yet. Configure() must be executed
                                    and returns successfully prior to invoke this function.

**/
EFI_STATUS
EFIAPI
RedfishRestExEventService (
  IN      EFI_REST_EX_PROTOCOL   *This,
  IN      EFI_HTTP_MESSAGE       *RequestMessage OPTIONAL,
  IN      EFI_REST_EX_TOKEN      *RestExToken
  )
{
  return EFI_UNSUPPORTED;
}

