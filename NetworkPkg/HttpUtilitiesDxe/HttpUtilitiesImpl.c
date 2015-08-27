/** @file
  The functions for HttpUtilities driver.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HttpUtilitiesDxe.h"


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
      return (CHAR8 *)(Token + 1);
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
      if (HeaderFields[Index].FieldName != NULL) {
        FreePool (HeaderFields[Index].FieldName);
      }
      if (HeaderFields[Index].FieldValue != NULL) {
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
  
  //
  // Replace ':' with 0
  //
  *(FieldValueStr - 1) = 0; 
  
  //
  // The field value MAY be preceded by any amount of LWS, though a single SP is preferred.
  //
  while (TRUE) {
    if (*FieldValueStr == ' ' || *FieldValueStr == '\t') {
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
  // Replace '\r' with 0
  //
  *(StrPtr - 2) = 0; 

  //
  // Get FieldName and FieldValue.
  //
  *FieldName = FieldNameStr;
  *FieldValue = FieldValueStr;
    
  return StrPtr;
}

