/** @file

Implementation of help functions to parse HTTP message header.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HttpDriver.h"

/**
  Get the next string, which is distinguished by specified seperator. 

  @param[in]  String             Pointer to the string.
  @param[in]  Seperator          Specified seperator used to distinguish where is the beginning 
                                 of next string.

  @return     Pointer to the next string.
  @return     NULL if not find or String is NULL.

**/
CHAR8 *
AsciiStrGetNextToken (
  IN CONST CHAR8 *String,
  IN       CHAR8 Seperator
  )
{
  CONST CHAR8 *Token;

  Token = String;
  while (TRUE) {
    if (*Token == 0) {
      return NULL;
    }
    if (*Token == Seperator) {
      return (CHAR8 *) (Token + 1);
    }
    Token++;
  }
}

/**
  Free existing HeaderFields.

  @param[in]  HeaderFields       Pointer to array of key/value header pairs waitting for free.
  @param[in]  FieldCount         The number of header pairs in HeaderFields.

**/
VOID
FreeHeaderFields (
  IN  EFI_HTTP_HEADER  *HeaderFields,
  IN  UINTN            FieldCount
  )
{
  UINTN                       Index;
  
  if (HeaderFields != NULL) {
    for (Index = 0; Index < FieldCount; Index++) {
      if(HeaderFields[Index].FieldName != NULL) {
        FreePool (HeaderFields[Index].FieldName);
      }
      if(HeaderFields[Index].FieldValue != NULL) {
        FreePool (HeaderFields[Index].FieldValue);
      }
    }

    FreePool (HeaderFields);
  }
}

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
  )
{
  UINTN                       Index;

  for (Index = 0; Index < FieldCount; Index++) {
    if (AsciiStrCmp (FieldName, HeaderFields[Index].FieldName) == 0) {
      //
      // Find the required header field.
      //
      return &HeaderFields[Index];
    }
  }
  return NULL;
}

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
  )
{
  UINTN                       Index;

  for (Index = 0; Index < DeleteCount; Index++) {
    if (AsciiStrCmp (FieldName, DeleteList[Index]) == 0) {
      return FALSE;
    }
  }
  
  return TRUE;
}

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
  )
{  
  UINTN                       FieldNameSize;
  UINTN                       FieldValueSize;

  if (HttpHeader->FieldName != NULL) {
    FreePool (HttpHeader->FieldName);
  }
  if (HttpHeader->FieldValue != NULL) {
    FreePool (HttpHeader->FieldValue);
  }

  FieldNameSize = AsciiStrSize (FieldName);
  HttpHeader->FieldName = AllocateZeroPool (FieldNameSize);
  if (HttpHeader->FieldName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (HttpHeader->FieldName, FieldName, FieldNameSize);
  HttpHeader->FieldName[FieldNameSize - 1] = 0;

  FieldValueSize = AsciiStrSize (FieldValue);
  HttpHeader->FieldValue = AllocateZeroPool (FieldValueSize);
  if (HttpHeader->FieldValue == NULL) {
    FreePool (HttpHeader->FieldName);
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (HttpHeader->FieldValue, FieldValue, FieldValueSize);
  HttpHeader->FieldValue[FieldValueSize - 1] = 0;

  return EFI_SUCCESS;
}

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
  )
{
  CHAR8  *FieldNameStr;
  CHAR8  *FieldValueStr;
  CHAR8  *StrPtr;

  if (String == NULL || FieldName == NULL || FieldValue == NULL) {
    return NULL;
  }
  
  *FieldName    = NULL;
  *FieldValue   = NULL;
  FieldNameStr  = NULL;
  FieldValueStr = NULL;
  StrPtr        = NULL;

  //
  // Each header field consists of a name followed by a colon (":") and the field value.
  //
  FieldNameStr = String;
  FieldValueStr = AsciiStrGetNextToken (FieldNameStr, ':');
  if (FieldValueStr == NULL) {
    return NULL;
  }
  
  *(FieldValueStr - 1) = 0; /// Replace ':' with 0
  
  //
  // The field value MAY be preceded by any amount of LWS, though a single SP is preferred.
  //
  while (TRUE) {
    if(*FieldValueStr == ' ' || *FieldValueStr == '\t') {
      FieldValueStr ++;
    } else if (*FieldValueStr == '\r' && *(FieldValueStr + 1) == '\n' && 
               (*(FieldValueStr + 2) == ' ' || *(FieldValueStr + 2) == '\t')) {
      FieldValueStr = FieldValueStr + 3;
    } else {
      break;
    }
  }

  //
  // Header fields can be extended over multiple lines by preceding each extra
  // line with at least one SP or HT.
  //
  StrPtr = FieldValueStr;
  do {
    StrPtr = AsciiStrGetNextToken (StrPtr, '\r');
    if (StrPtr == NULL || *StrPtr != '\n') {
      return NULL;
    }
    
    StrPtr++;
  } while (*StrPtr == ' ' || *StrPtr == '\t');

  //
  // Replace '\r' with 0.
  //
  *(StrPtr - 2) = 0;

  //
  // Get FieldName and FieldValue.
  //
  *FieldName = FieldNameStr;
  *FieldValue = FieldValueStr;
    
  return StrPtr;
}

/**
  This function is used to manage the headers portion of an HTTP message by providing 
  the ability to add, remove, or replace HTTP headers.

  @param[in]   SeedMessageSize       Size in bytes of the initial HTTP header. This can be zero.  
  @param[in]   SeedMessage           Initial raw unformatted HTTP header to be used as a base for 
                                     building a new unformatted HTTP header. If NULL, SeedMessageSize 
                                     is ignored. The buffer containing this message will be allocated 
                                     and released by the caller.           
  @param[in]   DeleteCount           Number of null-terminated HTTP header field names in DeleteList.
  @param[in]   DeleteList            List of null-terminated HTTP header field names to remove from SeedMessage. 
                                     Only the field names are in this list because the field values are irrelevant 
                                     to this operation. If NULL, DeleteCount is ignored. The buffer containing the 
                                     list will be allocated and released by the caller.
  @param[in]   AppendCount           Number of header fields in AppendList. 
  @param[in]   AppendList            List of HTTP headers to populate NewMessage with. If SeedMessage is not NULL, 
                                     AppendList will be appended to the existing list from SeedMessage in NewMessage.
  @param[out]  NewMessageSize        Pointer to the size in bytes of the new unformatted HTTP header in NewMessage.       
  @param[out]  NewMessage            Pointer to a new unformatted HTTP header. The storage for this NewMessage is 
                                     allocated by the driver publishing this protocol, and must be freed by the caller. 
  
  @retval EFI_SUCCESS                Add, remove, and replace operations succeeded.
  @retval EFI_OUT_OF_RESOURCES       Could not allocate memory for NewMessage.
  
**/
EFI_STATUS
HttpUtilitiesBuild(
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

  if (SeedMessage != NULL) {
    Status = HttpUtilitiesParse (
              SeedMessage,
              SeedMessageSize,
              &SeedHeaderFields,
              &SeedFieldCount
              );
    if (EFI_ERROR (Status)){
      goto ON_EXIT;
    }
  }

  //
  // Handle DeleteList
  //
  if(SeedFieldCount != 0 && DeleteCount != 0) {
    TempHeaderFields = AllocateZeroPool (SeedFieldCount * sizeof(EFI_HTTP_HEADER));
    if (TempHeaderFields == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }
    
    for (Index = 0, TempFieldCount = 0; Index < SeedFieldCount; Index++) {
      //
      // Check whether each SeedHeaderFields member is in DeleteList
      //
      if (IsValidHttpHeader(DeleteList, DeleteCount, SeedHeaderFields[Index].FieldName)) {
        Status = SetFieldNameAndValue(
                   &TempHeaderFields[TempFieldCount],
                   SeedHeaderFields[Index].FieldName,
                   SeedHeaderFields[Index].FieldValue
                   );
        if (EFI_ERROR (Status)){
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
  NewHeaderFields = AllocateZeroPool ((TempFieldCount + AppendCount) * sizeof(EFI_HTTP_HEADER));
  if (NewHeaderFields == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  for (Index = 0; Index < TempFieldCount; Index++) {
    Status = SetFieldNameAndValue(
               &NewHeaderFields[Index],
               TempHeaderFields[Index].FieldName,
               TempHeaderFields[Index].FieldValue
               );
    if (EFI_ERROR (Status)){
      goto ON_EXIT;
    }
  }
  
  NewFieldCount = TempFieldCount;

  for (Index = 0; Index < AppendCount; Index++) {
    HttpHeader = FindHttpHeader(NewHeaderFields, NewFieldCount, AppendList[Index]->FieldName);
    if(HttpHeader != NULL) {
      Status = SetFieldNameAndValue(
                 HttpHeader,
                 AppendList[Index]->FieldName,
                 AppendList[Index]->FieldValue
                 );
      if (EFI_ERROR (Status)){
        goto ON_EXIT;
      }
    } else {
      Status = SetFieldNameAndValue
                 (&NewHeaderFields[NewFieldCount],
                 AppendList[Index]->FieldName,
                 AppendList[Index]->FieldValue
                 );
      if (EFI_ERROR (Status)){
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

    StrLength = sizeof(HTTP_CRLF_STR) - 1;
    *NewMessageSize += StrLength;
  }
  StrLength = sizeof(HTTP_CRLF_STR) - 1;
  *NewMessageSize += StrLength;
  //
  // Final 0 for end flag.
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

    StrLength = sizeof(HTTP_CRLF_STR) - 1;
    CopyMem (NewMessagePtr, HTTP_CRLF_STR, StrLength);
    NewMessagePtr += StrLength;
  }
  StrLength = sizeof(HTTP_CRLF_STR) - 1;
  CopyMem (NewMessagePtr, HTTP_CRLF_STR, StrLength);
  NewMessagePtr += StrLength;

  *NewMessagePtr = 0;

  ASSERT (*NewMessageSize == (UINTN) NewMessagePtr - (UINTN) (*NewMessage) + 1);

  //
  // Free allocated buffer 
  //
ON_EXIT:
  if(SeedHeaderFields != NULL) {
    FreeHeaderFields(SeedHeaderFields, SeedFieldCount);
  }
  
  if(TempHeaderFields != NULL) {
    FreeHeaderFields(TempHeaderFields, TempFieldCount);
  }

  if(NewHeaderFields != NULL) {
    FreeHeaderFields(NewHeaderFields, NewFieldCount);
  }
  
  return Status;
}

/**
  This function is used to transform data stored in HttpMessage into a list of fields 
  paired with their corresponding values.

  @param[in]   HttpMessage           Contains raw unformatted HTTP header string. The buffer for this string will 
                                     be allocated and released by the caller.
  @param[in]   HttpMessageSize       Size in bytes of raw unformatted HTTP header.      
  @param[out]  HeaderFields          Array of key/value header pairs. The storage for all header pairs is allocated
                                     by the driver publishing this protocol, and must be freed by the caller. 
  @param[out]  FieldCount            Number of headers in HeaderFields.
  
  @retval EFI_SUCCESS                Parse HTTP header into array of key/value pairs succeeded.
  @retval EFI_OUT_OF_RESOURCES       Could not allocate memory for NewMessage.
  @retval EFI_INVALID_PARAMETER      One or more of the following conditions is TRUE:
                                     HttpMessage is NULL.
                                     HeaderFields is NULL.
                                     FieldCount is NULL.
  
**/
EFI_STATUS
HttpUtilitiesParse(
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

  if (HttpMessage == NULL || HeaderFields == NULL || FieldCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }
 
  Status          = EFI_SUCCESS;
  TempHttpMessage = NULL;
  *FieldCount     = 0;
  Token           = NULL;
  NextToken       = NULL;
  FieldName       = NULL;
  FieldValue      = NULL;
  Index           = 0;  

  TempHttpMessage = AllocateZeroPool (HttpMessageSize);
  if (TempHttpMessage == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TempHttpMessage, HttpMessage, HttpMessageSize);
  
  //
  // Get header number
  //
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

  if(*FieldCount == 0) {
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

    Status = SetFieldNameAndValue(&(*HeaderFields)[Index], FieldName, FieldValue);
    if(EFI_ERROR(Status)){
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
    FreePool(TempHttpMessage);
  }
  
  return Status;
}
