/** @file
  Header file for 'acpiview' configuration.

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_VIEW_CONFIG_H_
#define ACPI_VIEW_CONFIG_H_

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
  A structure holding the user selection detailing which
  ACPI table is to be examined by the AcpiView code.
**/
typedef struct {
  UINT32              Type;    ///< 32bit signature of the selected ACPI table.
  CONST CHAR16*       Name;    ///< User friendly name of the selected ACPI table.
  BOOLEAN             Found;   ///< The selected table has been found in the system.
} SELECTED_ACPI_TABLE;

/**
  Configuration struct for the acpiview command.
**/
typedef struct {
  BOOLEAN        ConsistencyCheck;            ///< Enable consistency checks when processing tables
  BOOLEAN        ColourHighlighting;          ///< Enable UEFI shell colour codes in output
  EREPORT_OPTION ReportType;                  ///< Type of report to create
  BOOLEAN        MandatoryTableValidate;      ///< Validate presence of mandatory tables
  UINTN          MandatoryTableSpec;          ///< Specification of which tables are mandatory
} ACPI_VIEW_CONFIG;

/**
  This function selects an ACPI table in current context.
  The string name of the table is converted into UINT32
  table signature.

  @param [in] TableName The name of the ACPI table to select.
**/
VOID
EFIAPI
AcpiViewConfigSelectTable (
  CONST CHAR16 *TableName
  );

/**
  Reset the AcpiView user configuration to defaults.
**/
VOID
EFIAPI
AcpiViewConfigSetDefaults (
  VOID
  );

extern ACPI_VIEW_CONFIG    mConfig;
extern SELECTED_ACPI_TABLE mSelectedAcpiTable;

#endif // ACPI_VIEW_CONFIG_H_

