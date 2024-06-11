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

struct MockEdkiiSmmVarCheckProtocol {
  MOCK_INTERFACE_DECLARATION (MockEdkiiSmmVarCheckProtocol);

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

MOCK_INTERFACE_DEFINITION (MockEdkiiSmmVarCheckProtocol);
MOCK_FUNCTION_DEFINITION (MockEdkiiSmmVarCheckProtocol, SmmRegisterSetVariableCheckHandler, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEdkiiSmmVarCheckProtocol, SmmVariablePropertySet, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEdkiiSmmVarCheckProtocol, SmmVariablePropertyGet, 3, EFIAPI);

//
// Mock protocol instantiation for external use in test cases
//
#define MOCK_EDKII_SMM_VAR_CHECK_PROTOCOL_INSTANCE(NAME)  \
  EDKII_SMM_VAR_CHECK_PROTOCOL NAME##_INSTANCE = {        \
    SmmRegisterSetVariableCheckHandler,                   \
    SmmVariablePropertySet,                               \
    GetRng };                                             \
  EDKII_SMM_VAR_CHECK_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // MOCK_SMM_VAR_CHECK_H_
