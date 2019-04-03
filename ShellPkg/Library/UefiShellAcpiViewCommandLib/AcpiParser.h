/** @file
  Header file for ACPI parser

  Copyright (c) 2016 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPIPARSER_H_
#define ACPIPARSER_H_

#define OUTPUT_FIELD_COLUMN_WIDTH  36

/// The RSDP table signature is "RSD PTR " (8 bytes)
/// However The signature for ACPI tables is 4 bytes.
/// To work around this oddity define a signature type
/// that allows us to process the log options.
#define RSDP_TABLE_INFO  SIGNATURE_32('R', 'S', 'D', 'P')

/**
  This function increments the ACPI table error counter.
**/
VOID
EFIAPI
IncrementErrorCount (
  VOID
  );

/**
  This function increments the ACPI table warning counter.
**/
VOID
EFIAPI
IncrementWarningCount (
  VOID
  );

/**
  This function verifies the ACPI table checksum.

  This function verifies the checksum for the ACPI table and optionally
  prints the status.

  @param [in] Log     If TRUE log the status of the checksum.
  @param [in] Ptr     Pointer to the start of the table buffer.
  @param [in] Length  The length of the buffer.

  @retval TRUE         The checksum is OK.
  @retval FALSE        The checksum failed.
**/
BOOLEAN
EFIAPI
VerifyChecksum (
  IN BOOLEAN Log,
  IN UINT8*  Ptr,
  IN UINT32  Length
  );

/**
  This function performs a raw data dump of the ACPI table.

  @param [in] Ptr     Pointer to the start of the table buffer.
  @param [in] Length  The length of the buffer.
**/
VOID
EFIAPI
DumpRaw (
  IN UINT8* Ptr,
  IN UINT32 Length
  );

/**
  This function traces 1 byte of datum as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint8 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/**
  This function traces 2 bytes of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint16 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/**
  This function traces 4 bytes of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint32 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/**
  This function traces 8 bytes of data as specified by the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint64 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/**
  This function traces 3 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump3Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces 4 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump4Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces 6 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump6Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces 8 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump8Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function indents and prints the ACPI table Field Name.

  @param [in] Indent      Number of spaces to add to the global table
                          indent. The global table indent is 0 by default;
                          however this value is updated on entry to the
                          ParseAcpi() by adding the indent value provided to
                          ParseAcpi() and restored back on exit. Therefore
                          the total indent in the output is dependent on from
                          where this function is called.
  @param [in] FieldName   Pointer to the Field Name.
**/
VOID
EFIAPI
PrintFieldName (
  IN UINT32         Indent,
  IN CONST CHAR16*  FieldName
  );

/**
  This function pointer is the template for customizing the trace output

  @param [in] Format  Format string for tracing the data as specified by
                      the 'Format' member of ACPI_PARSER.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
typedef VOID (EFIAPI *FNPTR_PRINT_FORMATTER)(CONST CHAR16* Format, UINT8* Ptr);

/**
  This function pointer is the template for validating an ACPI table field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information as specified by
                      the 'Context' member of the ACPI_PARSER.
                      e.g. this could be a pointer to the ACPI table header.
**/
typedef VOID (EFIAPI *FNPTR_FIELD_VALIDATOR)(UINT8* Ptr, VOID* Context);

/**
  The ACPI_PARSER structure describes the fields of an ACPI table and
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

/**
  A structure used to store the pointers to the members of the
  ACPI description header structure that was parsed.
**/
typedef struct AcpiDescriptionHeaderInfo {
  /// ACPI table signature
  UINT32* Signature;
  /// Length of the ACPI table
  UINT32* Length;
  /// Revision
  UINT8*  Revision;
  /// Checksum
  UINT8*  Checksum;
  /// OEM Id - length is 6 bytes
  UINT8*  OemId;
  /// OEM table Id
  UINT64* OemTableId;
  /// OEM revision Id
  UINT32* OemRevision;
  /// Creator Id
  UINT32* CreatorId;
  /// Creator revision
  UINT32* CreatorRevision;
} ACPI_DESCRIPTION_HEADER_INFO;

/**
  This function is used to parse an ACPI table buffer.

  The ACPI table buffer is parsed using the ACPI table parser information
  specified by a pointer to an array of ACPI_PARSER elements. This parser
  function iterates through each item on the ACPI_PARSER array and logs the
  ACPI table fields.

  This function can optionally be used to parse ACPI tables and fetch specific
  field values. The ItemPtr member of the ACPI_PARSER structure (where used)
  is updated by this parser function to point to the selected field data
  (e.g. useful for variable length nested fields).

  @param [in] Trace        Trace the ACPI fields TRUE else only parse the
                           table.
  @param [in] Indent       Number of spaces to indent the output.
  @param [in] AsciiName    Optional pointer to an ASCII string that describes
                           the table being parsed.
  @param [in] Ptr          Pointer to the start of the buffer.
  @param [in] Length       Length of the buffer pointed by Ptr.
  @param [in] Parser       Pointer to an array of ACPI_PARSER structure that
                           describes the table being parsed.
  @param [in] ParserItems  Number of items in the ACPI_PARSER array.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
ParseAcpi (
  IN BOOLEAN            Trace,
  IN UINT32             Indent,
  IN CONST CHAR8*       AsciiName OPTIONAL,
  IN UINT8*             Ptr,
  IN UINT32             Length,
  IN CONST ACPI_PARSER* Parser,
  IN UINT32             ParserItems
  );

/**
   This is a helper macro to pass parameters to the Parser functions.

  @param [in] Parser The name of the ACPI_PARSER array describing the
              ACPI table fields.
**/
#define PARSER_PARAMS(Parser) Parser, sizeof (Parser) / sizeof (Parser[0])

