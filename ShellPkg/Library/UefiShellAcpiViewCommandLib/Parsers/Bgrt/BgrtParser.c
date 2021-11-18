/** @file
  BGRT table parser

  Copyright (c) 2017 - 2018, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI BDRT Table.
**/
STATIC CONST ACPI_PARSER  BgrtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Version",                    2,  36, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Status",                     1,  38, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Image Type",                 1,  39, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Image Address",              8,  40, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Image Offset X",             4,  48, L"%d",    NULL, NULL, NULL, NULL },
  { L"Image Offset Y",             4,  52, L"%d",    NULL, NULL, NULL, NULL }
};

/**
  This function parses the ACPI BGRT table.
  When trace is enabled this function parses the BGRT table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiBgrt (
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
    Trace,
    0,
    "BGRT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (BgrtParser)
    );
}
