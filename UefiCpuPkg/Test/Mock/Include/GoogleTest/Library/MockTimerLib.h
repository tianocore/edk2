/** @file MockTimerLib.h
  Google Test mocks for TimerLib

  Copyright (c) Microsoft Corporation.
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
    (
     IN UINTN  MicroSeconds
    )
    );
};

#endif //MOCK_TIMER_LIB_H_
