/** @file MockPciExpressLib.cpp
  Google Test mocks for PciExpressLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockPciExpressLib.h>

MOCK_INTERFACE_DEFINITION (MockPciExpressLib);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressRead8, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressRead16, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressRead32, 1, EFIAPI);
