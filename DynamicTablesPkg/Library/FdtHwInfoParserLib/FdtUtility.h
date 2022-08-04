/** @file
  Flattened device tree utility.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Device tree Specification - Release v0.3
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
  - linux//Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
**/

#ifndef FDT_UTILITY_H_
#define FDT_UTILITY_H_

/** Get the offset of an address in a "reg" Device Tree property.

  In a Device Tree, the "reg" property stores address/size couples.
  They are stored on N 32-bits cells.
  Based on the value of the #address-cells, the #size-cells and the
  index in the "reg" property, compute the number of 32-bits cells
  to skip.

  @param [in]  Index        Index in the reg property.
  @param [in]  AddrCells    Number of cells used to store an address.
  @param [in]  SizeCells    Number of cells used to store the size of
                            an address.

  @retval  Number of 32-bits cells to skip to access the address.
*/
#define GET_DT_REG_ADDRESS_OFFSET(Index, AddrCells, SizeCells)  (           \
          (Index) * ((AddrCells) + (SizeCells))                             \
          )

/** Get the offset of an address size in a "reg" Device Tree property.

  In a Device Tree, the "reg" property stores address/size couples.
  They are stored on N 32-bits cells.
  Based on the value of the #address-cells, the #size-cells and the
  index in the "reg" property, compute the number of 32-bits cells
  to skip.

  @param [in]  Index        Index in the reg property.
  @param [in]  AddrCells    Number of cells used to store an address.
  @param [in]  SizeCells    Number of cells used to store the size of
                            an address.

  @retval  Number of 32-bits cells to skip to access the address size.
*/
#define GET_DT_REG_SIZE_OFFSET(Index, AddrCells, SizeCells)  (              \
          GET_DT_REG_ADDRESS_OFFSET ((Index), (AddrCells), (SizeCells)) +   \
          (SizeCells)                                                       \
          )

/// Maximum string length for compatible names.
#define COMPATIBLE_STR_LEN  (32U)

/// Interrupt macros
#define PPI_OFFSET  (16U)
#define SPI_OFFSET  (32U)
#define DT_PPI_IRQ  (1U)
#define DT_SPI_IRQ  (0U)
#define DT_IRQ_IS_EDGE_TRIGGERED(x)  ((((x) & (BIT0 | BIT1)) != 0))
#define DT_IRQ_IS_ACTIVE_LOW(x)      ((((x) & (BIT1 | BIT3)) != 0))
#define IRQ_TYPE_OFFSET    (0U)
#define IRQ_NUMBER_OFFSET  (1U)
#define IRQ_FLAGS_OFFSET   (2U)

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
  );

/** Get the ACPI interrupt flags of an interrupt described in a fdt.

  Data must describe a GIC interrupt. A GIC interrupt is on at least
  3 UINT32 cells.

  @param [in]  Data   Pointer to the first cell of an "interrupts" property.

  @retval  The interrupt flags (for ACPI).
**/
UINT32
EFIAPI
FdtGetInterruptFlags (
  UINT32 CONST  *Data
  );

/** A structure describing a compatibility string.
*/
typedef struct CompatStr {
  CONST CHAR8    CompatStr[COMPATIBLE_STR_LEN];
} COMPATIBILITY_STR;

/** Structure containing a list of compatible names and their count.
*/
typedef struct CompatibilityInfo {
  /// Count of entries in the NAME_TABLE.
  UINT32                     Count;

  /// Pointer to a table storing the names.
  CONST COMPATIBILITY_STR    *CompatTable;
} COMPATIBILITY_INFO;

/** Operate a check on a Device Tree node.

  @param [in]  Fdt          Pointer to a Flattened Device Tree.
  @param [in]  NodeOffset   Offset of the node to compare input string.
  @param [in]  Context      Context to operate the check on the node.

  @retval True    The check is correct.
  @retval FALSE   Otherwise, or error.
**/
typedef
BOOLEAN
(EFIAPI *NODE_CHECKER_FUNC)(
  IN  CONST VOID    *Fdt,
  IN        INT32     NodeOffset,
  IN  CONST VOID    *Context
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/** Count the number of nodes in a branch with at least
    one compatible property.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  FdtBranch        Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]  CompatibleTable  Table of compatible strings to
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
  );

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
  );

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
  );

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
  OUT       INT32  *InterruptCells
  );

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
  );

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
  );

#endif // FDT_UTILITY_H_
