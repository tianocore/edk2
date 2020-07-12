/** @file
  State and accessors for 'acpiview' configuration.

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "AcpiViewConfig.h"

// Main configuration structure
ACPI_VIEW_CONFIG mConfig;

// User selection of which ACPI table should be checked
SELECTED_ACPI_TABLE mSelectedAcpiTable;

/**
 Reset the AcpiView user configuration to defaults
**/
VOID
EFIAPI
AcpiViewConfigSetDefaults (
  VOID
  )
{
  // Generic defaults
  mConfig.ReportType = ReportAll;
  mConfig.ConsistencyCheck = TRUE;
  mConfig.MandatoryTableValidate = FALSE;
  mConfig.MandatoryTableSpec = 0;
  mConfig.ColourHighlighting = FALSE;

  // Unselect acpi table
  mSelectedAcpiTable.Type = 0;
  mSelectedAcpiTable.Name = NULL;
  mSelectedAcpiTable.Found = FALSE;
}

/**
  This function converts a string to ACPI table signature.

  @param [in] Str   Pointer to the string to be converted to the
                    ACPI table signature.

  @retval The ACPI table signature.
**/
STATIC
UINT32
ConvertStrToAcpiSignature (
  IN CONST CHAR16 *Str
  )
{
  UINT8 Index;
  CHAR8 Ptr[4];

  ZeroMem (Ptr, sizeof (Ptr));
  Index = 0;

  // Convert to Upper case and convert to ASCII
  while ((Index < 4) && (Str[Index] != 0)) {
    if (Str[Index] >= L'a' && Str[Index] <= L'z') {
      Ptr[Index] = (CHAR8)(Str[Index] - (L'a' - L'A'));
    } else {
      Ptr[Index] = (CHAR8)Str[Index];
    }
    Index++;
  }
  return *(UINT32 *) Ptr;
}

/**
  This function selects an ACPI table in current context.
  The string name of the table is converted into UINT32
  table signature.

  @param [in] TableName The name of the ACPI table to select.
**/
VOID
EFIAPI
AcpiViewConfigSelectTable (
  IN CONST CHAR16 *TableName
  )
{
  ASSERT (TableName != NULL);

  mSelectedAcpiTable.Name = TableName;
  mSelectedAcpiTable.Type = ConvertStrToAcpiSignature (mSelectedAcpiTable.Name);
}
