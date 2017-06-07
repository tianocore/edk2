/** @file
*
*  Copyright (c) 2016 - 2017, ARM Limited. All rights reserved.
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

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"

// Local Variables
STATIC CONST UINT64* SlitSystemLocalityCount;


/** An ACPI_PARSER array describing the ACPI SLIT table.

**/
STATIC CONST ACPI_PARSER SlitParser[] = {
  PARSE_ACPI_HEADER (NULL, NULL, NULL),
  {L"Number of System Localities", 8, 36, L"0x%lx", NULL,
   (VOID**)&SlitSystemLocalityCount, NULL, NULL}
};


/** Macro to get the value of a System Locality

**/
#define SLIT_ELEMENT(Ptr, i, j) *(Ptr + (i * LocalityCount) + j)


/** This function parses the ACPI SLIT table.
  This function parses the SLIT table and optionally traces the ACPI
  table fields.

  This function also validates System Localities for the following:
    - Diagonal elements have a normalized value of 10
    - Relative distance from System Locality at i*N+j is same as
      j*N+i

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiSlit (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT64 i;
  UINT64 j;
  UINT64 LocalityCount;
  UINT8* LocalityPtr;
  CHAR16 Buffer[80];  // Used for AsciiName param of ParseAcpi

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
  LocalityPtr = Ptr + Offset;

  LocalityCount = *SlitSystemLocalityCount;
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
    for (j = 0; j < LocalityCount; j++) {
      Print (L" (%3d) ", j);
    }
    Print (L"\n");
    for (i = 0; i < LocalityCount; i++) {
      Print (L" (%3d) ", i);
      for (j = 0; j < LocalityCount; j++) {
        Print (L"  %3d  ", SLIT_ELEMENT (LocalityPtr, i, j));
      }
      Print (L"\n");
    }
  }
  // Validate
  for (i = 0; i < LocalityCount; i++) {
    for (j = 0; j < LocalityCount; j++) {
      // Element[x][x] must be equal to 10
      if ((i == j) && (SLIT_ELEMENT (LocalityPtr, i, j) != 10)) {
        IncrementErrorCount ();
        Print (
          L"ERROR: Diagonal Element[0x%lx][0x%lx] (%3d)."
            " Normalized Value is not 10\n",
          i,
          j,
          SLIT_ELEMENT (LocalityPtr, i, j)
          );
      }
      // Element[i][j] must be equal to Element[j][i]
      if (SLIT_ELEMENT (LocalityPtr, i, j) !=
          SLIT_ELEMENT (LocalityPtr, j, i)) {
        IncrementErrorCount ();
        Print (
          L"ERROR: Relative distances for Element[0x%lx][0x%lx] (%3d) and \n"
           "Element[0x%lx][0x%lx] (%3d) do not match.\n",
          i,
          j,
          SLIT_ELEMENT (LocalityPtr, i, j),
          j,
          i,
          SLIT_ELEMENT (LocalityPtr, j, i)
          );
      }
    }
  }

}
