/** @file
  Flattened device tree utility.

  Copyright (c) 2021 - 2026, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Device tree Specification - Release v0.3
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
  - linux//Documentation/devicetree/bindings/interrupt-controller/arm%2Cgic.yaml
**/

#pragma once

#include <Library/FdtLib.h>

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
#define IRQ_TYPE_OFFSET          (0U)
#define IRQ_NUMBER_OFFSET        (1U)
#define IRQ_FLAGS_OFFSET         (2U)
#define RISCV_IRQ_NUMBER_OFFSET  (0U)
#define RISCV_IRQ_FLAGS_OFFSET   (1U)

/** Get the interrupt Id of an interrupt described in a fdt.

  Data must describe a GIC interrupt. A GIC interrupt is on at least
  3 UINT32 cells.
  This function DOES NOT SUPPORT extended SPI range and extended PPI range.

  @param [in]  Data   Pointer to the first cell of an "interrupts" property.
  @param [in]  Size   Number of cells used to encode an interrupt.

  @retval  The interrupt id.
**/
UINT32
EFIAPI
FdtGetInterruptId (
  UINT32 CONST  *Data,
  UINT32        Size
  );

/** Get the ACPI interrupt flags of an interrupt described in a fdt.

  Data must describe a GIC interrupt. A GIC interrupt is on at least
  3 UINT32 cells.

  PPI interrupt cpu mask on bits [15:8] are ignored.

  @param [in]  Data   Pointer to the first cell of an "interrupts" property.
  @param [in]  Size   Number of cells used to encode an interrupt.

  @retval  The interrupt flags (for ACPI).
**/
UINT32
EFIAPI
FdtGetInterruptFlags (
  UINT32 CONST  *Data,
  UINT32        Size
  );

/** Parsed "interrupt-map" entry information.
*/
typedef struct {
  /// Child unit-address cells. Points into the FDT blob unless masked.
  CONST UINT32    *ChildAddress;
  /// Number of child unit-address cells.
  INT32           ChildAddressCells;
  /// Child interrupt specifier cells. Points into the FDT blob unless masked.
  CONST UINT32    *ChildInterrupt;
  /// Number of child interrupt specifier cells.
  INT32           ChildInterruptCells;
  /// "interrupt-parent" phandle cell from the map entry.
  CONST UINT32    *InterruptParent;
  /// Parent unit-address cells. Points into the FDT blob.
  CONST UINT32    *ParentAddress;
  /// Number of parent unit-address cells.
  INT32           ParentAddressCells;
  /// Parent interrupt specifier cells. Points into the FDT blob.
  CONST UINT32    *ParentInterrupt;
  /// Number of parent interrupt specifier cells.
  INT32           ParentInterruptCells;
  /// Storage for masked child unit-address cells when ApplyIntMask is TRUE.
  UINT32          MaskedChildAddress[FDT_MAX_NCELLS];
  /// Storage for masked child interrupt cells when ApplyIntMask is TRUE.
  UINT32          MaskedChildInterrupt[FDT_MAX_NCELLS];
} INTERRUPT_MAP_ENTRY_INFO;

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
  );

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
  );

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
  );

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
  );

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
  );

/** Resolve an interrupt specifier for a node through interrupt nexus nodes.

  If the interrupt parent domain of Node is already an interrupt-controller,
  the requested interrupt specifier is returned directly from the node
  "interrupts" property. Otherwise, the interrupt specifier is resolved
  recursively through one or more parent nexus "interrupt-map" properties
  until a final interrupt-controller is reached.

  @param [in]  Fdt             Pointer to a Flattened Device Tree (Fdt).
  @param [in]  Node            Node to get the interrupt from.
  @param [in]  Index           Index of the interrupt to get.
  @param [out] Interrupt       If success, contains the resolved interrupt
                               specifier.
  @param [out] InterruptCells  If success, contains the "#interrupt-cells"
                               value of the final interrupt-controller.

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

/** For relevant architectures, get the "#address-cells" and/or "#size-cells"
    property of the node.

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

  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FdtGetIntcAddressCells (
  IN  CONST VOID *Fdt,
  IN        INT32 Node,
  OUT       INT32 *AddressCells, OPTIONAL
  OUT       INT32     *SizeCells       OPTIONAL
  );

/** Convert the IRQ number in DT to ACPI GSI number

  In RISC-V, GSI space is divided among PLIC/APLICs. Hence, based
  on the parent interrupt controller, the IRQ number need to be
  converted into appropriate GSI number.

  @param [in]  ExtIntcNode    Parent interrupt controller node.
  @param [in]  Irq       Irq number to convert.

**/
UINT32
FdtConvertToGsi (
  IN INT32   ExtIntcNode,
  IN UINT32  Irq
  );

/** Create list of external interrupt controllers.

  In RISC-V, GSI space is divided among PLIC/APLICs. To convert
  the IRQ number in DT of a device to appropriate GSI number,
  create a list of external interrupt controllers.

  @param [in]  ExtIntcNode    External interrupt controller node.
  @param [in]  GsiBase        GSI base of the interrupt controller.

  @retval EFI_SUCCESS           External interrupt controller node is added in the list.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the list.
**/
EFI_STATUS
FdtCreateExtIntcList (
  INT32   ExtIntcNode,
  UINT32  GsiBase
  );
