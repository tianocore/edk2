/** @file
  HPET table parser

  Copyright (c) 2024, Arm Limited. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - HPET spec, version 1.0a
**/

#include <Library/UefiLib.h>
#include "AcpiParser.h"

STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  This function prints HPET page protection flags.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpHpetPageProtectionFlag (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT8 *)Ptr);
    return;
  }

  Print (L"0x%X ", *(UINT8 *)Ptr);
  switch (*Ptr) {
    case 0:
      Print (L"(no guarantee for page protection)");
      break;
    case 1:
      Print (L"(4K page protection)");
      break;
    case 2:
      Print (L"(64K page protection)");
      break;
    default:
      IncrementErrorCount ();
      Print (L"(OEM Reserved)");
      break;
  }

  return;
}

/**
  An ACPI_PARSER array describing the ACPI HPET flags.
**/
STATIC CONST ACPI_PARSER  DumpHpetFlagParser[] = {
  { L"Page Protection Flag", 4, 0, NULL,    DumpHpetPageProtectionFlag, NULL, NULL, NULL },
  { L"OEM Attributes",       4, 4, L"0x%x", NULL,                       NULL, NULL, NULL }
};

/**
  This function prints HPET Flags fields.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpHpetFlag (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT8 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT8 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (DumpHpetFlagParser)
    );
}

/**
  This function prints HPET Counter size fields.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpCounterSize (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X ", *(UINT32 *)Ptr);
  if (*Ptr == 0) {
    Print (L"(Max 32-bit counter size)");
  } else {
    Print (L"(Max 64-bit counter size)");
  }
}

/**
  This function validates the flags.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateHpetRevId (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if ((*(UINT8 *)Ptr) == 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: HPET Hardware Rev ID must be set."
      );
  }
}

/**
  An ACPI_PARSER array describing the ACPI HPET Event Timer Block ID.
**/
STATIC CONST ACPI_PARSER  HpetEventTimerBlockIdFlagParser[] = {
  { L"Hardware Rev ID",                  8,  0,  L"0x%x", NULL,            NULL, ValidateHpetRevId, NULL },
  { L"Comparators in 1st Timer Block",   5,  8,  L"0x%x", NULL,            NULL, NULL,              NULL },
  { L"Counter max size",                 1,  13, NULL,    DumpCounterSize, NULL, NULL,              NULL },
  { L"Reserved",                         1,  14, L"%d",   NULL,            NULL, NULL,              NULL },
  { L"LegacyReplacement IRQ Routing",    1,  15, L"%d",   NULL,            NULL, NULL,              NULL },
  { L"PCI Vendor ID of 1st Timer Block", 16, 16, L"0x%x", NULL,            NULL, NULL,              NULL }
};

/**
  This function prints Hardware ID of HPET Event timer block.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpHpetEventTimerBlockId (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (HpetEventTimerBlockIdFlagParser)
    );
}

/**
  An ACPI_PARSER array describing the ACPI HPET Table.
**/
STATIC CONST ACPI_PARSER  HpetParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Event Timer Block ID",       4,   36, NULL,    DumpHpetEventTimerBlockId, NULL, NULL, NULL },
  { L"Base Address",               12,  40, NULL,    DumpGas,                   NULL, NULL, NULL },
  { L"HPET Number",                1,   52, L"0x%x", NULL,                      NULL, NULL, NULL },
  { L"Minimum Clock Ticks",        2,   53, L"0x%x", NULL,                      NULL, NULL, NULL },
  { L"Page Protection and OEM Attributes",1,   55, NULL,    DumpHpetFlag,              NULL, NULL, NULL }
};

/**
  This function parses the ACPI HPET table.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiHpet (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  ParseAcpi (
    Trace,
    0,
    "HPET",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (HpetParser)
    );
}
