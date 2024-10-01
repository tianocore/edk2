/** @file
  RAS2 table parser

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

// Local variables
STATIC CONST UINT16  *Ras2PccDescriptors;

STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI RAS2 Table.
*/
STATIC CONST ACPI_PARSER  Ras2Parser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Reserved",                   2,  36, L"0x%x", NULL, NULL,                         NULL, NULL },
  { L"PCC Descriptors",            2,  38, L"%d",   NULL, (VOID **)&Ras2PccDescriptors, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the RAS2 PCC ID Entry
*/
STATIC CONST ACPI_PARSER  Ras2StructurePccDescriptor[] = {
  { L"PCC ID",       1, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",     1, 1, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",     1, 2, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Feature Type", 1, 3, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Instance",     4, 4, L"0x%x", NULL, NULL, NULL, NULL }
};

STATIC
VOID
DumpPccEntry (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "PCC Descriptor Entry",
    Ptr,
    Length,
    PARSER_PARAMS (Ras2StructurePccDescriptor)
    );
}

/**
  This function parses the ACPI RAS2 table.
  When trace is enabled this function parses the RAS2 table and
  traces the ACPI table fields.

  This function parses the following RAS2 structures:
    - Pcc Instries
    - Entry Pcc ID
    - Entry Feature Type
    - Entry Pcc Instance

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiRas2 (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;

  if (!Trace) {
    return;
  }

  // Parse ACPI Header + RAS2 "fixed" fields
  Offset = ParseAcpi (
             Trace,
             0,
             "RAS2",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (Ras2Parser)
             );

  // Table is too small to contain data
  if (Offset >= AcpiTableLength) {
    return;
  }

  // Loop over rest of table for PCC Entries and dump them
  while (Offset <= (AcpiTableLength - sizeof (EFI_ACPI_RAS2_PCC_DESCRIPTOR))) {
    DumpPccEntry (
      Ptr + Offset,
      sizeof (EFI_ACPI_RAS2_PCC_DESCRIPTOR)
      );
    Offset += sizeof (EFI_ACPI_RAS2_PCC_DESCRIPTOR);
  } // while
}
