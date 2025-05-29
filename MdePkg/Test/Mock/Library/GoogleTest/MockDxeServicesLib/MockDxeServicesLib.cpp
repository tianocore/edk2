/** @file MockDxeServicesLib.cpp
  Google Test mocks for DxeServicesLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockDxeServicesLib.h>

MOCK_INTERFACE_DEFINITION (MockDxeServicesLib);
MOCK_FUNCTION_DEFINITION (MockDxeServicesLib, GetSectionFromAnyFvByFileType, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesLib, GetSectionFromAnyFv, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesLib, GetSectionFromFv, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesLib, GetSectionFromFfs, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesLib, GetFileBufferByFilePath, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesLib, GetFileDevicePathFromAnyFv, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesLib, AllocatePeiAccessiblePages, 2, EFIAPI);
