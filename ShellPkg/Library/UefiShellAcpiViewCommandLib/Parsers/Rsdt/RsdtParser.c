/** @file
  RSDT table parser

  Copyright (c) 2016 - 2019, ARM Limited. All rights reserved.
  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/** An ACPI_PARSER array describing the ACPI RSDT table.
*/
STATIC CONST ACPI_PARSER  RsdtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo)
};

/**
  Get the ACPI RSDT header info.
**/
CONST ACPI_DESCRIPTION_HEADER_INFO *
EFIAPI
GetAcpiRsdtHeaderInfo (
  VOID
  )
{
  return &AcpiHdrInfo;
}

/**
  This function parses the ACPI RSDT table and optionally traces the ACPI table fields.

  This function also performs validation of the RSDT table.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiRsdt (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;
  UINT32  TableOffset;
  UINT32  *TablePointer;
  UINTN   EntryIndex;
  CHAR16  Buffer[32];

  Offset = ParseAcpi (
             Trace,
             0,
             "RSDT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (RsdtParser)
             );

  TableOffset = Offset;

  if (Trace) {
    EntryIndex   = 0;
    TablePointer = (UINT32 *)(Ptr + TableOffset);
    while (Offset < AcpiTableLength) {
      CONST UINT32  *Signature;
      CONST UINT32  *Length;
      CONST UINT8   *Revision;

      if ((UINT32 *)(UINTN)(*TablePointer) != NULL) {
        UINT8  *SignaturePtr;

        ParseAcpiHeader (
          (UINT8 *)(UINTN)(*TablePointer),
          &Signature,
          &Length,
          &Revision
          );

        SignaturePtr = (UINT8 *)Signature;

        UnicodeSPrint (
          Buffer,
          sizeof (Buffer),
          L"Entry[%d] - %c%c%c%c",
          EntryIndex++,
          SignaturePtr[0],
          SignaturePtr[1],
          SignaturePtr[2],
          SignaturePtr[3]
          );
      } else {
        UnicodeSPrint (
          Buffer,
          sizeof (Buffer),
          L"Entry[%d]",
          EntryIndex++
          );
      }

      PrintFieldName (2, Buffer);
      Print (L"0x%lx\n", *TablePointer);

      // Validate the table pointers are not NULL
      if ((UINT32 *)(UINTN)(*TablePointer) == NULL) {
        IncrementErrorCount ();
        Print (
          L"ERROR: Invalid table entry at 0x%lx, table address is 0x%lx\n",
          TablePointer,
          *TablePointer
          );
      }

      Offset += sizeof (UINT32);
      TablePointer++;
    } // while
  }

  // Process the tables
  Offset       = TableOffset;
  TablePointer = (UINT32 *)(Ptr + TableOffset);
  while (Offset < AcpiTableLength) {
    if ((UINT32 *)(UINTN)(*TablePointer) != NULL) {
      ProcessAcpiTable ((UINT8 *)(UINTN)(*TablePointer));
    }

    Offset += sizeof (UINT32);
    TablePointer++;
  } // while
}
