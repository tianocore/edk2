/** @file MockSmmVarCheck.h
  This file declares a mock of Smm Variable check Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SMM_VAR_CHECK_H_
#define MOCK_SMM_VAR_CHECK_H_

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

extern "C" {
  extern EDKII_SMM_VAR_CHECK_PROTOCOL  *gEdkiiSmmVarCheckProtocol;
}

#endif // MOCK_SMM_VAR_CHECK_H_
