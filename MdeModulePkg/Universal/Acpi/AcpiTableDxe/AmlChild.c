/** @file
  ACPI Sdt Protocol Driver

  Copyright (c) 2010, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AcpiTable.h"

/**
  Return the child objects buffer from AML Handle's buffer.
  
  @param[in]        AmlParentHandle Parent handle.
  @param[in]        CurrentBuffer   The current child buffer.
  @param[out]       Buffer          On return, points to the next returned child buffer or NULL if there are no
                                    child buffer.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     AmlParentHandle does not refer to a valid ACPI object.                                
**/
EFI_STATUS
AmlGetChildFromObjectBuffer (
  IN EFI_AML_HANDLE         *AmlParentHandle,
  IN UINT8                  *CurrentBuffer,
  OUT VOID                  **Buffer
  )
{
  AML_BYTE_ENCODING   *AmlByteEncoding;
  UINTN               DataSize;

  //
  // Root is considered as SCOPE, which has TermList.
  // We need return only Object in TermList.
  //
  while ((UINTN)CurrentBuffer < (UINTN)(AmlParentHandle->Buffer + AmlParentHandle->Size)) {
    AmlByteEncoding = AmlSearchByOpByte (CurrentBuffer);
    if (AmlByteEncoding == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // NOTE: We need return everything, because user might need parse the returned object.
    //
    if ((AmlByteEncoding->Attribute & AML_IS_NAME_CHAR) == 0) {
      *Buffer = CurrentBuffer;
      return EFI_SUCCESS;
    }

    DataSize = AmlGetObjectSize (
                 AmlByteEncoding,
                 CurrentBuffer,
                 (UINTN)AmlParentHandle->Buffer + AmlParentHandle->Size - (UINTN)CurrentBuffer
                 );
    if (DataSize == 0) {
      return EFI_INVALID_PARAMETER;
    }
    CurrentBuffer += DataSize;
  }

  //
  // No more
  //
  *Buffer = NULL;
  return EFI_SUCCESS;
}

/**
  Return the child ACPI objects from Root Handle.
  
  @param[in]        AmlParentHandle Parent handle. It is Root Handle.
  @param[in]        AmlHandle       The previously returned handle or NULL to start with the first handle.
  @param[out]       Buffer          On return, points to the next returned ACPI handle or NULL if there are no
                                    child objects.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     ParentHandle is NULL or does not refer to a valid ACPI object.                                
**/
EFI_STATUS
AmlGetChildFromRoot (
  IN EFI_AML_HANDLE         *AmlParentHandle,
  IN EFI_AML_HANDLE         *AmlHandle,
  OUT VOID                  **Buffer
  )
{
  UINT8               *CurrentBuffer;

  if (AmlHandle == NULL) {
    //
    // First One
    //
    CurrentBuffer = (VOID *)AmlParentHandle->Buffer;
  } else {
    CurrentBuffer = (VOID *)(AmlHandle->Buffer + AmlHandle->Size);
  }

  return AmlGetChildFromObjectBuffer (AmlParentHandle, CurrentBuffer, Buffer);
}

/**
  Return the child objects buffer from AML Handle's option list.
  
  @param[in]        AmlParentHandle Parent handle.
  @param[in]        AmlHandle       The current child handle.
  @param[out]       Buffer          On return, points to the next returned child buffer or NULL if there are no
                                    child buffer.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     AmlParentHandle does not refer to a valid ACPI object.                                
**/
EFI_STATUS
AmlGetChildFromOptionList (
  IN EFI_AML_HANDLE         *AmlParentHandle,
  IN EFI_AML_HANDLE         *AmlHandle,
  OUT VOID                  **Buffer
  )
{
  EFI_ACPI_DATA_TYPE  DataType;
  VOID                *Data;
  UINTN               DataSize;
  AML_OP_PARSE_INDEX  Index;
  EFI_STATUS          Status;
  AML_OP_PARSE_INDEX  MaxTerm;

  Index = AML_OP_PARSE_INDEX_GET_TERM1;
  MaxTerm = AmlParentHandle->AmlByteEncoding->MaxIndex;
  while (Index <= MaxTerm) {
    Status = AmlParseOptionHandleCommon (
               AmlParentHandle,
               (AML_OP_PARSE_INDEX)Index,
               &DataType,
               (VOID **)&Data,
               &DataSize
               );
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
    if (DataType == EFI_ACPI_DATA_TYPE_NONE) {
      //
      // Not found
      //
      break;
    }

    //
    // Find it, and Check Data
    //
    if ((DataType == EFI_ACPI_DATA_TYPE_CHILD) &&
        ((UINTN)AmlHandle->Buffer < (UINTN)Data)) {
      //
      // Buffer < Data means current node is next one
      //
      *Buffer = Data;
      return EFI_SUCCESS;
    }
    //
    // Not Child
    //
    Index ++;
  }

  *Buffer = NULL;
  return EFI_SUCCESS;
}

/**
  Return the child objects buffer from AML Handle's object child list.
  
  @param[in]        AmlParentHandle Parent handle.
  @param[in]        AmlHandle       The current child handle.
  @param[out]       Buffer          On return, points to the next returned child buffer or NULL if there are no
                                    child buffer.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     AmlParentHandle does not refer to a valid ACPI object.                                
**/
EFI_STATUS
AmlGetChildFromObjectChildList (
  IN EFI_AML_HANDLE         *AmlParentHandle,
  IN EFI_AML_HANDLE         *AmlHandle,
  OUT VOID                  **Buffer
  )
{
  EFI_STATUS          Status;
  UINT8               *CurrentBuffer;

  if ((AmlParentHandle->AmlByteEncoding->Attribute & AML_HAS_CHILD_OBJ) == 0) {
    //
    // No ObjectList
    //
    *Buffer = NULL;
    return EFI_SUCCESS;
  }

  //
  // Do we need add node within METHOD?
  // Yes, just add Object is OK. But we need filter NameString for METHOD invoke.
  //

  //
  // Now, we get the last node.
  //
  Status = AmlGetOffsetAfterLastOption (AmlParentHandle, &CurrentBuffer);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Go through all the reset buffer.
  //
  if ((UINTN)AmlHandle->Buffer < (UINTN)CurrentBuffer) {
    //
    // Buffer < Data means next node is first object
    //
  } else if ((UINTN)AmlHandle->Buffer + AmlHandle->Size < (UINTN)AmlParentHandle->Buffer + AmlParentHandle->Size) {
    //
    // There is still more node
    //
    CurrentBuffer = AmlHandle->Buffer + AmlHandle->Size;
  } else {
    //
    // No more data
    //
    *Buffer = NULL;
    return EFI_SUCCESS;
  }

  return AmlGetChildFromObjectBuffer (AmlParentHandle, CurrentBuffer, Buffer);
}

/**
  Return the child ACPI objects from Non-Root Handle.
  
  @param[in]        AmlParentHandle Parent handle. It is Non-Root Handle.
  @param[in]        AmlHandle       The previously returned handle or NULL to start with the first handle.
  @param[out]       Buffer          On return, points to the next returned ACPI handle or NULL if there are no
                                    child objects.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     ParentHandle is NULL or does not refer to a valid ACPI object.                                
**/
EFI_STATUS
AmlGetChildFromNonRoot (
  IN EFI_AML_HANDLE         *AmlParentHandle,
  IN EFI_AML_HANDLE         *AmlHandle,
  OUT VOID                  **Buffer
  )
{
  EFI_STATUS          Status;

  if (AmlHandle == NULL) {
    //
    // NULL means first one
    //
    AmlHandle = AmlParentHandle;
  }

  //
  // 1. Get Option
  //
  Status = AmlGetChildFromOptionList (AmlParentHandle, AmlHandle, Buffer);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }
  if (*Buffer != NULL) {
    return EFI_SUCCESS;
  }

  //
  // 2. search ObjectList
  //
  return AmlGetChildFromObjectChildList (AmlParentHandle, AmlHandle, Buffer);
}
