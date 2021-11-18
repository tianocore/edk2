/** @file
  AML Tree Traversal.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_TREE_TRAVERSAL_H_
#define AML_TREE_TRAVERSAL_H_

#include <AmlNodeDefines.h>

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
  IN  CONST AML_NODE_HEADER  *Node,
  IN  CONST AML_NODE_HEADER  *ChildNode
  );

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
  IN  CONST  AML_NODE_HEADER  *Node,
  IN  CONST  AML_NODE_HEADER  *ChildNode
  );

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
  IN  CONST AML_NODE_HEADER  *Node
  );

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
  IN  CONST  AML_NODE_HEADER  *Node
  );

#endif // AML_TREE_TRAVERSAL_H_
