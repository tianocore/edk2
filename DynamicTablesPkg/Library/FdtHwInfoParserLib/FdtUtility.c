/** @file
  Flattened device tree utility.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Device tree Specification - Release v0.3
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
  - linux//Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
**/

#include <FdtHwInfoParserInclude.h>
#include "FdtUtility.h"

/** Get the interrupt Id of an interrupt described in a fdt.

  Data must describe a GIC interrupt. A GIC interrupt is on at least
  3 UINT32 cells.
  This function DOES NOT SUPPORT extended SPI range and extended PPI range.

  @param [in]  Data   Pointer to the first cell of an "interrupts" property.

  @retval  The interrupt id.
**/
UINT32
EFIAPI
FdtGetInterruptId (
  UINT32 CONST  *Data
  )
{
  UINT32  IrqType;
  UINT32  IrqId;

  ASSERT (Data != NULL);

  IrqType = fdt32_to_cpu (Data[IRQ_TYPE_OFFSET]);
  IrqId   = fdt32_to_cpu (Data[IRQ_NUMBER_OFFSET]);

  switch (IrqType) {
    case DT_SPI_IRQ:
      IrqId += SPI_OFFSET;
      break;

    case DT_PPI_IRQ:
      IrqId += PPI_OFFSET;
      break;

    default:
      ASSERT (0);
      IrqId = 0;
  }

  return IrqId;
}

/** Get the ACPI interrupt flags of an interrupt described in a fdt.

  Data must describe a GIC interrupt. A GIC interrupt is on at least
  3 UINT32 cells.

  PPI interrupt cpu mask on bits [15:8] are ignored.

  @param [in]  Data   Pointer to the first cell of an "interrupts" property.

  @retval  The interrupt flags (for ACPI).
**/
UINT32
EFIAPI
FdtGetInterruptFlags (
  UINT32 CONST  *Data
  )
{
  UINT32  IrqFlags;
  UINT32  AcpiIrqFlags;

  ASSERT (Data != NULL);

  IrqFlags = fdt32_to_cpu (Data[IRQ_FLAGS_OFFSET]);

  AcpiIrqFlags  = DT_IRQ_IS_EDGE_TRIGGERED (IrqFlags) ? BIT0 : 0;
  AcpiIrqFlags |= DT_IRQ_IS_ACTIVE_LOW (IrqFlags) ? BIT1 : 0;

  return AcpiIrqFlags;
}

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
  NodeName = fdt_offset_ptr (Fdt, Node + FDT_TAGSIZE, Length + 1);
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
  Prop = fdt_getprop (Fdt, Node, "compatible", &PropLen);
  if ((Prop == NULL) || (PropLen < 0)) {
    return FALSE;
  }

  for (Index = 0; Index < Count; Index++) {
    if (fdt_stringlist_contains (
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

  Prop = fdt_getprop (Fdt, Node, PropertyName, &Size);
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
    CurrNode = fdt_next_node (Fdt, CurrNode, Depth);
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
    for (CurrNode = fdt_next_node (Fdt, CurrNode, &Depth);
         (CurrNode >= 0) && (Depth > 0);
         CurrNode = fdt_next_node (Fdt, CurrNode, &Depth))
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
  @param [out] IntcNode         If success, contains the offset of the
                                interrupt-controller node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           No interrupt-controller node found.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
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
  CONST VOID    *Prop;

  if ((Fdt == NULL) ||
      (IntcNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  while (TRUE) {
    // Check whether the node has the "interrupt-controller" property.
    Prop = fdt_getprop (Fdt, Node, "interrupt-controller", &Size);
    if ((Prop != NULL) && (Size >= 0)) {
      // The interrupt-controller has been found.
      *IntcNode = Node;
      return EFI_SUCCESS;
    } else {
      // Check whether the node has the "interrupt-parent" property.
      PHandle = fdt_getprop (Fdt, Node, "interrupt-parent", &Size);
      if ((PHandle != NULL) && (Size == sizeof (UINT32))) {
        // The phandle of the interrupt-controller has been found.
        // Search the node having this phandle and return it.
        Node = fdt_node_offset_by_phandle (Fdt, fdt32_to_cpu (*PHandle));
        if (Node < 0) {
          ASSERT (0);
          return EFI_ABORTED;
        }

        *IntcNode = Node;
        return EFI_SUCCESS;
      } else if (Size != -FDT_ERR_NOTFOUND) {
        ASSERT (0);
        return EFI_ABORTED;
      }
    }

    if (Node == 0) {
      // We are at the root of the tree. Not parent available.
      return EFI_NOT_FOUND;
    }

    // Get the parent of the node.
    Node = fdt_parent_offset (Fdt, Node);
    if (Node < 0) {
      // An error occurred.
      ASSERT (0);
      return EFI_ABORTED;
    }
  } // while
}

/** Get the "interrupt-cells" property value of the node.

  The "interrupts" property requires to know the number of cells used
  to encode an interrupt. This information is stored in the
  interrupt-controller of the input Node.

  @param [in]  Fdt          Pointer to a Flattened Device Tree (Fdt).
  @param [in]  IntcNode     Offset of an interrupt-controller node.
  @param [out] IntCells     If success, contains the "interrupt-cells"
                            property of the IntcNode.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
FdtGetInterruptCellsInfo (
  IN  CONST VOID   *Fdt,
  IN        INT32  IntcNode,
  OUT       INT32  *IntCells
  )
{
  CONST UINT32  *Data;
  INT32         Size;

  if ((Fdt == NULL) ||
      (IntCells == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Data = fdt_getprop (Fdt, IntcNode, "#interrupt-cells", &Size);
  if ((Data == NULL) || (Size != sizeof (UINT32))) {
    // If error or not on one UINT32 cell.
    ASSERT (0);
    return EFI_ABORTED;
  }

  *IntCells = fdt32_to_cpu (*Data);

  return EFI_SUCCESS;
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
    *AddressCells = fdt_address_cells (Fdt, Node);
    if (*AddressCells < 0) {
      ASSERT (0);
      return EFI_ABORTED;
    }
  }

  if (SizeCells != NULL) {
    *SizeCells = fdt_size_cells (Fdt, Node);
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

  Node = fdt_parent_offset (Fdt, Node);
  if (Node < 0) {
    // End of the tree, or an error occurred.
    ASSERT (0);
    return EFI_ABORTED;
  }

  return FdtGetAddressInfo (Fdt, Node, AddressCells, SizeCells);
}
