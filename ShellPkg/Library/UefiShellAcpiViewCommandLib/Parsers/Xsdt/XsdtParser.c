/** @file
  XSDT table parser

  Copyright (c) 2016 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewLog.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/** An ACPI_PARSER array describing the ACPI XSDT table.
*/
STATIC CONST ACPI_PARSER XsdtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo)
};

/**
  Get the ACPI XSDT header info.
**/
CONST ACPI_DESCRIPTION_HEADER_INFO *
EFIAPI
GetAcpiXsdtHeaderInfo (VOID)
{
  return &AcpiHdrInfo;
}

/**
  This function parses the ACPI XSDT table and optionally traces the ACPI table fields.

  This function also performs validation of the XSDT table.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiXsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32        TableOffset;
  UINT64**      TablePointer;
  UINTN         EntryIndex;

  TableOffset = ParseAcpi (
    Trace, 0, "XSDT", Ptr, AcpiTableLength, PARSER_PARAMS (XsdtParser));

  EntryIndex = 0;
  if (Trace) {
    for (TablePointer = (UINT64 **)(Ptr + TableOffset);
         (UINT8 *) TablePointer < Ptr + AcpiTableLength;
         TablePointer++) {

      CONST UINT32* Signature;
      CONST UINT32* Length;
      CONST UINT8*  Revision;

      if (*TablePointer != NULL) {
        ParseAcpiHeader (*TablePointer, &Signature, &Length, &Revision);
        PrintFieldName (2, L"Entry[%d] - %.4a", EntryIndex++, Signature);
        AcpiInfo (L"0x%lx", *TablePointer);
      } else {
        PrintFieldName (2, L"Entry[%d]", EntryIndex++);
        AcpiInfo (L"NULL");
        AcpiError (ACPI_ERROR_VALUE, L"Invalid table entry");
      }
    }
  }

  // Process the tables
  for (TablePointer = (UINT64 **)(Ptr + TableOffset);
       (UINT8 *) TablePointer < Ptr + AcpiTableLength;
       TablePointer++) {

    if (*TablePointer != NULL) {
      ProcessAcpiTable (*TablePointer);
    }

  }
}
