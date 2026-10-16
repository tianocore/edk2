/** @file MockAcpiTable.h
  This file declares a mock of Acpi table protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/AcpiTable.h>
}

struct MockAcpiTableProtocol {
  MOCK_INTERFACE_DECLARATION (MockAcpiTableProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    InstallAcpiTable,
    (
     IN   EFI_ACPI_TABLE_PROTOCOL       *This,
     IN   VOID                          *AcpiTableBuffer,
     IN   UINTN                         AcpiTableBufferSize,
     OUT  UINTN                         *TableKey
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    UninstallAcpiTable,
    (
     IN  EFI_ACPI_TABLE_PROTOCOL       *This,
     IN  UINTN                         TableKey
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockAcpiTableProtocol);
MOCK_FUNCTION_DEFINITION (MockAcpiTableProtocol, InstallAcpiTable, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiTableProtocol, UninstallAcpiTable, 2, EFIAPI);

static EFI_ACPI_TABLE_PROTOCOL  ACPI_TABLE_PROTOCOL_INSTANCE = {
  InstallAcpiTable,   // EFI_ACPI_TABLE_INSTALL_ACPI_TABLE
  UninstallAcpiTable  // EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE
};

extern "C" {
  EFI_ACPI_TABLE_PROTOCOL  *gAcpiTableProtocol = &ACPI_TABLE_PROTOCOL_INSTANCE;
}
