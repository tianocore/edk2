/** @file
  ACPI Sdt Protocol Driver

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiTable.h"

/**
  Retrieve option term according to AmlByteEncoding and Buffer.

  @param[in]    AmlByteEncoding      AML Byte Encoding.
  @param[in]    Buffer               AML buffer.
  @param[in]    MaxBufferSize        AML buffer MAX size. The parser can not parse any data exceed this region.
  @param[in]    TermIndex            Index of the data to retrieve from the object.
  @param[out]   DataType             Points to the returned data type or EFI_ACPI_DATA_TYPE_NONE if no data exists
                                     for the specified index.
  @param[out]   Data                 Upon return, points to the pointer to the data.
  @param[out]   DataSize             Upon return, points to the size of Data.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER Buffer does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlParseOptionTerm (
  IN AML_BYTE_ENCODING   *AmlByteEncoding,
  IN UINT8               *Buffer,
  IN UINTN               MaxBufferSize,
  IN AML_OP_PARSE_INDEX  TermIndex,
  OUT EFI_ACPI_DATA_TYPE *DataType,
  OUT VOID               **Data,
  OUT UINTN              *DataSize
  )
{
  AML_BYTE_ENCODING   *ChildAmlByteEncoding;
  EFI_STATUS          Status;

  if (DataType != NULL) {
    *DataType = AmlTypeToAcpiType (AmlByteEncoding->Format[TermIndex - 1]);
  }
  if (Data != NULL) {
    *Data = Buffer;
  }
  //
  // Parse term according to AML type
  //
  switch (AmlByteEncoding->Format[TermIndex - 1]) {
  case AML_UINT8:
    *DataSize = sizeof(UINT8);
    break;
  case AML_UINT16:
    *DataSize = sizeof(UINT16);
    break;
  case AML_UINT32:
    *DataSize = sizeof(UINT32);
    break;
  case AML_UINT64:
    *DataSize = sizeof(UINT64);
    break;
  case AML_STRING:
    *DataSize = AsciiStrSize((CHAR8 *)Buffer);
    break;
  case AML_NAME:
    Status = AmlGetNameStringSize (Buffer, DataSize);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
    break;
  case AML_OBJECT:
    ChildAmlByteEncoding = AmlSearchByOpByte (Buffer);
    if (ChildAmlByteEncoding == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // NOTE: We need override DataType here, if there is a case the AML_OBJECT is AML_NAME.
    // We need convert type from EFI_ACPI_DATA_TYPE_CHILD to EFI_ACPI_DATA_TYPE_NAME_STRING.
    // We should not return CHILD because there is NO OpCode for NameString.
    //
    if ((ChildAmlByteEncoding->Attribute & AML_IS_NAME_CHAR) != 0) {
      if (DataType != NULL) {
        *DataType = AmlTypeToAcpiType (AML_NAME);
      }
      Status = AmlGetNameStringSize (Buffer, DataSize);
      if (EFI_ERROR (Status)) {
        return EFI_INVALID_PARAMETER;
      }
      break;
    }

    //
    // It is real AML_OBJECT
    //
    *DataSize = AmlGetObjectSize (
                     ChildAmlByteEncoding,
                     Buffer,
                     MaxBufferSize
                     );
    if (*DataSize == 0) {
      return EFI_INVALID_PARAMETER;
    }
    break;
  case AML_NONE:
    //
    // No term
    //
  case AML_OPCODE:
  default:
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }
  if (*DataSize > MaxBufferSize) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

/**
  Retrieve information according to AmlByteEncoding and Buffer.

  @param[in]    AmlByteEncoding      AML Byte Encoding.
  @param[in]    Buffer               AML buffer.
  @param[in]    MaxBufferSize        AML buffer MAX size. The parser can not parse any data exceed this region.
  @param[in]    Index                Index of the data to retrieve from the object. In general, indexes read from left-to-right
                                     in the ACPI encoding, with index 0 always being the ACPI opcode.
  @param[out]   DataType             Points to the returned data type or EFI_ACPI_DATA_TYPE_NONE if no data exists
                                     for the specified index.
  @param[out]   Data                 Upon return, points to the pointer to the data.
  @param[out]   DataSize             Upon return, points to the size of Data.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER Buffer does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlParseOptionCommon (
  IN AML_BYTE_ENCODING   *AmlByteEncoding,
  IN UINT8               *Buffer,
  IN UINTN               MaxBufferSize,
  IN AML_OP_PARSE_INDEX  Index,
  OUT EFI_ACPI_DATA_TYPE *DataType,
  OUT VOID               **Data,
  OUT UINTN              *DataSize
  )
{
  UINT8               *CurrentBuffer;
  UINTN               PkgLength;
  UINTN               OpLength;
  UINTN               PkgOffset;
  AML_OP_PARSE_INDEX  TermIndex;
  EFI_STATUS          Status;

  ASSERT ((Index <= AmlByteEncoding->MaxIndex) || (Index == AML_OP_PARSE_INDEX_GET_SIZE));

  //
  // 0. Check if this is NAME string.
  //
  if ((AmlByteEncoding->Attribute & AML_IS_NAME_CHAR) != 0) {
    //
    // Only allow GET_SIZE
    //
    if (Index != AML_OP_PARSE_INDEX_GET_SIZE) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // return NameString size
    //
    Status = AmlGetNameStringSize (Buffer, DataSize);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
    if (*DataSize > MaxBufferSize) {
      return EFI_INVALID_PARAMETER;
    }
    return EFI_SUCCESS;
  }

  //
  // Not NAME string, start parsing
  //
  CurrentBuffer = Buffer;

  //
  // 1. Get OpCode
  //
  if (Index != AML_OP_PARSE_INDEX_GET_SIZE) {
    *DataType = EFI_ACPI_DATA_TYPE_OPCODE;
    *Data = (VOID *)CurrentBuffer;
  }
  if (*CurrentBuffer == AML_EXT_OP) {
    OpLength = 2;
  } else {
    OpLength = 1;
  }
  *DataSize = OpLength;
  if (Index == AML_OP_PARSE_INDEX_GET_OPCODE) {
    return EFI_SUCCESS;
  }
  if (OpLength > MaxBufferSize) {
    return EFI_INVALID_PARAMETER;
  }
  CurrentBuffer += OpLength;

  //
  // 2. Skip PkgLength field, if have
  //
  if ((AmlByteEncoding->Attribute & AML_HAS_PKG_LENGTH) != 0) {
    PkgOffset = AmlGetPkgLength(CurrentBuffer, &PkgLength);
    //
    // Override MaxBufferSize if it is valid PkgLength
    //
    if (OpLength + PkgLength > MaxBufferSize) {
      return EFI_INVALID_PARAMETER;
    } else {
      MaxBufferSize = OpLength + PkgLength;
    }
  } else {
    PkgOffset = 0;
    PkgLength = 0;
  }
  CurrentBuffer += PkgOffset;

  //
  // 3. Get Term one by one.
  //
  TermIndex = AML_OP_PARSE_INDEX_GET_TERM1;
  while ((Index >= TermIndex) && (TermIndex <= AmlByteEncoding->MaxIndex) && ((UINTN)CurrentBuffer < (UINTN)Buffer + MaxBufferSize)) {
    Status = AmlParseOptionTerm (
               AmlByteEncoding,
               CurrentBuffer,
               (UINTN)Buffer + MaxBufferSize - (UINTN)CurrentBuffer,
               TermIndex,
               DataType,
               Data,
               DataSize
               );
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }

    if (Index == TermIndex) {
      //
      // Done
      //
      return EFI_SUCCESS;
    }

    //
    // Parse next one
    //
    CurrentBuffer += *DataSize;
    TermIndex ++;
  }

  //
  // Finish all options, but no option found.
  //
  if ((UINTN)CurrentBuffer > (UINTN)Buffer + MaxBufferSize) {
    return EFI_INVALID_PARAMETER;
  }
  if ((UINTN)CurrentBuffer == (UINTN)Buffer + MaxBufferSize) {
    if (Index != AML_OP_PARSE_INDEX_GET_SIZE) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // 4. Finish parsing all node, return size
  //
  ASSERT (Index == AML_OP_PARSE_INDEX_GET_SIZE);
  if ((AmlByteEncoding->Attribute & AML_HAS_PKG_LENGTH) != 0) {
    *DataSize = OpLength + PkgLength;
  } else {
    *DataSize = (UINTN)CurrentBuffer - (UINTN)Buffer;
  }

  return EFI_SUCCESS;
}

/**
  Return object size.

  @param[in]    AmlByteEncoding      AML Byte Encoding.
  @param[in]    Buffer               AML object buffer.
  @param[in]    MaxBufferSize        AML object buffer MAX size. The parser can not parse any data exceed this region.

  @return       Size of the object.
**/
UINTN
AmlGetObjectSize (
  IN AML_BYTE_ENCODING   *AmlByteEncoding,
  IN UINT8               *Buffer,
  IN UINTN               MaxBufferSize
  )
{
  EFI_STATUS   Status;
  UINTN        DataSize;

  Status = AmlParseOptionCommon (
               AmlByteEncoding,
               Buffer,
               MaxBufferSize,
               AML_OP_PARSE_INDEX_GET_SIZE,
               NULL,
               NULL,
               &DataSize
               );
  if (EFI_ERROR (Status)) {
    return 0;
  } else {
    return DataSize;
  }
}

