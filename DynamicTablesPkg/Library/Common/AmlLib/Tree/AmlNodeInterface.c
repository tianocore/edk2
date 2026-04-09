/** @file
  AML Node Interface.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlNodeDefines.h>

#include <AmlCoreInterface.h>
#include <ResourceData/AmlResourceData.h>
#include <String/AmlString.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>
#include <Utils/AmlUtility.h>

/** Returns the tree node type (Root/Object/Data).

  @param [in] Node  Pointer to a Node.

  @return The node type.
           EAmlNodeUnknown if invalid parameter.
**/
EAML_NODE_TYPE
EFIAPI
AmlGetNodeType (
  IN  AML_NODE_HEADER  *Node
  )
{
  if (!IS_AML_NODE_VALID (Node)) {
    ASSERT (0);
    return EAmlNodeUnknown;
  }

  return Node->NodeType;
}

/** Get the RootNode information.
    The Node must be a root node.

  @param  [in]  RootNode          Pointer to a root node.
  @param  [out] SdtHeaderBuffer   Buffer to copy the ACPI DSDT/SSDT header to.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetRootNodeInfo (
  IN  AML_ROOT_NODE                *RootNode,
  OUT EFI_ACPI_DESCRIPTION_HEADER  *SdtHeaderBuffer
  )
{
  if (!IS_AML_ROOT_NODE (RootNode)  ||
      (SdtHeaderBuffer == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (
    SdtHeaderBuffer,
    RootNode->SdtHeader,
    sizeof (EFI_ACPI_DESCRIPTION_HEADER)
    );

  return EFI_SUCCESS;
}

/** Get the ObjectNode information.
    The Node must be an object node.

  @ingroup NodeInterfaceApi

  @param  [in]  ObjectNode        Pointer to an object node.
  @param  [out] OpCode            Pointer holding the OpCode.
                                  Optional, can be NULL.
  @param  [out] SubOpCode         Pointer holding the SubOpCode.
                                  Optional, can be NULL.
  @param  [out] PkgLen            Pointer holding the PkgLen.
                                  The PkgLen is 0 for nodes
                                  not having the Pkglen attribute.
                                  Optional, can be NULL.
  @param  [out] IsNameSpaceNode   Pointer holding TRUE if the node is defining
                                  or changing the NameSpace scope.
                                  E.g.: The "Name ()" and "Scope ()" ASL
                                  statements add/modify the NameSpace scope.
                                  Their corresponding node are NameSpace nodes.
                                  Optional, can be NULL.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetObjectNodeInfo (
  IN  AML_OBJECT_NODE  *ObjectNode,
  OUT UINT8            *OpCode            OPTIONAL,
  OUT UINT8            *SubOpCode         OPTIONAL,
  OUT UINT32           *PkgLen            OPTIONAL,
  OUT BOOLEAN          *IsNameSpaceNode   OPTIONAL
  )
{
  if (!IS_AML_OBJECT_NODE (ObjectNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (OpCode != NULL) {
    *OpCode = ObjectNode->AmlByteEncoding->OpCode;
  }

  if (SubOpCode != NULL) {
    *SubOpCode = ObjectNode->AmlByteEncoding->SubOpCode;
  }

  if (PkgLen != NULL) {
    *PkgLen = ObjectNode->PkgLen;
  }

  if (IsNameSpaceNode != NULL) {
    *IsNameSpaceNode = AmlNodeHasAttribute (ObjectNode, AML_IN_NAMESPACE);
  }

  return EFI_SUCCESS;
}

/** Returns the count of the fixed arguments for the input Node.

  @param  [in]  Node  Pointer to an object node.

  @return Number of fixed arguments of the object node.
          Return 0 if the node is not an object node.
**/
UINT8
AmlGetFixedArgumentCount (
  IN  AML_OBJECT_NODE  *Node
  )
{
  if (IS_AML_OBJECT_NODE (Node) &&
      (Node->AmlByteEncoding != NULL))
  {
    return (UINT8)Node->AmlByteEncoding->MaxIndex;
  }

  return 0;
}

