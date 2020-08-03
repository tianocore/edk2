/** @file
  AML Tree Traversal.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Tree/AmlTreeTraversal.h>

#include <AmlCoreInterface.h>
#include <Tree/AmlTree.h>

/** Get the sibling node among the nodes being in
    the same variable argument list.

  (ParentNode)  /-i                 # Child of fixed argument b
      \        /
       |- [a][b][c][d]              # Fixed Arguments
       |- {(VarArgNode)->(f)->(g)}  # Variable Arguments
             \
              \-h                   # Child of variable argument e

  Node must be in a variable list of arguments.
  Traversal Order: VarArgNode, f, g, NULL

  @ingroup CoreNavigationApis

  @param  [in]  VarArgNode  Pointer to a node.
                            Must be in a variable list of arguments.

  @return The next node after VarArgNode in the variable list of arguments.
          Return NULL if
          - VarArgNode is the last node of the list, or
          - VarArgNode is not part of a variable list of arguments.
**/
AML_NODE_HEADER *
EFIAPI
AmlGetSiblingVariableArgument (
  IN  AML_NODE_HEADER   * VarArgNode
  )
{
  EAML_PARSE_INDEX    Index;
  AML_NODE_HEADER   * ParentNode;

  // VarArgNode must be an object node or a data node,
  // and be in a variable list of arguments.
  if ((!IS_AML_OBJECT_NODE (VarArgNode) &&
       !IS_AML_DATA_NODE (VarArgNode))  ||
      AmlIsNodeFixedArgument (VarArgNode, &Index)) {
    ASSERT (0);
    return NULL;
  }

  ParentNode = AmlGetParent (VarArgNode);
  if (!IS_AML_NODE_VALID (ParentNode)) {
    ASSERT (0);
    return NULL;
  }

  return AmlGetNextVariableArgument (ParentNode, VarArgNode);
}

/** Get the next variable argument.

  (Node)        /-i           # Child of fixed argument b
      \        /
       |- [a][b][c][d]        # Fixed Arguments
       |- {(e)->(f)->(g)}     # Variable Arguments
             \
              \-h             # Child of variable argument e

  Traversal Order: e, f, g, NULL

  @param  [in]  Node        Pointer to a Root node or Object Node.
  @param  [in]  CurrVarArg  Pointer to the Current Variable Argument.

  @return The node after the CurrVarArg in the variable list of arguments.
          If CurrVarArg is NULL, return the first node of the
          variable argument list.
          Return NULL if
          - CurrVarArg is the last node of the list, or
          - Node does not have a variable list of arguments.
**/
AML_NODE_HEADER *
EFIAPI
AmlGetNextVariableArgument (
  IN  AML_NODE_HEADER  * Node,
  IN  AML_NODE_HEADER  * CurrVarArg
  )
{
  CONST LIST_ENTRY       * StartLink;
  CONST LIST_ENTRY       * NextLink;

  // Node must be a RootNode or an Object Node
  // and the CurrVarArg must not be a Root Node.
  if ((!IS_AML_ROOT_NODE (Node)           &&
       !IS_AML_OBJECT_NODE (Node))        ||
      ((CurrVarArg != NULL)               &&
       (!IS_AML_OBJECT_NODE (CurrVarArg)  &&
        !IS_AML_DATA_NODE (CurrVarArg)))) {
    ASSERT (0);
    return NULL;
  }

  StartLink = AmlNodeGetVariableArgList (Node);
  if (StartLink == NULL) {
    return NULL;
  }

  // Get the first child of the variable list of arguments.
  if (CurrVarArg == NULL) {
    NextLink = StartLink->ForwardLink;
    if (NextLink != StartLink) {
      return (AML_NODE_HEADER*)NextLink;
    }
    // List is empty.
    return NULL;
  }

  // Check if CurrVarArg is in the VariableArgument List.
  if (!IsNodeInList (StartLink, &CurrVarArg->Link)) {
    ASSERT (0);
    return NULL;
  }

  // Get the node following the CurrVarArg.
  NextLink = CurrVarArg->Link.ForwardLink;
  if (NextLink != StartLink) {
    return (AML_NODE_HEADER*)NextLink;
  }

  // End of the list has been reached.
  return NULL;
}

