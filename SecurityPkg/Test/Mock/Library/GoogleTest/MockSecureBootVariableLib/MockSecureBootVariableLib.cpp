/** @file
  Google Test mocks for SecureBootVariableLib

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockSecureBootVariableLib.h>

MOCK_INTERFACE_DEFINITION (MockSecureBootVariableLib);

MOCK_FUNCTION_DEFINITION (MockSecureBootVariableLib, IsSecureBootEnabled, 0, EFIAPI);
