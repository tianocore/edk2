/** @file
  Read UFDT properties and decode addresses and interrupts.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include "UfdtDispatcherLibInternal.h"

// Max number of cells used to encode interrupts
#define UFDT_MAX_INTERRUPT_CELLS  4

/** Return a borrowed value from a node's copied property list.

  @param[in]  Node    Node whose properties are searched.
  @param[in]  Name    Property name to get.
  @param[out] Length  Optional, return the length on success.
                      - 0 if not found.
                      - -1 if an argument or the stored property is invalid.

  @retval NULL   The property is absent or invalid, or an argument is invalid.
                 Length, when supplied, distinguishes absence from an error.
  @retval Other  Borrowed property value, including zero-length properties.
**/
CONST VOID *
EFIAPI
UfdtNodeGetProperty (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN CONST CHAR8             *Name,
  OUT INT32                  *Length OPTIONAL
  )
{
  CONST UFDT_NODE_PROPERTY  *Property;

  if (Length != NULL) {
    *Length = -1;
  }

  if ((Node == NULL) || (Name == NULL)) {
    return NULL;
  }

  for (Property = Node->Properties; Property != NULL; Property = Property->Next) {
    if (AsciiStrCmp (Property->Name, Name) == 0) {
      if ((Property->Length > MAX_INT32) || (Property->Value == NULL)) {
        return NULL;
      }

      if (Length != NULL) {
        *Length = (INT32)Property->Length;
      }

      return Property->Value;
    }
  }

  if (Length != NULL) {
    *Length = 0;
  }

  return NULL;
}

/** Read a node's cell-count property with a default for absence.

  @param[in] Node     Node whose copied property is read.
  @param[in] Name     Property name to get.
  @param[in] Default  Value returned only when the property is absent.

  @retval -1     Invalid parameter or malformed cell-count property.
  @retval Other  Cell count, or Default when absent.
**/
STATIC
INT32
NodeCells (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN CONST CHAR8             *Name,
  IN INT32                   Default
  )
{
  CONST VOID  *Property;
  INT32       Length;
  UINT32      Cells;

  Property = UfdtNodeGetProperty (Node, Name, &Length);
  if (Property == NULL) {
    return (Length == 0) ? Default : -1;
  }

  if (Length != sizeof (UINT32)) {
    return -1;
  }

  Cells = SwapBytes32 (ReadUnaligned32 (Property));
  return (Cells <= MAX_INT32) ? (INT32)Cells : -1;
}

/** Read the address-cell count declared on a node.

  Read #address-cells from Node only.
  Default to 2 when absent, cf. DeviceTree specification.

  @param[in] Node  Node whose address-cell count is requested.

  @retval -1     Invalid node or cell-count property.
  @retval 2      The property is absent or explicitly specifies two cells.
  @retval Other  The cell count.
**/
INT32
EFIAPI
UfdtNodeAddressCells (
  IN CONST UFDT_DEVICE_NODE  *Node
  )
{
  return NodeCells (Node, "#address-cells", 2);
}

/** Read the size-cell count declared on a node.

  Read #size-cells from Node only.
  Default to 1 when absent, cf. DeviceTree specification.
  Zero is valid.

  @param[in] Node  Node whose size-cell count is requested.

  @retval -1     Invalid node or cell-count property.
  @retval 1      The property is absent or explicitly specifies one cell.
  @retval Other  The cell count.
**/
INT32
EFIAPI
UfdtNodeSizeCells (
  IN CONST UFDT_DEVICE_NODE  *Node
  )
{
  return NodeCells (Node, "#size-cells", 1);
}

