/**
  Header file for AcpiView

  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef ACPIVIEW_H_
#define ACPIVIEW_H_

/** A macro to define the max file name length
*/
#define MAX_FILE_NAME_LEN    128

/** Offset to the RSDP revision from the start of the RSDP
*/
#define RSDP_REVISION_OFFSET 15

/** Offset to the RSDP length from the start of the RSDP
*/
#define RSDP_LENGTH_OFFSET   20

/** The EREPORT_OPTION enum describes ACPI table Reporting options.
*/
typedef enum ReportOption {
  EREPORT_ALL,            ///< Report All tables.
  EREPORT_SELECTED,       ///< Report Selected table.
  EREPORT_TABLE_LIST,     ///< Report List of tables.
  EREPORT_DUMP_BIN_FILE,  ///< Dump selected table to a file.
  EREPORT_MAX
} EREPORT_OPTION;

/** This function resets the ACPI table error counter to Zero.
*/
VOID
ResetErrorCount (
  VOID
  );

/** This function returns the ACPI table error count.

  @retval Returns the count of errors detected in the ACPI tables.
*/
UINT32
GetErrorCount (
  VOID
  );

/** This function resets the ACPI table warning counter to Zero.
*/
VOID
ResetWarningCount (
  VOID
  );

/** This function returns the ACPI table warning count.

  @retval Returns the count of warning detected in the ACPI tables.
*/
UINT32
GetWarningCount (
  VOID
  );

/** This function returns the colour highlighting status.

  @retval TRUE if colour highlighting is enabled.
*/
BOOLEAN
GetColourHighlighting (
  VOID
  );

/** This function sets the colour highlighting status.

*/
VOID
SetColourHighlighting (
  BOOLEAN Highlight
  );

/** This function processes the table reporting options for the ACPI table.

  @param [in] Signature The ACPI table Signature.
  @param [in] TablePtr  Pointer to the ACPI table data.
  @param [in] Length    The length fo the ACPI table.

  @retval Returns TRUE if the ACPI table should be traced.
*/
BOOLEAN
ProcessTableReportOptions (
  IN CONST UINT32  Signature,
  IN CONST UINT8*  TablePtr,
  IN CONST UINT32  Length
  );

#endif // ACPIVIEW_H_
