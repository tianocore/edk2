/** @file
  Helper Library for ACPI

  Copyright (c) 2014-2016, ARM Ltd. All rights reserved.
  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>

#include <IndustryStandard/Acpi10.h>
#include <Protocol/AcpiSystemDescriptionTable.h>

typedef
BOOLEAN
(EFIAPI *EFI_LOCATE_ACPI_CHECK)(
  IN  EFI_ACPI_DESCRIPTION_HEADER *AcpiHeader
  );

/**
  Locate and Install the ACPI tables from the Firmware Volume if it verifies
  the function condition.

  @param  AcpiFile                Guid of the ACPI file into the Firmware Volume
  @param  CheckAcpiTableFunction  Function that checks if the ACPI table should be installed

  @return EFI_SUCCESS             The function completed successfully.
  @return EFI_NOT_FOUND           The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES    There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateAndInstallAcpiFromFvConditional (
  IN CONST EFI_GUID         *AcpiFile,
  IN EFI_LOCATE_ACPI_CHECK  CheckAcpiTableFunction
  );

/**
  Locate and Install the ACPI tables from the Firmware Volume

  @param  AcpiFile              Guid of the ACPI file into the Firmware Volume

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateAndInstallAcpiFromFv (
  IN CONST EFI_GUID  *AcpiFile
  );

/**
  This function calculates and updates a UINT8 checksum
  in an ACPI description table header.

  @param  Buffer          Pointer to buffer to checksum
  @param  Size            Number of bytes to checksum

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
EFIAPI
AcpiUpdateChecksum (
  IN OUT  UINT8  *Buffer,
  IN      UINTN  Size
  );

/**
  This function uses the ACPI SDT protocol to search an ACPI table
  with a given signature.

  @param  AcpiTableSdtProtocol    Pointer to ACPI SDT protocol.
  @param  TableSignature          ACPI table signature.
  @param  Index                   The zero-based index of the table where to search the table.
                                  The index will be updated to the next instance if the table
                                  is found with the matched TableSignature.
  @param  Table                   Pointer to the table.
  @param  TableKey                Pointer to the table key.

  @return EFI_SUCCESS             The function completed successfully.
  @return EFI_INVALID_PARAMETER   At least one of parameters is invalid.
  @retval EFI_NOT_FOUND           The requested index is too large and a table was not found.

**/
EFI_STATUS
EFIAPI
AcpiLocateTableBySignature (
  IN      EFI_ACPI_SDT_PROTOCOL        *AcpiSdtProtocol,
  IN      UINT32                       TableSignature,
  IN OUT  UINTN                        *Index,
  OUT     EFI_ACPI_DESCRIPTION_HEADER  **Table,
  OUT     UINTN                        *TableKey
  );

/**
  This function updates the integer value of an AML Object.

  @param  AcpiTableSdtProtocol    Pointer to ACPI SDT protocol.
  @param  TableHandle             Points to the table representing the starting point
                                  for the object path search.
  @param  AsciiObjectPath         Pointer to the ACPI path of the object being updated.
  @param  Value                   New value to write to the object.

  @return EFI_SUCCESS             The function completed successfully.
  @return EFI_INVALID_PARAMETER   At least one of parameters is invalid or the data type
                                  of the ACPI object is not an integer value.
  @retval EFI_NOT_FOUND           The object is not found with the given path.

**/
EFI_STATUS
EFIAPI
AcpiAmlObjectUpdateInteger (
  IN  EFI_ACPI_SDT_PROTOCOL  *AcpiSdtProtocol,
  IN  EFI_ACPI_HANDLE        TableHandle,
  IN  CHAR8                  *AsciiObjectPath,
  IN  UINTN                  Value
  );
