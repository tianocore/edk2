/** @file
  Header file for AcpiView

  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPIVIEW_H_
#define ACPIVIEW_H_
#include <Library/ShellLib.h>

/**
  A macro to define the max file name length
**/
#define MAX_FILE_NAME_LEN    128

/**
  Offset to the RSDP revision from the start of the RSDP
**/
#define RSDP_REVISION_OFFSET 15

/**
  Offset to the RSDP length from the start of the RSDP
**/
#define RSDP_LENGTH_OFFSET   20

/**
  This function finds a filename not already used by adding a number in between
  the BaseFileName and the extension.

  Make sure the buffer FileName is big enough before calling the function. A
  size of MAX_FILE_NAME_LEN is recommended.

  @param [in]      BaseFileName      Start of the desired file name.
  @param [in]      Extension         Extension of the desired file name
                                     (without '.').
  @param [in, out] FileName          Preallocated buffer for the returned file
                                     name.
  @param [in]      FileNameBufferLen Size of FileName buffer..
**/
EFI_STATUS
GetNewFileName (
  IN     CONST CHAR16* BaseFileName,
  IN     CONST CHAR16* Extension,
  IN OUT       CHAR16* FileName,
  IN           UINT32  FileNameBufferLen
  );

/**
  This function resets the ACPI table error counter to Zero.
**/
VOID
ResetErrorCount (
  VOID
  );

/**
  This function returns the ACPI table error count.

  @retval Returns the count of errors detected in the ACPI tables.
**/
UINT32
GetErrorCount (
  VOID
  );

/**
  This function resets the ACPI table warning counter to Zero.
**/
VOID
ResetWarningCount (
  VOID
  );

/**
  This function returns the ACPI table warning count.

  @retval Returns the count of warning detected in the ACPI tables.
**/
UINT32
GetWarningCount (
  VOID
  );

/**
  This function processes the table reporting options for the ACPI table.

  @param [in] Signature The ACPI table Signature.
  @param [in] TablePtr  Pointer to the ACPI table data.
  @param [in] Length    The length of the ACPI table.

  @retval Returns TRUE if the ACPI table should be traced.
**/
BOOLEAN
ProcessTableReportOptions (
  IN CONST UINT32  Signature,
  IN CONST UINT8*  TablePtr,
  IN CONST UINT32  Length
  );

/**
  This function iterates the configuration table entries in the
  system table, retrieves the RSDP pointer and starts parsing the ACPI tables.

  @param [in] SystemTable Pointer to the EFI system table.

  @retval EFI_NOT_FOUND   The RSDP pointer was not found.
  @retval EFI_UNSUPPORTED The RSDP version was less than 2.
  @retval EFI_SUCCESS     The command was successful.
**/
EFI_STATUS
EFIAPI
AcpiView (
  IN EFI_SYSTEM_TABLE* SystemTable
  );

#endif // ACPIVIEW_H_
