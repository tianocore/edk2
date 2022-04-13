/** @file
  RestExDxe support functions implementation.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include "RedfishRestExInternal.h"

/**
  Create a new TLS session becuase the previous on is closed.
  status.

  @param[in]  Instance            Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                  REST service.
  @retval EFI_SUCCESS             operation succeeded.
  @retval EFI_ERROR               Other errors.

**/
EFI_STATUS
ResetHttpTslSession (
  IN   RESTEX_INSTANCE  *Instance
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a: TCP connection is finished. Could be TSL session closure, reset HTTP instance for the new TLS session.\n", __FUNCTION__));

  Status = Instance->HttpIo.Http->Configure (Instance->HttpIo.Http, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error to reset HTTP instance.\n", __FUNCTION__));
    return Status;
  }

  Status = Instance->HttpIo.Http->Configure (Instance->HttpIo.Http, &((EFI_REST_EX_HTTP_CONFIG_DATA *)Instance->ConfigData)->HttpConfigData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error to re-initiate HTTP instance.\n", __FUNCTION__));
  }

  return Status;
}

/**
  This function check Http receive status.

  @param[in]  Instance             Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                   REST service.
  @param[in]  HttpIoReceiveStatus  This is the status return from HttpIoRecvResponse

  @retval EFI_SUCCESS           The payload receive from Redfish service in successfully.
  @retval EFI_NOT_READY         May need to resend the HTTP request.
  @retval EFI_DEVICE_ERROR      Something wrong and can't be resolved.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
RedfishCheckHttpReceiveStatus (
  IN RESTEX_INSTANCE  *Instance,
  IN EFI_STATUS       HttpIoReceiveStatus
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus;

  if (!EFI_ERROR (HttpIoReceiveStatus)) {
    ReturnStatus = EFI_SUCCESS;
  } else if (HttpIoReceiveStatus != EFI_CONNECTION_FIN) {
    if ((Instance->Flags & RESTEX_INSTANCE_FLAGS_TCP_ERROR_RETRY) == 0) {
      DEBUG ((DEBUG_ERROR, "%a: TCP error, reset HTTP session.\n", __FUNCTION__));
      Instance->Flags |= RESTEX_INSTANCE_FLAGS_TCP_ERROR_RETRY;
      gBS->Stall (500);
      Status = ResetHttpTslSession (Instance);
      if (!EFI_ERROR (Status)) {
        return EFI_NOT_READY;
      }

      DEBUG ((DEBUG_ERROR, "%a: Reset HTTP instance fail.\n", __FUNCTION__));
    }

    ReturnStatus = EFI_DEVICE_ERROR;
  } else {
    if ((Instance->Flags & RESTEX_INSTANCE_FLAGS_TLS_RETRY) != 0) {
      DEBUG ((DEBUG_ERROR, "%a: REST_EX Send and receive fail even with a new TLS session.\n", __FUNCTION__));
      ReturnStatus = EFI_DEVICE_ERROR;
    }

    Instance->Flags |= RESTEX_INSTANCE_FLAGS_TLS_RETRY;
    Status           = ResetHttpTslSession (Instance);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Reset HTTP instance fail.\n", __FUNCTION__));
      ReturnStatus = EFI_DEVICE_ERROR;
    }

    return EFI_NOT_READY;
  }

  //
  // Clean TLS new session retry and error try flags.
  //
  Instance->Flags &= ~(RESTEX_INSTANCE_FLAGS_TLS_RETRY | RESTEX_INSTANCE_FLAGS_TCP_ERROR_RETRY);
  return ReturnStatus;
}

/**
  This function send the HTTP request without body to see
  if the write to URL is permitted by Redfish service. This function
  checks if the HTTP request has Content-length in HTTP header. If yes,
  set HTTP body to NULL and then send to service. Check the HTTP status
  for the firther actions.

  @param[in]  This                    Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                      REST service.
  @param[in]  RequestMessage          Pointer to the HTTP request data for this resource
  @param[in]  PreservedRequestHeaders The pointer to save the request headers
  @param[in]  ItsWrite                This is write method to URL.

  @retval EFI_INVALID_PARAMETER  Improper given parameters.
  @retval EFI_SUCCESS            This HTTP request is free to send to Redfish service.
  @retval EFI_OUT_OF_RESOURCES   NOt enough memory to process.
  @retval EFI_ACCESS_DENIED      Not allowed to write to this URL.

  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
RedfishHttpAddExpectation (
  IN EFI_REST_EX_PROTOCOL  *This,
  IN EFI_HTTP_MESSAGE      *RequestMessage,
  IN EFI_HTTP_HEADER       **PreservedRequestHeaders,
  IN BOOLEAN               *ItsWrite
  )
{
  EFI_HTTP_HEADER  *NewHeaders;

  if ((This == NULL) || (RequestMessage == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *ItsWrite = FALSE;
  if (PreservedRequestHeaders != NULL) {
    *PreservedRequestHeaders = RequestMessage->Headers;
  }

  if ((RequestMessage->Data.Request->Method != HttpMethodPut) && (RequestMessage->Data.Request->Method != HttpMethodPost) &&
      (RequestMessage->Data.Request->Method != HttpMethodPatch))
  {
    return EFI_SUCCESS;
  }

  *ItsWrite = TRUE;

  NewHeaders = AllocateZeroPool ((RequestMessage->HeaderCount + 1) * sizeof (EFI_HTTP_HEADER));
  CopyMem ((VOID *)NewHeaders, (VOID *)RequestMessage->Headers, RequestMessage->HeaderCount * sizeof (EFI_HTTP_HEADER));
  HttpSetFieldNameAndValue (NewHeaders + RequestMessage->HeaderCount, HTTP_HEADER_EXPECT, HTTP_EXPECT_100_CONTINUE);
  RequestMessage->HeaderCount++;
  RequestMessage->Headers = NewHeaders;
  return EFI_SUCCESS;
}
