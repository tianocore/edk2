/** @file
  EDKII PCI Host Memory Policy Protocol.

  PCI memory BARs default to a conservative uncacheable (UC) firmware cache
  attribute, because generic PCI bus code cannot know whether an individual
  BAR is ordinary device register space or host-backed memory. This is safe
  but, for a prefetchable BAR that in fact maps host DRAM/VRAM (for example a
  VirtIO-GPU hostmem/blob resource aperture), a firmware-installed UC MTRR is
  actively harmful: once set, it cannot be relaxed by the operating system's
  own PAT-based mapping, because the most restrictive of MTRR and PAT always
  wins.

  This protocol allows a platform to identify, without any device-specific
  knowledge in the generic PCI bus driver, which prefetchable BARs should be
  left without a firmware-assigned cache attribute so the operating system
  can choose the appropriate policy itself. The generic PCI bus driver
  retains its legacy behavior for all BARs unless a platform driver publishes
  this protocol and returns TRUE for a specific BAR.

  Copyright (c) 2026, Collabora Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCI_HOST_MEMORY_POLICY_H_
#define PCI_HOST_MEMORY_POLICY_H_

#include <Protocol/PciIo.h>

#define EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL_GUID \
  { \
    0x8e3c4a2f, 0x6b1d, 0x4f7a, { 0x9e, 0x2b, 0x1a, 0x5c, 0x7d, 0x3e, 0x8f, 0x60 } \
  }

typedef struct _EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL;

/**
  Determine whether a prefetchable PCI memory BAR maps host-backed memory
  (DRAM/VRAM) that should be left without a firmware-assigned cache
  attribute, instead of receiving the default UC policy.

  This is only ever called by the generic PCI bus driver for BARs that are
  already known to be prefetchable (PciBarTypePMem32 or PciBarTypePMem64).

  @param[in]  This       Instance of this protocol.
  @param[in]  PciIo      The EFI_PCI_IO_PROTOCOL instance for the PCI device
                         that owns the BAR.
  @param[in]  BarIndex   Index of the BAR being programmed.

  @retval TRUE   The BAR should be left without a firmware cache attribute.
  @retval FALSE  The BAR should receive the default UC policy.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_PCI_HOST_MEMORY_POLICY_IS_HOST_BACKED)(
  IN EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL  *This,
  IN EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN UINT8                                  BarIndex
  );

struct _EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL {
  EDKII_PCI_HOST_MEMORY_POLICY_IS_HOST_BACKED    IsHostBackedPrefetchableBar;
};

extern EFI_GUID  gEdkiiPciHostMemoryPolicyProtocolGuid;

#endif
