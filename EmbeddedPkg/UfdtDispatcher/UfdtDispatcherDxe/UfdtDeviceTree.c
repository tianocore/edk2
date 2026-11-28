/** @file
  Expand a flat device tree into separately allocated nodes and properties.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UfdtDispatcherInternal.h"
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include <Library/MemoryAllocationLib.h>

/** Populate UFDT_DEVICE_NODE's properties.

  Preserve property order and byte order.

  @param[in]     Fdt     Device tree.
  @param[in]     Node    Offset of the node to parse.
  @param[in,out] Device  UFDT_DEVICE_NODE node to populate the properties.

  @retval EFI_SUCCESS           Success.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
**/
STATIC
EFI_STATUS
PopulateNodeProperties (
  IN     CONST VOID        *Fdt,
  IN     INT32             Node,
  IN OUT UFDT_DEVICE_NODE  *Device
  )
{
  CONST FDT_PROPERTY  *Flat;
  CONST CHAR8         *Name;
  UFDT_NODE_PROPERTY  *Property;
  UFDT_NODE_PROPERTY  **Next;
  INT32               Offset;
  INT32               Length;
  INT32               NameLength;
  UINTN               EntrySize;
  BOOLEAN             HasPhandle;

  Next       = &Device->Properties;
  HasPhandle = FALSE;
  for (Offset = FdtFirstPropertyOffset (Fdt, Node); Offset >= 0;
       Offset = FdtNextPropertyOffset (Fdt, Offset))
  {
    Flat = FdtGetPropertyByOffset (Fdt, Offset, &Length);
    if ((Flat == NULL) || (Length < 0)) {
      return EFI_COMPROMISED_DATA;
    }

    Name = FdtGetString (Fdt, Fdt32ToCpu (Flat->NameOffset), &NameLength);
    if ((Name == NULL) || (NameLength <= 0)) {
      return EFI_COMPROMISED_DATA;
    }

    if ((UINTN)Length > MAX_UINTN - sizeof (*Property) - (UINTN)NameLength - 1) {
      return EFI_OUT_OF_RESOURCES;
    }

    EntrySize = sizeof (*Property) + (UINTN)Length + (UINTN)NameLength + 1;
    Property  = AllocatePool (EntrySize);
    if (Property == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Property->Length = (UINT32)Length;
    Property->Value  = Property + 1;
    Property->Name   = (CHAR8 *)(Property + 1) + Length;
    Property->Next   = NULL;
    CopyMem ((VOID *)Property->Value, Flat->Data, Length);
    CopyMem ((VOID *)Property->Name, Name, (UINTN)NameLength + 1);
    *Next = Property;
    Next  = &Property->Next;

    if ((NameLength == 4) && (AsciiStrnCmp (Name, "name", 5) == 0)) {
      Device->Name = Property->Value;
    } else if ((Length == sizeof (UINT32)) && (NameLength == 7) && (AsciiStrnCmp (Name, "phandle", 8) == 0)) {
      Device->Phandle = Fdt32ToCpu (ReadUnaligned32 ((CONST UINT32 *)Property->Value));
      HasPhandle      = TRUE;
    } else if ((Length == sizeof (UINT32)) && !HasPhandle && (NameLength == 13) && (AsciiStrnCmp (Name, "linux,phandle", 14) == 0)) {
      // "phandle" property takes precedence.
      Device->Phandle = Fdt32ToCpu (ReadUnaligned32 ((CONST UINT32 *)Property->Value));
    }
  }

  return (Offset == -FDT_ERR_NOTFOUND) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
}

/** Release an unpublished tree, including a partially parsed tree.

  Remove installed protocols and free device paths before calling this function.

  @param[in] Root  Root of the entire tree, with Parent == NULL, or NULL.
**/
VOID
EFIAPI
UfdtFreeDeviceTree (
  IN UFDT_DEVICE_NODE  *Root
  )
{
  UFDT_DEVICE_NODE    *Device;
  UFDT_DEVICE_NODE    *Next;
  UFDT_NODE_PROPERTY  *Property;

  Device = Root;
  while (Device != NULL) {
    if (Device->Child != NULL) {
      Device = Device->Child;
      continue;
    }

    Next = Device->Sibling;
    if (Device->Parent != NULL) {
      Device->Parent->Child = Next;
      if (Next == NULL) {
        Next = Device->Parent;
      }
    }

    while (Device->Properties != NULL) {
      Property           = Device->Properties;
      Device->Properties = Property->Next;
      FreePool (Property);
    }

    FreePool (Device);
    Device = Next;
  }
}

/** Allocate a node, copy its properties, and recursively parse its children.

  On failure, the caller must free the partial subtree through its owning tree.

  @param[in]  Fdt         Device tree.
  @param[in]  Offset      Node offset.
  @param[in]  Parent      UFDT_DEVICE_NODE struct of the parent,
                          or NULL when parsing the root.
  @param[out] Node        On success, populated UFDT_DEVICE_NODE struct.
  @param[out] LastOffset  On success, last parsed node offset.

  @retval EFI_SUCCESS           Success.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
**/
STATIC
EFI_STATUS
ParseDeviceNode (
  IN  VOID              *Fdt,
  IN  INT32             Offset,
  IN  UFDT_DEVICE_NODE  *Parent OPTIONAL,
  OUT UFDT_DEVICE_NODE  **Node,
  OUT INT32             *LastOffset
  )
{
  EFI_STATUS        Status;
  INT32             Length;
  INT32             ChildOffset;
  UINTN             ParentLength;
  UINTN             PathSize;
  UINTN             NameSize;
  UFDT_DEVICE_NODE  *Device;
  UFDT_DEVICE_NODE  **NextChild;
  CONST CHAR8       *Name;
  CHAR8             *Path;

  *Node = NULL;
  Name  = FdtGetName (Fdt, Offset, &Length);
  if ((Name == NULL) || (Length < 0) || ((Offset != 0) && (Length == 0))) {
    return EFI_COMPROMISED_DATA;
  }

  ParentLength = ((Parent == NULL) || (Parent->Parent == NULL)) ? 0 : AsciiStrLen (Parent->FullName);
  PathSize     = (Parent == NULL) ? 2 : ParentLength + (UINTN)Length + 2;
  if (PathSize > MAX_INT32) {
    return EFI_COMPROMISED_DATA;
  }

  // Don't keep the device's address.
  for (NameSize = 0; (NameSize < (UINTN)Length) && (Name[NameSize] != '@'); NameSize++) {
  }

  NameSize++;
  if (NameSize > MAX_UINTN - sizeof (*Device) - PathSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  Device = AllocateZeroPool (sizeof (*Device) + PathSize + NameSize);
  if (Device == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Device->Parent   = Parent;
  Device->FullName = (CHAR8 *)(Device + 1);
  Device->Path     = Device->FullName;
  Device->Name     = Device->FullName + PathSize;
  Path             = (CHAR8 *)Device->FullName;
  if (ParentLength != 0) {
    CopyMem (Path, Parent->FullName, ParentLength);
  }

  Path[ParentLength] = '/';
  if (Parent != NULL) {
    CopyMem (Path + ParentLength + 1, Name, (UINTN)Length);
  }

  CopyMem ((VOID *)Device->Name, Name, NameSize - 1);
  *Node  = Device;
  Status = PopulateNodeProperties (Fdt, Offset, Device);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *LastOffset = Offset;
  NextChild   = &Device->Child;
  for (ChildOffset = FdtFirstSubnode (Fdt, Offset); ChildOffset >= 0;
       ChildOffset = FdtNextSubnode (Fdt, ChildOffset))
  {
    Status = ParseDeviceNode (Fdt, ChildOffset, Device, NextChild, LastOffset);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    NextChild = &(*NextChild)->Sibling;
  }

  return (ChildOffset == -FDT_ERR_NOTFOUND) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
}

/** Unflatten a DTB into UFDT_DEVICE_NODE structures.

  Copy node names, paths and properties. Release the result with
  UfdtFreeDeviceTree(). On failure, free partial allocations and set *Root to NULL.

  @param[in]  Fdt   Device tree.
  @param[out] Root  Root UFDT_DEVICE_NODE.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
  @retval EFI_OUT_OF_RESOURCES   Out of memory.
**/
EFI_STATUS
EFIAPI
UfdtParseDeviceTree (
  IN  VOID              *Fdt,
  OUT UFDT_DEVICE_NODE  **Root
  )
{
  EFI_STATUS  Status;
  INT32       LastOffset;

  if (Root == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Root = NULL;
  if ((Fdt == NULL) || (FdtCheckHeader (Fdt) != 0) || (FdtTotalSize (Fdt) > MAX_INT32)) {
    return EFI_COMPROMISED_DATA;
  }

  Status = ParseDeviceNode (Fdt, 0, NULL, Root, &LastOffset);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  // Reject unexpected nodes after the final parsed node. This traversal
  // check does not provide full structural validation of the blob.
  if (FdtNextNode (Fdt, LastOffset, NULL) != -FDT_ERR_NOTFOUND) {
    Status = EFI_COMPROMISED_DATA;
    goto Failed;
  }

  return Status;

Failed:
  UfdtFreeDeviceTree (*Root);
  *Root = NULL;
  return Status;
}
