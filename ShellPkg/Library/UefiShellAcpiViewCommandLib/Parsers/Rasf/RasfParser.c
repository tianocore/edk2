/** @file
  RASF table parser

  Copyright (c) 2024, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.5 Specification - August 2022
**/

#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"

STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI RASF Table.
**/
STATIC CONST ACPI_PARSER  RasfParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"RASF PCC Identifier",        12,36, L"%02X %02X %02X %02X - %02X %02X %02X %02X - %02X %02X %02X %02X", Dump12Chars, NULL, NULL, NULL }
};

/**
  This function parses the ACPI RASF table.
  When trace is enabled this function parses the RASF table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiRasf (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  // Parse ACPI Header + RASF "fixed" fields
  ParseAcpi (
    Trace,
    0,
    "RASF",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (RasfParser)
    );
}
