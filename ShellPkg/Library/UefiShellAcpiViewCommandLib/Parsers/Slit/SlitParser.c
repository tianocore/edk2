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

// Local Variables
STATIC CONST UINT64                  *SlitSystemLocalityCount;
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI SLIT table.
**/
STATIC CONST ACPI_PARSER  SlitParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Number of System Localities",   8,     36, L"0x%lx", NULL,
    (VOID **)&SlitSystemLocalityCount,NULL,  NULL }
};

/**
  Macro to get the value of a System Locality
**/
#define SLIT_ELEMENT(Ptr, i, j)  *(Ptr + (i * LocalityCount) + j)

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
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;
  UINT32  Count;
  UINT32  Index;
  UINT32  LocalityCount;
  UINT8   *LocalityPtr;
  CHAR16  Buffer[80]; // Used for AsciiName param of ParseAcpi

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
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient table length. AcpiTableLength = %d.\n",
      AcpiTableLength
      );
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
  if (*SlitSystemLocalityCount > MAX_UINT16) {
    IncrementErrorCount ();
    Print (
      L"ERROR: The Number of System Localities provided can't be represented " \
      L"in the SLIT table. SlitSystemLocalityCount = %ld. " \
      L"MaxLocalityCountAllowed = %d.\n",
      *SlitSystemLocalityCount,
      MAX_UINT16
      );
    return;
  }

  LocalityCount = (UINT32)*SlitSystemLocalityCount;

  // Make sure system localities fit in the table buffer provided
  if (Offset + (LocalityCount * LocalityCount) > AcpiTableLength) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Number of System Localities. " \
      L"SlitSystemLocalityCount = %ld. AcpiTableLength = %d.\n",
      *SlitSystemLocalityCount,
      AcpiTableLength
      );
    return;
  }

  LocalityPtr = Ptr + Offset;

  // We only print the Localities if the count is less than 16
  // If the locality count is more than 16 then refer to the
  // raw data dump.
  if (LocalityCount < 16) {
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"Entry[0x%lx][0x%lx]",
      LocalityCount,
      LocalityCount
      );
    PrintFieldName (0, Buffer);
    Print (L"\n");
    Print (L"       ");
    for (Index = 0; Index < LocalityCount; Index++) {
      Print (L" (%3d) ", Index);
    }

    Print (L"\n");
    for (Count = 0; Count < LocalityCount; Count++) {
      Print (L" (%3d) ", Count);
      for (Index = 0; Index < LocalityCount; Index++) {
        Print (L"  %3d  ", SLIT_ELEMENT (LocalityPtr, Count, Index));
      }

      Print (L"\n");
    }
  }

  // Validate
  for (Count = 0; Count < LocalityCount; Count++) {
    for (Index = 0; Index < LocalityCount; Index++) {
      // Element[x][x] must be equal to 10
      if ((Count == Index) && (SLIT_ELEMENT (LocalityPtr, Count, Index) != 10)) {
        IncrementErrorCount ();
        Print (
          L"ERROR: Diagonal Element[0x%lx][0x%lx] (%3d)."
          L" Normalized Value is not 10\n",
          Count,
          Index,
          SLIT_ELEMENT (LocalityPtr, Count, Index)
          );
      }

      // Element[i][j] must be equal to Element[j][i]
      if (SLIT_ELEMENT (LocalityPtr, Count, Index) !=
          SLIT_ELEMENT (LocalityPtr, Index, Count))
      {
        IncrementErrorCount ();
        Print (
          L"ERROR: Relative distances for Element[0x%lx][0x%lx] (%3d) and \n"
          L"Element[0x%lx][0x%lx] (%3d) do not match.\n",
          Count,
          Index,
          SLIT_ELEMENT (LocalityPtr, Count, Index),
          Index,
          Count,
          SLIT_ELEMENT (LocalityPtr, Index, Count)
          );
      }
    }
  }
}
