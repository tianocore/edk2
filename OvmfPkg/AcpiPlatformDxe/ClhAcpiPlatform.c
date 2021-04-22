/** @file
  OVMF ACPI Platform Driver for Cloud Hypervisor

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatform.h"

/**
  Effective entrypoint of Acpi Platform driver.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
EFIAPI
InstallAcpiTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiTable
  )
{
  EFI_STATUS                         Status;

  Status = InstallClhAcpiTable (AcpiTable);
  return Status;
}
