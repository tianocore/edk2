/** @file
  Google Test mocks for PeCoffLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockPeCoffLib.h>

MOCK_INTERFACE_DEFINITION (MockPeCoffLib);

MOCK_FUNCTION_DEFINITION (MockPeCoffLib, PeCoffLoaderGetImageInfo, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPeCoffLib, PeCoffLoaderRelocateImage, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPeCoffLib, PeCoffLoaderLoadImage, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPeCoffLib, PeCoffLoaderImageReadFromMemory, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPeCoffLib, PeCoffLoaderRelocateImageForRuntime, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPeCoffLib, PeCoffLoaderUnloadImage, 1, EFIAPI);
