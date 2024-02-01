/** @file
  Definitions of RedfishHttpOperation

  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_HTTP_OPERATION_H_
#define EDKII_REDFISH_HTTP_OPERATION_H_

#include "RedfishHttpDxe.h"

#define REDFISH_CONTENT_LENGTH_SIZE              80
#define REDFISH_COMMON_HEADER_SIZE               5
#define REDFISH_HTTP_HEADER_ODATA_VERSION_STR    "OData-Version"
#define REDFISH_HTTP_HEADER_ODATA_VERSION_VALUE  "4.0"
#define REDFISH_HTTP_HEADER_USER_AGENT_VALUE     "edk2redfish"
#define REDFISH_HTTP_HEADER_CONNECTION_STR       "Connection"
#define REDFISH_HTTP_HEADER_CONNECTION_VALUE     "Keep-Alive"
#define REDFISH_HTTP_CONTENT_ENCODING_NONE       "None"
#define ASCII_STR_DUPLICATE(a)  (AllocateCopyPool (AsciiStrSize ((a)), (a)))

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
  );

/**
  This function free resources in given Response.

  @param[in]  Response     HTTP response to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
ReleaseRedfishResponse (
  IN  REDFISH_RESPONSE  *Response
  );

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
  );

#endif
