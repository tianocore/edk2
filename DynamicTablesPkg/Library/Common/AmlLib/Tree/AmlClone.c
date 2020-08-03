/** @file
  AML Clone.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlNodeDefines.h>

#include <AmlCoreInterface.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>

/** Clone a node.

  This function does not clone the children nodes.
  The cloned node returned is not attached to any tree.

  @param  [in]  Node        Pointer to a node.
  @param  [out] ClonedNode  Pointer holding the cloned node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCloneNode (
  IN  AML_NODE_HEADER   * Node,
  OUT AML_NODE_HEADER  ** ClonedNode
  )
{
  EFI_STATUS              Status;

  AML_OBJECT_NODE       * ObjectNode;
  AML_DATA_NODE         * DataNode;
  AML_ROOT_NODE         * RootNode;

  if (!IS_AML_NODE_VALID (Node) ||
      (ClonedNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *ClonedNode = NULL;

  if (IS_AML_DATA_NODE (Node)) {
    DataNode = (AML_DATA_NODE*)Node;
    Status = AmlCreateDataNode (
                DataNode->DataType,
                DataNode->Buffer,
                DataNode->Size,
                (AML_DATA_NODE**)ClonedNode
                );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  } else if (IS_AML_OBJECT_NODE (Node)) {
    ObjectNode = (AML_OBJECT_NODE*)Node;

    Status = AmlCreateObjectNode (
                ObjectNode->AmlByteEncoding,
                ObjectNode->PkgLen,
                (AML_OBJECT_NODE**)ClonedNode
                );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  } else if (IS_AML_ROOT_NODE (Node)) {
    RootNode = (AML_ROOT_NODE*)Node;

    Status = AmlCreateRootNode (
               RootNode->SdtHeader,
               (AML_ROOT_NODE**)ClonedNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Clone a node and its children (clone a tree branch).

  The cloned branch returned is not attached to any tree.

  @param  [in]  Node        Pointer to a node.
                            Node is the head of the branch to clone.
  @param  [out] ClonedNode  Pointer holding the head of the created cloned
                            branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCloneTree (
  IN  AML_NODE_HEADER   * Node,
  OUT AML_NODE_HEADER  ** ClonedNode
  )
{
  EFI_STATUS              Status;

  AML_NODE_HEADER       * HeadNode;
  AML_NODE_HEADER       * ClonedChildNode;
  AML_NODE_HEADER       * FixedArgNode;

  EAML_PARSE_INDEX        Index;
  EAML_PARSE_INDEX        MaxIndex;

  LIST_ENTRY            * StartLink;
  LIST_ENTRY            * CurrentLink;

  if (!IS_AML_NODE_VALID (Node) ||
      (ClonedNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCloneNode (Node, &HeadNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Clone the fixed arguments and bind them to their parent.
  MaxIndex = (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (
                                 (AML_OBJECT_NODE*)Node
                                 );
  for (Index = EAmlParseIndexTerm0; Index < MaxIndex; Index++) {
    FixedArgNode = AmlGetFixedArgument ((AML_OBJECT_NODE*)Node, Index);
    if (FixedArgNode == NULL) {
      Status = EFI_INVALID_PARAMETER;
      ASSERT (0);
      goto error_handler;
    }

    // Clone child.
    Status = AmlCloneTree (
               FixedArgNode,
               &ClonedChildNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }

    // Bind child.
    Status = AmlSetFixedArgument (
               (AML_OBJECT_NODE*)HeadNode,
               Index,
               ClonedChildNode
               );
    if (EFI_ERROR (Status)) {
      AmlDeleteTree (ClonedChildNode);
      ASSERT (0);
      goto error_handler;
    }
  } // for

  // Clone the variable arguments and bind them to their parent.
  StartLink = AmlNodeGetVariableArgList (Node);
  if (StartLink != NULL) {
    CurrentLink = StartLink->ForwardLink;
    while (CurrentLink != StartLink) {
      // Clone child.
      Status = AmlCloneTree ((AML_NODE_HEADER*)CurrentLink, &ClonedChildNode);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        goto error_handler;
      }

      // Bind child.
      Status = AmlVarListAddTailInternal (
                 HeadNode,
                 ClonedChildNode
                 );
      if (EFI_ERROR (Status)) {
        AmlDeleteTree (ClonedChildNode);
        ASSERT (0);
        goto error_handler;
      }

      CurrentLink = CurrentLink->ForwardLink;
    } // while
  }

  *ClonedNode = HeadNode;
  return Status;

error_handler:
  *ClonedNode = NULL;

  if (HeadNode != NULL) {
    AmlDeleteTree (HeadNode);
  }

  return Status;
}
