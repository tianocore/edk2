/** @file
  bhyve ACPI Platform Driver

  Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ACPI_PLATFORM_H_INCLUDED_
#define _ACPI_PLATFORM_H_INCLUDED_

#include <PiDxe.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PciIo.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/XenPlatformLib.h>
#include <IndustryStandard/Acpi.h>

typedef struct S3_CONTEXT S3_CONTEXT;

EFI_STATUS
EFIAPI
InstallAcpiTable (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol,
  IN   VOID                     *AcpiTableBuffer,
  IN   UINTN                    AcpiTableBufferSize,
  OUT  UINTN                    *TableKey
  );

EFI_STATUS
EFIAPI
BhyveInstallAcpiTable (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol,
  IN   VOID                     *AcpiTableBuffer,
  IN   UINTN                    AcpiTableBufferSize,
  OUT  UINTN                    *TableKey
  );

EFI_STATUS
EFIAPI
InstallXenTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol
  );

EFI_STATUS
EFIAPI
InstallAcpiTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  );

#endif /* _ACPI_PLATFORM_H_INCLUDED_ */
