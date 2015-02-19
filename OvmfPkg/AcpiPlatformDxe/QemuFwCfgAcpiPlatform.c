/** @file
  OVMF ACPI Platform Driver using QEMU's fw-cfg interface

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "AcpiPlatform.h"

/**
  Effective entrypoint of QEMU fw-cfg Acpi Platform driver.

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

  Status = InstallQemuFwCfgTables (AcpiTable);
  return Status;
}
