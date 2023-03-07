/** @file
  This file defines the Redfish debug library interface.

  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_DEBUG_LIB_H_
#define REDFISH_DEBUG_LIB_H_

#include <Uefi.h>
#include <Library/JsonLib.h>
#include <Library/RedfishLib.h>

#define DEBUG_REDFISH_NETWORK         DEBUG_INFO   ///< Debug error level for Redfish networking function
#define DEBUG_REDFISH_HOST_INTERFACE  DEBUG_INFO   ///< Debug error level for Redfish networking function

/**

  This function dump the Json string in given error level.

  @param[in]  ErrorLevel  DEBUG macro error level
  @param[in]  JsonValue   Json value to dump.

  @retval     EFI_SUCCESS         Json string is printed.
  @retval     Others              Errors occur.

**/
EFI_STATUS
DumpJsonValue (
  IN UINTN             ErrorLevel,
  IN EDKII_JSON_VALUE  JsonValue
  );

/**

  This function dump the status code, header and body in given
  Redfish payload.

  @param[in]  ErrorLevel  DEBUG macro error level
  @param[in]  Payload     Redfish payload to dump

  @retval     EFI_SUCCESS         Redfish payload is printed.
  @retval     Others              Errors occur.

**/
EFI_STATUS
DumpRedfishPayload (
  IN UINTN            ErrorLevel,
  IN REDFISH_PAYLOAD  Payload
  );

/**

  This function dump the status code, header and body in given
  Redfish response.

  @param[in]  Message     Message string
  @param[in]  ErrorLevel  DEBUG macro error level
  @param[in]  Response    Redfish response to dump

  @retval     EFI_SUCCESS         Redfish response is printed.
  @retval     Others              Errors occur.

**/
EFI_STATUS
DumpRedfishResponse (
  IN CONST CHAR8       *Message,
  IN UINTN             ErrorLevel,
  IN REDFISH_RESPONSE  *Response
  );

/**

  This function dump the HTTP status code.

  @param[in]  ErrorLevel     DEBUG macro error level
  @param[in]  HttpStatusCode HTTP status code

  @retval     EFI_SUCCESS    HTTP status code is printed

**/
EFI_STATUS
DumpHttpStatusCode (
  IN UINTN                 ErrorLevel,
  IN EFI_HTTP_STATUS_CODE  HttpStatusCode
  );

#endif
