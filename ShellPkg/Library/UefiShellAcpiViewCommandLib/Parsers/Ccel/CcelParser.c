/** @file
  CCEL table parser

  Copyright (c) 2025, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.5 Specification - 29 Aug 2022
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewConfig.h"

// Local variables.
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  Print the CC Type.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
VOID
EFIAPI
PrintCcType (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  UINT8  CcType;

  CcType = *Ptr;
  switch (CcType) {
    case EFI_ACPI_6_5_CC_TYPE_NONE:
      Print (L"CC Type '%u' - None", CcType);
      break;
    case EFI_ACPI_6_5_CC_TYPE_SEV:
      Print (L"CC Type '%u' - SEV", CcType);
      break;
    case EFI_ACPI_6_5_CC_TYPE_TDX:
      Print (L"CC Type '%u' - TDX", CcType);
      break;
    case EFI_ACPI_6_5_CC_TYPE_APTEE:
      Print (L"CC Type '%u' - APTEE", CcType);
      break;
    case EFI_ACPI_6_5_CC_TYPE_ARMCCA:
      Print (L"CC Type '%u' - Arm CCA", CcType);
      break;
    default:
      Print (L"CC Type '%u' - Unknown", CcType);
  } // switch
}

/**
  This function validates CC Type field.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information.
**/
VOID
EFIAPI
ValidateCcType (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  CcType;

  CcType = *Ptr;

  if (CcType > EFI_ACPI_6_5_CC_TYPE_ARMCCA) {
    IncrementErrorCount ();
    Print (L"\nERROR : Invalid/Unknown CC Type - %u.\n", CcType);
  }
}

/**
  An ACPI_PARSER array describing the ACPI CCEL Table.
**/
STATIC CONST ACPI_PARSER  CcelParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"CC Type",                    1,  36, L"%d",    NULL,         NULL, NULL,             NULL },
  { L"CC Subtype",                 1,  37, L"%d",    PrintCcType,  NULL, ValidateCcType,   NULL },
  { L"Reserved",                   2,  38, NULL,     DumpReserved, NULL, ValidateReserved, NULL },
  { L"Log Area Minimum Length (LAML)",8,  40, L"0x%lx", NULL,         NULL, NULL,             NULL },
  { L"Log Area Start Address (LASA)",8,  48, L"0x%lx", NULL,         NULL, NULL,             NULL }
};

/**
  This function parses the ACPI CCEL table.
  When trace is enabled this function parses the CCEL table and
  traces the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiCcel (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  ParseAcpi (
    TRUE,
    0,
    "CCEL",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (CcelParser)
    );
}