/**
  This is a helper macro for describing the ACPI header fields.

  @param [out] Info  Pointer to retrieve the ACPI table header information.
**/
#define PARSE_ACPI_HEADER(Info)                   \
  { L"Signature", 4, 0, NULL, Dump4Chars,         \
    (VOID**)&(Info)->Signature , NULL, NULL },    \
  { L"Length", 4, 4, L"%d", NULL,                 \
    (VOID**)&(Info)->Length, NULL, NULL },        \
  { L"Revision", 1, 8, L"%d", NULL,               \
    (VOID**)&(Info)->Revision, NULL, NULL },      \
  { L"Checksum", 1, 9, L"0x%X", NULL,             \
    (VOID**)&(Info)->Checksum, NULL, NULL },      \
  { L"Oem ID", 6, 10, NULL, Dump6Chars,           \
    (VOID**)&(Info)->OemId, NULL, NULL },         \
  { L"Oem Table ID", 8, 16, NULL, Dump8Chars,     \
    (VOID**)&(Info)->OemTableId, NULL, NULL },    \
  { L"Oem Revision", 4, 24, L"0x%X", NULL,        \
    (VOID**)&(Info)->OemRevision, NULL, NULL },   \
  { L"Creator ID", 4, 28, NULL, Dump4Chars,       \
    (VOID**)&(Info)->CreatorId, NULL, NULL },     \
  { L"Creator Revision", 4, 32, L"0x%X", NULL,    \
    (VOID**)&(Info)->CreatorRevision, NULL, NULL }

/**
  Length of the ACPI GAS structure.

  NOTE: This might normally be defined as
        sizeof (EFI_ACPI_6_2_GENERIC_ADDRESS_STRUCTURE).
        However, we deliberately minimise any reference to the EDK2 ACPI
        headers in an attempt to provide cross checking.
**/
#define GAS_LENGTH                     12

/**
  Length of the ACPI Header structure.

  NOTE: This might normally be defined as
        sizeof (EFI_ACPI_DESCRIPTION_HEADER).
        However, we deliberately minimise any reference to the EDK2 ACPI
        headers in an attempt to provide cross checking.
**/
#define ACPI_DESCRIPTION_HEADER_LENGTH  36

/**
  This function indents and traces the GAS structure as described by the GasParser.

  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Indent  Number of spaces to indent the output.
**/
VOID
EFIAPI
DumpGasStruct (
  IN UINT8*        Ptr,
  IN UINT32        Indent
  );

/**
  This function traces the GAS structure as described by the GasParser.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpGas (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces the ACPI header as described by the AcpiHeaderParser.

  @param [in] Ptr          Pointer to the start of the buffer.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
DumpAcpiHeader (
  IN UINT8* Ptr
  );

/**
  This function parses the ACPI header as described by the AcpiHeaderParser.

  This function optionally returns the Signature, Length and revision of the
  ACPI table.

  @param [in]  Ptr        Pointer to the start of the buffer.
  @param [out] Signature  Gets location of the ACPI table signature.
  @param [out] Length     Gets location of the length of the ACPI table.
  @param [out] Revision   Gets location of the revision of the ACPI table.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
ParseAcpiHeader (
  IN  UINT8*         Ptr,
  OUT CONST UINT32** Signature,
  OUT CONST UINT32** Length,
  OUT CONST UINT8**  Revision
  );

/**
  This function parses the ACPI BGRT table.
  When trace is enabled this function parses the BGRT table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiBgrt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI DBG2 table.
  When trace is enabled this function parses the DBG2 table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiDbg2 (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI DSDT table.
  When trace is enabled this function parses the DSDT table and
  traces the ACPI table fields.
  For the DSDT table only the ACPI header fields are parsed and
  traced.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiDsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI FADT table.
  This function parses the FADT table and optionally traces the ACPI
  table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiFadt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI GTDT table.
  When trace is enabled this function parses the GTDT table and
  traces the ACPI table fields.

  This function also parses the following platform timer structures:
    - GT Block timer
    - Watchdog timer

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiGtdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI IORT table.
  When trace is enabled this function parses the IORT table and
  traces the ACPI fields.

  This function also parses the following nodes:
    - ITS Group
    - Named Component
    - Root Complex
    - SMMUv1/2
    - SMMUv3
    - PMCG

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiIort (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI MADT table.
  When trace is enabled this function parses the MADT table and
  traces the ACPI table fields.

  This function currently parses the following Interrupt Controller
  Structures:
    - GICC
    - GICD
    - GIC MSI Frame
    - GICR
    - GIC ITS

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiMadt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI MCFG table.
  When trace is enabled this function parses the MCFG table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiMcfg (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI PPTT table.
  When trace is enabled this function parses the PPTT table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiPptt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI RSDP table.

  This function invokes the parser for the XSDT table.
  * Note - This function does not support parsing of RSDT table.

  This function also performs a RAW dump of the ACPI table and
  validates the checksum.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiRsdp (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

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
  );

/**
  This function parses the ACPI SPCR table.
  When trace is enabled this function parses the SPCR table and
  traces the ACPI table fields.

  This function also performs validations of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSpcr (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI SRAT table.
  When trace is enabled this function parses the SRAT table and
  traces the ACPI table fields.

  This function parses the following Resource Allocation Structures:
    - Processor Local APIC/SAPIC Affinity Structure
    - Memory Affinity Structure
    - Processor Local x2APIC Affinity Structure
    - GICC Affinity Structure

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSrat (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI SSDT table.
  When trace is enabled this function parses the SSDT table and
  traces the ACPI table fields.
  For the SSDT table only the ACPI header fields are
  parsed and traced.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI XSDT table
  and optionally traces the ACPI table fields.

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
  );

#endif // ACPIPARSER_H_
