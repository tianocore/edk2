/** @file MockSmmMemLib.h
  Google Test mocks for SmmMemLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SMM_MEM_LIB_H_
#define MOCK_SMM_MEM_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/SmmMemLib.h>
}

//
// Declarations to handle usage of the SmmMemLib by creating mock
//
struct MockSmmMemLib {
  MOCK_INTERFACE_DECLARATION (MockSmmMemLib);

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    SmmIsBufferOutsideSmmValid,
    (
     IN EFI_PHYSICAL_ADDRESS  Buffer,
     IN UINT64                Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmCopyMemToSmram,
    (
     OUT VOID       *DestinationBuffer,
     IN CONST VOID  *SourceBuffer,
     IN UINTN       Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmCopyMemFromSmram,
    (
     OUT VOID       *DestinationBuffer,
     IN CONST VOID  *SourceBuffer,
     IN UINTN       Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmCopyMem,
    (
     OUT VOID       *DestinationBuffer,
     IN CONST VOID  *SourceBuffer,
     IN UINTN       Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SmmSetMem,
    (
     OUT VOID  *Buffer,
     IN UINTN  Length,
     IN UINT8  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    SmmCommBufferValid,
    (
     IN EFI_PHYSICAL_ADDRESS  Buffer,
     IN UINT64                Length
    )
    );
};

#endif
