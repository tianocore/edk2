/** @file
  Google Test mocks for UefiLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockUefiLib.h>

MOCK_INTERFACE_DEFINITION (MockUefiLib);

MOCK_FUNCTION_DEFINITION (MockUefiLib, GetVariable2, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiLib, GetEfiGlobalVariable2, 3, EFIAPI);
