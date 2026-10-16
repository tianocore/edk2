/** @file MockSmmVariable.h
  Declare mock SMM Variable Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Protocol/SmmVariable.h>
}

struct MockSmmVariableProtocol {
  MOCK_INTERFACE_DECLARATION (MockSmmVariableProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmGetVariable,
    (
     IN     CHAR16      *VariableName,
     IN     EFI_GUID    *VendorGuid,
     OUT    UINT32      *Attributes     OPTIONAL,
     IN OUT UINTN       *DataSize,
     OUT    VOID        *Data           OPTIONAL
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmGetNextVariableName,
    (
     IN OUT UINTN                    *VariableNameSize,
     IN OUT CHAR16                   *VariableName,
     IN OUT EFI_GUID                 *VendorGuid
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmSetVariable,
    (
     IN  CHAR16                       *VariableName,
     IN  EFI_GUID                     *VendorGuid,
     IN  UINT32                       Attributes,
     IN  UINTN                        DataSize,
     IN  VOID                         *Data
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmQueryVariableInfo,
    (
     IN  UINT32            Attributes,
     OUT UINT64            *MaximumVariableStorageSize,
     OUT UINT64            *RemainingVariableStorageSize,
     OUT UINT64            *MaximumVariableSize
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockSmmVariableProtocol);
MOCK_FUNCTION_DEFINITION (MockSmmVariableProtocol, SmmGetVariable, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmVariableProtocol, SmmGetNextVariableName, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmVariableProtocol, SmmSetVariable, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmVariableProtocol, SmmQueryVariableInfo, 4, EFIAPI);

#define MOCK_EFI_SMM_VARIABLE_PROTOCOL_INSTANCE(NAME)  \
 EFI_SMM_VARIABLE_PROTOCOL NAME##_INSTANCE = {         \
  SmmGetVariable,                                      \
  SmmGetNextVariableName,                              \
  SmmSetVariable,                                      \
  SmmQueryVariableInfo                                 \
 };                                                    \
 EFI_SMM_VARIABLE_PROTOCOL  *NAME = &NAME##_INSTANCE;
