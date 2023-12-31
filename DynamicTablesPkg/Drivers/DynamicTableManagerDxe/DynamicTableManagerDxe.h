/** @file

  Copyright (c) 2017 - 2023, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DYNAMIC_TABLE_MANAGER_DXE_H_
#define DYNAMIC_TABLE_MANAGER_DXE_H_

#include <AcpiTableGenerator.h>

///
/// Bit definitions for acceptable ACPI table presence formats.
/// Currently only ACPI tables present in the ACPI info list and
/// already installed will count towards "Table Present" during
/// verification routine.
///
#define ACPI_TABLE_PRESENT_INFO_LIST  BIT0
#define ACPI_TABLE_PRESENT_INSTALLED  BIT1

///
/// Private data structure to verify the presence of mandatory
/// or optional ACPI tables.
///
typedef struct {
  /// ESTD ID for the ACPI table of interest.
  ESTD_ACPI_TABLE_ID    EstdTableId;
  /// Standard UINT32 ACPI signature.
  UINT32                AcpiTableSignature;
  /// 4 character ACPI table name (the 5th char8 is for null terminator).
  CHAR8                 AcpiTableName[sizeof (UINT32) + 1];
  /// Indicator on whether the ACPI table is required.
  BOOLEAN               IsMandatory;
  /// Formats of verified presences, as defined by ACPI_TABLE_PRESENT_*
  /// This field should be initialized to 0 and will be populated during
  /// verification routine.
  UINT16                Presence;
} ACPI_TABLE_PRESENCE_INFO;

BOOLEAN
IsFadtPresentInstalled (
  VOID
  );

/** The function checks if the Configuration Manager has provided the
    mandatory ACPI tables for installation.

  @param [in]  AcpiTableInfo      Pointer to the ACPI Table Info list.
  @param [in]  AcpiTableCount     Count of ACPI Table Info.

  @retval EFI_SUCCESS           Success.
  @retval EFI_NOT_FOUND         If mandatory table is not found.
  @retval EFI_ALREADY_STARTED   If mandatory table found in AcpiTableInfo is already installed.
**/
EFI_STATUS
EFIAPI
VerifyMandatoryTablesArePresent (
  IN CONST CM_STD_OBJ_ACPI_TABLE_INFO  *CONST  AcpiTableInfo,
  IN       UINT32                              AcpiTableCount
  );

#endif // DYNAMIC_TABLE_MANAGER_DXE_H_
