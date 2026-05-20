/** @file MockCpuLib.h
  Google Test mocks for the CPU Library

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
}

struct MockCpuLib {
  MOCK_INTERFACE_DECLARATION (MockCpuLib);

  MOCK_FUNCTION_DECLARATION (
    VOID,
    CpuSleep,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    CpuFlushTlb,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    InitializeFloatingPointUnits,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    StandardSignatureIsAuthenticAMD,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    GetCpuFamilyModel,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    GetCpuSteppingId,
    (
    )
    );
};
