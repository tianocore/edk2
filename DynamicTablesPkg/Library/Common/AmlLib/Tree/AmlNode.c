/** @file
  AML Node.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Tree/AmlNode.h>

#include <AmlCoreInterface.h>
#include <Tree/AmlTree.h>

/** Initialize an AML_NODE_HEADER structure.

  @param  [in]  Node      Pointer to a node header.
  @param  [in]  NodeType  NodeType to initialize the Node with.
                          Must be an EAML_NODE_TYPE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlInitializeNodeHeader (
  IN  AML_NODE_HEADER   * Node,
  IN  EAML_NODE_TYPE      NodeType
  )
{
  if (Node == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  InitializeListHead (&Node->Link);

  Node->Parent = NULL;
  Node->NodeType = NodeType;

  return EFI_SUCCESS;
}

/** Delete a root node and its ACPI DSDT/SSDT header.

  It is the caller's responsibility to check the RootNode has been removed
  from the tree and is not referencing any other node in the tree.

  @param  [in]  RootNode  Pointer to a root node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlDeleteRootNode (
  IN  AML_ROOT_NODE  * RootNode
  )
{
  if (!IS_AML_ROOT_NODE (RootNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if ((RootNode->SdtHeader != NULL)) {
    FreePool (RootNode->SdtHeader);
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (RootNode);
  return EFI_SUCCESS;
}

/** Create an AML_ROOT_NODE.
    This node will be the root of the tree.

  @param  [in]  SdtHeader       Pointer to an ACPI DSDT/SSDT header to copy
                                the data from.
  @param  [out] NewRootNodePtr  The created AML_ROOT_NODE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateRootNode (
  IN  CONST EFI_ACPI_DESCRIPTION_HEADER   * SdtHeader,
  OUT       AML_ROOT_NODE                ** NewRootNodePtr
  )
{
  EFI_STATUS        Status;
  AML_ROOT_NODE   * RootNode;

  if ((SdtHeader == NULL) ||
      (NewRootNodePtr == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  RootNode = AllocateZeroPool (sizeof (AML_ROOT_NODE));
  if (RootNode == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AmlInitializeNodeHeader (&RootNode->NodeHeader, EAmlNodeRoot);
  if (EFI_ERROR (Status)) {
    FreePool (RootNode);
    ASSERT (0);
    return Status;
  }

  InitializeListHead (&RootNode->VariableArgs);

  RootNode->SdtHeader = AllocateCopyPool (
                          sizeof (EFI_ACPI_DESCRIPTION_HEADER),
                          SdtHeader
                          );
  if (RootNode->SdtHeader == NULL) {
    ASSERT (0);
    AmlDeleteRootNode (RootNode);
    return EFI_OUT_OF_RESOURCES;
  }

  *NewRootNodePtr = RootNode;

  return EFI_SUCCESS;
}

/** Delete an object node.

  It is the caller's responsibility to check the ObjectNode has been removed
  from the tree and is not referencing any other node in the tree.

  @param  [in]  ObjectNode  Pointer to an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlDeleteObjectNode (
  IN  AML_OBJECT_NODE   * ObjectNode
  )
{
  if (!IS_AML_OBJECT_NODE (ObjectNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (ObjectNode);
  return EFI_SUCCESS;
}

/** Create an AML_OBJECT_NODE.

  @param  [in]  AmlByteEncoding   Byte encoding entry.
  @param  [in]  PkgLength         PkgLength of the node if the AmlByteEncoding
                                  has the PkgLen attribute.
                                  0 otherwise.
  @param  [out] NewObjectNodePtr  The created AML_OBJECT_NODE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateObjectNode (
  IN  CONST  AML_BYTE_ENCODING   * AmlByteEncoding,
  IN         UINT32                PkgLength,
  OUT        AML_OBJECT_NODE    ** NewObjectNodePtr
  )
{
  EFI_STATUS            Status;
  AML_OBJECT_NODE     * ObjectNode;

  if ((AmlByteEncoding == NULL)  ||
      (NewObjectNodePtr == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjectNode = AllocateZeroPool (sizeof (AML_OBJECT_NODE));
  if (ObjectNode == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AmlInitializeNodeHeader (&ObjectNode->NodeHeader, EAmlNodeObject);
  if (EFI_ERROR (Status)) {
    FreePool (ObjectNode);
    ASSERT (0);
    return Status;
  }

  InitializeListHead (&ObjectNode->VariableArgs);

  // ObjectNode->FixedArgs[...] is already initialised to NULL as the
  // ObjectNode is Zero allocated.
  ObjectNode->AmlByteEncoding = AmlByteEncoding;
  ObjectNode->PkgLen = PkgLength;

  *NewObjectNodePtr = ObjectNode;

  return EFI_SUCCESS;
}

/** Delete a data node and its buffer.

  It is the caller's responsibility to check the DataNode has been removed
  from the tree and is not referencing any other node in the tree.

  @param  [in]  DataNode  Pointer to a data node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlDeleteDataNode (
  IN  AML_DATA_NODE   * DataNode
  )
{
  if (!IS_AML_DATA_NODE (DataNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (DataNode->Buffer != NULL) {
    FreePool (DataNode->Buffer);
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (DataNode);
  return EFI_SUCCESS;
}

/** Create an AML_DATA_NODE.

  @param  [in]  DataType        DataType of the node.
  @param  [in]  Data            Pointer to the AML bytecode corresponding to
                                this node. Data is copied from there.
  @param  [in]  DataSize        Number of bytes to consider at the address
                                pointed by Data.
  @param  [out] NewDataNodePtr  The created AML_DATA_NODE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateDataNode (
  IN        EAML_NODE_DATA_TYPE     DataType,
  IN  CONST UINT8                 * Data,
  IN        UINT32                  DataSize,
  OUT       AML_DATA_NODE        ** NewDataNodePtr
  )
{
  EFI_STATUS        Status;
  AML_DATA_NODE   * DataNode;

  // A data node must not be created for certain data types.
  if ((DataType == EAmlNodeDataTypeNone)       ||
      (DataType == EAmlNodeDataTypeReserved1)  ||
      (DataType == EAmlNodeDataTypeReserved2)  ||
      (DataType == EAmlNodeDataTypeReserved3)  ||
      (DataType == EAmlNodeDataTypeReserved4)  ||
      (DataType == EAmlNodeDataTypeReserved5)  ||
      (Data == NULL)                           ||
      (DataSize == 0)                          ||
      (NewDataNodePtr == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  DataNode = AllocateZeroPool (sizeof (AML_DATA_NODE));
  if (DataNode == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AmlInitializeNodeHeader (&DataNode->NodeHeader, EAmlNodeData);
  if (EFI_ERROR (Status)) {
    FreePool (DataNode);
    ASSERT (0);
    return Status;
  }

  DataNode->Buffer = AllocateCopyPool (DataSize, Data);
  if (DataNode->Buffer == NULL) {
    AmlDeleteDataNode (DataNode);
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  DataNode->DataType = DataType;
  DataNode->Size = DataSize;

  *NewDataNodePtr = DataNode;

  return EFI_SUCCESS;
}

/** Delete a Node.

  @param  [in]  Node  Pointer to a Node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeleteNode (
  IN  AML_NODE_HEADER   * Node
  )
{
  EFI_STATUS          Status;
  EAML_PARSE_INDEX    Index;

  // Check that the node being deleted is unlinked.
  // When removing the node, its parent and list are reset
  // with InitializeListHead. Thus it must be empty.
  if (!IS_AML_NODE_VALID (Node) ||
      !AML_NODE_IS_DETACHED (Node)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  switch (Node->NodeType) {
    case EAmlNodeRoot:
    {
      // Check the variable list of arguments has been cleaned.
      if (!IsListEmpty (AmlNodeGetVariableArgList (Node))) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      Status = AmlDeleteRootNode ((AML_ROOT_NODE*)Node);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
      }
      break;
    }

    case EAmlNodeObject:
    {
      // Check the variable list of arguments has been cleaned.
      if (!IsListEmpty (AmlNodeGetVariableArgList (Node))) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Check the fixed argument list has been cleaned.
      for (Index = EAmlParseIndexTerm0; Index < EAmlParseIndexMax; Index++) {
        if (((AML_OBJECT_NODE*)Node)->FixedArgs[Index] != NULL) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }
      }

      Status = AmlDeleteObjectNode ((AML_OBJECT_NODE*)Node);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
      }
      break;
    }

    case EAmlNodeData:
    {
      Status = AmlDeleteDataNode ((AML_DATA_NODE*)Node);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
      }
      break;
    }

    default:
    {
      ASSERT (0);
      Status = EFI_INVALID_PARAMETER;
      break;
    }
  } // switch

  return Status;
}

/** Check whether ObjectNode has the input attribute.
    This function can be used to check ObjectNode is an object node
    at the same time.

  @param  [in]  ObjectNode  Pointer to an object node.
  @param  [in]  Attribute   Attribute to check for.

  @retval TRUE    The node is an AML object and the attribute is present.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlNodeHasAttribute (
  IN  CONST AML_OBJECT_NODE   * ObjectNode,
  IN        AML_OP_ATTRIBUTE    Attribute
  )
{
  if (!IS_AML_OBJECT_NODE (ObjectNode) ||
      (ObjectNode->AmlByteEncoding == NULL)) {
    return FALSE;
  }

  return ((ObjectNode->AmlByteEncoding->Attribute &
           Attribute) == 0 ? FALSE : TRUE);
}

/** Check whether ObjectNode has the input OpCode/SubOpcode couple.

  @param  [in]  ObjectNode  Pointer to an object node.
  @param  [in]  OpCode      OpCode to check
  @param  [in]  SubOpCode   SubOpCode to check

  @retval TRUE    The node is an AML object and
                  the Opcode and the SubOpCode match.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlNodeCompareOpCode (
  IN  CONST  AML_OBJECT_NODE  * ObjectNode,
  IN         UINT8              OpCode,
  IN         UINT8              SubOpCode
  )
{
  if (!IS_AML_OBJECT_NODE (ObjectNode) ||
      (ObjectNode->AmlByteEncoding == NULL)) {
    return FALSE;
  }

  ASSERT (AmlIsOpCodeValid (OpCode, SubOpCode));

  return ((ObjectNode->AmlByteEncoding->OpCode == OpCode) &&
           (ObjectNode->AmlByteEncoding->SubOpCode == SubOpCode)) ?
           TRUE : FALSE;
}

/** Check whether a Node is an integer node.

  By integer node we mean an object node having one of the following opcode:
   - AML_BYTE_PREFIX;
   - AML_WORD_PREFIX;
   - AML_DWORD_PREFIX;
   - AML_QWORD_PREFIX.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node is an integer node.
  @retval FALSE Otherwise.
**/
BOOLEAN
EFIAPI
IsIntegerNode (
  IN  AML_OBJECT_NODE   * Node
  )
{
  UINT8   OpCode;

  if (!IS_AML_OBJECT_NODE (Node)  ||
      (Node->AmlByteEncoding == NULL)) {
    return FALSE;
  }

  // Check Node is an integer node.
  OpCode = Node->AmlByteEncoding->OpCode;
  if ((OpCode != AML_BYTE_PREFIX)   &&
      (OpCode != AML_WORD_PREFIX)   &&
      (OpCode != AML_DWORD_PREFIX)  &&
      (OpCode != AML_QWORD_PREFIX)) {
    return FALSE;
  }

  return TRUE;
}

