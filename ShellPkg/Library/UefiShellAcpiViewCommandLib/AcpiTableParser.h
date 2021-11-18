/** @file
  Header file for ACPI table parser

  Copyright (c) 2016 - 2020, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPITABLEPARSER_H_
#define ACPITABLEPARSER_H_

/**
  The maximum number of ACPI table parsers.
*/
#define MAX_ACPI_TABLE_PARSERS  32

/** An invalid/NULL signature value.
*/
#define ACPI_PARSER_SIGNATURE_NULL  0

/**
  A function that parses the ACPI table.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
typedef
VOID
(EFIAPI *PARSE_ACPI_TABLE_PROC)(
  IN BOOLEAN Trace,
  IN UINT8 *Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  The ACPI table parser information
**/
typedef struct AcpiTableParser {
  /// ACPI table signature
  UINT32                   Signature;

  /// The ACPI table parser function.
  PARSE_ACPI_TABLE_PROC    Parser;
} ACPI_TABLE_PARSER;

/**
  Register the ACPI table Parser

  This function registers the ACPI table parser.

  @param [in] Signature   The ACPI table signature.
  @param [in] ParserProc  The ACPI table parser.

  @retval EFI_SUCCESS           The parser is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The parser for the Table
                                was already registered.
  @retval EFI_OUT_OF_RESOURCES  No space to register the
                                parser.
**/
EFI_STATUS
EFIAPI
RegisterParser (
  IN  UINT32                 Signature,
  IN  PARSE_ACPI_TABLE_PROC  ParserProc
  );

/**
  Deregister the ACPI table Parser

  This function deregisters the ACPI table parser.

  @param [in] Signature   The ACPI table signature.

  @retval EFI_SUCCESS           The parser was deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         A registered parser was not found.
**/
EFI_STATUS
EFIAPI
DeregisterParser (
  IN  UINT32  Signature
  );

/**
  This function processes the ACPI tables.
  This function calls ProcessTableReportOptions() to list the ACPI
  tables, perform binary dump of the tables and determine if the
  ACPI fields should be traced.

  This function also invokes the parser for the ACPI tables.

  This function also performs a RAW dump of the ACPI table including
  the unknown/unparsed ACPI tables and validates the checksum.

  @param [in] Ptr                Pointer to the start of the ACPI
                                 table data buffer.
**/
VOID
EFIAPI
ProcessAcpiTable (
  IN UINT8  *Ptr
  );

/**
  Get the ACPI table Parser

  This function returns the ACPI table parser proc from the list of
  registered parsers.

  @param [in]  Signature   The ACPI table signature.
  @param [out] ParserProc  Pointer to a ACPI table parser proc.

  @retval EFI_SUCCESS           The parser was returned successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         A registered parser was not found.
**/
EFI_STATUS
EFIAPI
GetParser (
  IN  UINT32                 Signature,
  OUT PARSE_ACPI_TABLE_PROC  *ParserProc
  );

#endif // ACPITABLEPARSER_H_
