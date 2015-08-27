/** @file
  The header files of Http Utilities functions for HttpUtilities driver.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_HTTP_UTILITIES_DXE_H__
#define __EFI_HTTP_UTILITIES_DXE_H__

#include <Uefi.h>

//
// Libraries
//
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>

//
// Consumed Protocols
//
#include <Protocol/HttpUtilities.h>
#include <Protocol/Http.h>

//
// Protocol instances
//
extern EFI_HTTP_UTILITIES_PROTOCOL mHttpUtilitiesProtocol;


/**
  Free existing HeaderFields.

  @param[in]  HeaderFields       Pointer to array of key/value header pairs waitting for free.
  @param[in]  FieldCount         The number of header pairs in HeaderFields.

**/
VOID
FreeHeaderFields (
  IN  EFI_HTTP_HEADER  *HeaderFields,
  IN  UINTN            FieldCount
  );


/**
  Find required header field in HeaderFields.

  @param[in]  HeaderFields        Pointer to array of key/value header pairs.
  @param[in]  FieldCount          The number of header pairs.
  @param[in]  FieldName           Pointer to header field's name.

  @return     Pointer to the queried header field.
  @return     NULL if not find this required header field.

**/
EFI_HTTP_HEADER *
FindHttpHeader (
  IN  EFI_HTTP_HEADER  *HeaderFields,
  IN  UINTN            FieldCount,
  IN  CHAR8            *FieldName
  );


/**
  Check whether header field called FieldName is in DeleteList.

  @param[in]  DeleteList        Pointer to array of key/value header pairs.
  @param[in]  DeleteCount       The number of header pairs.
  @param[in]  FieldName         Pointer to header field's name.

  @return     TRUE if FieldName is not in DeleteList, that means this header field is valid.
  @return     FALSE if FieldName is in DeleteList, that means this header field is invalid.

**/
BOOLEAN
IsValidHttpHeader (
  IN  CHAR8            *DeleteList[],
  IN  UINTN            DeleteCount,
  IN  CHAR8            *FieldName
  );


/**
  Set FieldName and FieldValue into specified HttpHeader.

  @param[in]  HttpHeader          Specified HttpHeader.
  @param[in]  FieldName           FieldName of this HttpHeader.
  @param[in]  FieldValue          FieldValue of this HttpHeader.


  @retval EFI_SUCCESS             The FieldName and FieldValue are set into HttpHeader successfully.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate resources.

**/
EFI_STATUS
SetFieldNameAndValue (
  IN  EFI_HTTP_HEADER     *HttpHeader,
  IN  CHAR8               *FieldName, 
  IN  CHAR8               *FieldValue
  );


/**
  Get one key/value header pair from the raw string.

  @param[in]  String             Pointer to the raw string.
  @param[out] FieldName          Pointer to header field's name.
  @param[out] FieldValue         Pointer to header field's value.

  @return     Pointer to the next raw string.
  @return     NULL if no key/value header pair from this raw string.

**/
CHAR8 *
GetFieldNameAndValue (
  IN  CHAR8   *String,
  OUT CHAR8   **FieldName,
  OUT CHAR8   **FieldValue
  );


/**
  Create HTTP header based on a combination of seed header, fields
  to delete, and fields to append.

  The Build() function is used to manage the headers portion of an
  HTTP message by providing the ability to add, remove, or replace
  HTTP headers.

  @param[in]  This                Pointer to EFI_HTTP_UTILITIES_PROTOCOL instance.
  @param[in]  SeedMessageSize     Size of the initial HTTP header. This can be zero.
  @param[in]  SeedMessage         Initial HTTP header to be used as a base for
                                  building a new HTTP header. If NULL,
                                  SeedMessageSize is ignored.
  @param[in]  DeleteCount         Number of null-terminated HTTP header field names
                                  in DeleteList.
  @param[in]  DeleteList          List of null-terminated HTTP header field names to
                                  remove from SeedMessage. Only the field names are
                                  in this list because the field values are irrelevant
                                  to this operation.
  @param[in]  AppendCount         Number of header fields in AppendList.
  @param[in]  AppendList          List of HTTP headers to populate NewMessage with.
                                  If SeedMessage is not NULL, AppendList will be
                                  appended to the existing list from SeedMessage in
                                  NewMessage.
  @param[out] NewMessageSize      Pointer to number of header fields in NewMessage.
  @param[out] NewMessage          Pointer to a new list of HTTP headers based on.

  @retval EFI_SUCCESS             Add, remove, and replace operations succeeded.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory for NewMessage.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
**/
EFI_STATUS
EFIAPI
HttpUtilitiesBuild (
  IN     EFI_HTTP_UTILITIES_PROTOCOL *This,
  IN     UINTN                       SeedMessageSize,
  IN     VOID                        *SeedMessage, OPTIONAL
  IN     UINTN                       DeleteCount,
  IN     CHAR8                       *DeleteList[], OPTIONAL
  IN     UINTN                       AppendCount,
  IN     EFI_HTTP_HEADER             *AppendList[], OPTIONAL
     OUT UINTN                       *NewMessageSize,
     OUT VOID                        **NewMessage
  );


/**
  Parses HTTP header and produces an array of key/value pairs.

  The Parse() function is used to transform data stored in HttpHeader
  into a list of fields paired with their corresponding values.

  @param[in]  This                Pointer to EFI_HTTP_UTILITIES_PROTOCOL instance.
  @param[in]  HttpMessage         Contains raw unformatted HTTP header string.
  @param[in]  HttpMessageSize     Size of HTTP header.
  @param[out] HeaderFields        Array of key/value header pairs.
  @param[out] FieldCount          Number of headers in HeaderFields.

  @retval EFI_SUCCESS             Allocation succeeded.
  @retval EFI_NOT_STARTED         This EFI HTTP Protocol instance has not been
                                  initialized.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  HttpMessage is NULL.
                                  HeaderFields is NULL.
                                  FieldCount is NULL.
**/
EFI_STATUS
EFIAPI
HttpUtilitiesParse (
  IN  EFI_HTTP_UTILITIES_PROTOCOL  *This,
  IN  CHAR8                        *HttpMessage,
  IN  UINTN                        HttpMessageSize,
  OUT EFI_HTTP_HEADER              **HeaderFields,
  OUT UINTN                        *FieldCount
  );

#endif
