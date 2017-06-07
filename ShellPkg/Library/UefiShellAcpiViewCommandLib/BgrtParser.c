/** @file
*
*  Copyright (c) 2017, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiTableParser.h"

/** An ACPI_PARSER array describing the ACPI BDRT Table.

**/
STATIC CONST ACPI_PARSER BgrtParser[] = {
  PARSE_ACPI_HEADER (NULL, NULL, NULL),
  {L"Version", 2, 36, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Status", 1, 38, L"0x%x", NULL,  NULL, NULL, NULL},
  {L"Image Type", 1, 39, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Image Address", 8, 40, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Image Offset X", 4, 48, L"%d", NULL, NULL, NULL, NULL},
  {L"Image Offset Y", 4, 52, L"%d", NULL, NULL, NULL, NULL}
};


/** This function parses the ACPI BGRT table.
  This function parses the BGRT table and optionally traces the ACPI
  table fields.

  This function also parses the ACPI header for the DSDT table and
  invokes the parser for the ACPI DSDT table.

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiBgrt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  ParseAcpi (Trace, 0, "BGRT", Ptr, AcpiTableLength, PARSER_PARAMS (BgrtParser));
}
