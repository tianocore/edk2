/** @file MockStandaloneMmMemLib.h
  Google Test mocks for StandaloneMmMemLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_MM_MEM_LIB_H_
#define MOCK_MM_MEM_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/StandaloneMmMemLib.h>
}

//
// Declarations to handle usage of the MmMemLib by creating mock
//
struct MockMmMemLib {
  MOCK_INTERFACE_DECLARATION (MockMmMemLib);

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    MmIsBufferOutsideMmValid,
    (
     IN EFI_PHYSICAL_ADDRESS  Buffer,
     IN UINT64                Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MmCopyMemToMmram,
    (
     OUT VOID       *DestinationBuffer,
     IN CONST VOID  *SourceBuffer,
     IN UINTN       Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MmCopyMemFromMmram,
    (
     OUT VOID       *DestinationBuffer,
     IN CONST VOID  *SourceBuffer,
     IN UINTN       Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MmCopyMem,
    (
     OUT VOID       *DestinationBuffer,
     IN CONST VOID  *SourceBuffer,
     IN UINTN       Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MmSetMem,
    (
     OUT VOID  *Buffer,
     IN UINTN  Length,
     IN UINT8  Value
    )
    );
};

#endif
