/** @file
  This file defines the Redfish debug library interface.

  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_DEBUG_LIB_H_
#define REDFISH_DEBUG_LIB_H_

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <RedfishServiceData.h>
#include <Library/HiiUtilityLib.h>
#include <Library/JsonLib.h>

#include <Protocol/EdkIIRedfishPlatformConfig.h>

// Used with MdePKg DEBUG macro.
#define DEBUG_REDFISH_NETWORK          DEBUG_MANAGEABILITY          ///< Debug error level for Redfish networking function
#define DEBUG_REDFISH_HOST_INTERFACE   DEBUG_MANAGEABILITY          ///< Debug error level for Redfish Host INterface
#define DEBUG_REDFISH_PLATFORM_CONFIG  DEBUG_MANAGEABILITY          ///< Debug error level for Redfish Platform Configure Driver

//
// Definitions of Redfish debug capability in Redfish component scope, used with DEBUG_REDFISH macro
// For example, Redfish Platform Config Driver
//   DEBUG_REDFISH(DEBUG_REDFISH_PLATFORM_CONFIG_DXE, ...)
//
#define DEBUG_REDFISH_COMPONENT_PLATFORM_CONFIG_DXE  0x00000001

#define DEBUG_REDFISH(DebugCategory, ...) \
    do {                                                \
      if (!DebugPrintEnabled()) {                       \
        break;                                          \
      }                                                 \
      if (!DebugRedfishComponentEnabled (DebugCategory)) { \
        break;                                             \
      }                                                    \
      DEBUG ((DEBUG_MANAGEABILITY, ##__VA_ARGS__));       \
    } while (FALSE)

/**
  Determine whether the Redfish debug category is enabled in
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishDebugCategory.

  @param[in]  DebugCategory  Redfish debug category.

  @retval     TRUE   This debug category is enabled.
  @retval     FALSE  This debug category is disabled..
**/
BOOLEAN
DebugRedfishComponentEnabled (
  IN  UINT64  DebugCategory
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

/**
  Debug output raw data buffer.

  @param[in]    ErrorLevel  DEBUG macro error level
  @param[in]    Buffer      Debug output data buffer.
  @param[in]    BufferSize  The size of Buffer in byte.

  @retval EFI_SUCCESS             Debug dump finished.
  @retval EFI_INVALID_PARAMETER   Buffer is NULL.

**/
EFI_STATUS
DumpBuffer (
  IN  UINTN  ErrorLevel,
  IN  UINT8  *Buffer,
  IN  UINTN  BufferSize
  );

#endif
