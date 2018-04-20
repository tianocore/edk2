/**
  ACPI table parser

  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"

/** A list of registered ACPI table parsers.
*/
STATIC ACPI_TABLE_PARSER mTableParserList[MAX_ACPI_TABLE_PARSERS];

/** Register the ACPI table Parser

  This function registers the ACPI table parser.

  @param [in] Signature   The ACPI table signature.
  @param [in] ParserProc  The ACPI table parser.

  @retval EFI_SUCCESS           The parser is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The parser for the Table
                                was already registered.
  @retval EFI_OUT_OF_RESOURCES  No space to register the
                                parser.
*/
EFI_STATUS
EFIAPI
RegisterParser (
  IN  UINT32                  Signature,
  IN  PARSE_ACPI_TABLE_PROC   ParserProc
  )
{
  UINT32 index;

  if ((ParserProc == NULL) || (Signature == ACPI_PARSER_SIGNATURE_NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Search if a parser is already installed
  for (index = 0;
       index < (sizeof (mTableParserList) / sizeof (mTableParserList[0]));
       index++)
  {
    if (Signature == mTableParserList[index].Signature) {
      if (mTableParserList[index].Parser != NULL) {
        return EFI_ALREADY_STARTED;
      }
    }
  }

  // Find the first free slot and register the parser
  for (index = 0;
      index < (sizeof (mTableParserList) / sizeof (mTableParserList[0]));
      index++)
  {
    if (mTableParserList[index].Signature == ACPI_PARSER_SIGNATURE_NULL) {
      mTableParserList[index].Signature = Signature;
      mTableParserList[index].Parser = ParserProc;
      return EFI_SUCCESS;
    }
  }

  // No free slot found
  return EFI_OUT_OF_RESOURCES;
}

/** Deregister the ACPI table Parser

  This function deregisters the ACPI table parser.

  @param [in] Signature   The ACPI table signature.

  @retval EFI_SUCCESS           The parser was deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         A registered parser was not found.
*/
EFI_STATUS
EFIAPI
DeregisterParser (
  IN  UINT32                  Signature
  )
{
  UINT32 index;

  if (Signature == ACPI_PARSER_SIGNATURE_NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (index = 0;
       index < (sizeof (mTableParserList) / sizeof (mTableParserList[0]));
       index++)
  {
    if (Signature == mTableParserList[index].Signature) {
      mTableParserList[index].Signature = ACPI_PARSER_SIGNATURE_NULL;
      mTableParserList[index].Parser = NULL;
      return EFI_SUCCESS;
    }
  }

  // No matching registered parser found.
  return EFI_NOT_FOUND;
}

/** Get the ACPI table Parser

  This function returns the ACPI table parser proc from the list of
  registered parsers.

  @param [in]  Signature   The ACPI table signature.
  @param [out] ParserProc  Pointer to a ACPI table parser proc.

  @retval EFI_SUCCESS           The parser was returned successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         A registered parser was not found.
*/
EFI_STATUS
EFIAPI
GetParser (
  IN  UINT32                   Signature,
  OUT PARSE_ACPI_TABLE_PROC *  ParserProc
  )
{
  UINT32 index;

  if ((ParserProc == NULL) || (Signature == ACPI_PARSER_SIGNATURE_NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (index = 0;
       index < (sizeof (mTableParserList) / sizeof (mTableParserList[0]));
       index++)
  {
    if (Signature == mTableParserList[index].Signature) {
      *ParserProc = mTableParserList[index].Parser;
      return EFI_SUCCESS;
    }
  }

  // No matching registered parser found.
  return EFI_NOT_FOUND;
}

/** This function processes the ACPI tables.
  This function calls ProcessTableReportOptions() to list the ACPI
  tables, perform binary dump of the tables and determine if the
  ACPI fields should be traced.

  This function also invokes the parser for the ACPI tables.

  This function also performs a RAW dump of the ACPI table including
  the unknown/unparsed ACPI tables and validates the checksum.

  @param [in] Ptr                Pointer to the start of the ACPI
                                 table data buffer.
*/
VOID
EFIAPI
ProcessAcpiTable (
  IN UINT8* Ptr
  )
{
  EFI_STATUS    Status;
  BOOLEAN       Trace;
  CONST UINT32* AcpiTableSignature;
  CONST UINT32* AcpiTableLength;
  CONST UINT8*  AcpiTableRevision;
  PARSE_ACPI_TABLE_PROC ParserProc;

  ParseAcpiHeader (
    Ptr,
    &AcpiTableSignature,
    &AcpiTableLength,
    &AcpiTableRevision
    );

  Trace = ProcessTableReportOptions (
            *AcpiTableSignature,
            Ptr,
            *AcpiTableLength
            );

  if (Trace) {
    DumpRaw (Ptr, *AcpiTableLength);
    VerifyChecksum (TRUE, Ptr, *AcpiTableLength);
  }

  Status = GetParser (*AcpiTableSignature, &ParserProc);
  if (EFI_ERROR (Status)) {
    // No registered parser found, do default handling.
    if (Trace) {
      DumpAcpiHeader (Ptr);
    }
    return;
  }

  ParserProc (
    Trace,
    Ptr,
    *AcpiTableLength,
    *AcpiTableRevision
    );
}