/** Check whether a Node is a ZeroOp, a OneOp or a OnesOp.

  These two objects don't have a data node holding
  a value. This require special handling.

  @param  [in]  Node  The node to check.

  @retval TRUE  The Node is a ZeroOp or OneOp.
  @retval FALSE Otherwise.
**/
BOOLEAN
EFIAPI
IsSpecialIntegerNode (
  IN  AML_OBJECT_NODE   * Node
  )
{
  UINT8   OpCode;

  if (!IS_AML_OBJECT_NODE (Node)  ||
      (Node->AmlByteEncoding == NULL)) {
    return FALSE;
  }

  OpCode = Node->AmlByteEncoding->OpCode;

  if ((OpCode != AML_ZERO_OP) &&
      (OpCode != AML_ONE_OP)  &&
      (OpCode != AML_ONES_OP)) {
    return FALSE;
  }

  return TRUE;
}

/** Check whether Node corresponds to a method definition.

  A method definition can be introduced:
   - By a method object, having an AML_METHOD_OP OpCode;
   - By an external definition of a method, having an AML_EXTERNAL_OP OpCode
     and an ObjectType byte set to the MethodObj.

  Note:
  An alias node, having an AML_ALIAS_OP, can be resolved to a method
  definition. This function doesn't handle this case.

  @param [in] Node    Node to check whether it is a method definition.

  @retval TRUE  The Node is a method definition.
  @retval FALSE Otherwise.
**/
BOOLEAN
EFIAPI
AmlIsMethodDefinitionNode (
  IN  CONST AML_OBJECT_NODE   * Node
  )
{
  AML_DATA_NODE   * ObjectType;

  // Node is checked to be an object node aswell.
  if (AmlNodeCompareOpCode (Node, AML_METHOD_OP, 0)) {
    return TRUE;
  } else if (AmlNodeCompareOpCode (Node, AML_EXTERNAL_OP, 0)) {
    // If the node is an external definition, check this is a method.
    // DefExternal := ExternalOp NameString ObjectType ArgumentCount
    // ExternalOp := 0x15
    // ObjectType := ByteData
    // ArgumentCount := ByteData (0 - 7)
    ObjectType = (AML_DATA_NODE*)AmlGetFixedArgument (
                                   (AML_OBJECT_NODE*)Node,
                                   EAmlParseIndexTerm1
                                   );
    if (IS_AML_DATA_NODE (ObjectType)                   &&
        (ObjectType->DataType == EAmlNodeDataTypeUInt)  &&
        ((ObjectType->Size == 1))) {
      if (*((UINT8*)ObjectType->Buffer) == (UINT8)EAmlObjTypeMethodObj) {
        // The external definition is a method.
        return TRUE;
      } else {
        // The external definition is not a method.
        return FALSE;
      }
    } else {
      // The tree is inconsistent.
      ASSERT (0);
      return FALSE;
    }
  }

  // This is not a method definition.
  return FALSE;
}