/**
  Return object name.

  @param[in]    AmlHandle            AML handle.

  @return       Name of the object.
**/
CHAR8 *
AmlGetObjectName (
  IN EFI_AML_HANDLE      *AmlHandle
  )
{
  AML_BYTE_ENCODING   *AmlByteEncoding;
  VOID                *NameString;
  UINTN               NameSize;
  AML_OP_PARSE_INDEX  TermIndex;
  EFI_STATUS          Status;
  EFI_ACPI_DATA_TYPE  DataType;

  AmlByteEncoding = AmlHandle->AmlByteEncoding;

  ASSERT ((AmlByteEncoding->Attribute & AML_IN_NAMESPACE) != 0);

  //
  // Find out Last Name index, according to OpCode table.
  // The last name will be the node name by design.
  //
  TermIndex = AmlByteEncoding->MaxIndex;
  for (TermIndex = AmlByteEncoding->MaxIndex; TermIndex > 0; TermIndex--) {
    if (AmlByteEncoding->Format[TermIndex - 1] == AML_NAME) {
      break;
    }
  }
  ASSERT (TermIndex != 0);

  //
  // Get Name for this node.
  //
  Status = AmlParseOptionHandleCommon (
             AmlHandle,
             TermIndex,
             &DataType,
             &NameString,
             &NameSize
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_NAME_STRING);

  return NameString;
}

