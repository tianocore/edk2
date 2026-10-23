/** @file
  Google Test mocks for UefiBootManagerLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.
  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockUefiBootManagerLib.h>

MOCK_INTERFACE_DEFINITION (MockUefiBootManagerLib);

MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerGetLoadOptions, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerFreeLoadOptions, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerInitializeLoadOption, 8, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerFreeLoadOption, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerVariableToLoadOption, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerAddLoadOptionVariable, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerDeleteLoadOptionVariable, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerSortLoadOptionVariable, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerFindLoadOption, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerStartHotkeyService, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerHotkeyBoot, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerRefreshAllBootOption, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerBoot, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerGetBootManagerMenu, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerGetNextLoadOptionDevicePath, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerGetLoadOptionBuffer, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerRegisterLegacyBootSupport, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerRegisterBootDescriptionHandler, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerConnectAll, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerConnectDevicePath, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerDisconnectAll, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerConnectAllDefaultConsoles, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerUpdateConsoleVariable, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerConnectConsoleVariable, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerGetGopDevicePath, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerConnectVideoController, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerGetDriverHealthInfo, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerFreeDriverHealthInfo, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerProcessLoadOption, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerIsValidLoadOptionVariableName, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerDispatchDeferredImages, 0, EFIAPI);
