/** @file MockTimerLib.cpp
  Google Test mocks for TimerLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockTimerLib.h>

MOCK_INTERFACE_DEFINITION (MockTimerLib);
MOCK_FUNCTION_DEFINITION (MockTimerLib, MicroSecondDelay, 1, EFIAPI);
