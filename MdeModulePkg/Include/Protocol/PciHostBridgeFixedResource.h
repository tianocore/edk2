/** @file
  EDKII PCI Host Bridge Fixed Resource Protocol.

  This protocol may be installed by a PCI host bridge driver on the same handle
  as the EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL. It permits the PCI
  bus driver to inform the host bridge driver about resources below a root
  bridge that decode a fixed address range, such as BARs described by a PCI
  Enhanced Allocation capability, or PCI-PCI bridge windows that must cover
  such ranges.

  The host bridge driver must ensure that the resources it allocates to the
  root bridges in response to EfiPciHostBridgeAllocateResources do not overlap
  with the submitted fixed ranges, and must report the fixed ranges via the
  Configuration () method of the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

  Copyright (c) 2026, Google LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#define EDKII_PCI_HOST_BRIDGE_FIXED_RESOURCE_PROTOCOL_GUID \
  { 0xbb465aea, 0x5165, 0x4fdf, { 0xb4, 0xb0, 0xfd, 0x3a, 0x1d, 0x4c, 0xc6, 0xd8 } }

typedef struct _EDKII_PCI_HOST_BRIDGE_FIXED_RESOURCE_PROTOCOL EDKII_PCI_HOST_BRIDGE_FIXED_RESOURCE_PROTOCOL;

/**
  Submit the fixed I/O and memory ranges decoded by devices below the
  specified PCI root bridge.

  This function must be called after EfiPciHostBridgeBeginResourceAllocation
  and before EfiPciHostBridgeAllocateResources. A subsequent call for the same
  root bridge replaces the previously submitted set of fixed ranges, as long as
  they have not been claimed yet. Fixed ranges are released by
  EfiPciHostBridgeFreeResources.

  @param[in] This              The EDKII_PCI_HOST_BRIDGE_FIXED_RESOURCE_PROTOCOL
                               instance.
  @param[in] RootBridgeHandle  The PCI root bridge below which the fixed ranges
                               are decoded.
  @param[in] Configuration     A list of ACPI QWORD address space descriptors,
                               terminated by an end tag descriptor. Each memory
                               or I/O descriptor describes a fixed range by its
                               device address in AddrRangeMin, and its size in
                               AddrLen. For memory descriptors, SpecificFlag
                               indicates whether the range is prefetchable.
                               Other fields are ignored.

  @retval EFI_SUCCESS            The fixed ranges were accepted.
  @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a handle of a root
                                 bridge produced by this host bridge.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL, or contains a
                                 descriptor that is malformed or that describes
                                 a range that is not covered by an aperture of
                                 the root bridge.
  @retval EFI_ACCESS_DENIED      Fixed ranges were already claimed for this root
                                 bridge, and have not been released yet.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                 lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PCI_HOST_BRIDGE_SUBMIT_FIXED_RESOURCES)(
  IN EDKII_PCI_HOST_BRIDGE_FIXED_RESOURCE_PROTOCOL  *This,
  IN EFI_HANDLE                                     RootBridgeHandle,
  IN VOID                                           *Configuration
  );

struct _EDKII_PCI_HOST_BRIDGE_FIXED_RESOURCE_PROTOCOL {
  EDKII_PCI_HOST_BRIDGE_SUBMIT_FIXED_RESOURCES    SubmitFixedResources;
};

extern EFI_GUID  gEdkiiPciHostBridgeFixedResourceProtocolGuid;
