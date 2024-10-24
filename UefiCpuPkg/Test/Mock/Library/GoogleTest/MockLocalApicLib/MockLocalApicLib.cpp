/** @file MockLocalApicLib.cpp
  Google Test mocks for LocalApicLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockLocalApicLib.h>

MOCK_INTERFACE_DEFINITION (MockLocalApicLib);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetProcessorLocationByApicId, 4, EFIAPI);
