/** @file
  OVMF ACPI Platform Driver

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_PLATFORM_H_
#define ACPI_PLATFORM_H_

#include <Protocol/AcpiTable.h> // EFI_ACPI_TABLE_PROTOCOL

EFI_STATUS
EFIAPI
InstallCloudHvTablesTdx (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol
  );

EFI_STATUS
EFIAPI
InstallCloudHvTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol
  );

EFI_STATUS
EFIAPI
InstallAcpiTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  );

#endif
