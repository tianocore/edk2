/** @file
  AML Tree.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_TREE_H_
#define AML_TREE_H_

#include <AmlNodeDefines.h>

/** Get the root node from any node of the tree.
    This is done by climbing up the tree until the root node is reached.

  @param  [in]  Node    Pointer to a node.

  @return The root node of the tree.
          NULL if error.
*/
AML_ROOT_NODE *
EFIAPI
AmlGetRootNode (
  IN  CONST AML_NODE_HEADER  *Node
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif // AML_TREE_H_
