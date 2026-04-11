/** @file
  AML Tree.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Tree/AmlTree.h>

#include <AmlCoreInterface.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTreeTraversal.h>
#include <Utils/AmlUtility.h>

/** Get the parent node of the input Node.

  @param [in] Node  Pointer to a node.

  @return The parent node of the input Node.
          NULL otherwise.
**/
AML_NODE_HEADER *
EFIAPI
AmlGetParent (
  IN  AML_NODE_HEADER  *Node
  )
{
  if (IS_AML_DATA_NODE (Node) ||
      IS_AML_OBJECT_NODE (Node))
  {
    return Node->Parent;
  }

  return NULL;
}

/** Get the root node from any node of the tree.
    This is done by climbing up the tree until the root node is reached.

  @param  [in]  Node    Pointer to a node.

  @return The root node of the tree.
          NULL if error.
**/
AML_ROOT_NODE *
EFIAPI
AmlGetRootNode (
  IN  CONST AML_NODE_HEADER  *Node
  )
{
  if (!IS_AML_NODE_VALID (Node)) {
    ASSERT (0);
    return NULL;
  }

  while (!IS_AML_ROOT_NODE (Node)) {
    Node = Node->Parent;
    if (!IS_AML_NODE_VALID (Node)) {
      ASSERT (0);
      return NULL;
    }
  }

  return (AML_ROOT_NODE *)Node;
}