/** Get the previous variable argument.

  (Node)        /-i           # Child of fixed argument b
      \        /
       |- [a][b][c][d]        # Fixed Arguments
       |- {(e)->(f)->(g)}     # Variable Arguments
             \
              \-h             # Child of variable argument e

  Traversal Order: g, f, e, NULL

  @param  [in]  Node        Pointer to a root node or an object node.
  @param  [in]  CurrVarArg  Pointer to the Current Variable Argument.

  @return The node before the CurrVarArg in the variable list of
          arguments.
          If CurrVarArg is NULL, return the last node of the
          variable list of arguments.
          Return NULL if:
          - CurrVarArg is the first node of the list, or
          - Node doesn't have a variable list of arguments.
**/
AML_NODE_HEADER *
EFIAPI
AmlGetPreviousVariableArgument (
  IN  AML_NODE_HEADER  * Node,
  IN  AML_NODE_HEADER  * CurrVarArg
  )
{
  CONST LIST_ENTRY       * StartLink;
  CONST LIST_ENTRY       * PreviousLink;

  // Node must be a RootNode or an Object Node
  // and the CurrVarArg must not be a Root Node.
  if ((!IS_AML_ROOT_NODE (Node)           &&
       !IS_AML_OBJECT_NODE (Node))        ||
      ((CurrVarArg != NULL)               &&
       (!IS_AML_OBJECT_NODE (CurrVarArg)  &&
        !IS_AML_DATA_NODE (CurrVarArg)))) {
    ASSERT (0);
    return NULL;
  }

  StartLink = AmlNodeGetVariableArgList (Node);
  if (StartLink == NULL) {
    return NULL;
  }

  // Get the last child of the variable list of arguments.
  if (CurrVarArg == NULL) {
    PreviousLink = StartLink->BackLink;
    if (PreviousLink != StartLink) {
      return (AML_NODE_HEADER*)PreviousLink;
    }
    // List is empty.
    return NULL;
  }

  // Check if CurrVarArg is in the VariableArgument List.
  if (!IsNodeInList (StartLink, &CurrVarArg->Link)) {
    ASSERT (0);
    return NULL;
  }

  // Get the node before the CurrVarArg.
  PreviousLink = CurrVarArg->Link.BackLink;
  if (PreviousLink != StartLink) {
    return (AML_NODE_HEADER*)PreviousLink;
  }

  // We have reached the beginning of the list.
  return NULL;
}

