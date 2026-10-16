/** @file MockReadOnlyVariable2.h
  This file declares a mock of Read-only Variable Service2 PPI.

  This PPI permits read-only access to the UEFI variable store during the PEI phase.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Pi/PiPeiCis.h>
  #include <Ppi/ReadOnlyVariable2.h>
}

struct MockReadOnlyVariable2 {
  MOCK_INTERFACE_DECLARATION (MockReadOnlyVariable2);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetVariable,
    (IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *This,
     IN CONST  CHAR16                           *VariableName,
     IN CONST  EFI_GUID                         *VariableGuid,
     OUT       UINT32                           *Attributes,
     IN OUT    UINTN                            *DataSize,
     OUT       VOID                             *Data OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    NextVariableName,
    (IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI *This,
     IN OUT    UINTN                           *VariableNameSize,
     IN OUT    CHAR16                          *VariableName,
     IN OUT    EFI_GUID                        *VariableGuid)
    );
};

extern "C" {
  extern EFI_PEI_READ_ONLY_VARIABLE2_PPI  *PpiReadOnlyVariableServices;
}
