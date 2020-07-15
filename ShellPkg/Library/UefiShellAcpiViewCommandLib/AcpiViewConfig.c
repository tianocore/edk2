/** @file
  State and accessors for 'acpiview' configuration.

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "AcpiViewConfig.h"

// Report variables
STATIC BOOLEAN        mConsistencyCheck;
STATIC BOOLEAN        mColourHighlighting;
STATIC EREPORT_OPTION mReportType;
STATIC BOOLEAN        mMandatoryTableValidate;
STATIC UINTN          mMandatoryTableSpec;

// User selection of which ACPI table should be checked
SELECTED_ACPI_TABLE mSelectedAcpiTable;

/**
 Reset the AcpiView user configuration to defaults
**/
VOID
EFIAPI
AcpiConfigSetDefaults (
  VOID
  )
{
  mReportType = ReportAll;
  mSelectedAcpiTable.Type = 0;
  mSelectedAcpiTable.Name = NULL;
  mSelectedAcpiTable.Found = FALSE;
  mConsistencyCheck = TRUE;
  mMandatoryTableValidate = FALSE;
  mMandatoryTableSpec = 0;
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
SelectAcpiTable (
  IN CONST CHAR16 *TableName
  )
{
  ASSERT (TableName != NULL);

  mSelectedAcpiTable.Name = TableName;
  mSelectedAcpiTable.Type = ConvertStrToAcpiSignature (mSelectedAcpiTable.Name);
}

/**
  This function returns the selected ACPI table.

  @param [out] SelectedAcpiTable Pointer that will contain the returned struct.
**/
VOID
EFIAPI
GetSelectedAcpiTable (
  OUT SELECTED_ACPI_TABLE **SelectedAcpiTable
  )
{
  *SelectedAcpiTable = &mSelectedAcpiTable;
}

/**
  This function returns the colour highlighting status.

  @retval TRUE Colour highlighting is enabled.
**/
BOOLEAN
EFIAPI
GetColourHighlighting (
  VOID
  )
{
  return mColourHighlighting;
}

/**
  This function sets the colour highlighting status.

  @param [in] Highlight The highlight status.
**/
VOID
EFIAPI
SetColourHighlighting (
  BOOLEAN Highlight
  )
{
  mColourHighlighting = Highlight;
}

/**
  This function returns the consistency checking status.

  @retval TRUE Consistency checking is enabled.
**/
BOOLEAN
EFIAPI
GetConsistencyChecking (
  VOID
  )
{
  return mConsistencyCheck;
}

/**
  This function sets the consistency checking status.

  @param [in] ConsistencyChecking   The consistency checking status.
**/
VOID
EFIAPI
SetConsistencyChecking (
  BOOLEAN ConsistencyChecking
  )
{
  mConsistencyCheck = ConsistencyChecking;
}

/**
  This function returns the report options.

  @return The current report option.
**/
EREPORT_OPTION
EFIAPI
GetReportOption (
  VOID
  )
{
  return mReportType;
}

/**
  This function sets the report options.

  @param [in] ReportType The report option to set.
**/
VOID
EFIAPI
SetReportOption (
  EREPORT_OPTION ReportType
  )
{
  mReportType = ReportType;
}

/**
  This function returns the ACPI table requirements validation flag.

  @retval TRUE Check for mandatory table presence should be performed.
**/
BOOLEAN
EFIAPI
GetMandatoryTableValidate (
  VOID
  )
{
  return mMandatoryTableValidate;
}

/**
  This function sets the ACPI table requirements validation flag.

  @param [in] Validate Enable/Disable ACPI table requirements validation.
**/
VOID
EFIAPI
SetMandatoryTableValidate (
  BOOLEAN Validate
  )
{
  mMandatoryTableValidate = Validate;
}

/**
  This function returns the identifier of specification to validate ACPI table
  requirements against.

  @return ID of specification listing mandatory tables.
**/
UINTN
EFIAPI
GetMandatoryTableSpec (
  VOID
  )
{
  return mMandatoryTableSpec;
}

/**
  This function sets the identifier of specification to validate ACPI table
  requirements against.

  @param [in] Spec ID of specification listing mandatory tables.
**/
VOID
EFIAPI
SetMandatoryTableSpec (
  UINTN Spec
  )
{
  mMandatoryTableSpec = Spec;
}
