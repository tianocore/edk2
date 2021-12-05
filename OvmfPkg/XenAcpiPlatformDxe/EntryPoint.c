/** @file
  Entry point of OVMF ACPI Platform Driver for Xen guests

  Copyright (C) 2015-2021, Red Hat, Inc.
  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>                    // ASSERT_EFI_ERROR()
#include <Library/UefiBootServicesTableLib.h>    // gBS
#include <Protocol/AcpiTable.h>                  // EFI_ACPI_TABLE_PROTOCOL

#include "AcpiPlatform.h"

STATIC
EFI_ACPI_TABLE_PROTOCOL *
FindAcpiTableProtocol (
  VOID
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  ASSERT_EFI_ERROR (Status);
  return AcpiTable;
}

EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return InstallAcpiTables (FindAcpiTableProtocol ());
}
