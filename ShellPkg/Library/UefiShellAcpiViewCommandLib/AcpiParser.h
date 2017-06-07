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

#ifndef ACPIPARSER_H_
#define ACPIPARSER_H_

#define OUTPUT_FIELD_COLUMN_WIDTH  36

/** This function prints a GUID to STDOUT.

  @params [in] Guid    Pointer to a GUID to print.

  @retval EFI_SUCCESS             The GUID was printed.
  @retval EFI_INVALID_PARAMETER   The input was NULL.
**/
EFI_STATUS
PrintGuid (
  IN EFI_GUID* Guid
  );

/** This function verifies the ACPI table checksum.

  This function verifies the checksum for the ACPI table and optionally
  prints the status.

  @params [in] Log     If TRUE log the status of the checksum.
  @params [in] Ptr     Pointer to the start of the table buffer.
  @params [in] Length  The length of the buffer.

  @retval TRUE         The checksum is OK.
  @retval FALSE        The checksum failed.
**/
BOOLEAN
VerifyChecksum (
  IN BOOLEAN Log,
  IN UINT8*  Ptr,
  IN UINT32  Length
  );

/** This function performs a raw data dump of the ACPI table.

  @params [in] Ptr     Pointer to the start of the table buffer.
  @params [in] Length  The length of the buffer.

**/
VOID
DumpRaw (
  IN UINT8* Ptr,
  IN UINT32 Length
  );

/** This function traces 1 byte of datum as specified in the
    format string.

  @params [in] Format  The format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
DumpUint8 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );


/** This function traces 2 bytes of data as specified in the
    format string.

  @params [in] Format  The format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
DumpUint16 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/** This function traces 4 bytes of data as specified in the
    format string.

  @params [in] Format  The format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/

VOID
DumpUint32 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/** This function traces 8 bytes of data as specified by the
    format string.

  @params [in] Format  The format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
DumpUint64 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );


/** This function traces 3 characters which can be optionally
   formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @params [in] Format  Optional format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
Dump3Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/** This function traces 4 characters which can be optionally
   formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @params [in] Format  Optional format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
Dump4Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/** This function traces 6 characters which can be optionally
   formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @params [in] Format  Optional format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
Dump6Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/** This function traces 8 characters which can be optionally
   formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @params [in] Format  Optional format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
Dump8Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );


/** This function indents and prints the ACPI table Field Name.

  @params [in] Indent      Number of spaces to add to the global table indent. The
                           global table indent is 0 by defautl; however this value is
                           is updated on entry to the ParseAcpi() by adding the indent
                           value provided to ParseAcpi() and restored back on exit.
                           Therefore the total indent in the output is dependent on from
                           where this function is called.
  @params [in] FieldName   Pointer to the Field Name.

**/
VOID
PrintFieldName (
  IN UINT32         Indent,
  IN CONST CHAR16*  FieldName
);

/** This function pointer is the template for customizing the trace output

  @params [in] Format  Format string for tracing the data as specified by
                       the 'Format' member of ACPI_PARSER.
  @params [in] Ptr     Pointer to the start of the buffer.

**/

typedef VOID (*FNPTR_PRINT_FORMATTER)(CONST CHAR16* Format, UINT8* Ptr);

/** This function pointer is the template for validating an ACPI table field.

  @params [in] Ptr     Pointer to the start of the field data.
  @params [in] Context Pointer to context specific information as specified by
                       the 'Context' member of the ACPI_PARSER.
                       e.g. this could be a pointer to the ACPI table header.

**/
typedef VOID (*FNPTR_FIELD_VALIDATOR)(UINT8* Ptr, VOID* Context);

/** The ACPI_PARSER structure describes the fields of an ACPI table and
    provides means for the parser to interpret and trace appropriately.

  The first three members are populated based on information present in
  in the ACPI table specifications. The remaining members describe how
  the parser should report the field information, validate the field data
  and/or update an external pointer to the field (ItemPtr).

  ParseAcpi() uses the format string specified by 'Format' for tracing
  the field data. If the field is more complex and requires additional
  processing for formatting and representation a print formatter function
  can be specified in 'PrintFormatter'.
  The PrintFormatter function may choose to use the format string
  specified by 'Format' or use its own internal format string.

  The 'Format' and 'PrintFormatter' members allow flexibility for
  representing the field data.

**/
typedef struct AcpiParser {

  /// String describing the ACPI table field
  /// (Field column from ACPI table spec)
  CONST CHAR16*         NameStr;

  /// The length of the field.
  /// (Byte Length column from ACPI table spec)
  UINT32                Length;

  /// The offset of the field from the start of the table.
  /// (Byte Offset column from ACPI table spec)
  UINT32                Offset;

  /// Optional Print() style format string for tracing the data. If not
  /// used this must be set to NULL.
  CONST CHAR16*         Format;

  /// Optional pointer to a print formatter function which
  /// is typically used to trace complex field information.
  /// If not used this must be set to NULL.
  /// The Format string is passed to the PrintFormatter function
  /// but may be ignored by the implementation code.
  FNPTR_PRINT_FORMATTER PrintFormatter;

  /// Optional pointer which may be set to request the parser to update
  /// a pointer to the field data. If unused this must be set to NULL.
  VOID**                ItemPtr;

  /// Optional pointer to a field validator function.
  /// The function should directly report any appropriate error or warning
  /// and invoke the appropriate counter update function.
  /// If not used this parameter must be set to NULL.
  FNPTR_FIELD_VALIDATOR FieldValidator;

  /// Optional pointer to context specific information,
  /// which the Field Validator function can use to determine
  /// additional information about the ACPI table and make
  /// decisions about the field being validated.
  /// e.g. this could be a pointer to the ACPI table header
  VOID*                 Context;
} ACPI_PARSER;

