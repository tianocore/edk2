/** @file
  A hook-in library for MdeModulePkg/Bus/Pci/PciHostBridgeDxe.

  Plugging this library instance into PciHostBridgeDxe makes
  PciHostBridgeDxe depend on the platform's dynamic decision whether
  to provide IOMMU implementation (usually through IoMmuDxe driver).

  Copyright (C) 2017, Red Hat, Inc.
  Copyright (C) 2017, AMD, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