/** Get the data type of the DataNode.
    The Node must be a data node.

  @param  [in]  DataNode  Pointer to a data node.
  @param  [out] DataType  Pointer holding the data type of the data buffer.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetNodeDataType (
  IN  AML_DATA_NODE        *DataNode,
  OUT EAML_NODE_DATA_TYPE  *DataType
  )
{
  if (!IS_AML_DATA_NODE (DataNode)  ||
      (DataType == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *DataType = DataNode->DataType;

  return EFI_SUCCESS;
}

/** Get the descriptor Id of the resource data element
    contained in the DataNode.

  The Node must be a data node.
  The Node must have the resource data type, i.e. have the
  EAmlNodeDataTypeResourceData data type.

  @param  [in]  DataNode          Pointer to a data node containing a
                                  resource data element.
  @param  [out] ResourceDataType  Pointer holding the descriptor Id of
                                  the resource data.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetResourceDataType (
  IN  AML_DATA_NODE  *DataNode,
  OUT AML_RD_HEADER  *ResourceDataType
  )
{
  if (!IS_AML_DATA_NODE (DataNode)  ||
      (ResourceDataType == NULL)    ||
      (DataNode->DataType != EAmlNodeDataTypeResourceData))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *ResourceDataType = AmlRdGetDescId (DataNode->Buffer);

  return EFI_SUCCESS;
}

/** Get the data buffer and size of the DataNode.
    The Node must be a data node.

  BufferSize is always updated to the size of buffer of the DataNode.

  If:
   - the content of BufferSize is >= to the DataNode's buffer size;
   - Buffer is not NULL;
  then copy the content of the DataNode's buffer in Buffer.

  @param  [in]      DataNode      Pointer to a data node.
  @param  [out]     Buffer        Buffer to write the data to.
                                  Optional, if NULL, only update BufferSize.
  @param  [in, out] BufferSize    Pointer holding:
                                   - At entry, the size of the Buffer;
                                   - At exit, the size of the DataNode's
                                     buffer size.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetDataNodeBuffer (
  IN      AML_DATA_NODE  *DataNode,
  OUT UINT8              *Buffer        OPTIONAL,
  IN  OUT UINT32         *BufferSize
  )
{
  if (!IS_AML_DATA_NODE (DataNode) ||
      (BufferSize == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if ((*BufferSize >= DataNode->Size)  &&
      (Buffer != NULL))
  {
    CopyMem (Buffer, DataNode->Buffer, DataNode->Size);
  }

  *BufferSize = DataNode->Size;

  return EFI_SUCCESS;
}

/** Update the ACPI DSDT/SSDT table header.

  The input SdtHeader information is copied to the tree RootNode.
  The table Length field is automatically updated.
  The checksum field is only updated when serializing the tree.

  @param  [in]  RootNode    Pointer to a root node.
  @param  [in]  SdtHeader   Pointer to an ACPI DSDT/SSDT table header.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlUpdateRootNode (
  IN        AML_ROOT_NODE                *RootNode,
  IN  CONST EFI_ACPI_DESCRIPTION_HEADER  *SdtHeader
  )
{
  EFI_STATUS  Status;
  UINT32      Length;

  if (!IS_AML_ROOT_NODE (RootNode)  ||
      (SdtHeader == NULL)           ||
      ((SdtHeader->Signature !=
        EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) &&
       (SdtHeader->Signature !=
        EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (
    RootNode->SdtHeader,
    SdtHeader,
    sizeof (EFI_ACPI_DESCRIPTION_HEADER)
    );

  // Update the Length field.
  Status = AmlComputeSize ((AML_NODE_HEADER *)RootNode, &Length);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  RootNode->SdtHeader->Length = Length +
                                (UINT32)sizeof (EFI_ACPI_DESCRIPTION_HEADER);

  return Status;
}

/** Update an object node representing an integer with a new value.

  The object node must have one of the following OpCodes:
   - AML_BYTE_PREFIX
   - AML_WORD_PREFIX
   - AML_DWORD_PREFIX
   - AML_QWORD_PREFIX
   - AML_ZERO_OP
   - AML_ONE_OP

  The following OpCode is not supported:
   - AML_ONES_OP

  @param  [in] IntegerOpNode   Pointer an object node containing an integer.
                               Must not be an object node with an AML_ONES_OP
                               OpCode.
  @param  [in] NewInteger      New integer value to set.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlUpdateInteger (
  IN  AML_OBJECT_NODE  *IntegerOpNode,
  IN  UINT64           NewInteger
  )
{
  EFI_STATUS  Status;

  INT8  ValueWidthDiff;

  if (!IS_AML_OBJECT_NODE (IntegerOpNode)     ||
      (!IsIntegerNode (IntegerOpNode)         &&
       !IsSpecialIntegerNode (IntegerOpNode)) ||
      AmlNodeCompareOpCode (IntegerOpNode, AML_ONES_OP, 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlNodeSetIntegerValue (IntegerOpNode, NewInteger, &ValueWidthDiff);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // If the new size is different from the old size, propagate the new size.
  if (ValueWidthDiff != 0) {
    // Propagate the information.
    Status = AmlPropagateInformation (
               (AML_NODE_HEADER *)IntegerOpNode,
               (ValueWidthDiff > 0) ? TRUE : FALSE,
               ABS (ValueWidthDiff),
               0
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  }

  return Status;
}

/** Update the buffer of a data node.

  Note: The data type of the buffer's content must match the data type of the
        DataNode. This is a hard restriction to prevent undesired behaviour.

  @param  [in]  DataNode  Pointer to a data node.
  @param  [in]  DataType  Data type of the Buffer's content.
  @param  [in]  Buffer    Buffer containing the new data. The content of
                          the Buffer is copied.
  @param  [in]  Size      Size of the Buffer.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Operation not supporter.
**/
EFI_STATUS
EFIAPI
AmlUpdateDataNode (
  IN  AML_DATA_NODE        *DataNode,
  IN  EAML_NODE_DATA_TYPE  DataType,
  IN  UINT8                *Buffer,
  IN  UINT32               Size
  )
{
  EFI_STATUS  Status;

  UINT32               ExpectedSize;
  AML_OBJECT_NODE      *ParentNode;
  EAML_NODE_DATA_TYPE  ExpectedArgType;
  EAML_PARSE_INDEX     Index;

  if (!IS_AML_DATA_NODE (DataNode)      ||
      (DataType > EAmlNodeDataTypeMax)  ||
      (Buffer == NULL)                  ||
      (Size == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ParentNode = (AML_OBJECT_NODE *)AmlGetParent ((AML_NODE_HEADER *)DataNode);
  if (!IS_AML_OBJECT_NODE (ParentNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // The NewNode and OldNode must have the same type.
  // We do not allow to change the argument type of a data node.
  // If required, the initial ASL template should be modified
  // accordingly.
  // It is however possible to interchange a raw buffer and a
  // resource data element, since raw data can be misinterpreted
  // as a resource data element.
  ExpectedArgType = DataNode->DataType;
  if ((ExpectedArgType != DataType)                         &&
      (((ExpectedArgType != EAmlNodeDataTypeRaw)            &&
        (ExpectedArgType != EAmlNodeDataTypeResourceData))  ||
       ((DataType != EAmlNodeDataTypeRaw)                   &&
        (DataType != EAmlNodeDataTypeResourceData))))
  {
    ASSERT (0);
    return EFI_UNSUPPORTED;
  }

  // Perform some compatibility checks.
  switch (DataType) {
    case EAmlNodeDataTypeNameString:
    {
      // Check the name contained in the Buffer is an AML name
      // with the right size.
      Status = AmlGetNameStringSize ((CONST CHAR8 *)Buffer, &ExpectedSize);
      if (EFI_ERROR (Status)  ||
          (Size != ExpectedSize))
      {
        ASSERT (0);
        return Status;
      }

      break;
    }
    case EAmlNodeDataTypeString:
    {
      ExpectedSize = 0;
      while (ExpectedSize < Size) {
        // Cf ACPI 6.3 specification 20.2.3 Data Objects Encoding.
        // AsciiCharList := Nothing | <AsciiChar AsciiCharList>
        // AsciiChar := 0x01 - 0x7F
        // NullChar := 0x00
        if (Buffer[ExpectedSize] > 0x7F) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        ExpectedSize++;
      }

      if (ExpectedSize != Size) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      break;
    }
    case EAmlNodeDataTypeUInt:
    {
      if (AmlIsNodeFixedArgument ((CONST AML_NODE_HEADER *)DataNode, &Index)) {
        if ((ParentNode->AmlByteEncoding == NULL) ||
            (ParentNode->AmlByteEncoding->Format == NULL))
        {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        // It is not possible to change the size of a fixed length UintX.
        // E.g. for PackageOp the first fixed argument is of type EAmlUInt8
        // and represents the count of elements. This type cannot be changed.
        if ((ParentNode->AmlByteEncoding->Format[Index] != EAmlObject) &&
            (DataNode->Size != Size))
        {
          ASSERT (0);
          return EFI_UNSUPPORTED;
        }
      }

      break;
    }
    case EAmlNodeDataTypeRaw:
    {
      // Check if the parent node has the byte list flag set.
      if (!AmlNodeHasAttribute (ParentNode, AML_HAS_BYTE_LIST)) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      break;
    }
    case EAmlNodeDataTypeResourceData:
    {
      // The resource data can be either small or large resource data.
      // Small resource data must be at least 1 byte.
      // Large resource data must be at least as long as the header
      // of a large resource data.
      if (AML_RD_IS_LARGE (Buffer)  &&
          (Size < sizeof (ACPI_LARGE_RESOURCE_HEADER)))
      {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Check if the parent node has the byte list flag set.
      if (!AmlNodeHasAttribute (ParentNode, AML_HAS_BYTE_LIST)) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Check the size of the buffer is equal to the resource data size
      // encoded in the input buffer.
      ExpectedSize = AmlRdGetSize (Buffer);
      if (ExpectedSize != Size) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      Status = AmlSetRdListCheckSum (ParentNode, 0);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }

      break;
    }
    case EAmlNodeDataTypeFieldPkgLen:
    {
      // Check the parent is a FieldNamed field element.
      if (!AmlNodeCompareOpCode (ParentNode, AML_FIELD_NAMED_OP, 0)) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      break;
    }
    // None and reserved types.
    default:
    {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
      break;
    }
  } // switch

  // If the new size is different from the old size, propagate the new size.
  if (DataNode->Size != Size) {
    // Propagate the information.
    Status = AmlPropagateInformation (
               DataNode->NodeHeader.Parent,
               (Size > DataNode->Size) ? TRUE : FALSE,
               (Size > DataNode->Size) ?
               (Size - DataNode->Size) :
               (DataNode->Size - Size),
               0
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Free the old DataNode buffer and allocate a new buffer to store the
    // new data.
    FreePool (DataNode->Buffer);
    DataNode->Buffer = AllocateZeroPool (Size);
    if (DataNode->Buffer == NULL) {
      ASSERT (0);
      return EFI_OUT_OF_RESOURCES;
    }

    DataNode->Size = Size;
  }

  CopyMem (DataNode->Buffer, Buffer, Size);

  return EFI_SUCCESS;
}
