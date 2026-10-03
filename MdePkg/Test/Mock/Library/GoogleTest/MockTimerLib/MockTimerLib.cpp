/** @file
  Google Test mocks for TimerLib

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockTimerLib.h>

MOCK_INTERFACE_DEFINITION (MockTimerLib);
MOCK_FUNCTION_DEFINITION (MockTimerLib, MicroSecondDelay, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockTimerLib, NanoSecondDelay, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockTimerLib, GetPerformanceCounter, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockTimerLib, GetPerformanceCounterProperties, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockTimerLib, GetTimeInNanoSecond, 1, EFIAPI);
