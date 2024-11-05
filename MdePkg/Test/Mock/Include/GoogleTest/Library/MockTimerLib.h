/** @file MockTimerLib.h
  Google Test mocks for TimerLib

  Copyright (c) Microsoft Corporation.
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_TIMER_LIB_H_
#define MOCK_TIMER_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Library/TimerLib.h>
}

struct MockTimerLib {
  MOCK_INTERFACE_DECLARATION (MockTimerLib);

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    MicroSecondDelay,
    (IN UINTN  MicroSeconds)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    NanoSecondDelay,
    (IN UINTN  NanoSeconds)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    GetPerformanceCounter,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    GetPerformanceCounterProperties,
    (OUT UINT64  *StartValue OPTIONAL,
     OUT UINT64  *EndValue OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    GetTimeInNanoSecond,
    (IN UINT64  Ticks)
    );
};

#endif