/** This function is used to parse an ACPI table buffer.

  The ACPI table buffer is parsed using the ACPI table parser information
  specified by a pointer to an array of ACPI_PARSER elements. This parser
  function iterates through each item on the ACPI_PARSER array and logs the
  ACPI table fields.

  This function can optionally be used to parse ACPI tables and fetch specific
  field values. The ItemPtr member of the ACPI_PARSER structure (where used)
  is updated by this parser function to point to the selected field data
  (e.g. useful for variable length nested fields).

  @params [in] Trace        Trace the ACPI fields TRUE else only parse the
                            table.
  @params [in] Indent       Number of spaces to indent the output.
  @params [in] AsciiName    Optional pointer to an ASCII string that describes
                            the table being parsed.
  @params [in] Ptr          Pointer to the start of the buffer.
  @params [in] Length       Length of the buffer pointed by Ptr.
  @params [in] Parser       Pointer to an array of ACPI_PARSER structure that
                            describes the table being parsed.
  @params [in] ParserItems  Number of items in the ACPI_PARSER array.

  @retval Number of bytes parsed.
**/
UINT32
ParseAcpi (
  IN BOOLEAN            Trace,
  IN UINT32             Indent,
  IN CONST CHAR8*       AsciiName OPTIONAL,
  IN UINT8*             Ptr,
  IN UINT32             Length,
  IN CONST ACPI_PARSER* Parser,
  IN UINT32             ParserItems
);

/** This is a helper macro to pass parameters to the Parser functions.

  @params [in] Parser The name of the ACPI_PARSER array describing the
               ACPI table fields.
**/

#define PARSER_PARAMS(Parser) Parser, sizeof (Parser) / sizeof (Parser[0])


/** This is a helper macro for describing the ACPI header fields.

  @params [out] Signature  Pointer to retrieve the ACPI table signature.
  @params [out] Length     Pointer to retrieve the ACPI table length.
  @params [out] Revision   Pointer to retrieve the ACPI table revision.
**/

#define PARSE_ACPI_HEADER(Signature, Length, Revision)                        \
  { L"Signature", 4, 0, NULL, Dump4Chars, (VOID**)Signature , NULL, NULL }, \
  { L"Length", 4, 4, L"%d", NULL, (VOID**)Length, NULL, NULL },             \
  { L"Revision", 1, 8, L"%d", NULL, (VOID**)Revision, NULL, NULL },         \
  { L"Checksum", 1, 9, L"0x%X", NULL, NULL, NULL, NULL },                   \
  { L"Oem ID", 6, 10, NULL, Dump6Chars, NULL, NULL, NULL },                 \
  { L"Oem Table ID", 8, 16, NULL, Dump8Chars, NULL, NULL, NULL },           \
  { L"Oem Revision", 4, 24, L"0x%x", NULL, NULL, NULL, NULL },                \
  { L"Creator ID", 4, 28, NULL, Dump4Chars, NULL, NULL, NULL },             \
  { L"Creator Revision", 4, 32, L"0x%x", NULL, NULL, NULL, NULL }


/** Length of the ACPI GAS structure.

  NOTE: This might normally be defined as
            sizeof (EFI_ACPI_6_1_GENERIC_ADDRESS_STRUCTURE).
        However, we deliberately minimise any reference to the EDK2 ACPI
        headers in an attempt to provide cross checking.
**/
#define GAS_LENGTH                     12


/** Length of the ACPI Header structure.

  NOTE: This might normally be defined as
            sizeof (EFI_ACPI_DESCRIPTION_HEADER).
        However, we deliberately minimise any reference to the EDK2 ACPI
        headers in an attempt to provide cross checking.
**/
#define ACPI_DESCRIPTION_HEADER_LENGTH  36

/** This function indents and traces the GAS structure as described
    by the GasParser.

  @params [in] Ptr     Pointer to the start of the buffer.
  @params [in] Indent  Number of spaces to indent the output.
**/

VOID
DumpGasStruct (
  IN UINT8*        Ptr,
  IN UINT32        Indent
  );

/** This function traces the GAS structure as described by the GasParser.

  @params [in] Format  Optional format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.
**/

VOID
DumpGas (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );


/** This function traces the ACPI header as described by the AcpiHeaderParser.

  @params [in] Ptr          Pointer to the start of the buffer.

  @retval Number of bytes parsed.
**/
UINT32
DumpAcpiHeader (
  IN UINT8* Ptr
  );

/** This function parses the ACPI header as described by the AcpiHeaderParser.

  This function optionally returns the Signature, Length and revision of the
  ACPI table.

  @params [in]  Ptr        Pointer to the start of the buffer.
  @params [out] Signature  Gets location of the ACPI table signature.
  @params [out] Length     Gets location of the length of the ACPI table.
  @params [out] Revision   Gets location of the revision of the ACPI table.

  @retval Number of bytes parsed.
**/
UINT32
ParseAcpiHeader (
  IN  UINT8*         Ptr,
  OUT UINT32** Signature,
  OUT UINT32** Length,
  OUT UINT8**  Revision
  );

#endif // ACPIPARSER_H_
