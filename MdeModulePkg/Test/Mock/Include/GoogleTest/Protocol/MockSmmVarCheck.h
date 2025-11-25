/** @file MockSmmVarCheck.h
  This file declares a mock of Smm Variable check Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SmmVarCheck.h>
}

struct MockSmmVarCheck {
  MOCK_INTERFACE_DECLARATION (MockSmmVarCheck);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmRegisterSetVariableCheckHandler,
    (
     IN VAR_CHECK_SET_VARIABLE_CHECK_HANDLER   Handler
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockSmmVarCheck);
MOCK_FUNCTION_DEFINITION (MockSmmVarCheck, SmmRegisterSetVariableCheckHandler, 1, EFIAPI);

static EDKII_SMM_VAR_CHECK_PROTOCOL  SMMVARCHECK_PROTOCOL_INSTANCE = {
  SmmRegisterSetVariableCheckHandler,
  NULL,
  NULL
};

extern "C" {
  EDKII_SMM_VAR_CHECK_PROTOCOL  *gEdkiiSmmVarCheckProtocol = &SMMVARCHECK_PROTOCOL_INSTANCE;
}
