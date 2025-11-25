/** @file
  Google Test mocks for CapsuleLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/CapsuleLib.h>
}

struct MockCapsuleLib {
  MOCK_INTERFACE_DECLARATION (MockCapsuleLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SupportCapsuleImage,
    (IN EFI_CAPSULE_HEADER  *CapsuleHeader)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ProcessCapsuleImage,
    (IN EFI_CAPSULE_HEADER  *CapsuleHeader)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    StageCapsuleImage,
    (IN EFI_CAPSULE_HEADER  *CapsuleHeader)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ProcessCapsules,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    CoDCheckCapsuleOnDiskFlag,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CoDClearCapsuleOnDiskFlag,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CoDRelocateCapsule,
    (UINTN  MaxRetry)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CoDRemoveTempFile,
    (UINTN  MaxRetry)
    );
};
