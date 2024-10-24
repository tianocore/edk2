/** @file MockPlatformHookLib.h
  Google Test mocks for PlatformHookLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PLATFORM_HOOK_LIB_
#define MOCK_PLATFORM_HOOK_LIB_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PlatformHookLib.h>
}

struct MockPlatformHookLib {
  MOCK_INTERFACE_DECLARATION (MockPlatformHookLib);

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    PlatformHookSerialPortInitialize,
    (

    )
    );
};

#endif
