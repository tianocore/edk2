/** @file MockBaseMemoryLib.h
  Google Test mocks for BaseMemoryLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_BASE_MEMORY_LIB_H_
#define MOCK_BASE_MEMORY_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseMemoryLib.h>
}

//
// Declarations to handle usage of the BaseMemoryLib by creating mock
//
struct MockBaseMemoryLib {
  MOCK_INTERFACE_DECLARATION (MockBaseMemoryLib);

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    CopyMem,
    (
     OUT VOID       *DestinationBuffer,
     IN CONST VOID  *SourceBuffer,
     IN UINTN       Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    SetMem,
    (
     OUT VOID  *Buffer,
     IN UINTN  Length,
     IN UINT8  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    SetMem16,
    (
     OUT VOID   *Buffer,
     IN UINTN   Length,
     IN UINT16  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    SetMem32,
    (
     OUT VOID   *Buffer,
     IN UINTN   Length,
     IN UINT32  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    SetMem64,
    (
     OUT VOID   *Buffer,
     IN UINTN   Length,
     IN UINT64  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    SetMemN,
    (
     OUT VOID  *Buffer,
     IN UINTN  Length,
     IN UINTN  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ZeroMem,
    (
     OUT VOID  *Buffer,
     IN UINTN  Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    INTN,
    CompareMem,
    (
     IN CONST VOID  *DestinationBuffer,
     IN CONST VOID  *SourceBuffer,
     IN UINTN       Length
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ScanMem8,
    (
     IN CONST VOID  *Buffer,
     IN UINTN       Length,
     IN UINT8       Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ScanMem16,
    (
     IN CONST VOID  *Buffer,
     IN UINTN       Length,
     IN UINT16      Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ScanMem32,
    (
     IN CONST VOID  *Buffer,
     IN UINTN       Length,
     IN UINT32      Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ScanMem64,
    (
     IN CONST VOID  *Buffer,
     IN UINTN       Length,
     IN UINT64      Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ScanMemN,
    (
     IN CONST VOID  *Buffer,
     IN UINTN       Length,
     IN UINTN       Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    GUID *,
    CopyGuid,
    (
     OUT GUID       *DestinationGuid,
     IN CONST GUID  *SourceGuid
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    CompareGuid,
    (
     IN CONST GUID  *Guid1,
     IN CONST GUID  *Guid2
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ScanGuid,
    (
     IN CONST VOID  *Buffer,
     IN UINTN       Length,
     IN CONST GUID  *Guid
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    IsZeroGuid,
    (
     IN CONST GUID  *Guid
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    IsZeroBuffer,
    (
     IN CONST VOID  *Buffer,
     IN UINTN       Length
    )
    );
};

#endif
