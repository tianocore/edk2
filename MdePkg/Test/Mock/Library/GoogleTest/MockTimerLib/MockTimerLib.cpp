/** @file MockTimerLib.cpp
  Google Test mocks for TimerLib

  Copyright (c) Microsoft Corporation.
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockTimerLib.h>

MOCK_INTERFACE_DEFINITION (MockTimerLib);

MOCK_FUNCTION_DEFINITION (MockTimerLib, MicroSecondDelay, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockTimerLib, NanoSecondDelay, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockTimerLib, GetPerformanceCounter, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockTimerLib, GetPerformanceCounterProperties, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockTimerLib, GetTimeInNanoSecond, 1, EFIAPI);