/**
  Return offset of last option.

  @param[in]    AmlHandle            AML Handle.
  @param[out]   Buffer               Upon return, points to the offset after last option.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER AmlHandle does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlGetOffsetAfterLastOption (
  IN EFI_AML_HANDLE         *AmlHandle,
  OUT UINT8                 **Buffer
  )
{
  EFI_ACPI_DATA_TYPE  DataType;
  VOID                *Data;
  UINTN               DataSize;
  EFI_STATUS          Status;

  Status = AmlParseOptionHandleCommon (
             AmlHandle,
             AmlHandle->AmlByteEncoding->MaxIndex,
             &DataType,
             &Data,
             &DataSize
             );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We need to parse the rest buffer after last node.
  //
  *Buffer = (UINT8 *)((UINTN)Data + DataSize);

  //
  // We need skip PkgLength if no Option
  //
  if (DataType == EFI_ACPI_DATA_TYPE_OPCODE) {
    *Buffer += AmlGetPkgLength (*Buffer, &DataSize);
  }
  return EFI_SUCCESS;
}

/**
  Retrieve information according to AmlHandle

  @param[in]    AmlHandle            AML handle.
  @param[in]    Index                Index of the data to retrieve from the object. In general, indexes read from left-to-right
                                     in the ACPI encoding, with index 0 always being the ACPI opcode.
  @param[out]   DataType             Points to the returned data type or EFI_ACPI_DATA_TYPE_NONE if no data exists
                                     for the specified index.
  @param[out]   Data                 Upon return, points to the pointer to the data.
  @param[out]   DataSize             Upon return, points to the size of Data.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER AmlHandle does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlParseOptionHandleCommon (
  IN EFI_AML_HANDLE      *AmlHandle,
  IN AML_OP_PARSE_INDEX  Index,
  OUT EFI_ACPI_DATA_TYPE *DataType,
  OUT VOID               **Data,
  OUT UINTN              *DataSize
  )
{
  return AmlParseOptionCommon (
           AmlHandle->AmlByteEncoding,
           AmlHandle->Buffer,
           AmlHandle->Size,
           Index,
           DataType,
           Data,
           DataSize
           );
}
