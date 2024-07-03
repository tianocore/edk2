/** @file
  SPMI table parser

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - IPMI 2.0 Specification - October 2013
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  mAcpiHdrInfo;

/**
  Validate Interface Type.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateInterfaceType (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*Ptr > 5) {
    IncrementErrorCount ();
    Print (L"\nError: Invalid IPMI Interface type %d", *Ptr);
  }
}

/// An ACPI_PARSER array describing the ACPI SPMI table.
STATIC CONST ACPI_PARSER  SpmiParser[] = {
  PARSE_ACPI_HEADER (&mAcpiHdrInfo),
  { L"Interface Type",              1,   36, L"%d",   NULL,    NULL, ValidateInterfaceType, NULL },
  { L"Reserved",                    1,   37, L"%x",   NULL,    NULL, NULL,                  NULL },
  { L"Specification Revision",      2,   38, L"0x%x", NULL,    NULL, NULL,                  NULL },
  { L"Interrupt Type",              1,   40, L"%d",   NULL,    NULL, NULL,                  NULL },
  { L"GPE",                         1,   41, L"%d",   NULL,    NULL, NULL,                  NULL },
  { L"Reserved",                    1,   42, L"%x",   NULL,    NULL, NULL,                  NULL },
  { L"PCI Device Flag",             1,   43, L"0x%x", NULL,    NULL, NULL,                  NULL },
  { L"Global System Interrupt",     4,   44, L"0x%x", NULL,    NULL, NULL,                  NULL },
  { L"Base Address",                12,  48, NULL,    DumpGas, NULL, NULL,                  NULL },
  { L"PCI Segment Group No. / UID Byte 1",1,   60, L"0x%x", NULL,    NULL, NULL,                  NULL },
  { L"PCI Bus No. / UID Byte 2",    1,   61, L"0x%x", NULL,    NULL, NULL,                  NULL },
  { L"PCI Device No. / UID Byte 3", 1,   62, L"0x%x", NULL,    NULL, NULL,                  NULL },
  { L"PCI Function No. / UID Byte 4",1,   63, L"0x%x", NULL,    NULL, NULL,                  NULL },
  { L"Reserved",                    1,   64, L"%x",   NULL,    NULL, NULL,                  NULL }
};

/**
  This function parses the ACPI SPMI table.
  When trace is enabled this function parses the SPMI table and
  traces the ACPI table fields.

  This function also performs validations of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSpmi (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  // Dump the SPMI
  ParseAcpi (
    TRUE,
    0,
    "SPMI",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (SpmiParser)
    );
}