/** Get the index at which the name of the node is stored.

  @param  [in]  ObjectNode  Pointer to an object node.
                            Must have the AML_IN_NAMESPACE attribute.
  @param  [out] Index       Index of the name in the fixed list of arguments.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
AmlNodeGetNameIndex (
  IN  CONST AML_OBJECT_NODE     * ObjectNode,
  OUT       EAML_PARSE_INDEX    * Index
  )
{
  EAML_PARSE_INDEX    NameIndex;

  if (!AmlNodeHasAttribute (ObjectNode, AML_IN_NAMESPACE)   ||
      (ObjectNode->AmlByteEncoding == NULL)                 ||
      (Index == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  NameIndex = ObjectNode->AmlByteEncoding->NameIndex;

  if ((NameIndex > ObjectNode->AmlByteEncoding->MaxIndex)   ||
      (ObjectNode->AmlByteEncoding->Format[NameIndex] != EAmlName)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *Index = NameIndex;

  return EFI_SUCCESS;
}

/** Get the name of the Node.

  Node must be part of the namespace.

  @param [in] ObjectNode    Pointer to an object node,
                            which is part of the namespace.

  @return A pointer to the name.
          NULL otherwise.
          Return NULL for the root node.
**/
CHAR8 *
EFIAPI
AmlNodeGetName (
  IN  CONST AML_OBJECT_NODE   * ObjectNode
  )
{
  EFI_STATUS          Status;
  EAML_PARSE_INDEX    NameIndex;
  AML_DATA_NODE     * DataNode;

  if (!AmlNodeHasAttribute (ObjectNode, AML_IN_NAMESPACE)) {
    ASSERT (0);
    return NULL;
  }

  // Get the index at which the name is stored in the fixed arguments list.
  Status = AmlNodeGetNameIndex (ObjectNode, &NameIndex);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return NULL;
  }

  // The name is stored in a Data node.
  DataNode = (AML_DATA_NODE*)ObjectNode->FixedArgs[NameIndex];
  if (IS_AML_DATA_NODE (DataNode) &&
      (DataNode->DataType == EAmlNodeDataTypeNameString)) {
    return (CHAR8*)DataNode->Buffer;
  }

  /* Return NULL if no name is found.
     This can occur if the name of a node is defined as a further
     fixed argument.
     E.g.:  CreateField (BD03, 0x28, Add (ID03 + 0x08), BF33)
                                     ^
                             The parser is here.
     The parent of the Add statement is the CreateField statement. This
     statement defines a name in the AML namespace. This name defined as
     the fourth fixed argument. It hasn't been parsed yet.
  */
  return NULL;
}
