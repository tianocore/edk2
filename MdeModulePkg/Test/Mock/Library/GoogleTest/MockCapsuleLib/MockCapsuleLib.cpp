/** @file
  Google Test mocks for CapsuleLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockCapsuleLib.h>

MOCK_INTERFACE_DEFINITION (MockCapsuleLib);

MOCK_FUNCTION_DEFINITION (MockCapsuleLib, SupportCapsuleImage, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockCapsuleLib, ProcessCapsuleImage, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockCapsuleLib, StageCapsuleImage, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockCapsuleLib, ProcessCapsules, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockCapsuleLib, CoDCheckCapsuleOnDiskFlag, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockCapsuleLib, CoDClearCapsuleOnDiskFlag, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockCapsuleLib, CoDRelocateCapsule, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockCapsuleLib, CoDRemoveTempFile, 1, EFIAPI);
