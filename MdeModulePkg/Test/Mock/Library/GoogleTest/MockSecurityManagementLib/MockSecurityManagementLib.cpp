/** @file
  Google Test mocks for SecurityManagementLib

  Copyright (c) 2025, Yandex. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockSecurityManagementLib.h>

MOCK_INTERFACE_DEFINITION (MockSecurityManagementLib);

MOCK_FUNCTION_DEFINITION (MockSecurityManagementLib, RegisterSecurityHandler, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSecurityManagementLib, ExecuteSecurityHandlers, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSecurityManagementLib, RegisterSecurity2Handler, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSecurityManagementLib, ExecuteSecurity2Handlers, 6, EFIAPI);
