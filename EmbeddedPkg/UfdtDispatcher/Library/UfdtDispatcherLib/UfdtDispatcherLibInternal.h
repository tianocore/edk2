/** @file
  Internal helpers shared by UfdtDispatcherLib implementations.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/UfdtDispatcherLib.h>

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
  );

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
  );
