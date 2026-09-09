/** @file
  Google Test mocks for RngLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/RngLib.h>
}

struct MockRngLib {
  MOCK_INTERFACE_DECLARATION (MockRngLib);

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    GetRandomNumber16,
    (OUT UINT16  *Rand)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    GetRandomNumber32,
    (OUT UINT32  *Rand)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    GetRandomNumber64,
    (OUT UINT64  *Rand)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    GetRandomNumber128,
    (OUT UINT64  *Rand)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetRngGuid,
    (GUID  *RngGuid)
    );
};
