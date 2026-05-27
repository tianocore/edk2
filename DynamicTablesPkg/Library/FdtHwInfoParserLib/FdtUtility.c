/** @file
  Flattened device tree utility.

  Copyright (c) 2021 - 2026, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Device tree Specification - Release v0.3
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
  - linux//Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include <FdtHwInfoParserInclude.h>
#include "FdtUtility.h"

/** Check whether a node has the input name.

  @param [in]  Fdt          Pointer to a Flattened Device Tree.
  @param [in]  Node         Offset of the node to check the name.
  @param [in]  SearchName   Node name to search.
                            This is a NULL terminated string.

  @retval True    The node has the input name.
  @retval FALSE   Otherwise, or error.
**/
STATIC
BOOLEAN
EFIAPI
FdtNodeHasName (
  IN  CONST VOID   *Fdt,
  IN        INT32  Node,
  IN  CONST VOID   *SearchName
  )
{
  CONST CHAR8  *NodeName;
  UINT32       Length;

  if ((Fdt == NULL) ||
      (SearchName == NULL))
  {
    ASSERT (0);
    return FALSE;
  }

  // Always compare the whole string. Don't stop at the "@" char.
  Length = (UINT32)AsciiStrLen (SearchName);

  // Get the address of the node name.
  NodeName = FdtOffsetPointer (Fdt, Node + FDT_TAGSIZE, Length + 1);
  if (NodeName == NULL) {
    return FALSE;
  }

  // SearchName must be longer than the node name.
  if (Length > AsciiStrLen (NodeName)) {
    return FALSE;
  }

  if (AsciiStrnCmp (NodeName, SearchName, Length) != 0) {
    return FALSE;
  }

  // The name matches perfectly, or
  // the node name is XXX@addr and the XXX matches.
  if ((NodeName[Length] == '\0') ||
      (NodeName[Length] == '@'))
  {
    return TRUE;
  }

  return FALSE;
}

/** Iterate through the list of strings in the Context,
    and check whether at least one string is matching the
    "compatible" property of the node.

  @param [in]  Fdt          Pointer to a Flattened Device Tree.
  @param [in]  Node         Offset of the node to operate the check on.
  @param [in]  CompatInfo   COMPATIBILITY_INFO containing the list of compatible
                            strings to compare with the "compatible" property
                            of the node.

  @retval TRUE    At least one string matched, the node is compatible.
  @retval FALSE   Otherwise, or error.
**/
BOOLEAN
EFIAPI
FdtNodeIsCompatible (
  IN  CONST VOID   *Fdt,
  IN        INT32  Node,
  IN  CONST VOID   *CompatInfo
  )
{
  UINT32                   Index;
  CONST COMPATIBILITY_STR  *CompatibleTable;
  UINT32                   Count;
  CONST VOID               *Prop;
  INT32                    PropLen;

  if ((Fdt == NULL) ||
      (CompatInfo == NULL))
  {
    ASSERT (0);
    return FALSE;
  }

  Count           = ((COMPATIBILITY_INFO *)CompatInfo)->Count;
  CompatibleTable = ((COMPATIBILITY_INFO *)CompatInfo)->CompatTable;

  // Get the "compatible" property.
  Prop = FdtGetProp (Fdt, Node, "compatible", &PropLen);
  if ((Prop == NULL) || (PropLen < 0)) {
    return FALSE;
  }

  for (Index = 0; Index < Count; Index++) {
    if (FdtStringListContains (
          Prop,
          PropLen,
          CompatibleTable[Index].CompatStr
          ))
    {
      return TRUE;
    }
  } // for

  return FALSE;
}

/** Check whether a node has a property.

  @param [in]  Fdt          Pointer to a Flattened Device Tree.
  @param [in]  Node         Offset of the node to operate the check on.
  @param [in]  PropertyName Name of the property to search.
                            This is a NULL terminated string.

  @retval True    The node has the property.
  @retval FALSE   Otherwise, or error.
**/
BOOLEAN
EFIAPI
FdtNodeHasProperty (
  IN  CONST VOID   *Fdt,
  IN        INT32  Node,
  IN  CONST VOID   *PropertyName
  )
{
  INT32       Size;
  CONST VOID  *Prop;

  if ((Fdt == NULL) ||
      (PropertyName == NULL))
  {
    ASSERT (0);
    return FALSE;
  }

  Prop = FdtGetProp (Fdt, Node, PropertyName, &Size);
  if ((Prop == NULL) || (Size < 0)) {
    return FALSE;
  }

  return TRUE;
}

