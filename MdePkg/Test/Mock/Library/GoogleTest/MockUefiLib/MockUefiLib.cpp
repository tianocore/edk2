/** @file
  Google Test mocks for UefiLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockUefiLib.h>

MOCK_INTERFACE_DEFINITION (MockUefiLib);

MOCK_FUNCTION_DEFINITION (MockUefiLib, GetVariable2, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiLib, GetEfiGlobalVariable2, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiLib, EfiCreateProtocolNotifyEvent, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiLib, EfiTestManagedDevice, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiLib, LookupUnicodeString2, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiLib, AddUnicodeString2, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiLib, FreeUnicodeStringTable, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiLib, EfiLibInstallDriverBindingComponentName2, 6, EFIAPI);
