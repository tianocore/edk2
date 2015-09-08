/** @file
  Implementation of EFI_HTTP_PROTOCOL protocol interfaces.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HttpUtilitiesDxe.h"

EFI_HTTP_UTILITIES_PROTOCOL mHttpUtilitiesProtocol = {
  HttpUtilitiesBuild,
  HttpUtilitiesParse
};


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
  )
{
  EFI_STATUS                Status;
  EFI_HTTP_HEADER           *SeedHeaderFields;
  UINTN                     SeedFieldCount;
  UINTN                     Index;
  EFI_HTTP_HEADER           *TempHeaderFields;
  UINTN                     TempFieldCount;
  EFI_HTTP_HEADER           *NewHeaderFields;
  UINTN                     NewFieldCount;
  EFI_HTTP_HEADER           *HttpHeader;
  UINTN                     StrLength;
  UINT8                     *NewMessagePtr;

  SeedHeaderFields = NULL;
  SeedFieldCount   = 0;
  TempHeaderFields = NULL;
  TempFieldCount   = 0;
  NewHeaderFields  = NULL;
  NewFieldCount    = 0;

  HttpHeader       = NULL;
  StrLength        = 0;
  NewMessagePtr    = NULL;
  *NewMessageSize  = 0;
  Status           = EFI_SUCCESS;
  
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (SeedMessage != NULL) {
    Status = This->Parse (
                     This, 
                     SeedMessage, 
                     SeedMessageSize, 
                     &SeedHeaderFields, 
                     &SeedFieldCount
                     );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Handle DeleteList
  //
  if (SeedFieldCount != 0 && DeleteCount != 0) {
    TempHeaderFields = AllocateZeroPool (SeedFieldCount * sizeof(EFI_HTTP_HEADER));
    if (TempHeaderFields == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }
    
    for (Index = 0, TempFieldCount = 0; Index < SeedFieldCount; Index++) {
      //
      // Check whether each SeedHeaderFields member is in DeleteList
      //
      if (IsValidHttpHeader( DeleteList, DeleteCount, SeedHeaderFields[Index].FieldName)) {
        Status = SetFieldNameAndValue (
                   &TempHeaderFields[TempFieldCount], 
                   SeedHeaderFields[Index].FieldName, 
                   SeedHeaderFields[Index].FieldValue
                   );
        if (EFI_ERROR (Status)) {
          goto ON_EXIT;
        }
        TempFieldCount++;
      }
    }
  } else {
    TempHeaderFields = SeedHeaderFields;
    TempFieldCount = SeedFieldCount;
  }

  //
  // Handle AppendList
  //
  NewHeaderFields = AllocateZeroPool ((TempFieldCount + AppendCount) * sizeof (EFI_HTTP_HEADER));
  if (NewHeaderFields == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  for (Index = 0; Index < TempFieldCount; Index++) {
    Status = SetFieldNameAndValue (
               &NewHeaderFields[Index], 
               TempHeaderFields[Index].FieldName, 
               TempHeaderFields[Index].FieldValue
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }
  
  NewFieldCount = TempFieldCount;

  for (Index = 0; Index < AppendCount; Index++) {
    HttpHeader = FindHttpHeader (NewHeaderFields, NewFieldCount, AppendList[Index]->FieldName);
    if (HttpHeader != NULL) {
      Status = SetFieldNameAndValue (
                 HttpHeader, 
                 AppendList[Index]->FieldName, 
                 AppendList[Index]->FieldValue
                 );
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }
    } else {
      Status = SetFieldNameAndValue (
                 &NewHeaderFields[NewFieldCount], 
                 AppendList[Index]->FieldName, 
                 AppendList[Index]->FieldValue
                 );
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }
      NewFieldCount++;
    }
  }

  //
  // Calculate NewMessageSize, then build NewMessage
  //
  for (Index = 0; Index < NewFieldCount; Index++) {
    HttpHeader = &NewHeaderFields[Index];

    StrLength = AsciiStrLen (HttpHeader->FieldName);
    *NewMessageSize += StrLength;

    StrLength = sizeof(": ") - 1;
    *NewMessageSize += StrLength;

    StrLength = AsciiStrLen (HttpHeader->FieldValue);
    *NewMessageSize += StrLength;

    StrLength = sizeof("\r\n") - 1;
    *NewMessageSize += StrLength;
  }
  StrLength = sizeof("\r\n") - 1;
  *NewMessageSize += StrLength;

  //
  // Final 0 for end flag
  //
  *NewMessageSize += 1; 

  *NewMessage = AllocateZeroPool (*NewMessageSize);
  if (*NewMessage == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  NewMessagePtr = (UINT8 *)(*NewMessage);

  for (Index = 0; Index < NewFieldCount; Index++) {
    HttpHeader = &NewHeaderFields[Index];

    StrLength = AsciiStrLen (HttpHeader->FieldName);
    CopyMem (NewMessagePtr, HttpHeader->FieldName, StrLength);
    NewMessagePtr += StrLength;

    StrLength = sizeof(": ") - 1;
    CopyMem (NewMessagePtr, ": ", StrLength);
    NewMessagePtr += StrLength;

    StrLength = AsciiStrLen (HttpHeader->FieldValue);
    CopyMem (NewMessagePtr, HttpHeader->FieldValue, StrLength);
    NewMessagePtr += StrLength;

    StrLength = sizeof("\r\n") - 1;
    CopyMem (NewMessagePtr, "\r\n", StrLength);
    NewMessagePtr += StrLength;
  }
  StrLength = sizeof("\r\n") - 1;
  CopyMem (NewMessagePtr, "\r\n", StrLength);
  NewMessagePtr += StrLength;

  *NewMessagePtr = 0;

  ASSERT (*NewMessageSize == (UINTN)NewMessagePtr - (UINTN)(*NewMessage) + 1);

  //
  // Free allocated buffer 
  //
ON_EXIT:
  if (SeedHeaderFields != NULL) {
    FreeHeaderFields(SeedHeaderFields, SeedFieldCount);
  }
  
  if (TempHeaderFields != NULL) {
    FreeHeaderFields(TempHeaderFields, TempFieldCount);
  }

  if (NewHeaderFields != NULL) {
    FreeHeaderFields(NewHeaderFields, NewFieldCount);
  }
  
  return Status;
}


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
  )
{
  EFI_STATUS                Status;
  CHAR8                     *TempHttpMessage;
  CHAR8                     *Token;
  CHAR8                     *NextToken;
  CHAR8                     *FieldName;
  CHAR8                     *FieldValue;
  UINTN                     Index;
  
  Status          = EFI_SUCCESS;
  TempHttpMessage = NULL;
  Token           = NULL;
  NextToken       = NULL;
  FieldName       = NULL;
  FieldValue      = NULL;
  Index           = 0;
  
  if (This == NULL || HttpMessage == NULL || HeaderFields == NULL || FieldCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  TempHttpMessage = AllocateZeroPool (HttpMessageSize);
  if (TempHttpMessage == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TempHttpMessage, HttpMessage, HttpMessageSize);
  
  //
  // Get header number
  //
  *FieldCount = 0;
  Token = TempHttpMessage;
  while (TRUE) {
    FieldName     = NULL;
    FieldValue    = NULL;
    NextToken = GetFieldNameAndValue (Token, &FieldName, &FieldValue);
    Token     = NextToken;
    if (FieldName == NULL || FieldValue == NULL) {
      break;
    }

    (*FieldCount)++;
  }

  if (*FieldCount == 0) {
    Status =  EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  
  //
  // Allocate buffer for header
  //
  *HeaderFields = AllocateZeroPool ((*FieldCount) * sizeof(EFI_HTTP_HEADER));
  if (*HeaderFields == NULL) {
    *FieldCount = 0;
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  
  CopyMem (TempHttpMessage, HttpMessage, HttpMessageSize);
  
  //
  // Set Field and Value to each header
  //
  Token = TempHttpMessage;
  while (Index < *FieldCount) {
    FieldName     = NULL;
    FieldValue    = NULL;
    NextToken = GetFieldNameAndValue (Token, &FieldName, &FieldValue);
    Token     = NextToken;
    if (FieldName == NULL || FieldValue == NULL) {
      break;
    }

    Status = SetFieldNameAndValue (&(*HeaderFields)[Index], FieldName, FieldValue);
    if (EFI_ERROR (Status)) {
      *FieldCount = 0;
      FreeHeaderFields (*HeaderFields, Index);
      goto ON_EXIT;
    }
    
    Index++;
  }

  //
  // Free allocated buffer 
  //
ON_EXIT:
  if (TempHttpMessage != NULL) {
    FreePool (TempHttpMessage);
  }
  
  return Status;
}