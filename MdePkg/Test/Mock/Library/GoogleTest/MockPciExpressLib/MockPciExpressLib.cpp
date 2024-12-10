/** @file MockPciExpressLib.cpp
  Google Test mocks for PciExpressLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockPciExpressLib.h>

MOCK_INTERFACE_DEFINITION (MockPciExpressLib);

MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressRegisterForRuntimeAccess, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressRead8, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressWrite8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressOr8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressAnd8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressAndThenOr8, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldRead8, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldWrite8, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldOr8, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldAnd8, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldAndThenOr8, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressRead16, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressWrite16, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressOr16, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressAnd16, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressAndThenOr16, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldRead16, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldWrite16, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldOr16, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldAnd16, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldAndThenOr16, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressRead32, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressWrite32, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressOr32, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressAnd32, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressAndThenOr32, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldRead32, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldWrite32, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldOr32, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldAnd32, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressBitFieldAndThenOr32, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressReadBuffer, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciExpressLib, PciExpressWriteBuffer, 3, EFIAPI);