/** Get the node at the input Index in the fixed argument list of the input
    ObjectNode.

  @param  [in]  ObjectNode  Pointer to an object node.
  @param  [in]  Index       The Index of the fixed argument to get.

  @return The node at the input Index in the fixed argument list
          of the input ObjectNode.
          NULL otherwise, e.g. if the node is not an object node, or no
          node is available at this Index.
**/
AML_NODE_HEADER *
EFIAPI
AmlGetFixedArgument (
  IN  AML_OBJECT_NODE   *ObjectNode,
  IN  EAML_PARSE_INDEX  Index
  )
{
  if (IS_AML_OBJECT_NODE (ObjectNode)) {
    if (Index < (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (ObjectNode)) {
      return ObjectNode->FixedArgs[Index];
    }
  }

  return NULL;
}

/** Check whether the input Node is in the fixed argument list of its parent
    node.

  If so, IndexPtr contains this Index.

  @param  [in]  Node          Pointer to a Node.
  @param  [out] IndexPtr      Pointer holding the Index of the Node in
                              its parent's fixed argument list.

  @retval TRUE   The node is a fixed argument and the index
                 in IndexPtr is valid.
  @retval FALSE  The node is not a fixed argument.
**/
BOOLEAN
EFIAPI
AmlIsNodeFixedArgument (
  IN  CONST  AML_NODE_HEADER   *Node,
  OUT        EAML_PARSE_INDEX  *IndexPtr
  )
{
  AML_NODE_HEADER  *ParentNode;

  EAML_PARSE_INDEX  Index;
  EAML_PARSE_INDEX  MaxIndex;

  if ((IndexPtr == NULL)               ||
      (!IS_AML_DATA_NODE (Node)        &&
       !IS_AML_OBJECT_NODE (Node)))
  {
    ASSERT (0);
    return FALSE;
  }

  ParentNode = AmlGetParent ((AML_NODE_HEADER *)Node);
  if (IS_AML_ROOT_NODE (ParentNode)) {
    return FALSE;
  } else if (IS_AML_DATA_NODE (ParentNode)) {
    // Tree is inconsistent.
    ASSERT (0);
    return FALSE;
  }

  // Check whether the Node is in the fixed argument list.
  MaxIndex = (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (
                                 (AML_OBJECT_NODE *)ParentNode
                                 );
  for (Index = EAmlParseIndexTerm0; Index < MaxIndex; Index++) {
    if (AmlGetFixedArgument ((AML_OBJECT_NODE *)ParentNode, Index) == Node) {
      *IndexPtr = Index;
      return TRUE;
    }
  }

  return FALSE;
}

/** Set the fixed argument of the ObjectNode at the Index to the NewNode.

  It is the caller's responsibility to save the old node, if desired,
  otherwise the reference to the old node will be lost.
  If NewNode is not NULL, set its parent to ObjectNode.

  @param  [in]  ObjectNode    Pointer to an object node.
  @param  [in]  Index         Index in the fixed argument list of
                              the ObjectNode to set.
  @param  [in]  NewNode       Pointer to the NewNode.
                              Can be NULL, a data node or an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlSetFixedArgument (
  IN  AML_OBJECT_NODE   *ObjectNode,
  IN  EAML_PARSE_INDEX  Index,
  IN  AML_NODE_HEADER   *NewNode
  )
{
  if (IS_AML_OBJECT_NODE (ObjectNode)                                     &&
      (Index <= (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (ObjectNode))  &&
      ((NewNode == NULL)                                                  ||
       IS_AML_OBJECT_NODE (NewNode)                                       ||
       IS_AML_DATA_NODE (NewNode)))
  {
    ObjectNode->FixedArgs[Index] = NewNode;

    // If NewNode is a data node or an object node, set its parent.
    if (NewNode != NULL) {
      NewNode->Parent = (AML_NODE_HEADER *)ObjectNode;
    }

    return EFI_SUCCESS;
  }

  ASSERT (0);
  return EFI_INVALID_PARAMETER;
}

/** If the given AML_NODE_HEADER has a variable list of arguments,
    return a pointer to this list.
    Return NULL otherwise.

  @param  [in]  Node  Pointer to the AML_NODE_HEADER to check.

  @return The list of variable arguments if there is one.
          NULL otherwise.
**/
LIST_ENTRY *
EFIAPI
AmlNodeGetVariableArgList (
  IN  CONST AML_NODE_HEADER  *Node
  )
{
  if (IS_AML_ROOT_NODE (Node)) {
    return &(((AML_ROOT_NODE *)Node)->VariableArgs);
  } else if (IS_AML_OBJECT_NODE (Node)) {
    return &(((AML_OBJECT_NODE *)Node)->VariableArgs);
  }

  return NULL;
}

/** Remove the Node from its parent's variable list of arguments.

  The function will fail if the Node is in its parent's fixed
  argument list.
  The Node is not deleted. The deletion is done separately
  from the removal.

  @param  [in]  Node  Pointer to a Node.
                      Must be a data node or an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlRemoveNodeFromVarArgList (
  IN  AML_NODE_HEADER  *Node
  )
{
  EFI_STATUS       Status;
  AML_NODE_HEADER  *ParentNode;
  UINT32           Size;

  if ((!IS_AML_DATA_NODE (Node) &&
       !IS_AML_OBJECT_NODE (Node)))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ParentNode = AmlGetParent (Node);
  if (!IS_AML_ROOT_NODE (ParentNode)  &&
      !IS_AML_OBJECT_NODE (ParentNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Check the node is in its parent variable list of arguments.
  if (!IsNodeInList (
         AmlNodeGetVariableArgList (ParentNode),
         &Node->Link
         ))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Unlink Node from the tree.
  RemoveEntryList (&Node->Link);
  InitializeListHead (&Node->Link);
  Node->Parent = NULL;

  // Get the size of the node removed.
  Status = AmlComputeSize (Node, &Size);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Propagate the information.
  Status = AmlPropagateInformation (ParentNode, FALSE, Size, 1);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Detach the Node from the tree.

  The function will fail if the Node is in its parent's fixed
  argument list.
  The Node is not deleted. The deletion is done separately
  from the removal.

  @param  [in]  Node  Pointer to a Node.
                      Must be a data node or an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDetachNode (
  IN  AML_NODE_HEADER  *Node
  )
{
  return AmlRemoveNodeFromVarArgList (Node);
}

/** Add the NewNode to the head of the variable list of arguments
    of the ParentNode.

  @param  [in]  ParentNode  Pointer to the parent node.
                            Must be a root or an object node.
  @param  [in]  NewNode     Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddHead (
  IN  AML_NODE_HEADER  *ParentNode,
  IN  AML_NODE_HEADER  *NewNode
  )
{
  EFI_STATUS  Status;
  UINT32      NewSize;
  LIST_ENTRY  *ChildrenList;

  // Check arguments and that NewNode is not already attached to a tree.
  // ParentNode != Data Node AND NewNode != Root Node AND NewNode != attached.
  if ((!IS_AML_ROOT_NODE (ParentNode)     &&
       !IS_AML_OBJECT_NODE (ParentNode))  ||
      (!IS_AML_DATA_NODE (NewNode)        &&
       !IS_AML_OBJECT_NODE (NewNode))     ||
      !AML_NODE_IS_DETACHED (NewNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Insert it at the head of the list.
  ChildrenList = AmlNodeGetVariableArgList (ParentNode);
  if (ChildrenList == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  InsertHeadList (ChildrenList, &NewNode->Link);
  NewNode->Parent = ParentNode;

  // Get the size of the NewNode.
  Status = AmlComputeSize (NewNode, &NewSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Propagate the new information.
  Status = AmlPropagateInformation (ParentNode, TRUE, NewSize, 1);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Add the NewNode to the tail of the variable list of arguments
    of the ParentNode.

  NOTE: This is an internal function which does not propagate the size
        when a new node is added.

  @param  [in]  ParentNode  Pointer to the parent node.
                            Must be a root or an object node.
  @param  [in]  NewNode     Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddTailInternal (
  IN  AML_NODE_HEADER  *ParentNode,
  IN  AML_NODE_HEADER  *NewNode
  )
{
  LIST_ENTRY  *ChildrenList;

  // Check arguments and that NewNode is not already attached to a tree.
  // ParentNode != Data Node AND NewNode != Root Node AND NewNode != attached.
  if ((!IS_AML_ROOT_NODE (ParentNode)     &&
       !IS_AML_OBJECT_NODE (ParentNode))  ||
      (!IS_AML_DATA_NODE (NewNode)        &&
       !IS_AML_OBJECT_NODE (NewNode))     ||
      !AML_NODE_IS_DETACHED (NewNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Insert it at the tail of the list.
  ChildrenList = AmlNodeGetVariableArgList (ParentNode);
  if (ChildrenList == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  InsertTailList (ChildrenList, &NewNode->Link);
  NewNode->Parent = ParentNode;

  return EFI_SUCCESS;
}

/** Add the NewNode to the tail of the variable list of arguments
    of the ParentNode.

  @param  [in]  ParentNode  Pointer to the parent node.
                            Must be a root or an object node.
  @param  [in]  NewNode     Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddTail (
  IN  AML_NODE_HEADER  *ParentNode,
  IN  AML_NODE_HEADER  *NewNode
  )
{
  EFI_STATUS  Status;
  UINT32      NewSize;

  // Add the NewNode and check arguments.
  Status = AmlVarListAddTailInternal (ParentNode, NewNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the size of the NewNode.
  Status = AmlComputeSize (NewNode, &NewSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Propagate the new information.
  Status = AmlPropagateInformation (ParentNode, TRUE, NewSize, 1);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Add the NewNode before the Node in the list of variable
    arguments of the Node's parent.

  @param  [in]  Node      Pointer to a node.
                          Must be a root or an object node.
  @param  [in]  NewNode   Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddBefore (
  IN  AML_NODE_HEADER  *Node,
  IN  AML_NODE_HEADER  *NewNode
  )
{
  EFI_STATUS       Status;
  AML_NODE_HEADER  *ParentNode;
  UINT32           NewSize;

  // Check arguments and that NewNode is not already attached to a tree.
  if ((!IS_AML_DATA_NODE (NewNode)        &&
       !IS_AML_OBJECT_NODE (NewNode))     ||
      !AML_NODE_IS_DETACHED (NewNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ParentNode = AmlGetParent (Node);
  if (!IS_AML_ROOT_NODE (ParentNode)    &&
      !IS_AML_OBJECT_NODE (ParentNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Insert it before the input Node.
  InsertTailList (&Node->Link, &NewNode->Link);
  NewNode->Parent = ParentNode;

  // Get the size of the NewNode.
  Status = AmlComputeSize (NewNode, &NewSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Propagate the new information.
  Status = AmlPropagateInformation (ParentNode, TRUE, NewSize, 1);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Add the NewNode after the Node in the variable list of arguments
    of the Node's parent.

  @param  [in]  Node      Pointer to a node.
                          Must be a root or an object node.
  @param  [in]  NewNode   Pointer to the node to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlVarListAddAfter (
  IN  AML_NODE_HEADER  *Node,
  IN  AML_NODE_HEADER  *NewNode
  )
{
  EFI_STATUS       Status;
  AML_NODE_HEADER  *ParentNode;
  UINT32           NewSize;

  // Check arguments and that NewNode is not already attached to a tree.
  if ((!IS_AML_DATA_NODE (NewNode)        &&
       !IS_AML_OBJECT_NODE (NewNode))     ||
      !AML_NODE_IS_DETACHED (NewNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ParentNode = AmlGetParent (Node);
  if (!IS_AML_ROOT_NODE (ParentNode)    &&
      !IS_AML_OBJECT_NODE (ParentNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Insert the new node after the input Node.
  InsertHeadList (&Node->Link, &NewNode->Link);
  NewNode->Parent = ParentNode;

  // Get the size of the NewNode.
  Status = AmlComputeSize (NewNode, &NewSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Propagate the new information.
  Status = AmlPropagateInformation (ParentNode, TRUE, NewSize, 1);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Append a Resource Data node to the BufferOpNode.

  The Resource Data node is added at the end of the variable
  list of arguments of the BufferOpNode, but before the End Tag.
  If no End Tag is found, the function returns an error.

  @param  [in]  BufferOpNode  Buffer node containing resource data elements.
  @param  [in]  NewRdNode     The new Resource Data node to add.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlAppendRdNode (
  IN  AML_OBJECT_NODE  *BufferOpNode,
  IN  AML_DATA_NODE    *NewRdNode
  )
{
  EFI_STATUS     Status;
  AML_DATA_NODE  *LastRdNode;

  if (!AmlNodeCompareOpCode (BufferOpNode, AML_BUFFER_OP, 0)  ||
      !IS_AML_DATA_NODE (NewRdNode)                           ||
      (NewRdNode->DataType != EAmlNodeDataTypeResourceData))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // To avoid re-computing checksums, if a new resource data elements is
  // added/removed/modified in a list of resource data elements, the AmlLib
  // resets the checksum to 0.
  // It is possible to have only one Resource Data in a BufferOp with
  // no EndTag, but it should not be possible to add a new Resource Data
  // in the list in this case.
  Status = AmlSetRdListCheckSum (BufferOpNode, 0);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the last Resource data node in the variable list of argument of the
  // BufferOp node. This must be an EndTag, otherwise setting the checksum
  // would have failed.
  LastRdNode = (AML_DATA_NODE *)AmlGetPreviousVariableArgument (
                                  (AML_NODE_HEADER *)BufferOpNode,
                                  NULL
                                  );
  if ((LastRdNode == NULL)             ||
      !IS_AML_DATA_NODE (LastRdNode)   ||
      (LastRdNode->DataType != EAmlNodeDataTypeResourceData))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Add NewRdNode before the EndTag.
  Status = AmlVarListAddBefore (
             (AML_NODE_HEADER *)LastRdNode,
             (AML_NODE_HEADER *)NewRdNode
             )
  ;
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Replace the fixed argument at the Index of the ParentNode with the NewNode.

  Note: This function unlinks the OldNode from the tree. It is the callers
        responsibility to delete the OldNode if needed.

  @param  [in]  ParentNode  Pointer to the parent node.
                            Must be an object node.
  @param  [in]  Index       Index of the fixed argument to replace.
  @param  [in]  NewNode     The new node to insert.
                            Must be an object node or a data node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlReplaceFixedArgument (
  IN  AML_OBJECT_NODE   *ParentNode,
  IN  EAML_PARSE_INDEX  Index,
  IN  AML_NODE_HEADER   *NewNode
  )
{
  EFI_STATUS  Status;

  AML_NODE_HEADER   *OldNode;
  UINT32            NewSize;
  UINT32            OldSize;
  AML_PARSE_FORMAT  FixedArgType;

  // Check arguments and that NewNode is not already attached to a tree.
  if (!IS_AML_OBJECT_NODE (ParentNode)  ||
      (!IS_AML_DATA_NODE (NewNode)      &&
       !IS_AML_OBJECT_NODE (NewNode))   ||
      !AML_NODE_IS_DETACHED (NewNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Perform some compatibility checks between NewNode and OldNode.
  FixedArgType = ParentNode->AmlByteEncoding->Format[Index];
  switch (FixedArgType) {
    case EAmlFieldPkgLen:
    {
      // A FieldPkgLen can only have a parent node with the
      // AML_IS_FIELD_ELEMENT flag.
      if (!AmlNodeHasAttribute (
             (AML_OBJECT_NODE *)ParentNode,
             AML_HAS_FIELD_LIST
             ))
      {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Fall through.
    }

    case EAmlUInt8:
    case EAmlUInt16:
    case EAmlUInt32:
    case EAmlUInt64:
    case EAmlName:
    case EAmlString:
    {
      // A uint, a name, a string and a FieldPkgLen can only be replaced by a
      // data node of the same type.
      // Note: This condition might be too strict, but safer.
      if (!IS_AML_DATA_NODE (NewNode) ||
          (((AML_DATA_NODE *)NewNode)->DataType !=
           AmlTypeToNodeDataType (FixedArgType)))
      {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      break;
    }

    case EAmlObject:
    {
      // If it's an object node, the grammar is too complex to do any check.
      break;
    }

    case EAmlNone:
    default:
    {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
      break;
    }
  } // switch

  // Replace the OldNode with the NewNode.
  OldNode = AmlGetFixedArgument (ParentNode, Index);
  if (!IS_AML_NODE_VALID (OldNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Unlink the old node.
  // Note: This function unlinks the OldNode from the tree. It is the callers
  //       responsibility to delete the OldNode if needed.
  OldNode->Parent = NULL;

  Status = AmlSetFixedArgument (ParentNode, Index, NewNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the size of the OldNode.
  Status = AmlComputeSize (OldNode, &OldSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the size of the NewNode.
  Status = AmlComputeSize (NewNode, &NewSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Propagate the new information.
  Status = AmlPropagateInformation (
             (AML_NODE_HEADER *)ParentNode,
             (NewSize > OldSize) ? TRUE : FALSE,
             (NewSize > OldSize) ? (NewSize - OldSize) : (OldSize - NewSize),
             0
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Replace the OldNode, which is in a variable list of arguments,
    with the NewNode.

  Note: This function unlinks the OldNode from the tree. It is the callers
        responsibility to delete the OldNode if needed.

  @param  [in]  OldNode   Pointer to the node to replace.
                          Must be a data node or an object node.
  @param  [in]  NewNode   The new node to insert.
                          Must be a data node or an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlReplaceVariableArgument (
  IN  AML_NODE_HEADER  *OldNode,
  IN  AML_NODE_HEADER  *NewNode
  )
{
  EFI_STATUS        Status;
  UINT32            NewSize;
  UINT32            OldSize;
  EAML_PARSE_INDEX  Index;

  AML_DATA_NODE    *NewDataNode;
  AML_NODE_HEADER  *ParentNode;
  LIST_ENTRY       *NextLink;

  // Check arguments, that NewNode is not already attached to a tree,
  // and that OldNode is attached and not in a fixed list of arguments.
  if ((!IS_AML_DATA_NODE (OldNode)      &&
       !IS_AML_OBJECT_NODE (OldNode))   ||
      (!IS_AML_DATA_NODE (NewNode)      &&
       !IS_AML_OBJECT_NODE (NewNode))   ||
      !AML_NODE_IS_DETACHED (NewNode)   ||
      AML_NODE_IS_DETACHED (OldNode)    ||
      AmlIsNodeFixedArgument (OldNode, &Index))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ParentNode = AmlGetParent (OldNode);
  if (!IS_AML_ROOT_NODE (ParentNode)    &&
      !IS_AML_OBJECT_NODE (ParentNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  NewDataNode = (AML_DATA_NODE *)NewNode;

  // Check attributes if the parent node is an object node.
  if (IS_AML_OBJECT_NODE (ParentNode)) {
    // A child node of a node with the HAS_CHILD flag must be either a
    // data node or an object node. This has already been checked. So,
    // check for other cases.

    if (AmlNodeHasAttribute ((AML_OBJECT_NODE *)ParentNode, AML_HAS_BYTE_LIST)) {
      if (!IS_AML_DATA_NODE (NewNode)                       ||
          ((NewDataNode->DataType != EAmlNodeDataTypeRaw)   &&
           (NewDataNode->DataType != EAmlNodeDataTypeResourceData)))
      {
        // A child node of a node with the BYTE_LIST flag must be a data node,
        // containing raw data or a resource data.
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }
    } else if (AmlNodeHasAttribute (
                 (AML_OBJECT_NODE *)ParentNode,
                 AML_HAS_FIELD_LIST
                 ))
    {
      if (!AmlNodeHasAttribute (
             (CONST AML_OBJECT_NODE *)NewNode,
             AML_IS_FIELD_ELEMENT
             ))
      {
        // A child node of a node with the FIELD_LIST flag must be an object
        // node with AML_IS_FIELD_ELEMENT flag.
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }
    }
  } else {
    // Parent node is a root node.
    // A root node cannot have a data node as its child.
    if (!IS_AML_DATA_NODE (NewNode)) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }
  }

  // Unlink OldNode from the tree.
  NextLink = RemoveEntryList (&OldNode->Link);
  InitializeListHead (&OldNode->Link);
  OldNode->Parent = NULL;

  // Add the NewNode.
  InsertHeadList (NextLink, &NewNode->Link);
  NewNode->Parent = ParentNode;

  // Get the size of the OldNode.
  Status = AmlComputeSize (OldNode, &OldSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the size of the NewNode.
  Status = AmlComputeSize (NewNode, &NewSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Propagate the new information.
  Status = AmlPropagateInformation (
             ParentNode,
             (NewSize > OldSize) ? TRUE : FALSE,
             (NewSize > OldSize) ? (NewSize - OldSize) : (OldSize - NewSize),
             0
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Replace the OldNode by the NewNode.

  Note: This function unlinks the OldNode from the tree. It is the callers
        responsibility to delete the OldNode if needed.

  @param  [in]  OldNode   Pointer to the node to replace.
                          Must be a data node or an object node.
  @param  [in]  NewNode   The new node to insert.
                          Must be a data node or an object node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlReplaceArgument (
  IN  AML_NODE_HEADER  *OldNode,
  IN  AML_NODE_HEADER  *NewNode
  )
{
  EFI_STATUS        Status;
  AML_NODE_HEADER   *ParentNode;
  EAML_PARSE_INDEX  Index;

  // Check arguments and that NewNode is not already attached to a tree.
  if ((!IS_AML_DATA_NODE (OldNode)      &&
       !IS_AML_OBJECT_NODE (OldNode))   ||
      (!IS_AML_DATA_NODE (NewNode)      &&
       !IS_AML_OBJECT_NODE (NewNode))   ||
      !AML_NODE_IS_DETACHED (NewNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // ParentNode can be a root node or an object node.
  ParentNode = AmlGetParent (OldNode);
  if (!IS_AML_ROOT_NODE (ParentNode)  &&
      !IS_AML_OBJECT_NODE (ParentNode))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (AmlIsNodeFixedArgument (OldNode, &Index)) {
    // OldNode is in its parent's fixed argument list at the Index.
    Status = AmlReplaceFixedArgument (
               (AML_OBJECT_NODE *)ParentNode,
               Index,
               NewNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } else {
    // OldNode is not in its parent's fixed argument list.
    // It must be in its variable list of arguments.
    Status = AmlReplaceVariableArgument (OldNode, NewNode);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/** Delete a Node and its children.

  The Node must be removed from the tree first,
  or must be the root node.

  @param  [in]  Node  Pointer to the node to delete.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeleteTree (
  IN  AML_NODE_HEADER  *Node
  )
{
  EFI_STATUS  Status;

  EAML_PARSE_INDEX  Index;
  EAML_PARSE_INDEX  MaxIndex;

  AML_NODE_HEADER  *Arg;
  LIST_ENTRY       *StartLink;
  LIST_ENTRY       *CurrentLink;
  LIST_ENTRY       *NextLink;

  // Check that the node being deleted is unlinked.
  // When removing the node, its parent pointer and
  // its lists data structure are reset with
  // InitializeListHead. Thus it must be detached
  // from the tree to avoid memory leaks.
  if (!IS_AML_NODE_VALID (Node)  ||
      !AML_NODE_IS_DETACHED (Node))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // 1. Recursively detach and delete the fixed arguments.
  //    Iterate through the fixed list of arguments.
  if (IS_AML_OBJECT_NODE (Node)) {
    MaxIndex = (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (
                                   (AML_OBJECT_NODE *)Node
                                   );
    for (Index = EAmlParseIndexTerm0; Index < MaxIndex; Index++) {
      Arg = AmlGetFixedArgument ((AML_OBJECT_NODE *)Node, Index);
      if (Arg == NULL) {
        // A fixed argument is missing. The tree is inconsistent.
        // Note: During CodeGeneration, the fixed arguments should be set
        //       with an incrementing index, and then the variable arguments
        //       should be added. This allows to free as many nodes as
        //       possible if a crash occurs.
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      // Remove the node from the fixed argument list.
      Arg->Parent = NULL;
      Status      = AmlSetFixedArgument ((AML_OBJECT_NODE *)Node, Index, NULL);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }

      Status = AmlDeleteTree (Arg);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    }
  }

  // 2. Recursively detach and delete the variable arguments.
  //    Iterate through the variable list of arguments.
  StartLink = AmlNodeGetVariableArgList (Node);
  if (StartLink != NULL) {
    NextLink = StartLink->ForwardLink;
    while (NextLink != StartLink) {
      CurrentLink = NextLink;

      // Unlink the node from the tree.
      NextLink = RemoveEntryList (CurrentLink);
      InitializeListHead (CurrentLink);
      ((AML_NODE_HEADER *)CurrentLink)->Parent = NULL;

      Status = AmlDeleteTree ((AML_NODE_HEADER *)CurrentLink);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    } // while
  }

  // 3. Delete the node.
  Status = AmlDeleteNode (Node);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
