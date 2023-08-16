/** @file
  Mock instance of the PCI Host Bridge Library.

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockPciHostBridgeLib.h>

MOCK_INTERFACE_DEFINITION (MockPciHostBridgeLib);

MOCK_FUNCTION_DEFINITION (MockPciHostBridgeLib, PciHostBridgeGetRootBridges, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciHostBridgeLib, PciHostBridgeFreeRootBridges, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciHostBridgeLib, PciHostBridgeResourceConflict, 2, EFIAPI);
