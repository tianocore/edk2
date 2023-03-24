/** @file
  Google Test mocks for PlatformPKProtectionLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PLATFORM_PK_PROTECTION_LIB_H_
#define MOCK_PLATFORM_PK_PROTECTION_LIB_H_

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

#endif
