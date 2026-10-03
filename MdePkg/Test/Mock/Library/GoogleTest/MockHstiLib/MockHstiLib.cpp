/** @file
  Google Test mocks for HstiLib

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockHstiLib.h>

MOCK_INTERFACE_DEFINITION (MockHstiLib);
MOCK_FUNCTION_DEFINITION (MockHstiLib, HstiLibSetTable, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHstiLib, HstiLibGetTable, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHstiLib, HstiLibSetFeaturesVerified, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHstiLib, HstiLibClearFeaturesVerified, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHstiLib, HstiLibAppendErrorString, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHstiLib, HstiLibSetErrorString, 3, EFIAPI);
