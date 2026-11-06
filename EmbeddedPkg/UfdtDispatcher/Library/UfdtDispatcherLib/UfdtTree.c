/** @file
  Navigate the parsed device tree.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include "UfdtDispatcherLibInternal.h"

/** Return the next node in depth-first order.

  Return the first valid node following this priority list:
  If Device == NULL:
  - The first child node of Root.
  If Device != NULL:
  - The first child node of Device
  - If no child, the sibling node of Device
  - If no sibling, the sibling node of the parent node of Device
  - If none, NULL

  Ascend through parents as needed, stopping at Root.

  @param[in] Root    Root of the subtree to traverse. Must not be NULL.
  @param[in] Device  Current node in Root's subtree, or NULL to start at Root's
                     first child.

  @retval NULL   Finished iterating over the nodes.
  @retval Other  The next node.
**/
UFDT_DEVICE_NODE *
EFIAPI
UfdtGetNextDeviceNode (
  IN CONST UFDT_DEVICE_NODE  *Root,
  IN UFDT_DEVICE_NODE        *Device OPTIONAL
  )
{
  if (Root == NULL) {
    ASSERT (FALSE);
    return NULL;
  }

  if (Device == NULL) {
    return Root->Child;
  }

  if (Device->Child != NULL) {
    return Device->Child;
  }

  while ((Device != NULL) && (Device != Root)) {
    if (Device->Sibling != NULL) {
      return Device->Sibling;
    }

    Device = Device->Parent;
  }

  return NULL;
}

/** Return the previous node in depth-first order.

  Return the first valid node in an inverse fashion from UfdtGetNextDeviceNode().
  Traversal is limited to Root's descendants and does not return Root.

  @param[in] Root    Root of the subtree to traverse. Must not be NULL.
  @param[in] Device  Current node in Root's subtree, or NULL to start at Root's
                     last descendant.

  @retval NULL   Finished iterating over the nodes.
  @retval Other  The previous node.
**/
UFDT_DEVICE_NODE *
EFIAPI
UfdtGetPrevDeviceNode (
  IN CONST UFDT_DEVICE_NODE  *Root,
  IN UFDT_DEVICE_NODE        *Device OPTIONAL
  )
{
  UFDT_DEVICE_NODE  *Previous;

  if (Root == NULL) {
    ASSERT (FALSE);
    return NULL;
  }

  if (Device == NULL) {
    Previous = Root->Child;
    if (Previous == NULL) {
      return NULL;
    }

    while (Previous->Sibling != NULL) {
      Previous = Previous->Sibling;
    }
  } else {
    if ((Device == Root) || (Device == Root->Child)) {
      return NULL;
    }

    Previous = Device->Parent->Child;
    if (Previous == Device) {
      return Device->Parent;
    }

    while (Previous->Sibling != Device) {
      Previous = Previous->Sibling;
    }
  }

  while (Previous->Child != NULL) {
    Previous = Previous->Child;
    while (Previous->Sibling != NULL) {
      Previous = Previous->Sibling;
    }
  }

  return Previous;
}

/** Return the root of the tree.

  @param[in] Node  Any node in the tree, or NULL.

  @retval NULL   Node is NULL.
  @retval Other  The root node.
**/
UFDT_DEVICE_NODE *
EFIAPI
UfdtGetRootDeviceNode (
  IN CONST UFDT_DEVICE_NODE  *Node
  )
{
  if (Node == NULL) {
    return NULL;
  }

  while (Node->Parent != NULL) {
    Node = Node->Parent;
  }

  return (UFDT_DEVICE_NODE *)Node;
}

/** Find a node by phandle.

  @param[in] Root     Root of the subtree to search, including Root itself, or NULL.
  @param[in] Phandle  Phandle to search; zero and MAX_UINT32 are invalid.

  @retval NULL   Invalid parameter or not found.
  @retval Other  The first node whose Phandle equals Phandle.
**/
UFDT_DEVICE_NODE *
EFIAPI
UfdtFindNodeByPhandle (
  IN CONST UFDT_DEVICE_NODE  *Root,
  IN UINT32                  Phandle
  )
{
  UFDT_DEVICE_NODE  *Current;

  if ((Phandle == 0) || (Phandle == MAX_UINT32)) {
    return NULL;
  }

  for (Current = (UFDT_DEVICE_NODE *)Root; Current != NULL;
       Current = UfdtGetNextDeviceNode (Root, Current))
  {
    if (Current->Phandle == Phandle) {
      return Current;
    }
  }

  return NULL;
}

