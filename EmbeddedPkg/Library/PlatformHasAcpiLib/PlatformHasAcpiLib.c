/** @file
  A hook-in library for MdeModulePkg/Universal/Acpi/AcpiTableDxe.

  Plugging this library instance into AcpiTableDxe makes
  EFI_ACPI_TABLE_PROTOCOL and (if enabled) EFI_ACPI_SDT_PROTOCOL depend on the
  platform's dynamic decision whether to expose an ACPI-based hardware
  description to the operating system.

  Universal and platform specific DXE drivers that produce ACPI tables depend
  on EFI_ACPI_TABLE_PROTOCOL / EFI_ACPI_SDT_PROTOCOL in turn.

  Copyright (C) 2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

RETURN_STATUS
EFIAPI
PlatformHasAcpiInitialize (
  VOID
  )
{
  //
  // Do nothing, just imbue AcpiTableDxe with a protocol dependency on
  // EDKII_PLATFORM_HAS_ACPI_GUID.
  //
  return RETURN_SUCCESS;
}
