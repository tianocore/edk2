/** @file MockSmmVarCheck.h
  This file declares a mock of Smm Variable Check Protocol.

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

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmVariablePropertySet,
    (
     IN CHAR16                         *Name,
     IN EFI_GUID                       *Guid,
     IN VAR_CHECK_VARIABLE_PROPERTY    *VariableProperty
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmVariablePropertyGet,
    (
     IN CHAR16                         *Name,
     IN EFI_GUID                       *Guid,
     OUT VAR_CHECK_VARIABLE_PROPERTY    *VariableProperty
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockSmmVarCheck);
MOCK_FUNCTION_DEFINITION (MockSmmVarCheck, SmmRegisterSetVariableCheckHandler, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmVarCheck, SmmVariablePropertySet, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmVarCheck, SmmVariablePropertyGet, 3, EFIAPI);

EDKII_SMM_VAR_CHECK_PROTOCOL  SMMVARCHECK_PROTOCOL_INSTANCE = {
  SmmRegisterSetVariableCheckHandler, // EFI_MM_READ_SAVE_STATE
  SmmVariablePropertySet,             // EDKII_VAR_CHECK_VARIABLE_PROPERTY_SET
  SmmVariablePropertyGet              // EDKII_VAR_CHECK_VARIABLE_PROPERTY_GET
};

extern "C" {
  EDKII_SMM_VAR_CHECK_PROTOCOL  *gEdkiiSmmVarCheckProtocol = &SMMVARCHECK_PROTOCOL_INSTANCE;
}

#endif // MOCK_SMM_VAR_CHECK_H_
