/** @file
  FACS table parser

  Copyright (c) 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

/**
  An ACPI_PARSER array describing the ACPI FACS Table.
**/
STATIC CONST ACPI_PARSER  FacsParser[] = {
  { L"Signature",                 4, 0,  L"%c%c%c%c",                Dump4Chars, NULL, NULL, NULL },
  { L"Length",                    4, 4,  L"%d",                      NULL,       NULL, NULL, NULL },
  { L"Hardware Signature",        4, 8,  L"0x%x",                    NULL,       NULL, NULL, NULL },
  { L"Firmware Waking Vector",    4, 12, L"0x%x",                    NULL,       NULL, NULL, NULL },
  { L"Global Lock",               4, 16, L"0x%x",                    NULL,       NULL, NULL, NULL },
  { L"Flags",                     4, 20, L"0x%x",                    NULL,       NULL, NULL, NULL },
  { L"X Firmware Walking Vector", 8, 24, L"0x%lx",                   NULL,       NULL, NULL, NULL },
  { L"Version",                   1, 32, L"%d",                      NULL,       NULL, NULL, NULL },
  { L"Reserved",                  3, 33, L"%x %x %x",                Dump3Chars, NULL, NULL, NULL },
  { L"OSPM Flags",                4, 36, L"0x%x",                    NULL,       NULL, NULL, NULL },
  { L"Reserved",                  8, 40, L"%x %x %x %x %x %x %x %x", Dump8Chars, NULL, NULL,
    NULL },
  { L"Reserved",                  8, 48, L"%x %x %x %x %x %x %x %x", Dump8Chars, NULL, NULL,
    NULL },
  { L"Reserved",                  8, 56, L"%x %x %x %x %x %x %x %x", Dump8Chars, NULL, NULL,
    NULL }
};

/**
  This function parses the ACPI FACS table.
  When trace is enabled this function parses the FACS table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiFacs (
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
    "FACS",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (FacsParser)
    );
}
