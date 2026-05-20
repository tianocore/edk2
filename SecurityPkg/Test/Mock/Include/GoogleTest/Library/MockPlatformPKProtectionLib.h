/** @file
  Google Test mocks for PlatformPKProtectionLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PlatformPKProtectionLib.h>
}

struct MockPlatformPKProtectionLib {
  MOCK_INTERFACE_DECLARATION (MockPlatformPKProtectionLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    DisablePKProtection,
    ()
    );
};
