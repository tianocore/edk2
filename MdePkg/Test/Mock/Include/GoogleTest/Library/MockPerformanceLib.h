/** @file
  Google Test mocks for PerformanceLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PERFORMANCE_LIB_H_
#define MOCK_PERFORMANCE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PerformanceLib.h>
}

struct MockPerformanceLib {
  MOCK_INTERFACE_DECLARATION (MockPerformanceLib);

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    StartPerformanceMeasurement,
    (IN CONST VOID   *Handle   OPTIONAL,
     IN CONST CHAR8  *Token    OPTIONAL,
     IN CONST CHAR8  *Module   OPTIONAL,
     IN UINT64       TimeStamp)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    EndPerformanceMeasurement,
    (IN CONST VOID   *Handle   OPTIONAL,
     IN CONST CHAR8  *Token    OPTIONAL,
     IN CONST CHAR8  *Module   OPTIONAL,
     IN UINT64       TimeStamp)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    GetPerformanceMeasurement,
    (IN  UINTN        LogEntryKey,
     OUT CONST VOID   **Handle,
     OUT CONST CHAR8  **Token,
     OUT CONST CHAR8  **Module,
     OUT UINT64       *StartTimeStamp,
     OUT UINT64       *EndTimeStamp)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    StartPerformanceMeasurementEx,
    (IN CONST VOID   *Handle   OPTIONAL,
     IN CONST CHAR8  *Token    OPTIONAL,
     IN CONST CHAR8  *Module   OPTIONAL,
     IN UINT64       TimeStamp,
     IN UINT32       Identifier)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    EndPerformanceMeasurementEx,
    (IN CONST VOID   *Handle   OPTIONAL,
     IN CONST CHAR8  *Token    OPTIONAL,
     IN CONST CHAR8  *Module   OPTIONAL,
     IN UINT64       TimeStamp,
     IN UINT32       Identifier)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    GetPerformanceMeasurementEx,
    (IN  UINTN        LogEntryKey,
     OUT CONST VOID   **Handle,
     OUT CONST CHAR8  **Token,
     OUT CONST CHAR8  **Module,
     OUT UINT64       *StartTimeStamp,
     OUT UINT64       *EndTimeStamp,
     OUT UINT32       *Identifier)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    PerformanceMeasurementEnabled,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    LogPerformanceMeasurementEnabled,
    (IN  CONST UINTN  Type)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LogPerformanceMeasurement,
    (IN CONST VOID   *CallerIdentifier  OPTIONAL,
     IN CONST VOID   *Guid     OPTIONAL,
     IN CONST CHAR8  *String   OPTIONAL,
     IN UINT64       Address   OPTIONAL,
     IN UINT32       Identifier)
    );
};

#endif
