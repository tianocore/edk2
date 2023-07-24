/** @file
  This file defines the Redfish debug library interface.

  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_DEBUG_LIB_H_
#define REDFISH_DEBUG_LIB_H_

#include <Uefi.h>
#include <Library/HiiUtilityLib.h>
#include <Library/JsonLib.h>
#include <Library/RedfishLib.h>

#include <Protocol/EdkIIRedfishPlatformConfig.h>

#define DEBUG_REDFISH_NETWORK         DEBUG_MANAGEABILITY   ///< Debug error level for Redfish networking function
#define DEBUG_REDFISH_HOST_INTERFACE  DEBUG_MANAGEABILITY   ///< Debug error level for Redfish networking function

/**
  Debug print the value of StatementValue.

  @param[in]  ErrorLevel     DEBUG macro error level.
  @param[in]  StatementValue The statement value to print.

  @retval     EFI_SUCCESS            StatementValue is printed.
  @retval     EFI_INVALID_PARAMETER  StatementValue is NULL.
**/
EFI_STATUS
DumpHiiStatementValue (
  IN UINTN                ErrorLevel,
  IN HII_STATEMENT_VALUE  *StatementValue
  );

/**
  Debug print the value of RedfishValue.

  @param[in]  ErrorLevel     DEBUG macro error level.
  @param[in]  RedfishValue   The statement value to print.

  @retval     EFI_SUCCESS            RedfishValue is printed.
  @retval     EFI_INVALID_PARAMETER  RedfishValue is NULL.
**/
EFI_STATUS
DumpRedfishValue (
  IN UINTN                ErrorLevel,
  IN EDKII_REDFISH_VALUE  *RedfishValue
  );

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

/**

  This function dump the IPv4 address in given error level.

  @param[in]  ErrorLevel  DEBUG macro error level
  @param[in]  Ipv4Address IPv4 address to dump

  @retval     EFI_SUCCESS         IPv4 address string is printed.
  @retval     Others              Errors occur.

**/
EFI_STATUS
DumpIpv4Address (
  IN UINTN             ErrorLevel,
  IN EFI_IPv4_ADDRESS  *Ipv4Address
  );

#endif
