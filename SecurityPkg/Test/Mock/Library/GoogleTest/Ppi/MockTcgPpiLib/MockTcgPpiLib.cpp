/** @file MockTcgPpiLib.cpp
  Google Test mocks for TcgPpi

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Ppi/MockTcgPpiLib.h>

MOCK_INTERFACE_DEFINITION (MockTcgPpiLib);

MOCK_FUNCTION_DEFINITION (MockTcgPpiLib, HashLogExtendEvent, 6, EFIAPI);

EDKII_TCG_PPI  gMockTcgPpi = {
  HashLogExtendEvent
};
