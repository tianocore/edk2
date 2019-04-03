/** @file
  This module provides help function for finding ACPI table.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiLibInternal.h"
#include <IndustryStandard/Acpi.h>

/**
  This function locates next ACPI table in XSDT/RSDT based on Signature and
  previous returned Table.

  If PreviousTable is NULL:
  This function will locate the first ACPI table in XSDT/RSDT based on
  Signature in gEfiAcpi20TableGuid system configuration table first, and then
  gEfiAcpi10TableGuid system configuration table.
  This function will locate in XSDT first, and then RSDT.
  For DSDT, this function will locate XDsdt in FADT first, and then Dsdt in
  FADT.
  For FACS, this function will locate XFirmwareCtrl in FADT first, and then
  FirmwareCtrl in FADT.

  If PreviousTable is not NULL:
  1. If it could be located in XSDT in gEfiAcpi20TableGuid system configuration
     table, then this function will just locate next table in XSDT in
     gEfiAcpi20TableGuid system configuration table.
  2. If it could be located in RSDT in gEfiAcpi20TableGuid system configuration
     table, then this function will just locate next table in RSDT in
     gEfiAcpi20TableGuid system configuration table.
  3. If it could be located in RSDT in gEfiAcpi10TableGuid system configuration
     table, then this function will just locate next table in RSDT in
     gEfiAcpi10TableGuid system configuration table.

  It's not supported that PreviousTable is not NULL but PreviousTable->Signature
  is not same with Signature, NULL will be returned.

  @param Signature          ACPI table signature.
  @param PreviousTable      Pointer to previous returned table to locate next
                            table, or NULL to locate first table.

  @return Next ACPI table or NULL if not found.

**/
EFI_ACPI_COMMON_HEADER *
EFIAPI
EfiLocateNextAcpiTable (
  IN UINT32                     Signature,
  IN EFI_ACPI_COMMON_HEADER     *PreviousTable OPTIONAL
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  This function locates first ACPI table in XSDT/RSDT based on Signature.

  This function will locate the first ACPI table in XSDT/RSDT based on
  Signature in gEfiAcpi20TableGuid system configuration table first, and then
  gEfiAcpi10TableGuid system configuration table.
  This function will locate in XSDT first, and then RSDT.
  For DSDT, this function will locate XDsdt in FADT first, and then Dsdt in
  FADT.
  For FACS, this function will locate XFirmwareCtrl in FADT first, and then
  FirmwareCtrl in FADT.

  @param Signature          ACPI table signature.

  @return First ACPI table or NULL if not found.

**/
EFI_ACPI_COMMON_HEADER *
EFIAPI
EfiLocateFirstAcpiTable (
  IN UINT32                     Signature
  )
{
  return EfiLocateNextAcpiTable (Signature, NULL);
}