/** Get the next node in the whole DT fulfilling a condition.

  The condition to fulfill is checked by the NodeChecker function.
  Context is passed to NodeChecker.

  The Device tree is traversed in a depth-first search, starting from Node.
  The input Node is skipped.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in, out]  Node        At entry: Node offset to start the search.
                                          This first node is skipped.
                                          Write (-1) to search the whole tree.
                                At exit:  If success, contains the offset of
                                          the next node fulfilling the
                                          condition.
  @param [in, out]  Depth       Depth is incremented/decremented of the depth
                                difference between the input Node and the
                                output Node.
                                E.g.: If the output Node is a child node
                                of the input Node, contains (+1).
  @param [in]  NodeChecker      Function called to check if the condition
                                is fulfilled.
  @param [in]  Context          Context for the NodeChecker.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching node found.
**/
STATIC
EFI_STATUS
EFIAPI
FdtGetNextCondNode (
  IN      CONST VOID               *Fdt,
  IN OUT        INT32              *Node,
  IN OUT        INT32              *Depth,
  IN            NODE_CHECKER_FUNC  NodeChecker,
  IN      CONST VOID               *Context
  )
{
  INT32  CurrNode;

  if ((Fdt == NULL)   ||
      (Node == NULL)  ||
      (Depth == NULL) ||
      (NodeChecker == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CurrNode = *Node;
  do {
    CurrNode = FdtNextNode (Fdt, CurrNode, Depth);
    if ((CurrNode == -FDT_ERR_NOTFOUND) ||
        (*Depth < 0))
    {
      // End of the tree, no matching node found.
      return EFI_NOT_FOUND;
    } else if (CurrNode < 0) {
      // An error occurred.
      ASSERT (0);
      return EFI_ABORTED;
    }
  } while (!NodeChecker (Fdt, CurrNode, Context));

  // Matching node found.
  *Node = CurrNode;
  return EFI_SUCCESS;
}

/** Get the next node in a branch fulfilling a condition.

  The condition to fulfill is checked by the NodeChecker function.
  Context is passed to NodeChecker.

  The Device tree is traversed in a depth-first search, starting from Node.
  The input Node is skipped.

  @param [in]       Fdt             Pointer to a Flattened Device Tree.
  @param [in]       FdtBranch       Only search in the sub-nodes of this
                                    branch.
                                    Write (-1) to search the whole tree.
  @param [in]       NodeChecker     Function called to check if the condition
                                    is fulfilled.
  @param [in]       Context         Context for the NodeChecker.
  @param [in, out]  Node            At entry: Node offset to start the search.
                                         This first node is skipped.
                                         Write (-1) to search the whole tree.
                                    At exit:  If success, contains the offset
                                         of the next node in the branch
                                         fulfilling the condition.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching node found.
**/
STATIC
EFI_STATUS
EFIAPI
FdtGetNextCondNodeInBranch (
  IN      CONST VOID               *Fdt,
  IN            INT32              FdtBranch,
  IN            NODE_CHECKER_FUNC  NodeChecker,
  IN      CONST VOID               *Context,
  IN OUT        INT32              *Node
  )
{
  EFI_STATUS  Status;
  INT32       CurrNode;
  INT32       Depth;

  if ((Fdt == NULL)   ||
      (Node == NULL)  ||
      (NodeChecker == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CurrNode = FdtBranch;
  Depth    = 0;

  // First, check the Node is in the sub-nodes of the branch.
  // This allows to find the relative depth of Node in the branch.
  if (CurrNode != *Node) {
    for (CurrNode = FdtNextNode (Fdt, CurrNode, &Depth);
         (CurrNode >= 0) && (Depth > 0);
         CurrNode = FdtNextNode (Fdt, CurrNode, &Depth))
    {
      if (CurrNode == *Node) {
        // Node found.
        break;
      }
    } // for

    if ((CurrNode < 0) || (Depth <= 0)) {
      // Node is not a node in the branch, or an error occurred.
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }
  }

  // Get the next node in the tree fulfilling the condition,
  // in any branch.
  Status = FdtGetNextCondNode (
             Fdt,
             Node,
             &Depth,
             NodeChecker,
             Context
             );
  if (EFI_ERROR (Status)) {
    ASSERT (Status == EFI_NOT_FOUND);
    return Status;
  }

  if (Depth <= 0) {
    // The node found is not in the right branch.
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/** Get the next node in a branch having a matching name.

  The Device tree is traversed in a depth-first search, starting from Node.
  The input Node is skipped.

  @param [in]       Fdt         Pointer to a Flattened Device Tree.
  @param [in]       FdtBranch   Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]       NodeName    The node name to search.
                                This is a NULL terminated string.
  @param [in, out]  Node        At entry: Node offset to start the search.
                                          This first node is skipped.
                                          Write (-1) to search the whole tree.
                                At exit:  If success, contains the offset of
                                          the next node in the branch
                                          having a matching name.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching node found.
**/
EFI_STATUS
EFIAPI
FdtGetNextNamedNodeInBranch (
  IN      CONST VOID   *Fdt,
  IN            INT32  FdtBranch,
  IN      CONST CHAR8  *NodeName,
  IN OUT        INT32  *Node
  )
{
  return FdtGetNextCondNodeInBranch (
           Fdt,
           FdtBranch,
           FdtNodeHasName,
           NodeName,
           Node
           );
}

/** Get the next node in a branch with at least one compatible property.

  The Device tree is traversed in a depth-first search, starting from Node.
  The input Node is skipped.

  @param [in]       Fdt         Pointer to a Flattened Device Tree.
  @param [in]       FdtBranch   Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]  CompatNamesInfo  Table of compatible strings to compare with
                                the compatible property of the node.
  @param [in, out]  Node        At entry: Node offset to start the search.
                                          This first node is skipped.
                                          Write (-1) to search the whole tree.
                                At exit:  If success, contains the offset of
                                          the next node in the branch
                                          being compatible.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching node found.
**/
EFI_STATUS
EFIAPI
FdtGetNextCompatNodeInBranch (
  IN      CONST VOID                *Fdt,
  IN            INT32               FdtBranch,
  IN      CONST COMPATIBILITY_INFO  *CompatNamesInfo,
  IN OUT        INT32               *Node
  )
{
  return FdtGetNextCondNodeInBranch (
           Fdt,
           FdtBranch,
           FdtNodeIsCompatible,
           (CONST VOID *)CompatNamesInfo,
           Node
           );
}

/** Get the next node in a branch having the PropName property.

  The Device tree is traversed in a depth-first search, starting from Node.
  The input Node is skipped.

  @param [in]       Fdt         Pointer to a Flattened Device Tree.
  @param [in]       FdtBranch   Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]       PropName    Name of the property to search.
                                This is a NULL terminated string.
  @param [in, out]  Node        At entry: Node offset to start the search.
                                          This first node is skipped.
                                          Write (-1) to search the whole tree.
                                At exit:  If success, contains the offset of
                                          the next node in the branch
                                          being compatible.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching node found.
**/
EFI_STATUS
EFIAPI
FdtGetNextPropNodeInBranch (
  IN      CONST VOID   *Fdt,
  IN            INT32  FdtBranch,
  IN      CONST CHAR8  *PropName,
  IN OUT        INT32  *Node
  )
{
  return FdtGetNextCondNodeInBranch (
           Fdt,
           FdtBranch,
           FdtNodeHasProperty,
           (CONST VOID *)PropName,
           Node
           );
}

/** Count the number of Device Tree nodes fulfilling a condition
    in a Device Tree branch.

  The condition to fulfill is checked by the NodeChecker function.
  Context is passed to NodeChecker.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  FdtBranch        Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]  NodeChecker      Function called to check the condition is
                                fulfilled.
  @param [in]  Context          Context for the NodeChecker.
  @param [out] NodeCount        If success, contains the count of nodes
                                fulfilling the condition.
                                Can be 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
FdtCountCondNodeInBranch (
  IN  CONST VOID               *Fdt,
  IN        INT32              FdtBranch,
  IN        NODE_CHECKER_FUNC  NodeChecker,
  IN  CONST VOID               *Context,
  OUT       UINT32             *NodeCount
  )
{
  EFI_STATUS  Status;
  INT32       CurrNode;

  if ((Fdt == NULL)         ||
      (NodeChecker == NULL) ||
      (NodeCount == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *NodeCount = 0;
  CurrNode   = FdtBranch;
  while (TRUE) {
    Status = FdtGetNextCondNodeInBranch (
               Fdt,
               FdtBranch,
               NodeChecker,
               Context,
               &CurrNode
               );
    if (EFI_ERROR (Status)  &&
        (Status != EFI_NOT_FOUND))
    {
      ASSERT (0);
      return Status;
    } else if (Status == EFI_NOT_FOUND) {
      break;
    }

    (*NodeCount)++;
  }

  return EFI_SUCCESS;
}

/** Count the number of nodes in a branch with the input name.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  FdtBranch        Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]  NodeName         Node name to search.
                                This is a NULL terminated string.
  @param [out] NodeCount        If success, contains the count of nodes
                                fulfilling the condition.
                                Can be 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtCountNamedNodeInBranch (
  IN  CONST VOID    *Fdt,
  IN        INT32   FdtBranch,
  IN  CONST CHAR8   *NodeName,
  OUT       UINT32  *NodeCount
  )
{
  return FdtCountCondNodeInBranch (
           Fdt,
           FdtBranch,
           FdtNodeHasName,
           NodeName,
           NodeCount
           );
}

/** Count the number of nodes in a branch with at least
    one compatible property.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  FdtBranch        Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]  CompatNamesInfo  Table of compatible strings to
                                compare with the compatible property
                                of the node.
  @param [out] NodeCount        If success, contains the count of nodes
                                fulfilling the condition.
                                Can be 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtCountCompatNodeInBranch (
  IN  CONST VOID                *Fdt,
  IN        INT32               FdtBranch,
  IN  CONST COMPATIBILITY_INFO  *CompatNamesInfo,
  OUT       UINT32              *NodeCount
  )
{
  return FdtCountCondNodeInBranch (
           Fdt,
           FdtBranch,
           FdtNodeIsCompatible,
           CompatNamesInfo,
           NodeCount
           );
}

/** Count the number of nodes in a branch having the PropName property.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  FdtBranch        Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]  PropName         Name of the property to search.
                                This is a NULL terminated string.
  @param [out] NodeCount        If success, contains the count of nodes
                                fulfilling the condition.
                                Can be 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtCountPropNodeInBranch (
  IN  CONST VOID    *Fdt,
  IN        INT32   FdtBranch,
  IN  CONST CHAR8   *PropName,
  OUT       UINT32  *NodeCount
  )
{
  return FdtCountCondNodeInBranch (
           Fdt,
           FdtBranch,
           FdtNodeHasProperty,
           PropName,
           NodeCount
           );
}

/** Get the interrupt parent of a node.

  The interrupt parent is:
  - if the "interrupt-parent" property is present, the node
    pointed by the property. Note that the node pointed by
    the interrupt parent is not necessarily an interrupt
    controller.
  - otherwise, the parent following the normal hierarchy.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  Node             Offset of the node to start the search.
  @param [out] IntcNode         If success, contains the offset of the
                                interrupt-controller node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           No interrupt-controller node found.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
FdtGetIntcParentNode (
  IN  CONST VOID   *Fdt,
  IN        INT32  Node,
  OUT       INT32  *IntcNode
  )
{
  CONST UINT32  *PHandle;
  INT32         Size;

  if ((Fdt == NULL) ||
      (IntcNode == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  // Check whether the node has the "interrupt-parent" property.
  PHandle = FdtGetProp (Fdt, Node, "interrupt-parent", &Size);
  if ((PHandle != NULL) && (Size == sizeof (UINT32))) {
    Node = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (*PHandle));
    if (Node < 0) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    *IntcNode = Node;
    return EFI_SUCCESS;
  } else if (Size != -FDT_ERR_NOTFOUND) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  if (Node == 0) {
    // Reached the root of the tree.
    return EFI_NOT_FOUND;
  }

  // Get the parent of the node.
  Node = FdtParentOffset (Fdt, Node);
  if (Node < 0) {
    // An error occurred.
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  *IntcNode = Node;
  return EFI_SUCCESS;
}

/** Check whether a node defines an interrupt domain.

  An interrupt domain is defined by the presence of one of these properties:
  - interrupt-controller
  - interrupt-map

  Note: It is possible for a node to define the "#interrupt-cells" property
  without being an interrupt domain, but it would be meaningless.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  Node             Offset of the node to start the search.
  @param [in]  IntControllerOnly If TRUE, only return an actual interrupt
                                 controller node. If FALSE, return the first
                                 interrupt domain node, which can be an
                                 interrupt-controller node or an interrupt
                                 nexus node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           No interrupt-controller node found.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
BOOLEAN
EFIAPI
FdtIsIntcDomainNode (
  IN  CONST VOID     *Fdt,
  IN        INT32    Node,
  IN        BOOLEAN  IntControllerOnly
  )
{
  INT32       Size;
  CONST VOID  *Prop;

  if (Fdt == NULL) {
    ASSERT (FALSE);
    return FALSE;
  }

  Prop = FdtGetProp (Fdt, Node, "interrupt-controller", &Size);
  if ((Prop != NULL) && (Size >= 0)) {
    return TRUE;
  } else if (Size != -FDT_ERR_NOTFOUND) {
    ASSERT (FALSE);
  }

  if (IntControllerOnly) {
    return FALSE;
  }

  Prop = FdtGetProp (Fdt, Node, "interrupt-map", &Size);
  if ((Prop != NULL) && (Size >= 0)) {
    return TRUE;
  } else if (Size != -FDT_ERR_NOTFOUND) {
    ASSERT (FALSE);
  }

  return FALSE;
}

/** Get the interrupt-controller node handling the interrupts of
    the input node.

  To do this, recursively search a node with either the "interrupt-controller"
  or the "interrupt-parent" property in the parents of Node.

  Devicetree Specification, Release v0.3,
  2.4.1 "Properties for Interrupt Generating Devices":
    Because the hierarchy of the nodes in the interrupt tree
    might not match the devicetree, the interrupt-parent
    property is available to make the definition of an
    interrupt parent explicit. The value is the phandle to the
    interrupt parent. If this property is missing from a
    device, its interrupt parent is assumed to be its devicetree
    parent.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  Node             Offset of the node to start the search.
  @param [in]  IntControllerOnly If TRUE, only return an actual interrupt
                                 controller node. If FALSE, return the first
                                 interrupt domain node, which can be an
                                 interrupt-controller node or an interrupt
                                 nexus node.
  @param [out] IntcNode         If success, contains the offset of the
                                interrupt-controller node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           No interrupt-controller node found.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
FdtGetIntcNode (
  IN  CONST VOID     *Fdt,
  IN        INT32    Node,
  IN        BOOLEAN  IntControllerOnly,
  OUT       INT32    *IntcNode
  )
{
  EFI_STATUS  Status;
  INT32       InputNode;

  if ((Fdt == NULL) ||
      (IntcNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  InputNode = Node;

  // Get the interrupt parent (which is not necessarily an interrupt-controller).
  Status = FdtGetIntcParentNode (Fdt, Node, IntcNode);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Node = *IntcNode;

  while (TRUE) {
    // Check whether the node has the "interrupt-controller" property.
    if (FdtIsIntcDomainNode (Fdt, Node, IntControllerOnly)) {
      // The interrupt-controller has been found.
      *IntcNode = Node;
      return EFI_SUCCESS;
    }

    Status = FdtGetIntcParentNode (Fdt, Node, IntcNode);
    if (Status == EFI_NOT_FOUND) {
      // Reached the root of the tree.
      break;
    } else if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Node = *IntcNode;
  } // while

  //
  // Reached the root of the tree without finding an interrupt parent.
  // If the input node is an interrupt controller, return it.
  //
  if (FdtIsIntcDomainNode (Fdt, InputNode, IntControllerOnly)) {
    *IntcNode = InputNode;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/** Get the interrupt domain parent node handling the interrupts of the input
    node.

  The interrupt domain parent must be one of the following:
  - an interrupt-controller
  - an interrupt nexus

  @param [in]  Fdt       Pointer to a Flattened Device Tree.
  @param [in]  Node      Offset of the node to start the search.
  @param [out] IntcNode  If success, contains the offset of the matching
                         interrupt domain node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           No interrupt domain node found.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtGetIntDomainNode (
  IN  CONST VOID   *Fdt,
  IN        INT32  Node,
  OUT       INT32  *IntcNode
  )
{
  return FdtGetIntcNode (Fdt, Node, FALSE, IntcNode);
}

/** Get the interrupt-controller parent node handling the interrupts of the
    input node.

  The interrupt domain parent must be an interrupt-controller.

  @param [in]  Fdt       Pointer to a Flattened Device Tree.
  @param [in]  Node      Offset of the node to start the search.
  @param [out] IntcNode  If success, contains the offset of the matching
                         interrupt-controller node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           No interrupt-controller node found.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtGetIntControllerNode (
  IN  CONST VOID   *Fdt,
  IN        INT32  Node,
  OUT       INT32  *IntcNode
  )
{
  return FdtGetIntcNode (Fdt, Node, TRUE, IntcNode);
}

/** Read up to two FDT cells as a UINT64 value.

  @param [in]  Data       Pointer to the first cell to read.
  @param [in]  CellCount  Number of cells to read.
  @param [out] Value      If success, contains the decoded value.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Unsupported cell count.
**/
EFI_STATUS
EFIAPI
ReadFdtCells64 (
  IN  CONST UINT32  *Data,
  IN        INT32   CellCount,
  OUT       UINT64  *Value
  )
{
  INT32  Index;

  if ((Data == NULL) || (Value == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if ((CellCount < 0) || (CellCount > 2)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  *Value = 0;
  for (Index = 0; Index < CellCount; Index++) {
    *Value = LShiftU64 (*Value, 32) | Fdt32ToCpu (Data[Index]);
  }

  return EFI_SUCCESS;
}

/** Get the number of cells used to encode an interrupt for a specific Node.

  @param [in]  Fdt                Pointer to a Flattened Device Tree (Fdt).
  @param [in]  Node               Offset of a node in the interrupt hierarchy.
  @param [in]  SearchInHierarchy  If TRUE, search from the parent node
                                  (i.e. get the parent #interrupt-cells).
                                  If FALSE, simply read the property directly from
                                  the input Node.
  @param [out] IntCells           If success, contains the "#interrupt-cells"
                                  property value relevant for Node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtGetInterruptCellsInfo (
  IN  CONST VOID     *Fdt,
  IN        INT32    Node,
  IN        BOOLEAN  SearchInHierarchy,
  OUT       INT32    *IntCells
  )
{
  EFI_STATUS    Status;
  CONST UINT32  *Data;
  INT32         Size;
  INT32         IntDomainNode;

  if ((Fdt == NULL) || (IntCells == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (SearchInHierarchy) {
    Status = FdtGetIntcNode (Fdt, Node, FALSE, &IntDomainNode);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  } else {
    IntDomainNode = Node;
  }

  Data = FdtGetProp (Fdt, IntDomainNode, "#interrupt-cells", &Size);
  if ((Data == NULL) || (Size != sizeof (UINT32))) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  *IntCells = Fdt32ToCpu (*Data);
  return EFI_SUCCESS;
}

/** Get one "interrupt-map" entry.

  This helper parses the "interrupt-map" property of a nexus node and returns
  the fully decoded entry identified by Index. The pointers stored in Entry
  point inside the FDT blob, except the child-side fields when ApplyIntMask is
  TRUE. In that case, the child-side fields point to masked copies stored in
  Entry.

  An "interrupt-map" is encoded as:
  <
    child-unit-address
    child-interrupt-specifier
    interrupt parent
    parent-unit-address
    parent-interrupt-specifier
  >

  @param [in]  Fdt        Pointer to a Flattened Device Tree (Fdt).
  @param [in]  NexusNode  Offset of the nexus node exposing "interrupt-map".
  @param [in]  Index      Zero-based interrupt-map entry index.
  @param [in]  ApplyIntMask  Whether to apply the "interrupt-map-mask" to the
                             child-side fields.
  @param [out] Entry      If success, contains the requested interrupt-map
                          entry.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           The requested entry was not found.
**/
EFI_STATUS
EFIAPI
FdtGetInterruptMap (
  IN  CONST VOID                      *Fdt,
  IN        INT32                     NexusNode,
  IN        UINT32                    Index,
  IN        BOOLEAN                   ApplyIntMask,
  OUT       INTERRUPT_MAP_ENTRY_INFO  *Entry
  )
{
  EFI_STATUS    Status;
  INT32         ChildAddressCells;
  INT32         ChildInterruptCells;
  CONST UINT32  *Map;
  CONST UINT32  *MapMask;
  INT32         MapSize;
  INT32         MapMaskSize;
  INT32         ParentNode;
  INT32         ParentAddressCells;
  INT32         ParentInterruptCells;
  UINT32        MapCells;
  UINT32        ChildCells;
  UINT32        CurrentIndex;
  UINT32        EntryOffset;
  UINT32        MaskIndex;
  UINT32        PHandle;
  UINT32        EntryCells;

  if ((Fdt == NULL) || (Entry == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Entry, sizeof (*Entry));

  Status = FdtGetInterruptCellsInfo (Fdt, NexusNode, FALSE, &ChildInterruptCells);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  ChildAddressCells = FdtAddressCells (Fdt, NexusNode);
  if (ChildAddressCells < 0) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  Map = FdtGetProp (Fdt, NexusNode, "interrupt-map", &MapSize);
  if ((Map == NULL) || (MapSize <= 0)) {
    return EFI_NOT_FOUND;
  }

  ASSERT ((MapSize % sizeof (UINT32)) == 0);
  ASSERT ((UINT32)ChildAddressCells <= FDT_MAX_NCELLS);
  ASSERT ((UINT32)ChildInterruptCells <= FDT_MAX_NCELLS);

  MapCells     = (UINT32)(MapSize / sizeof (UINT32));
  ChildCells   = (UINT32)ChildAddressCells + (UINT32)ChildInterruptCells;
  CurrentIndex = 0;
  EntryOffset  = 0;
  MapMask      = NULL;

  if (ApplyIntMask) {
    MapMask = FdtGetProp (Fdt, NexusNode, "interrupt-map-mask", &MapMaskSize);
    if ((MapMask == NULL) || ((UINT32)MapMaskSize != (ChildCells * sizeof (UINT32)))) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }
  }

  while (EntryOffset < MapCells) {
    if ((EntryOffset + ChildCells + 1U) > MapCells) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    //
    // An "interrupt-map" child-side key is encoded as:
    //   <child-unit-address child-interrupt-specifier interrupt-parent ...>
    // Get the interrupt-parent phandle.
    //
    PHandle    = Fdt32ToCpu (Map[EntryOffset + ChildCells]);
    ParentNode = FdtNodeOffsetByPhandle (Fdt, PHandle);
    if (ParentNode < 0) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    Status = FdtGetIntcAddressCells (Fdt, ParentNode, &ParentAddressCells, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }

    Status = FdtGetInterruptCellsInfo (Fdt, ParentNode, FALSE, &ParentInterruptCells);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }

    //
    // The size of an entry can be computed:
    // - child-unit-address::ChildAddressCells
    // - child-interrupt-specifier::ChildInterruptCells
    // - interrupt parent::1U
    // - parent-unit-address::ParentAddressCells
    // - parent-interrupt-specifier::ParentInterruptCells
    //
    EntryCells = ChildCells + 1U + (UINT32)ParentAddressCells + (UINT32)ParentInterruptCells;
    if ((EntryOffset + EntryCells) > MapCells) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    if (CurrentIndex == Index) {
      Entry->ChildAddress         = &Map[EntryOffset];
      Entry->ChildAddressCells    = ChildAddressCells;
      Entry->ChildInterrupt       = &Map[EntryOffset + ChildAddressCells];
      Entry->ChildInterruptCells  = ChildInterruptCells;
      Entry->InterruptParent      = &Map[EntryOffset + ChildCells];
      Entry->ParentAddress        = &Map[EntryOffset + ChildCells + 1U];
      Entry->ParentAddressCells   = ParentAddressCells;
      Entry->ParentInterrupt      = &Map[EntryOffset + ChildCells + 1U + (UINT32)ParentAddressCells];
      Entry->ParentInterruptCells = ParentInterruptCells;

      if (ApplyIntMask) {
        for (MaskIndex = 0; MaskIndex < (UINT32)ChildAddressCells; MaskIndex++) {
          Entry->MaskedChildAddress[MaskIndex] =
            Fdt32ToCpu (Entry->ChildAddress[MaskIndex]) &
            Fdt32ToCpu (MapMask[MaskIndex]);
        }

        for (MaskIndex = 0; MaskIndex < (UINT32)ChildInterruptCells; MaskIndex++) {
          Entry->MaskedChildInterrupt[MaskIndex] =
            Fdt32ToCpu (Entry->ChildInterrupt[MaskIndex]) &
            Fdt32ToCpu (MapMask[(UINT32)ChildAddressCells + MaskIndex]);
        }

        Entry->ChildAddress   = Entry->MaskedChildAddress;
        Entry->ChildInterrupt = Entry->MaskedChildInterrupt;
      }

      return EFI_SUCCESS;
    }

    EntryOffset += EntryCells;
    CurrentIndex++;
  }

  return EFI_NOT_FOUND;
}

/** Resolve a child interrupt specifier through interrupt nexus nodes.

  This helper supports nexus nodes whose interrupt-map matching does not depend
  on child unit-address cells. If an interrupt-map entry targets another nexus,
  resolution continues recursively until a real interrupt-controller is found.

  @param [in]  Fdt             Pointer to a Flattened Device Tree (Fdt).
  @param [in]  NexusNode       Offset of the interrupt nexus or controller
                               node.
  @param [in]  InterruptData   Pointer to the child interrupt specifier.
  @param [in]  InputInterruptCells Number of cells in InterruptData.
  @param [out] Interrupt       If success, points at the resolved interrupt
                               specifier for the final controller.
  @param [out] InterruptCells  If success, number of cells in Interrupt.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching interrupt-map entry found.
  @retval EFI_UNSUPPORTED         Unsupported interrupt-map format.
**/
STATIC
EFI_STATUS
EFIAPI
FdtResolveInterruptInternal (
  IN  CONST VOID    *Fdt,
  IN        INT32   NexusNode,
  IN  CONST UINT32  *InterruptData,
  IN        INT32   InputInterruptCells,
  OUT CONST UINT32  **Interrupt,
  OUT       INT32   *InterruptCells
  )
{
  EFI_STATUS                Status;
  INTERRUPT_MAP_ENTRY_INFO  MapEntry;
  CONST UINT32              *MapMask;
  INT32                     ChildAddressCells;
  INT32                     MapMaskSize;
  UINT32                    ChildCells;
  UINT32                    EntryIndex;
  UINT32                    Index;

  if ((Fdt == NULL) ||
      (InterruptData == NULL) ||
      (Interrupt == NULL) ||
      (InterruptCells == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (FdtNodeHasProperty (Fdt, NexusNode, "interrupt-controller")) {
    *Interrupt      = InterruptData;
    *InterruptCells = InputInterruptCells;
    return EFI_SUCCESS;
  }

  ChildAddressCells = FdtAddressCells (Fdt, NexusNode);
  if (ChildAddressCells < 0) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  //
  // An "interrupt-map" child-side key is encoded as:
  // <child-unit-address child-interrupt-specifier>.
  //
  ChildCells = (UINT32)ChildAddressCells + (UINT32)InputInterruptCells;
  if ((UINT32)ChildAddressCells > FDT_MAX_NCELLS) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  //
  // The "interrupt-map-mask" is used to mask children ids:
  // <child-unit-address child-interrupt-specifier>.
  //
  MapMask = FdtGetProp (Fdt, NexusNode, "interrupt-map-mask", &MapMaskSize);
  if ((MapMask == NULL) || (MapMaskSize != (INT32)(ChildCells * sizeof (UINT32)))) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  //
  // Child unit-address translation across intermediate buses is not handled
  // here. Support the common case where the interrupt-map ignores it.
  //
  for (Index = 0; Index < (UINT32)ChildAddressCells; Index++) {
    if (Fdt32ToCpu (MapMask[Index]) != 0) {
      return EFI_UNSUPPORTED;
    }
  }

  for (EntryIndex = 0; ; EntryIndex++) {
    INT32  ParentNode;

    Status = FdtGetInterruptMap (Fdt, NexusNode, EntryIndex, TRUE, &MapEntry);
    if (Status == EFI_NOT_FOUND) {
      break;
    } else if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (FALSE);
      return Status;
    }

    if ((MapEntry.ChildAddressCells != ChildAddressCells) ||
        (MapEntry.ChildInterruptCells != InputInterruptCells))
    {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    for (Index = 0; Index < (UINT32)InputInterruptCells; Index++) {
      if (MapEntry.ChildInterrupt[Index] !=
          (Fdt32ToCpu (InterruptData[Index]) &
           Fdt32ToCpu (MapMask[(UINT32)ChildAddressCells + Index])))
      {
        //
        // This helper only supports nexus mappings that ignore the child
        // unit-address, so only the interrupt specifier cells come from
        // InterruptData.
        //
        break;
      }
    }

    if (Index == (UINT32)InputInterruptCells) {
      ParentNode = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (*MapEntry.InterruptParent));
      if (ParentNode < 0) {
        ASSERT (FALSE);
        return EFI_ABORTED;
      }

      return FdtResolveInterruptInternal (
               Fdt,
               ParentNode,
               MapEntry.ParentInterrupt,
               MapEntry.ParentInterruptCells,
               Interrupt,
               InterruptCells
               );
    }
  }

  return EFI_NOT_FOUND;
}

/** Resolve an interrupt specifier for a node through interrupt nexus nodes.

  If the interrupt parent domain of Node is already an interrupt-controller,
  the requested interrupt specifier is returned directly from the node
  "interrupts" property. Otherwise, the interrupt specifier is resolved
  recursively through one or more parent nexus "interrupt-map" properties
  until a final interrupt-controller is reached.

  @param [in]  Fdt        Pointer to a Flattened Device Tree (Fdt).
  @param [in]  Node       Node to get the interrupt from.
  @param [in]  Index      Index of the interrupt to get.
  @param [out] Interrupt  If success, contains the resolved interrupt
                          specifier.
  @param [out] InterruptCells If success, number of cells in Interrupt.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           The requested interrupt was not found.
  @retval EFI_UNSUPPORTED         Unsupported interrupt-map format.
**/
EFI_STATUS
EFIAPI
FdtResolveInterrupt (
  IN  CONST VOID    *Fdt,
  IN        INT32   Node,
  IN        UINT32  Index,
  OUT CONST UINT32  **Interrupt,
  OUT       INT32   *InterruptCells
  )
{
  EFI_STATUS    Status;
  INT32         IntcNode;
  INT32         IntCells;
  CONST UINT32  *InterruptData;
  INT32         DataSize;
  UINTN         EntryOffset;

  if ((Fdt == NULL) || (Interrupt == NULL) || (InterruptCells == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = FdtGetIntDomainNode (Fdt, Node, &IntcNode);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  Status = FdtGetInterruptCellsInfo (Fdt, IntcNode, FALSE, &IntCells);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  InterruptData = FdtGetProp (Fdt, Node, "interrupts", &DataSize);
  if ((InterruptData == NULL) || (DataSize <= 0)) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  EntryOffset = (UINTN)Index * (UINTN)IntCells;
  if ((EntryOffset + (UINTN)IntCells) > ((UINTN)DataSize / sizeof (UINT32))) {
    return EFI_NOT_FOUND;
  }

  InterruptData += EntryOffset;

  return FdtResolveInterruptInternal (
           Fdt,
           IntcNode,
           InterruptData,
           IntCells,
           Interrupt,
           InterruptCells
           );
}

/** Get the "#address-cells" and/or "#size-cells" property of the node.

  According to the Device Tree specification, s2.3.5 "#address-cells and
  #size-cells":
  "If missing, a client program should assume a default value of 2 for
  #address-cells, and a value of 1 for #size-cells."

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  Node             Offset of the node having to get the
                                "#address-cells" and "#size-cells"
                                properties from.
  @param [out] AddressCells     If success, number of address-cells.
                                If the property is not available,
                                default value is 2.
  @param [out] SizeCells        If success, number of size-cells.
                                If the property is not available,
                                default value is 1.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtGetAddressInfo (
  IN  CONST VOID *Fdt,
  IN        INT32 Node,
  OUT       INT32 *AddressCells, OPTIONAL
  OUT       INT32     *SizeCells       OPTIONAL
  )
{
  if (Fdt == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (AddressCells != NULL) {
    *AddressCells = FdtAddressCells (Fdt, Node);
    if (*AddressCells < 0) {
      ASSERT (0);
      return EFI_ABORTED;
    }
  }

  if (SizeCells != NULL) {
    *SizeCells = FdtSizeCells (Fdt, Node);
    if (*SizeCells < 0) {
      ASSERT (0);
      return EFI_ABORTED;
    }
  }

  return EFI_SUCCESS;
}

/** Get the "#address-cells" and/or "#size-cells" property of the parent node.

  According to the Device Tree specification, s2.3.5 "#address-cells and
  #size-cells":
  "If missing, a client program should assume a default value of 2 for
  #address-cells, and a value of 1 for #size-cells."

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  Node             Offset of the node having to get the
                                "#address-cells" and "#size-cells"
                                properties from its parent.
  @param [out] AddressCells     If success, number of address-cells.
                                If the property is not available,
                                default value is 2.
  @param [out] SizeCells        If success, number of size-cells.
                                If the property is not available,
                                default value is 1.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtGetParentAddressInfo (
  IN  CONST VOID *Fdt,
  IN        INT32 Node,
  OUT       INT32 *AddressCells, OPTIONAL
  OUT       INT32     *SizeCells       OPTIONAL
  )
{
  if (Fdt == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Node = FdtParentOffset (Fdt, Node);
  if (Node < 0) {
    // End of the tree, or an error occurred.
    ASSERT (0);
    return EFI_ABORTED;
  }

  return FdtGetAddressInfo (Fdt, Node, AddressCells, SizeCells);
}

/** Get an address/size pair from a node "reg" property.

  The "reg" property stores addresses in the parent bus address space.
  This helper reads the requested entry without applying parent bus
  "ranges" translations.

  The helper supports address and size fields up to 64 bits.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  Node             Offset of the node owning the "reg" property.
  @param [in]  Index            Index of the address/size pair to read.
  @param [out] BaseAddress      If success, contains the raw base address.
  @param [out] BaseAddressSize  If success, contains the size associated with
                                the raw base address. This parameter is
                                optional.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           The requested "reg" entry was not found.
  @retval EFI_UNSUPPORTED         Unsupported address or size encoding.
**/
EFI_STATUS
EFIAPI
FdtGetReg (
  IN  CONST VOID    *Fdt,
  IN        INT32   Node,
  IN        UINT32  Index,
  OUT       UINT64  *BaseAddress,
  OUT       UINT64  *BaseAddressSize OPTIONAL
  )
{
  EFI_STATUS    Status;
  CONST UINT32  *Reg;
  UINTN         EntryOffset;
  UINTN         EntryStride;
  INT32         AddressCells;
  INT32         SizeCells;
  INT32         PropertySize;

  if ((Fdt == NULL) || (BaseAddress == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = FdtGetParentAddressInfo (Fdt, Node, &AddressCells, &SizeCells);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if ((AddressCells < 1) ||
      (AddressCells > 2) ||
      (SizeCells < 1)    ||
      (SizeCells > 2))
  {
    ASSERT (0);
    return EFI_UNSUPPORTED;
  }

  Reg = FdtGetProp (Fdt, Node, "reg", &PropertySize);
  if ((Reg == NULL) || (PropertySize < 0)) {
    ASSERT (0);
    return EFI_ABORTED;
  }

  //
  // One "reg" entry is encoded as:
  // <address size>.
  //
  EntryStride = (UINTN)AddressCells + (UINTN)SizeCells;
  if (((PropertySize % sizeof (UINT32)) != 0) ||
      (((UINTN)PropertySize / sizeof (UINT32)) < EntryStride))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  EntryOffset = (UINTN)Index * EntryStride;
  if ((EntryOffset + EntryStride) > ((UINTN)PropertySize / sizeof (UINT32))) {
    return EFI_NOT_FOUND;
  }

  //
  // EntryOffset points to the first address cell of the selected "reg"
  // tuple in the property.
  //
  Status = ReadFdtCells64 (Reg + EntryOffset, AddressCells, BaseAddress);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  if (BaseAddressSize != NULL) {
    //
    // The size field starts immediately after the address field in the
    // current "reg" tuple.
    //
    Status = ReadFdtCells64 (Reg + EntryOffset + AddressCells, SizeCells, BaseAddressSize);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  return EFI_SUCCESS;
}
