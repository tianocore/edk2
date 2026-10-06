/** @file
  Google Test mocks for RngLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockRngLib.h>

MOCK_INTERFACE_DEFINITION (MockRngLib);

MOCK_FUNCTION_DEFINITION (MockRngLib, GetRandomNumber16, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockRngLib, GetRandomNumber32, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockRngLib, GetRandomNumber64, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockRngLib, GetRandomNumber128, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockRngLib, GetRngGuid, 1, EFIAPI);
