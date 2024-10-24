/** @file MockPciLib.cpp
  Google Test mocks for PciLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockPciLib.h>

MOCK_INTERFACE_DEFINITION (MockPciLib);

MOCK_FUNCTION_DEFINITION (MockPciLib, PciRead8, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciWrite8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciOr8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciAnd8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciRead16, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciWrite16, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciOr16, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciAnd16, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciRead32, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciWrite32, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciOr32, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciLib, PciAnd32, 2, EFIAPI);
