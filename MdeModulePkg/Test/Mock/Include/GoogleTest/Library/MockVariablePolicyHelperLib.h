/** @file MockVariablePolicyHelper.h
  Google Test mocks for VariablePolicyHelper

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_VARIABLE_POLICY_HELPER_LIB_H_
#define MOCK_VARIABLE_POLICY_HELPER_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Protocol/VariablePolicy.h>
}

//
// Declarations to handle usage of the VariablePolicyHelperLib by creating mock
//
struct MockVariablePolicyHelperLib {
  MOCK_INTERFACE_DECLARATION (MockVariablePolicyHelperLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CreateBasicVariablePolicy,
    (IN CONST  EFI_GUID         *Namespace,
     IN CONST  CHAR16           *Name OPTIONAL,
     IN        UINT32           MinSize,
     IN        UINT32           MaxSize,
     IN        UINT32           AttributesMustHave,
     IN        UINT32           AttributesCantHave,
     IN        UINT8            LockPolicyType,
     OUT VARIABLE_POLICY_ENTRY  **NewEntry)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CreateVarStateVariablePolicy,
    (IN CONST  EFI_GUID         *Namespace,
     IN CONST  CHAR16           *Name OPTIONAL,
     IN        UINT32           MinSize,
     IN        UINT32           MaxSize,
     IN        UINT32           AttributesMustHave,
     IN        UINT32           AttributesCantHave,
     IN CONST  EFI_GUID         *VarStateNamespace,
     IN        UINT8            VarStateValue,
     IN CONST  CHAR16           *VarStateName,
     OUT VARIABLE_POLICY_ENTRY  **NewEntry)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RegisterBasicVariablePolicy,
    (IN        EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy,
     IN CONST  EFI_GUID                        *Namespace,
     IN CONST  CHAR16                          *Name OPTIONAL,
     IN        UINT32                          MinSize,
     IN        UINT32                          MaxSize,
     IN        UINT32                          AttributesMustHave,
     IN        UINT32                          AttributesCantHave,
     IN        UINT8                           LockPolicyType)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RegisterVarStateVariablePolicy,
    (IN        EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy,
     IN CONST  EFI_GUID                        *Namespace,
     IN CONST  CHAR16                          *Name OPTIONAL,
     IN        UINT32                          MinSize,
     IN        UINT32                          MaxSize,
     IN        UINT32                          AttributesMustHave,
     IN        UINT32                          AttributesCantHave,
     IN CONST  EFI_GUID                        *VarStateNamespace,
     IN CONST  CHAR16                          *VarStateName,
     IN        UINT8                           VarStateValue)
    );
};

#endif
