/** @file
  Google Test mocks for HstiLib

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/HstiLib.h>
}

struct MockHstiLib {
  MOCK_INTERFACE_DECLARATION (MockHstiLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HstiLibSetTable,
    (IN VOID   *Hsti,
     IN UINTN  HstiSize)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HstiLibGetTable,
    (IN  UINT32  Role,
     IN  CHAR16  *ImplementationID OPTIONAL,
     OUT VOID    **Hsti,
     OUT UINTN   *HstiSize)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HstiLibSetFeaturesVerified,
    (IN UINT32  Role,
     IN CHAR16  *ImplementationID  OPTIONAL,
     IN UINT32  ByteIndex,
     IN UINT8   BitMask)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HstiLibClearFeaturesVerified,
    (IN UINT32  Role,
     IN CHAR16  *ImplementationID  OPTIONAL,
     IN UINT32  ByteIndex,
     IN UINT8   BitMask)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HstiLibAppendErrorString,
    (IN UINT32  Role,
     IN CHAR16  *ImplementationID  OPTIONAL,
     IN CHAR16  *ErrorString)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HstiLibSetErrorString,
    (IN UINT32  Role,
     IN CHAR16  *ImplementationID  OPTIONAL,
     IN CHAR16  *ErrorString)
    );
};