/** Check a node's availability and that of every ancestor.

  Each status property must be absent, "ok" or "okay".

  @param[in] Node  Node whose availability is requested, or NULL.

  @retval TRUE   Node and every ancestor permit use.
  @retval FALSE  Invalid node or an unavailable node in its ancestor chain.
**/
BOOLEAN
EFIAPI
UfdtDeviceNodeIsAvailable (
  IN CONST UFDT_DEVICE_NODE  *Node
  )
{
  CONST CHAR8  *Status;
  INT32        Size;

  if (Node == NULL) {
    return FALSE;
  }

  for ( ; Node != NULL; Node = Node->Parent) {
    Status = UfdtNodeGetProperty (Node, "status", &Size);
    if (Status != NULL) {
      if (!(((Size == 3) && (AsciiStrnCmp (Status, "ok", 3) == 0)) ||
            ((Size == 5) && (AsciiStrnCmp (Status, "okay", 5) == 0))))
      {
        return FALSE;
      }
    } else if (Size != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/** Check whether Compatible string matches the Node.

  @param[in] Node        Node to search.
  @param[in] Compatible  Compatible string to match.

  @retval TRUE   A complete entry matches and every list entry is terminated.
  @retval FALSE  Invalid parameter, malformed property or no match.
**/
BOOLEAN
EFIAPI
UfdtNodeIsCompatible (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN CONST CHAR8             *Compatible
  )
{
  CONST CHAR8  *Data;
  INT32        Size;
  INT32        Offset;
  INT32        Length;
  BOOLEAN      Match;

  if (Compatible == NULL) {
    return FALSE;
  }

  Data = UfdtNodeGetProperty (Node, "compatible", &Size);
  if (Data == NULL) {
    return FALSE;
  }

  Match = FALSE;
  for (Offset = 0; Offset < Size; Offset += Length + 1) {
    for (Length = 0; (Length < Size - Offset) && (Data[Offset + Length] != '\0'); Length++) {
    }

    if (Length == Size - Offset) {
      return FALSE;
    }

    if (AsciiStrCmp (Data + Offset, Compatible) == 0) {
      Match = TRUE;
    }
  }

  return Match;
}

/** Get the interrupt domain parent node handling the input node's interrupts.

  The interrupt domain parent must be one of the following:
  - an interrupt-controller
  - an interrupt nexus

  If the ancestor search reaches the root without finding a domain, return
  the input node if it defines a domain. This does not check availability or
  resolve interrupt-map entries.

  @param[in]  Node      Node whose interrupt domain is requested.
  @param[out] IntcNode  Borrowed matching node on success, NULL on failure.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
STATIC
EFI_STATUS
UfdtGetIntDomainNode (
  IN CONST UFDT_DEVICE_NODE  *Node,
  OUT UFDT_DEVICE_NODE       **IntcNode
  )
{
  return GetIntcNode (Node, FALSE, IntcNode);
}

/** Read one or two big-endian cells without an alignment assumption.

  @param[in] Data   At least Cells * sizeof (UINT32) readable bytes.
  @param[in] Cells  Cell count, already validated as one or two by the caller.

  @retval Other  The decoded native-endian value.
**/
STATIC
UINT64
EFIAPI
ReadCells (
  IN CONST UINT8  *Data,
  IN INT32        Cells
  )
{
  UINT64  Value;

  Value = SwapBytes32 (ReadUnaligned32 (Data));
  if (Cells == 2) {
    Value = LShiftU64 (Value, 32) | SwapBytes32 (ReadUnaligned32 (Data + 4));
  }

  return Value;
}

/** Get the number of cells used to encode an interrupt for a specific Node.

  @param[in]  Node   Interrupt controller or nexus.
  @param[out] Cells  Cell count on success.

  @retval EFI_SUCCESS           Success.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
**/
STATIC
EFI_STATUS
GetInterruptCellsInfo (
  IN CONST UFDT_DEVICE_NODE  *Node,
  OUT UINT32                 *Cells
  )
{
  CONST VOID  *Property;
  INT32       Size;

  Property = UfdtNodeGetProperty (Node, "#interrupt-cells", &Size);
  if ((Property == NULL) || (Size != sizeof (UINT32))) {
    return EFI_COMPROMISED_DATA;
  }

  *Cells = SwapBytes32 (ReadUnaligned32 (Property));
  if (*Cells == 0) {
    return EFI_COMPROMISED_DATA;
  }

  return (*Cells <= UFDT_MAX_INTERRUPT_CELLS) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
}

/** Borrowed, big-endian specifiers from an interrupt-map entry. **/
typedef struct {
  CONST UINT8         *ChildInterrupt;
  UFDT_DEVICE_NODE    *Parent;
  CONST UINT8         *ParentInterrupt;
  UINT32              ParentInterruptCells;
} UFDT_INTERRUPT_MAP_ENTRY;

/** Get one "interrupt-map" entry.

  This helper parses the "interrupt-map" property of a nexus node and returns
  the entry at Offset, then advances Offset to the next entry. The specifier
  pointers stored in Entry point inside the copied property and remain
  big-endian. The Parent field points to the resolved parent node.

  An "interrupt-map" is encoded as:
  <
    child-unit-address
    child-interrupt-specifier
    interrupt parent
    parent-unit-address
    parent-interrupt-specifier
  >

  @param[in]     Nexus   Node containing the map.
  @param[in,out] Offset  Cell offset, initially zero; advanced on success.
  @param[out]    Entry   Borrowed entry fields on success.

  @retval EFI_SUCCESS           Success.
  @retval EFI_NOT_FOUND         Not Found.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
**/
STATIC
EFI_STATUS
GetInterruptMap (
  IN CONST UFDT_DEVICE_NODE     *Nexus,
  IN OUT UINT32                 *Offset,
  OUT UFDT_INTERRUPT_MAP_ENTRY  *Entry
  )
{
  CONST UINT8  *Map;
  INT32        Size;
  INT32        AddressCells;
  INT32        ParentAddressCells;
  UINT32       InterruptCells;
  UINT32       ChildCells;
  UINT32       MapCells;
  UINT32       EntryCells;
  UINT32       Phandle;
  EFI_STATUS   Status;

  Status = GetInterruptCellsInfo (Nexus, &InterruptCells);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AddressCells = UfdtNodeAddressCells (Nexus);
  if (AddressCells < 0) {
    return EFI_COMPROMISED_DATA;
  }

  if (AddressCells > UFDT_MAX_INTERRUPT_CELLS) {
    return EFI_COMPROMISED_DATA;
  }

  Map = UfdtNodeGetProperty (Nexus, "interrupt-map", &Size);
  if ((Size < 0) || ((Size % sizeof (UINT32)) != 0)) {
    return EFI_COMPROMISED_DATA;
  }

  MapCells = (UINT32)Size / sizeof (UINT32);
  if ((Map == NULL) || (*Offset >= MapCells)) {
    return EFI_NOT_FOUND;
  }

  ChildCells = (UINT32)AddressCells + InterruptCells;
  if (MapCells - *Offset < ChildCells + 1) {
    return EFI_COMPROMISED_DATA;
  }

  //
  // An "interrupt-map" child-side key is encoded as:
  //   <child-unit-address child-interrupt-specifier interrupt-parent ...>
  // Get the interrupt-parent phandle.
  //
  Map          += *Offset * sizeof (UINT32);
  Phandle       = SwapBytes32 (ReadUnaligned32 (Map + ChildCells * sizeof (UINT32)));
  Entry->Parent = UfdtFindNodeByPhandle (UfdtGetRootDeviceNode (Nexus), Phandle);
  if (Entry->Parent == NULL) {
    return EFI_COMPROMISED_DATA;
  }

  ParentAddressCells = UfdtNodeAddressCells (Entry->Parent);
  if (ParentAddressCells < 0) {
    return EFI_COMPROMISED_DATA;
  }

  if (ParentAddressCells > UFDT_MAX_INTERRUPT_CELLS) {
    return EFI_COMPROMISED_DATA;
  }

  Status = GetInterruptCellsInfo (Entry->Parent, &Entry->ParentInterruptCells);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // The size of an entry can be computed:
  // - child-unit-address::AddressCells
  // - child-interrupt-specifier::InterruptCells
  // - interrupt parent::1
  // - parent-unit-address::ParentAddressCells
  // - parent-interrupt-specifier::Entry->ParentInterruptCells
  //
  EntryCells = ChildCells + 1 + (UINT32)ParentAddressCells + Entry->ParentInterruptCells;
  if (EntryCells > MapCells - *Offset) {
    return EFI_COMPROMISED_DATA;
  }

  Entry->ChildInterrupt  = Map + (UINT32)AddressCells * sizeof (UINT32);
  Entry->ParentInterrupt = Map + (ChildCells + 1 + (UINT32)ParentAddressCells) * sizeof (UINT32);
  *Offset               += EntryCells;
  return EFI_SUCCESS;
}

/** Resolve a child interrupt specifier through interrupt nexus nodes.

  This helper supports nexus nodes whose interrupt-map matching does not depend
  on child unit-address cells. If an interrupt-map entry targets another nexus,
  resolution continues recursively until a real interrupt-controller is found.

  @param[in]  NexusNode            Interrupt nexus or controller node.
  @param[in]  InterruptData        Borrowed big-endian child interrupt specifier.
  @param[in]  InputInterruptCells  Number of cells in InterruptData.
  @param[out] Interrupt            Borrowed resolved specifier on success.
  @param[out] InterruptCells       Number of resolved cells on success.

  @retval EFI_SUCCESS           Success.
  @retval EFI_NOT_FOUND         Not Found.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
**/
STATIC
EFI_STATUS
ResolveInterruptInternal (
  IN CONST UFDT_DEVICE_NODE  *NexusNode,
  IN CONST UINT32            *InterruptData,
  IN UINT32                  InputInterruptCells,
  OUT CONST UINT32           **Interrupt,
  OUT INT32                  *InterruptCells
  )
{
  EFI_STATUS                Status;
  UFDT_INTERRUPT_MAP_ENTRY  MapEntry;
  CONST UINT32              *MapMask;
  CONST VOID                *Property;
  INT32                     ChildAddressCells;
  INT32                     MapMaskSize;
  INT32                     Size;
  UINT32                    DomainCells;
  UINT32                    ChildCells;
  UINT32                    EntryOffset;
  UINT32                    Index;
  UINT32                    CellMask;

  if (!UfdtDeviceNodeIsAvailable (NexusNode)) {
    return EFI_COMPROMISED_DATA;
  }

  Status = GetInterruptCellsInfo (NexusNode, &DomainCells);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DomainCells != InputInterruptCells) {
    return EFI_COMPROMISED_DATA;
  }

  Property = UfdtNodeGetProperty (NexusNode, "interrupt-controller", &Size);
  if (Size != 0) {
    return EFI_COMPROMISED_DATA;
  }

  if (Property != NULL) {
    *Interrupt      = InterruptData;
    *InterruptCells = (INT32)InputInterruptCells;
    return EFI_SUCCESS;
  }

  ChildAddressCells = UfdtNodeAddressCells (NexusNode);
  if ((ChildAddressCells < 0) || (ChildAddressCells > UFDT_MAX_INTERRUPT_CELLS)) {
    return EFI_COMPROMISED_DATA;
  }

  //
  // An "interrupt-map" child-side key is encoded as:
  // <child-unit-address child-interrupt-specifier>.
  //
  ChildCells = (UINT32)ChildAddressCells + InputInterruptCells;

  //
  // The "interrupt-map-mask" is used to mask child IDs:
  // <child-unit-address child-interrupt-specifier>.
  //
  MapMask = UfdtNodeGetProperty (NexusNode, "interrupt-map-mask", &MapMaskSize);
  if ((MapMask == NULL) || (MapMaskSize != (INT32)(ChildCells * sizeof (UINT32)))) {
    return EFI_COMPROMISED_DATA;
  }

  //
  // Child unit-address translation across intermediate buses is not handled
  // here. Support the common case where the interrupt-map ignores it.
  //
  for (Index = 0; Index < (UINT32)ChildAddressCells; Index++) {
    if (ReadUnaligned32 (&MapMask[Index]) != 0) {
      return EFI_COMPROMISED_DATA;
    }
  }

  EntryOffset = 0;
  while (TRUE) {
    Status = GetInterruptMap (NexusNode, &EntryOffset, &MapEntry);
    if (Status == EFI_NOT_FOUND) {
      break;
    } else if (EFI_ERROR (Status)) {
      return Status;
    }

    for (Index = 0; Index < InputInterruptCells; Index++) {
      CellMask = ReadUnaligned32 (&MapMask[(UINT32)ChildAddressCells + Index]);
      if ((ReadUnaligned32 (MapEntry.ChildInterrupt + Index * sizeof (UINT32)) & CellMask) !=
          (ReadUnaligned32 (&InterruptData[Index]) & CellMask))
      {
        //
        // This helper only supports nexus mappings that ignore the child
        // unit-address, so only the interrupt specifier cells come from
        // InterruptData.
        //
        break;
      }
    }

    if (Index == InputInterruptCells) {
      return ResolveInterruptInternal (
               MapEntry.Parent,
               (CONST UINT32 *)MapEntry.ParentInterrupt,
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
  the requested interrupt specifier is returned directly from the node's
  "interrupts" property. Otherwise, the interrupt specifier is resolved
  through one or more parent nexus "interrupt-map" properties until a final
  interrupt-controller is reached.

  Require an enabled final interrupt-controller. Support up to four address
  and interrupt cells. Maps require interrupt-map-mask with all child address
  bits cleared; address-dependent mappings are unsupported. The
  interrupt-map-pass-thru property is ignored. Returned data belongs to the
  copied tree, remains big-endian and may be unaligned. No raw DTB or boot
  services are required.

  @param[in]  Node            Node whose interrupt is requested.
  @param[in]  Index           Zero-based interrupt specifier index.
  @param[out] Interrupt       Borrowed final specifier, NULL on failure.
  @param[out] InterruptCells  Number of cells in Interrupt, zero on failure.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
EFI_STATUS
EFIAPI
UfdtResolveInterrupt (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN UINT32                  Index,
  OUT CONST UINT32           **Interrupt,
  OUT INT32                  *InterruptCells
  )
{
  CONST UINT8       *Data;
  UFDT_DEVICE_NODE  *Provider;
  UINT32            Cells;
  INT32             Size;
  EFI_STATUS        Status;

  if (Interrupt != NULL) {
    *Interrupt = NULL;
  }

  if (InterruptCells != NULL) {
    *InterruptCells = 0;
  }

  if ((Node == NULL) || (Interrupt == NULL) || (InterruptCells == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = UfdtGetIntDomainNode (Node, &Provider);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetInterruptCellsInfo (Provider, &Cells);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Data = UfdtNodeGetProperty (Node, "interrupts", &Size);
  if ((Size < 0) || ((UINT32)Size % (Cells * sizeof (UINT32)) != 0)) {
    return EFI_COMPROMISED_DATA;
  }

  if ((Data == NULL) || (Index >= (UINT32)Size / (Cells * sizeof (UINT32)))) {
    return EFI_NOT_FOUND;
  }

  Data += Index * Cells * sizeof (UINT32);

  return ResolveInterruptInternal (
           Provider,
           (CONST UINT32 *)Data,
           Cells,
           Interrupt,
           InterruptCells
           );
}

/** Read a reg tuple in the parent bus address space without translation.

  Support one or two address cells and zero, one or two size cells. Return a
  size of zero when the parent declares #size-cells = 0.

  @param[in]  Node   Parsed device-tree node.
  @param[in]  Index  Zero-based reg tuple index.
  @param[out] Base   Parent bus address, valid on success.
  @param[out] Size   Optional region length in bytes, valid on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
STATIC
EFI_STATUS
UfdtGetReg (
  IN  CONST UFDT_DEVICE_NODE  *Node,
  IN  UINT32                  Index,
  OUT UINT64                  *Base,
  OUT UINT64                  *Size OPTIONAL
  )
{
  CONST UFDT_DEVICE_NODE  *Parent;
  INT32                   AddressCells;
  INT32                   SizeCells;
  INT32                   Length;
  UINT32                  Stride;
  CONST UINT8             *Reg;

  if ((Node == NULL) || (Base == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Parent = Node->Parent;
  if (Parent == NULL) {
    return EFI_COMPROMISED_DATA;
  }

  AddressCells = UfdtNodeAddressCells (Parent);
  SizeCells    = UfdtNodeSizeCells (Parent);
  if ((AddressCells < 1) || (AddressCells > 2) || (SizeCells < 0) || (SizeCells > 2)) {
    return EFI_COMPROMISED_DATA;
  }

  Reg = UfdtNodeGetProperty (Node, "reg", &Length);
  if (Reg == NULL) {
    return (Length == 0) ? EFI_NOT_FOUND : EFI_COMPROMISED_DATA;
  }

  Stride = sizeof (UINT32) * (AddressCells + SizeCells);
  if ((Length <= 0) || (((UINT32)Length % Stride) != 0)) {
    return EFI_COMPROMISED_DATA;
  }

  if (Index >= (UINT32)Length / Stride) {
    return EFI_NOT_FOUND;
  }

  Reg  += Index * Stride;
  *Base = ReadCells (Reg, AddressCells);
  if (Size != NULL) {
    *Size = (SizeCells == 0) ? 0 : ReadCells (Reg + sizeof (UINT32) * AddressCells, SizeCells);
  }

  return EFI_SUCCESS;
}

/** Check whether a nonempty region fits the specified address width.

  @param[in] Base          Start address of the region.
  @param[in] Length        Region length in bytes.
  @param[in] AddressCells  Address cell count, validated as one or two by the caller.

  @retval TRUE   The complete region fits without overflowing the address width.
  @retval FALSE  The region is empty or exceeds the address width.
**/
STATIC
BOOLEAN
EFIAPI
IsRangeValid (
  IN UINT64  Base,
  IN UINT64  Length,
  IN INT32   AddressCells
  )
{
  UINT64  Limit;

  Limit = (AddressCells == 1) ? MAX_UINT32 : MAX_UINT64;
  return (Length != 0) && (Base <= Limit) && (Length - 1 <= Limit - Base);
}

/** Translate an address through a bus node "ranges" property.

  A "ranges" property is encoded as:
  <
    child-bus-address
    parent-bus-address
    length
  >

  The whole [*Address, *Address + Length) range must fit in a single "ranges"
  entry. Missing or empty ranges leave the address unchanged. Reject multiple
  entries covering the complete region.

  @param[in]     Bus      Bus node with a parent.
  @param[in]     Length   Nonzero region length in bytes.
  @param[in,out] Address  Bus address, translated to its parent on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
STATIC
EFI_STATUS
TranslateAddressByRanges (
  IN CONST UFDT_DEVICE_NODE  *Bus,
  IN UINT64                  Length,
  IN OUT UINT64              *Address
  )
{
  INT32        ChildCells;
  INT32        ParentCells;
  INT32        SizeCells;
  INT32        Size;
  INT32        Stride;
  INT32        Offset;
  CONST UINT8  *Ranges;
  UINT64       Child;
  UINT64       Host;
  UINT64       Span;
  UINT64       Translated;
  BOOLEAN      Found;

  Ranges = UfdtNodeGetProperty (Bus, "ranges", &Size);
  if (Ranges == NULL) {
    //
    // No "ranges" property: continue if the property is absent, or report
    // malformed property data.
    //
    return (Size == 0) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
  }

  ChildCells  = UfdtNodeAddressCells (Bus);
  ParentCells = UfdtNodeAddressCells (Bus->Parent);
  SizeCells   = UfdtNodeSizeCells (Bus);
  if ((ChildCells < 1) || (ChildCells > 2) ||
      (ParentCells < 1) || (ParentCells > 2) || (SizeCells < 1) || (SizeCells > 2))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Size == 0) {
    return IsRangeValid (*Address, Length, ParentCells) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
  }

  Stride = sizeof (UINT32) * (ChildCells + ParentCells + SizeCells);
  if ((Size < 0) || ((Size % Stride) != 0)) {
    return EFI_COMPROMISED_DATA;
  }

  Found      = FALSE;
  Translated = 0;
  for (Offset = 0; Offset < Size; Offset += Stride) {
    //
    // One "ranges" entry is encoded as:
    // <child-bus-address parent-bus-address length>.
    //
    Child = ReadCells (Ranges + Offset, ChildCells);
    Host  = ReadCells (Ranges + Offset + sizeof (UINT32) * ChildCells, ParentCells);
    //
    // The range length field starts after the child and parent bus
    // addresses in the current "ranges" entry.
    //
    Span = ReadCells (Ranges + Offset + sizeof (UINT32) * (ChildCells + ParentCells), SizeCells);
    if (!IsRangeValid (Child, Span, ChildCells) ||
        !IsRangeValid (Host, Span, ParentCells))
    {
      return EFI_COMPROMISED_DATA;
    }

    //
    // If [*Address, *Address + Length) fits in the range, a mapping has been
    // found. Translate the base address and continue checking for ambiguity.
    // Otherwise try the next range entry.
    //
    if ((*Address >= Child) && (*Address - Child < Span) && (Length <= Span - (*Address - Child))) {
      // Reject multiple mappings covering the complete region.
      if (Found) {
        return EFI_COMPROMISED_DATA;
      }

      Translated = Host + (*Address - Child);
      Found      = TRUE;
    }
  }

  if (!Found) {
    return EFI_NOT_FOUND;
  }

  *Address = Translated;
  return EFI_SUCCESS;
}

/** Translate a region from the node's parent address space to CPU space.

  Resolve parent bus "ranges" mappings up to the root bus to return a CPU
  physical address. The complete region must fit in a single "ranges" entry
  at each bus. Missing or empty ranges leave the address unchanged.

  @param[in]  Node        Parsed device-tree node.
  @param[in]  Address     Region base in that parent address space.
  @param[in]  Length      Nonzero byte length of the complete region.
  @param[out] CpuAddress  Translated base, valid on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
EFI_STATUS
EFIAPI
UfdtTranslateAddress (
  IN  CONST UFDT_DEVICE_NODE  *Node,
  IN  UINT64                  Address,
  IN  UINT64                  Length,
  OUT UINT64                  *CpuAddress
  )
{
  CONST UFDT_DEVICE_NODE  *Bus;
  EFI_STATUS              Status;

  if ((Node == NULL) || (CpuAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsRangeValid (Address, Length, 2)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Bus = Node->Parent; (Bus != NULL) && (Bus->Parent != NULL); Bus = Bus->Parent) {
    Status = TranslateAddressByRanges (Bus, Length, &Address);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if ((Bus == NULL) || !IsRangeValid (Address, Length, sizeof (UINTN) / sizeof (UINT32))) {
    return EFI_COMPROMISED_DATA;
  }

  *CpuAddress = Address;
  return EFI_SUCCESS;
}

/** Get a translated address/size pair from a node "reg" property.

  The "reg" property stores addresses in the parent bus address space.
  This helper reads the requested entry and resolves parent bus "ranges"
  mappings up to the root bus to return a CPU physical address.

  The helper supports one or two address and size cells. Require one matching
  entry per nonempty ranges property for the complete region. Missing or
  empty ranges leave the address unchanged.

  @param[in]  Node   Parsed device-tree node.
  @param[in]  Index  Zero-based reg tuple index.
  @param[out] Base   CPU physical base, valid on success.
  @param[out] Size   Optional region length in bytes, valid on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
EFI_STATUS
EFIAPI
UfdtGetTranslatedReg (
  IN  CONST UFDT_DEVICE_NODE  *Node,
  IN  UINT32                  Index,
  OUT UINT64                  *Base,
  OUT UINT64                  *Size OPTIONAL
  )
{
  EFI_STATUS  Status;
  INT32       RangeSize;
  UINT64      Reg;
  UINT64      RegSize;

  if ((Node == NULL) || (Base == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Address-only bus identifiers do not describe MMIO regions.
  if ((Node->Parent != NULL) && (UfdtNodeSizeCells (Node->Parent) == 0)) {
    return EFI_COMPROMISED_DATA;
  }

  Status = UfdtGetReg (Node, Index, &Reg, &RegSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsRangeValid (Reg, RegSize, 2)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Node = Node->Parent; Node->Parent != NULL; Node = Node->Parent) {
    if (UfdtNodeGetProperty (Node, "ranges", &RangeSize) == NULL) {
      if (RangeSize != 0) {
        return EFI_COMPROMISED_DATA;
      }

      //
      // No "ranges" property: continue.
      //
      continue;
    }

    //
    // "ranges" property found. Translate and continue.
    //
    Status = TranslateAddressByRanges (Node, RegSize, &Reg);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!IsRangeValid (Reg, RegSize, sizeof (UINTN) / sizeof (UINT32))) {
    return EFI_COMPROMISED_DATA;
  }

  *Base = Reg;
  if (Size != NULL) {
    *Size = RegSize;
  }

  return EFI_SUCCESS;
}
