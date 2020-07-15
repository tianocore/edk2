/** @file
  Header file for 'acpiview' configuration.

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_VIEW_CONFIG_H_
#define ACPI_VIEW_CONFIG_H_

/**
  This function returns the colour highlighting status.

  @retval TRUE Colour highlighting is enabled.
**/
BOOLEAN
EFIAPI
GetColourHighlighting (
  VOID
  );

/**
  This function sets the colour highlighting status.

  @param [in] Highlight The highlight status.
**/
VOID
EFIAPI
SetColourHighlighting (
  BOOLEAN Highlight
  );

/**
  This function returns the consistency checking status.

  @retval TRUE Consistency checking is enabled.
**/
BOOLEAN
EFIAPI
GetConsistencyChecking (
  VOID
  );

/**
  This function sets the consistency checking status.

  @param [in] ConsistencyChecking   The consistency checking status.
**/
VOID
EFIAPI
SetConsistencyChecking (
  BOOLEAN ConsistencyChecking
  );

/**
  This function returns the ACPI table requirements validation flag.

  @retval TRUE Check for mandatory table presence should be performed.
**/
BOOLEAN
EFIAPI
GetMandatoryTableValidate (
  VOID
  );

/**
  This function sets the ACPI table requirements validation flag.

  @param [in] Validate Enable/Disable ACPI table requirements validation.
**/
VOID
EFIAPI
SetMandatoryTableValidate (
  BOOLEAN Validate
  );

/**
  This function returns the identifier of specification to validate ACPI table
  requirements against.

  @return ID of specification listing mandatory tables.
**/
UINTN
EFIAPI
GetMandatoryTableSpec (
  VOID
  );

/**
  This function sets the identifier of specification to validate ACPI table
  requirements against.

  @param [in] Spec ID of specification listing mandatory tables.
**/
VOID
EFIAPI
SetMandatoryTableSpec (
  UINTN Spec
  );

/**
  The EREPORT_OPTION enum describes ACPI table Reporting options.
**/
typedef enum {
  ReportAll,          ///< Report All tables.
  ReportSelected,     ///< Report Selected table.
  ReportTableList,    ///< Report List of tables.
  ReportDumpBinFile,  ///< Dump selected table to a file.
  ReportMax,
} EREPORT_OPTION;

/**
  This function returns the report options.

  @return The current report option.
**/
EREPORT_OPTION
EFIAPI
GetReportOption (
  VOID
  );

/**
  This function sets the report options.

  @param [in] ReportType The report option to set.
**/
VOID
EFIAPI
SetReportOption (
  EREPORT_OPTION ReportType
  );

/**
  A structure holding the user selection detailing which
  ACPI table is to be examined by the AcpiView code.
**/
typedef struct {
  UINT32              Type;    ///< 32bit signature of the selected ACPI table.
  CONST CHAR16*       Name;    ///< User friendly name of the selected ACPI table.
  BOOLEAN             Found;   ///< The selected table has been found in the system.
} SELECTED_ACPI_TABLE;

/**
  This function returns the selected ACPI table.

  @param [out] SelectedAcpiTable Pointer that will contain the returned struct.
**/
VOID
EFIAPI
GetSelectedAcpiTable (
  OUT SELECTED_ACPI_TABLE** SelectedAcpiTable
  );

/**
  This function selects an ACPI table in current context.
  The string name of the table is converted into UINT32
  table signature.

  @param [in] TableName The name of the ACPI table to select.
**/
VOID
EFIAPI
SelectAcpiTable (
  CONST CHAR16* TableName
  );

/**
  Reset the AcpiView user configuration to defaults.
**/
VOID
EFIAPI
AcpiConfigSetDefaults (
  VOID
  );

#endif // ACPI_VIEW_CONFIG_H_
