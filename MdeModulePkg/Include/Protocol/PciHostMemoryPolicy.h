/** @file
  EDKII PCI Host Memory Policy Protocol.

  PCI memory BARs default to a conservative uncacheable (UC) firmware cache
  attribute. A platform may know that this policy is unnecessary for a
  particular class of PCI memory ranges and allow the operating system to
  choose the cache policy itself.

  This protocol allows a platform to identify, without any device-specific
  knowledge in the generic PCI bus driver, which PCI memory BARs should be
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
  Determine whether a PCI memory BAR should be left without a
  firmware-assigned cache attribute, instead of receiving the default UC
  policy.

  @param[in]  This       Instance of this protocol.
  @param[in]  PciIo      The EFI_PCI_IO_PROTOCOL instance for the PCI device
                         that owns the BAR.
  @param[in]  BarIndex   Index of the BAR being programmed.

  @retval TRUE   The BAR should be left without a firmware cache attribute.
  @retval FALSE  The BAR should receive the default UC policy.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_PCI_HOST_MEMORY_POLICY_SHOULD_SKIP_UC)(
  IN EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL  *This,
  IN EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN UINT8                                  BarIndex
  );

struct _EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL {
  EDKII_PCI_HOST_MEMORY_POLICY_SHOULD_SKIP_UC    ShouldSkipDefaultUcPolicy;
};

extern EFI_GUID  gEdkiiPciHostMemoryPolicyProtocolGuid;

#endif
