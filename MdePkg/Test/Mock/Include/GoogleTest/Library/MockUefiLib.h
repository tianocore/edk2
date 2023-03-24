/** @file
  Google Test mocks for UefiLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_UEFI_LIB_H_
#define MOCK_UEFI_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
#include <Uefi.h>
#include <Library/UefiLib.h>
}

struct MockUefiLib {
  MOCK_INTERFACE_DECLARATION (MockUefiLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetVariable2,
    (IN CONST CHAR16    *Name,
     IN CONST EFI_GUID  *Guid,
     OUT VOID           **Value,
     OUT UINTN          *Size OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetEfiGlobalVariable2,
    (IN CONST CHAR16  *Name,
     OUT VOID         **Value,
     OUT UINTN        *Size OPTIONAL)
    );
};

#endif
