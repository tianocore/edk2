/** @file MockAcpiSystemDescriptionTable.h
  This file declares a mock of ACPI system description tables protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/AcpiSystemDescriptionTable.h>
}

struct MockAcpiSdtProtocol {
  MOCK_INTERFACE_DECLARATION (MockAcpiSdtProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetAcpiTable,
    (
     IN    UINTN                   Index,
     OUT   EFI_ACPI_SDT_HEADER     **Table,
     OUT   EFI_ACPI_TABLE_VERSION  *Version,
     OUT   UINTN                   *TableKey
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RegisterNotify,
    (
     IN BOOLEAN                    Register,
     IN EFI_ACPI_NOTIFICATION_FN   Notification
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Open,
    (
     IN    VOID            *Buffer,
     OUT   EFI_ACPI_HANDLE *Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    OpenSdt,
    (
     IN    UINTN           TableKey,
     OUT   EFI_ACPI_HANDLE *Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Close,
    (
     IN EFI_ACPI_HANDLE Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetChild,
    (
     IN EFI_ACPI_HANDLE        ParentHandle,
     IN OUT EFI_ACPI_HANDLE    *Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetOption,
    (
     IN        EFI_ACPI_HANDLE     Handle,
     IN        UINTN               Index,
     OUT       EFI_ACPI_DATA_TYPE  *DataType,
     OUT CONST VOID                **Data,
     OUT       UINTN               *DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetOption,
    (
     IN        EFI_ACPI_HANDLE Handle,
     IN        UINTN           Index,
     IN CONST  VOID            *Data,
     IN        UINTN           DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FindPath,
    (
     IN    EFI_ACPI_HANDLE HandleIn,
     IN    VOID            *AcpiPath,
     OUT   EFI_ACPI_HANDLE *HandleOut
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockAcpiSdtProtocol);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, GetAcpiTable, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, RegisterNotify, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, Open, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, OpenSdt, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, Close, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, GetChild, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, GetOption, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, SetOption, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, FindPath, 3, EFIAPI);

static EFI_ACPI_SDT_PROTOCOL  ACPI_SDT_PROTOCOL_INSTANCE = {
  0,
  GetAcpiTable,   // EFI_ACPI_TABLE_INSTALL_ACPI_TABLE
  RegisterNotify, // EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE
  Open,
  OpenSdt,
  Close,
  GetChild,
  GetOption,
  SetOption,
  FindPath
};

extern "C" {
  EFI_ACPI_SDT_PROTOCOL  *gAcpiSdtProtocol = &ACPI_SDT_PROTOCOL_INSTANCE;
}
