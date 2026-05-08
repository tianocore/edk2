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
  An ACPI_PARSER array describing the ACPI CCEL Table.
**/
STATIC CONST ACPI_PARSER  CcelParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"CC Type",                    1,  36, L"%d",    NULL, NULL, NULL, NULL },
  { L"CC Subtype",                 1,  37, L"%d",    NULL, NULL, NULL, NULL },
  { L"Reserved",                   2,  38, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Log Area Minimum Length (LAML)",8,  40, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Log Area Start Address (LASA)",8,  48, L"0x%lx", NULL, NULL, NULL, NULL }
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
