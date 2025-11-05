/** @file
  Google Test mocks for PlatformPKProtectionLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockPlatformPKProtectionLib.h>

MOCK_INTERFACE_DEFINITION (MockPlatformPKProtectionLib);

MOCK_FUNCTION_DEFINITION (MockPlatformPKProtectionLib, DisablePKProtection, 0, EFIAPI);
