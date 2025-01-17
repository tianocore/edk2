/** @file
  Redfish debug library to debug Redfish application.

  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <RedfishCommon.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RedfishDebugLib.h>
#include <Library/RedfishHttpLib.h>
#include <Library/UefiLib.h>

#define REDFISH_JSON_STRING_LENGTH          200
#define REDFISH_JSON_OUTPUT_FORMAT          (EDKII_JSON_COMPACT | EDKII_JSON_INDENT(2))
#define REDFISH_PRINT_BUFFER_BYTES_PER_ROW  16

/**
  Determine whether the Redfish debug category is enabled in
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishDebugCategory.

  @param[in]  RedfishDebugCategory  Redfish debug category.

  @retval     TRUE   This debug category is enabled.
  @retval     FALSE  This debug category is disabled..
**/
BOOLEAN
DebugRedfishComponentEnabled (
  IN  UINT64  RedfishDebugCategory
  )
{
  UINT64  DebugCategory;

  DebugCategory = FixedPcdGet64 (PcdRedfishDebugCategory);
  return ((DebugCategory & RedfishDebugCategory) != 0);
}

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
  )
{
  UINTN  Index;

  if (RedfishValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((ErrorLevel, "Type:       0x%x\n", RedfishValue->Type));
  DEBUG ((ErrorLevel, "ArrayCount: 0x%x\n", RedfishValue->ArrayCount));

  switch (RedfishValue->Type) {
    case RedfishValueTypeInteger:
      DEBUG ((ErrorLevel, "Value:      0x%x\n", RedfishValue->Value.Integer));
      break;
    case RedfishValueTypeBoolean:
      DEBUG ((ErrorLevel, "Value:      %a\n", (RedfishValue->Value.Boolean ? "true" : "false")));
      break;
    case RedfishValueTypeString:
      DEBUG ((ErrorLevel, "Value:      %a\n", RedfishValue->Value.Buffer));
      break;
    case RedfishValueTypeStringArray:
      for (Index = 0; Index < RedfishValue->ArrayCount; Index++) {
        DEBUG ((ErrorLevel, "Value[%d]:      %a\n", Index, RedfishValue->Value.StringArray[Index]));
      }

      break;
    case RedfishValueTypeIntegerArray:
      for (Index = 0; Index < RedfishValue->ArrayCount; Index++) {
        DEBUG ((ErrorLevel, "Value[%d]:      0x%x\n", Index, RedfishValue->Value.IntegerArray[Index]));
      }

      break;
    case RedfishValueTypeBooleanArray:
      for (Index = 0; Index < RedfishValue->ArrayCount; Index++) {
        DEBUG ((ErrorLevel, "Value[%d]:      %a\n", Index, (RedfishValue->Value.BooleanArray[Index] ? "true" : "false")));
      }

      break;
    case RedfishValueTypeUnknown:
    case RedfishValueTypeMax:
    default:
      break;
  }

  return EFI_SUCCESS;
}

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
  )
{
  CHAR8  *String;
  CHAR8  *Runner;
  CHAR8  Buffer[REDFISH_JSON_STRING_LENGTH + 1];
  UINTN  StrLen;
  UINTN  Count;
  UINTN  Index;

  if (JsonValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  String = JsonDumpString (JsonValue, REDFISH_JSON_OUTPUT_FORMAT);
  if (String == NULL) {
    return EFI_UNSUPPORTED;
  }

  StrLen = AsciiStrLen (String);
  if (StrLen == 0) {
    return EFI_UNSUPPORTED;
  }

  Count  = StrLen / REDFISH_JSON_STRING_LENGTH;
  Runner = String;
  for (Index = 0; Index < Count; Index++) {
    AsciiStrnCpyS (Buffer, (REDFISH_JSON_STRING_LENGTH + 1), Runner, REDFISH_JSON_STRING_LENGTH);
    Buffer[REDFISH_JSON_STRING_LENGTH] = '\0';
    DEBUG ((ErrorLevel, "%a", Buffer));
    Runner += REDFISH_JSON_STRING_LENGTH;
  }

  Count = StrLen % REDFISH_JSON_STRING_LENGTH;
  if (Count > 0) {
    DEBUG ((ErrorLevel, "%a", Runner));
  }

  DEBUG ((ErrorLevel, "\n"));

  FreePool (String);
  return EFI_SUCCESS;
}

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
  )
{
  EDKII_JSON_VALUE  JsonValue;

  if (Payload == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  JsonValue = RedfishJsonInPayload (Payload);
  if (JsonValue != NULL) {
    DEBUG ((ErrorLevel, "Payload:\n"));
    DumpJsonValue (ErrorLevel, JsonValue);
  }

  return EFI_SUCCESS;
}

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
  )
{
  switch (HttpStatusCode) {
    case HTTP_STATUS_100_CONTINUE:
      DEBUG ((ErrorLevel, "Status code: 100 CONTINUE\n"));
      break;
    case HTTP_STATUS_200_OK:
      DEBUG ((ErrorLevel, "Status code: 200 OK\n"));
      break;
    case HTTP_STATUS_201_CREATED:
      DEBUG ((ErrorLevel, "Status code: 201 CREATED\n"));
      break;
    case HTTP_STATUS_202_ACCEPTED:
      DEBUG ((ErrorLevel, "Status code: 202 ACCEPTED\n"));
      break;
    case HTTP_STATUS_304_NOT_MODIFIED:
      DEBUG ((ErrorLevel, "Status code: 304 NOT MODIFIED\n"));
      break;
    case HTTP_STATUS_400_BAD_REQUEST:
      DEBUG ((ErrorLevel, "Status code: 400 BAD REQUEST\n"));
      break;
    case HTTP_STATUS_401_UNAUTHORIZED:
      DEBUG ((ErrorLevel, "Status code: 401 UNAUTHORIZED\n"));
      break;
    case HTTP_STATUS_403_FORBIDDEN:
      DEBUG ((ErrorLevel, "Status code: 403 FORBIDDEN\n"));
      break;
    case HTTP_STATUS_404_NOT_FOUND:
      DEBUG ((ErrorLevel, "Status code: 404 NOT FOUND\n"));
      break;
    case HTTP_STATUS_405_METHOD_NOT_ALLOWED:
      DEBUG ((ErrorLevel, "Status code: 405 METHOD NOT ALLOWED\n"));
      break;
    case HTTP_STATUS_500_INTERNAL_SERVER_ERROR:
      DEBUG ((ErrorLevel, "Status code: 500 INTERNAL SERVER ERROR\n"));
      break;
    default:
      DEBUG ((ErrorLevel, "Status code: 0x%x\n", HttpStatusCode));
      break;
  }

  return EFI_SUCCESS;
}

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
  )
{
  UINTN  Index;

  if (Response == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_EMPTY_STRING (Message)) {
    DEBUG ((ErrorLevel, "%a\n", Message));
  }

  //
  // status code
  //
  if (Response->StatusCode != NULL) {
    DumpHttpStatusCode (ErrorLevel, *(Response->StatusCode));
  }

  //
  // header
  //
  if (Response->HeaderCount > 0) {
    DEBUG ((ErrorLevel, "Header: %d\n", Response->HeaderCount));
    for (Index = 0; Index < Response->HeaderCount; Index++) {
      DEBUG ((ErrorLevel, "  %a: %a\n", Response->Headers[Index].FieldName, Response->Headers[Index].FieldValue));
    }
  }

  //
  // Body
  //
  if (Response->Payload != NULL) {
    DumpRedfishPayload (ErrorLevel, Response->Payload);
  }

  return EFI_SUCCESS;
}

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
  )
{
  if (Ipv4Address == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((ErrorLevel, "%d.%d.%d.%d\n", Ipv4Address->Addr[0], Ipv4Address->Addr[1], Ipv4Address->Addr[2], Ipv4Address->Addr[3]));

  return EFI_SUCCESS;
}

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
  )
{
  UINTN  Index;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((ErrorLevel, "Address: 0x%p size: %d\n", Buffer, BufferSize));
  for (Index = 0; Index < BufferSize; Index++) {
    if (Index % REDFISH_PRINT_BUFFER_BYTES_PER_ROW == 0) {
      DEBUG ((ErrorLevel, "\n%04X: ", Index));
    }

    DEBUG ((ErrorLevel, "%02X ", Buffer[Index]));
  }

  DEBUG ((ErrorLevel, "\n"));

  return EFI_SUCCESS;
}
