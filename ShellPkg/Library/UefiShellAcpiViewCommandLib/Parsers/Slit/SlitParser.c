/** @file
  SLIT table parser

  Copyright (c) 2016 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <IndustryStandard/Acpi.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewLog.h"

// Local Variables
STATIC CONST UINT64* SlitSystemLocalityCount;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI SLIT table.
**/
STATIC CONST ACPI_PARSER SlitParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"Number of System Localities", 8, 36, L"0x%lx", NULL,
   (VOID**)&SlitSystemLocalityCount, NULL, NULL}
};

/**
  Macro to get the value of a System Locality, simple
  2D variable size array item retrieval
**/
#define SLIT_ELEMENT(ArrayPtr, RowSize, Row, Column) \
  *((ArrayPtr) + ((Row) * (RowSize)) + (Column))

/**
  This function parses the ACPI SLIT table.
  When trace is enabled this function parses the SLIT table and
  traces the ACPI table fields.

  This function also validates System Localities for the following:
    - Diagonal elements have a normalized value of 10
    - Relative distance from System Locality at i*N+j is same as
      j*N+i

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSlit (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT32 Index1;
  UINT32 Index2;
  UINT32 LocalityCount;
  CHAR16 Buffer[256];
  UINTN  StrLen;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "SLIT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (SlitParser)
             );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if (SlitSystemLocalityCount == NULL) {
    AcpiError (ACPI_ERROR_PARSE, L"Failed to parse the SLIT table");
    return;
  }

  /*
    Despite the 'Number of System Localities' being a 64-bit field in SLIT,
    the maximum number of localities that can be represented in SLIT is limited
    by the 'Length' field of the ACPI table.

    Since the ACPI table length field is 32-bit wide. The maximum number of
    localities that can be represented in SLIT can be calculated as:

    MaxLocality = sqrt (MAX_UINT32 - sizeof (EFI_ACPI_6_3_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER))
                = 65535
                = MAX_UINT16
  */
  if (AssertConstraint (L"ACPI", *SlitSystemLocalityCount <= MAX_UINT16)) {
    return;
  }

  LocalityCount = (UINT32) *SlitSystemLocalityCount;

  // Make sure system localities fit in the table buffer provided
  if (AssertMemberIntegrity (
        Offset, (LocalityCount * LocalityCount), Ptr, AcpiTableLength)) {
    return;
  }

  // We only print the Localities if the count is less than 16
  // If the locality count is more than 16 then refer to the
  // raw data dump.
  if (LocalityCount < 16) {
    PrintFieldName (0, L"Entry[0x%lx][0x%lx]", LocalityCount, LocalityCount);
    AcpiInfo (L"");
    UnicodeSPrint (Buffer, sizeof (Buffer), L"       ");
    for (Index1 = 0; Index1 < LocalityCount; Index1++) {
      StrLen = StrnLenS (Buffer, sizeof (Buffer));
      UnicodeSPrint (
        Buffer + StrLen, sizeof (Buffer) - StrLen, L" (%3d) ", Index1);
    }
    AcpiInfo (L"%s", Buffer);

    for (Index1 = 0; Index1 < LocalityCount; Index1++) {
      UnicodeSPrint (Buffer, sizeof (Buffer), L" (%3d) ", Index1);
      for (Index2 = 0; Index2 < LocalityCount; Index2++) {
        StrLen = StrnLenS (Buffer, sizeof (Buffer));
        UnicodeSPrint (
          Buffer + StrLen,
          sizeof (Buffer) - StrLen,
          L"  %3d  ",
          SLIT_ELEMENT(Ptr + Offset, LocalityCount, Index1, Index2));
      }
      AcpiInfo (L"%s", Buffer);
    }
  }

  // Validate
  for (Index1 = 0; Index1 < LocalityCount; Index1++) {
    // Element[x][x] must be equal to 10
    if (SLIT_ELEMENT(Ptr + Offset, LocalityCount, Index1, Index1) != 10) {
      AcpiError (
        ACPI_ERROR_VALUE, L"SLIT Element[%d][%d] != 10", Index1, Index1);
    }
    for (Index2 = 0; Index2 < Index1; Index2++) {
      // Element[i][j] must be equal to Element[j][i]
      if (
        SLIT_ELEMENT(Ptr + Offset, LocalityCount, Index1, Index2) !=
        SLIT_ELEMENT(Ptr + Offset, LocalityCount, Index2, Index1)) {
        AcpiError (
          ACPI_ERROR_VALUE,
          L"SLIT Element[%d][%d] != SLIT Element[%d][%d]",
          Index1,
          Index2,
          Index2,
          Index1);
      }
    }
  }
}