/** Get the interrupt parent of a node.

  The interrupt parent is:
  - if the "interrupt-parent" property is present, the node
    pointed to by the property. Note that the node pointed to by
    the interrupt parent is not necessarily an interrupt
    controller.
  - otherwise, the parent following the normal hierarchy.

  @param[in]  Node      Node whose interrupt parent is requested.
  @param[out] IntcNode  Borrowed parent node on success, NULL on failure.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          No interrupt parent found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
STATIC
EFI_STATUS
GetIntcParentNode (
  IN CONST UFDT_DEVICE_NODE  *Node,
  OUT UFDT_DEVICE_NODE       **IntcNode
  )
{
  CONST VOID  *Property;
  INT32       Size;

  if (IntcNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *IntcNode = NULL;
  if (Node == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check whether the node has the "interrupt-parent" property.
  Property = UfdtNodeGetProperty (Node, "interrupt-parent", &Size);
  if (Property != NULL) {
    if (Size != sizeof (UINT32)) {
      return EFI_COMPROMISED_DATA;
    }

    *IntcNode = UfdtFindNodeByPhandle (UfdtGetRootDeviceNode (Node), SwapBytes32 (ReadUnaligned32 (Property)));
    return (*IntcNode != NULL) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
  } else if (Size != 0) {
    return EFI_COMPROMISED_DATA;
  }

  if (Node->Parent == NULL) {
    // Reached the root of the tree.
    return EFI_NOT_FOUND;
  }

  // Get the parent of the node.
  *IntcNode = Node->Parent;
  return EFI_SUCCESS;
}

/** Check whether a node defines an interrupt domain.

  An interrupt domain is defined by the presence of one of these properties:
  - interrupt-controller
  - interrupt-map

  @param[in] Node               Node to check.
  @param[in] IntControllerOnly  If TRUE, only accept an interrupt controller.
                               Otherwise, also accept an interrupt nexus.

  @retval TRUE   The node defines a matching interrupt domain.
  @retval FALSE  The node does not define a matching interrupt domain.
**/
STATIC
BOOLEAN
IsIntcDomainNode (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN BOOLEAN                 IntControllerOnly
  )
{
  if (UfdtNodeGetProperty (Node, "interrupt-controller", NULL) != NULL) {
    return TRUE;
  }

  if (IntControllerOnly) {
    return FALSE;
  }

  return UfdtNodeGetProperty (Node, "interrupt-map", NULL) != NULL;
}

/** Get the interrupt controller or domain handling the input node's interrupts.

  Follow interrupt-parent references, or structural parents when absent, until
  a matching domain is reached. If the ancestor search reaches the root without
  finding a match, return the input node if it defines a matching domain.
  This does not check provider availability or resolve interrupt-map entries.

  Devicetree Specification, Release v0.3,
  2.4.1 "Properties for Interrupt Generating Devices":
    Because the hierarchy of the nodes in the interrupt tree
    might not match the devicetree, the interrupt-parent
    property is available to make the definition of an
    interrupt parent explicit. The value is the phandle to the
    interrupt parent. If this property is missing from a
    device, its interrupt parent is assumed to be its devicetree
    parent.

  @param[in]  Node               Node whose interrupt domain is requested.
  @param[in]  IntControllerOnly  If TRUE, only return an interrupt controller.
                                Otherwise, also accept an interrupt nexus.
  @param[out] IntcNode           Borrowed matching node on success, NULL on failure.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          No matching interrupt domain found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
EFI_STATUS
EFIAPI
GetIntcNode (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN BOOLEAN                 IntControllerOnly,
  OUT UFDT_DEVICE_NODE       **IntcNode
  )
{
  EFI_STATUS              Status;
  CONST UFDT_DEVICE_NODE  *InputNode;
  UFDT_DEVICE_NODE        *Parent;

  if (IntcNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *IntcNode = NULL;
  if (Node == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InputNode = Node;

  // Get the interrupt parent (which is not necessarily an interrupt-controller).
  Status = GetIntcParentNode (Node, &Parent);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (TRUE) {
    // Check whether the node defines the requested interrupt domain.
    if (IsIntcDomainNode (Parent, IntControllerOnly)) {
      *IntcNode = Parent;
      return EFI_SUCCESS;
    }

    Status = GetIntcParentNode (Parent, &Parent);
    if (Status == EFI_NOT_FOUND) {
      //
      // Reached the root of the tree without finding an interrupt parent.
      // If the input node defines a matching interrupt domain, return it.
      //
      if (IsIntcDomainNode (InputNode, IntControllerOnly)) {
        *IntcNode = (UFDT_DEVICE_NODE *)InputNode;
        return EFI_SUCCESS;
      }

      return EFI_NOT_FOUND;
    } else if (EFI_ERROR (Status)) {
      return Status;
    }
  }
}
