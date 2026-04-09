/** @file MockHiiLib.cpp
  Google Test mocks for HiiLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockHiiLib.h>

//
// Global Variables that are not const
//

MOCK_INTERFACE_DEFINITION (MockHiiLib);

MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiRemovePackages, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiSetString, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiGetString, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiGetStringEx, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiGetPackageString, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiGetHiiHandles, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiGetFormSetFromHiiHandle, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiGetSupportedLanguages, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiConstructConfigHdr, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiSetToDefaults, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiValidateSettings, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiIsConfigHdrMatch, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiGetBrowserData, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiSetBrowserData, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiAllocateOpCodeHandle, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiFreeOpCodeHandle, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateRawOpCodes, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateEndOpCode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateOneOfOptionOpCode, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateDefaultOpCode, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateGuidOpCode, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateActionOpCode, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateSubTitleOpCode, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateGotoOpCode, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateGotoExOpCode, 9, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateCheckBoxOpCode, 9, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateNumericOpCode, 12, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateStringOpCode, 11, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateOneOfOpCode, 10, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateOrderedListOpCode, 12, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateTextOpCode, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateDateOpCode, 9, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiCreateTimeOpCode, 9, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiLib, HiiUpdateForm, 5, EFIAPI);