/** Get the next sibling node among the children of the input Node.

  This function traverses the FixedArguments followed by the
  VariableArguments at the same level in the hierarchy.

  Fixed arguments are before variable arguments.

  (Node)        /-i           # Child of fixed argument b
      \        /
       |- [a][b][c][d]        # Fixed Arguments
       |- {(e)->(f)->(g)}     # Variable Arguments
             \
              \-h             # Child of variable argument e

  Traversal Order: a, b, c, d, e, f, g, NULL


  @param  [in]  Node        Pointer to a root node or an object node.
  @param  [in]  ChildNode   Get the node after the ChildNode.

  @return The node after the ChildNode among the children of the input Node.
           - If ChildNode is NULL, return the first available node among
             the fixed argument list then variable list of arguments;
           - If ChildNode is the last node of the fixed argument list,
             return the first argument of the variable list of arguments;
           - If ChildNode is the last node of the variable list of arguments,
             return NULL.

**/
AML_NODE_HEADER *
EFIAPI
AmlGetNextSibling (
  IN  CONST AML_NODE_HEADER   * Node,
  IN  CONST AML_NODE_HEADER   * ChildNode
  )
{
  EAML_PARSE_INDEX    Index;
  AML_NODE_HEADER   * CandidateNode;

  // Node must be a RootNode or an Object Node
  // and the CurrVarArg must not be a Root Node.
  if ((!IS_AML_ROOT_NODE (Node)           &&
       !IS_AML_OBJECT_NODE (Node))        ||
      ((ChildNode != NULL)                &&
       (!IS_AML_OBJECT_NODE (ChildNode)   &&
        !IS_AML_DATA_NODE (ChildNode)))) {
    ASSERT (0);
    return NULL;
  }

  if (IS_AML_OBJECT_NODE (Node)) {
    if (ChildNode == NULL) {
      // Get the fixed argument at index 0 of the ChildNode.
      CandidateNode = AmlGetFixedArgument (
                        (AML_OBJECT_NODE*)Node,
                        EAmlParseIndexTerm0
                        );
      if (CandidateNode != NULL) {
        return CandidateNode;
      }
    } else {
      // (ChildNode != NULL)
      if (AmlIsNodeFixedArgument (ChildNode, &Index)) {
        // Increment index to point to the next fixed argument.
        Index++;
        // The node is part of the list of fixed arguments.
        if (Index == (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (
                                         (AML_OBJECT_NODE*)Node)
                                         ) {
        // It is at the last argument of the fixed argument list.
        // Get the first argument of the variable list of arguments.
          ChildNode = NULL;
        } else {
          // Else return the next node in the list of fixed arguments.
          return AmlGetFixedArgument ((AML_OBJECT_NODE*)Node, Index);
        }
      }
    }
  } // IS_AML_OBJECT_NODE (Node)

  // Else, get the next node in the variable list of arguments.
  return AmlGetNextVariableArgument (
           (AML_NODE_HEADER*)Node,
           (AML_NODE_HEADER*)ChildNode
           );
}

/** Get the previous sibling node among the children of the input Node.

  This function traverses the FixedArguments followed by the
  VariableArguments at the same level in the hierarchy.

  Fixed arguments are before variable arguments.

  (Node)        /-i           # Child of fixed argument b
      \        /
       |- [a][b][c][d]        # Fixed Arguments
       |- {(e)->(f)->(g)}     # Variable Arguments
             \
              \-h             # Child of variable argument e

  Traversal Order: g, f, e, d, c, b, a, NULL

  @param  [in]  Node        The node to get the fixed argument from.
  @param  [in]  ChildNode   Get the node before the ChildNode.

  @return The node before the ChildNode among the children of the input Node.
           - If ChildNode is NULL, return the last available node among
             the variable list of arguments then fixed argument list;
           - If ChildNode is the first node of the variable list of arguments,
             return the last argument of the fixed argument list;
           - If ChildNode is the first node of the fixed argument list,
             return NULL.
**/
AML_NODE_HEADER *
EFIAPI
AmlGetPreviousSibling (
  IN  CONST  AML_NODE_HEADER  * Node,
  IN  CONST  AML_NODE_HEADER  * ChildNode
  )
{
  EAML_PARSE_INDEX    Index;
  EAML_PARSE_INDEX    MaxIndex;

  AML_NODE_HEADER   * CandidateNode;

  // Node must be a Root Node or an Object Node
  // and the ChildNode must not be a Root Node.
  if ((!IS_AML_ROOT_NODE (Node)           &&
       !IS_AML_OBJECT_NODE (Node))        ||
      ((ChildNode != NULL)                &&
       (!IS_AML_OBJECT_NODE (ChildNode)   &&
        !IS_AML_DATA_NODE (ChildNode)))) {
    ASSERT (0);
    return NULL;
  }

  MaxIndex = (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (
                                 (AML_OBJECT_NODE*)Node
                                 );

  // Get the last variable argument if no ChildNode.
  // Otherwise the fixed argument list is checked first.
  if ((ChildNode != NULL)         &&
      IS_AML_OBJECT_NODE (Node)   &&
      (MaxIndex != EAmlParseIndexTerm0)) {
    if (AmlIsNodeFixedArgument (ChildNode, &Index)) {
      // The node is part of the list of fixed arguments.
      if (Index == EAmlParseIndexTerm0) {
        // The node is the first fixed argument, return NULL.
        return NULL;
      } else {
        // Return the previous node in the fixed argument list.
        return AmlGetFixedArgument (
                 (AML_OBJECT_NODE*)Node,
                 (EAML_PARSE_INDEX)(Index - 1)
                 );
      }
    }
  }

  // ChildNode is in the variable list of arguments.
  CandidateNode = AmlGetPreviousVariableArgument (
                    (AML_NODE_HEADER*)Node,
                    (AML_NODE_HEADER*)ChildNode
                    );
  if (CandidateNode != NULL) {
    if (!IS_AML_NODE_VALID (CandidateNode)) {
      ASSERT (0);
      return NULL;
    }
    // A Node has been found
    return CandidateNode;
  } else if (MaxIndex != EAmlParseIndexTerm0) {
    // ChildNode was the first node of the variable list of arguments.
    return AmlGetFixedArgument (
             (AML_OBJECT_NODE*)Node,
             (EAML_PARSE_INDEX)(MaxIndex - 1)
             );
  } else {
    // No fixed arguments or variable arguments.
    return NULL;
  }
}

/** Iterate through the nodes in the same order as the AML bytestream.

  The iteration is similar to a depth-first path.

  (Node)        /-i           # Child of fixed argument b
      \        /
       |- [a][b][c][d]        # Fixed Arguments
       |- {(e)->(f)->(g)}     # Variable Arguments
             \
              \-h             # Child of variable argument e

  Traversal Order: a, b, i, c, d, e, h, f, g, NULL
  Note: The branch i and h will be traversed if it has any children.

  @param  [in]  Node  Pointer to a node.

  @return The next node in the AML bytestream order.
          Return NULL if Node is the Node corresponding to the last
          bytecode of the tree.
**/
AML_NODE_HEADER *
EFIAPI
AmlGetNextNode (
  IN  CONST AML_NODE_HEADER   * Node
  )
{
  AML_NODE_HEADER   * ParentNode;
  AML_NODE_HEADER   * CandidateNode;

  if (!IS_AML_NODE_VALID (Node)) {
    ASSERT (0);
    return NULL;
  }

  if (IS_AML_ROOT_NODE (Node) || IS_AML_OBJECT_NODE (Node)) {
    // The node has children. Get the first child.
    CandidateNode = AmlGetNextSibling (Node, NULL);
    if (CandidateNode != NULL) {
      if (!IS_AML_NODE_VALID (CandidateNode)) {
        ASSERT (0);
        return NULL;
      }
      // A Node has been found
      return CandidateNode;
    } else if (IS_AML_ROOT_NODE (Node)) {
      // The node is the root node and it doesn't have children.
      return NULL;
    }
  }

  // We have traversed the current branch, go to the parent node
  // and start traversing the next branch.
  // Keep going up the tree until you reach the root node.
  while (1) {
    if (IS_AML_ROOT_NODE (Node)) {
      // This is the last node of the tree.
      return NULL;
    }

    ParentNode = AmlGetParent ((AML_NODE_HEADER*)Node);
    if (!IS_AML_NODE_VALID (ParentNode)) {
      ASSERT (0);
      return NULL;
    }

    CandidateNode = AmlGetNextSibling (ParentNode, Node);
    if (CandidateNode != NULL) {
      if (!IS_AML_NODE_VALID (CandidateNode)) {
        ASSERT (0);
        return NULL;
      }
      // A Node has been found
      return CandidateNode;
    }

    Node = ParentNode;
  } // while

  return NULL;
}

/** Iterate through the nodes in the reverse order of the AML bytestream.

  The iteration is similar to a depth-first path,
  but done in a reverse order.

  (Node)        /-i           # Child of fixed argument b
      \        /
       |- [a][b][c][d]        # Fixed Arguments
       |- {(e)->(f)->(g)}     # Variable Arguments
             \
              \-h             # Child of variable argument e

  Traversal Order: g, f, h, e, d, c, i, b, a, NULL
  Note: The branch i and h will be traversed if it has any children.

  @param  [in]  Node  Pointer to a node.

  @return The previous node in the AML bytestream order.
          Return NULL if Node is the Node corresponding to the last
          bytecode of the tree.
**/
AML_NODE_HEADER *
EFIAPI
AmlGetPreviousNode (
  IN  CONST  AML_NODE_HEADER * Node
  )
{
  AML_NODE_HEADER  * ParentNode;
  AML_NODE_HEADER  * CandidateNode;
  AML_NODE_HEADER  * PreviousNode;

  if (!IS_AML_NODE_VALID (Node)) {
    ASSERT (0);
    return NULL;
  }

  while (1) {

    if (IS_AML_ROOT_NODE (Node)) {
      // This is the root node.
      return NULL;
    }

    ParentNode = AmlGetParent ((AML_NODE_HEADER*)Node);
    CandidateNode = AmlGetPreviousSibling (ParentNode, Node);

    if (CandidateNode == NULL) {
      // Node is the first child of its parent.
      return ParentNode;
    } else if (IS_AML_DATA_NODE (CandidateNode)) {
      // CandidateNode is a data node, thus it has no children.
      return CandidateNode;
    } else if (IS_AML_OBJECT_NODE (CandidateNode)) {
      // Get the previous node in the list of children of ParentNode,
      // then get the last child of this node.
      // If this node has children, get its last child, etc.
      while (1) {
        PreviousNode = CandidateNode;
        CandidateNode = AmlGetPreviousSibling (PreviousNode, NULL);
        if (CandidateNode == NULL) {
          return PreviousNode;
        } else if (IS_AML_DATA_NODE (CandidateNode)) {
          return CandidateNode;
        }
      } // while

    } else {
      ASSERT (0);
      return NULL;
    }
  } // while
}
