/** @file MockPciExpressLib.h
  Google Test mocks for PciExpressLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PciExpressLib.h>
}

struct MockPciExpressLib {
  MOCK_INTERFACE_DECLARATION (MockPciExpressLib);

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressRead8,
    (
     IN UINTN  Address
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressRead16,
    (
     IN UINTN  Address
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressRead32,
    (
     IN UINTN  Address
    )
    );
};
