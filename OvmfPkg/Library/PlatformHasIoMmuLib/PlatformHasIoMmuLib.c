/** @file
  A hook-in library for MdeModulePkg/Bus/Pci/PciHostBridgeDxe.

  Plugging this library instance into PciHostBridgeDxe makes
  PciHostBridgeDxe depend on the platform's dynamic decision whether
  to provide IOMMU implementation (usually through IoMmuDxe driver).

  Copyright (C) 2017, Red Hat, Inc.
  Copyright (C) 2017, AMD, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

RETURN_STATUS
EFIAPI
PlatformHasIoMmuInitialize (
  VOID
  )
{
  //
  // Do nothing, just imbue PciHostBridgeDxe with a protocol dependency on
  // gIoMmuAbsentProtocolGuid OR gEdkiiIoMmuProtocolGuid.
  //
  return RETURN_SUCCESS;
}
