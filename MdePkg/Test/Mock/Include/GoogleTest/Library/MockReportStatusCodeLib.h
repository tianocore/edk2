/** @file MockReportStatusCodeLib.h
  Google Test mocks for ReportStatusCodeLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_REPORT_STATUS_CODE_LIB_H_
#define MOCK_REPORT_STATUS_CODE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/ReportStatusCodeLib.h>
}

struct MockReportStatusCodeLib {
  MOCK_INTERFACE_DECLARATION (MockReportStatusCodeLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReportStatusCode,
    (
     IN EFI_STATUS_CODE_TYPE   Type,
     IN EFI_STATUS_CODE_VALUE  Value
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReportStatusCodeWithDevicePath,
    (
     IN EFI_STATUS_CODE_TYPE            Type,
     IN EFI_STATUS_CODE_VALUE           Value,
     IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReportStatusCodeWithExtendedData,
    (
     IN EFI_STATUS_CODE_TYPE   Type,
     IN EFI_STATUS_CODE_VALUE  Value,
     IN CONST VOID             *ExtendedData,
     IN UINTN                  ExtendedDataSize
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReportStatusCodeEx,
    (
     IN EFI_STATUS_CODE_TYPE   Type,
     IN EFI_STATUS_CODE_VALUE  Value,
     IN UINT32                 Instance,
     IN CONST EFI_GUID         *CallerId          OPTIONAL,
     IN CONST EFI_GUID         *ExtendedDataGuid  OPTIONAL,
     IN CONST VOID             *ExtendedData      OPTIONAL,
     IN UINTN                  ExtendedDataSize
    )
    );
  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ReportProgressCodeEnabled,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ReportErrorCodeEnabled,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ReportDebugCodeEnabled,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    CodeTypeToPostCode,
    (
     IN  EFI_STATUS_CODE_TYPE   CodeType,
     IN  EFI_STATUS_CODE_VALUE  Value,
     OUT UINT8                  *PostCode
    )
    );
  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ReportStatusCodeExtractAssertInfo,
    (
     IN EFI_STATUS_CODE_TYPE        CodeType,
     IN EFI_STATUS_CODE_VALUE       Value,
     IN CONST EFI_STATUS_CODE_DATA  *Data,
     OUT CHAR8                      **Filename,
     OUT CHAR8                      **Description,
     OUT UINT32                     *LineNumber
    )
    );
  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ReportStatusCodeExtractDebugInfo,
    (
     IN CONST EFI_STATUS_CODE_DATA  *Data,
     OUT UINT32                     *ErrorLevel,
     OUT BASE_LIST                  *Marker,
     OUT CHAR8                      **Format
    )
    );
};

#endif
