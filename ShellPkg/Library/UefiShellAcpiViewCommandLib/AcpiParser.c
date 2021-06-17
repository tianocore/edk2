/** @file
  ACPI parser

  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"

STATIC UINT32   gIndent;
STATIC UINT32   mTableErrorCount;
STATIC UINT32   mTableWarningCount;

STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI header.
**/
STATIC CONST ACPI_PARSER AcpiHeaderParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo)
};

/**
  This function resets the ACPI table error counter to Zero.
**/
VOID
ResetErrorCount (
  VOID
  )
{
  mTableErrorCount = 0;
}

/**
  This function returns the ACPI table error count.

  @retval Returns the count of errors detected in the ACPI tables.
**/
UINT32
GetErrorCount (
  VOID
  )
{
  return mTableErrorCount;
}

/**
  This function resets the ACPI table warning counter to Zero.
**/
VOID
ResetWarningCount (
  VOID
  )
{
  mTableWarningCount = 0;
}

/**
  This function returns the ACPI table warning count.

  @retval Returns the count of warning detected in the ACPI tables.
**/
UINT32
GetWarningCount (
  VOID
  )
{
  return mTableWarningCount;
}

/**
  This function increments the ACPI table error counter.
**/
VOID
EFIAPI
IncrementErrorCount (
  VOID
  )
{
  mTableErrorCount++;
}

/**
  This function increments the ACPI table warning counter.
**/
VOID
EFIAPI
IncrementWarningCount (
  VOID
  )
{
  mTableWarningCount++;
}

/**
  This function verifies the ACPI table checksum.

  This function verifies the checksum for the ACPI table and optionally
  prints the status.

  @param [in] Log     If TRUE log the status of the checksum.
  @param [in] Ptr     Pointer to the start of the table buffer.
  @param [in] Length  The length of the buffer.

  @retval TRUE        The checksum is OK.
  @retval FALSE       The checksum failed.
**/
BOOLEAN
EFIAPI
VerifyChecksum (
  IN BOOLEAN Log,
  IN UINT8*  Ptr,
  IN UINT32  Length
  )
{
  UINTN ByteCount;
  UINT8 Checksum;
  UINTN OriginalAttribute;

  //
  // set local variables to suppress incorrect compiler/analyzer warnings
  //
  OriginalAttribute = 0;
  ByteCount = 0;
  Checksum = 0;

  while (ByteCount < Length) {
    Checksum += *(Ptr++);
    ByteCount++;
  }

  if (Log) {
    OriginalAttribute = gST->ConOut->Mode->Attribute;
    if (Checksum == 0) {
      if (GetColourHighlighting ()) {
        gST->ConOut->SetAttribute (
                       gST->ConOut,
                       EFI_TEXT_ATTR (EFI_GREEN,
                         ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                       );
      }
      Print (L"Table Checksum : OK\n\n");
    } else {
      IncrementErrorCount ();
      if (GetColourHighlighting ()) {
        gST->ConOut->SetAttribute (
                       gST->ConOut,
                       EFI_TEXT_ATTR (EFI_RED,
                         ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                       );
      }
      Print (L"Table Checksum : FAILED (0x%X)\n\n", Checksum);
    }
    if (GetColourHighlighting ()) {
      gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
    }
  }

  return (Checksum == 0);
}

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
  )
{
  UINTN ByteCount;
  UINTN PartLineChars;
  UINTN AsciiBufferIndex;
  CHAR8 AsciiBuffer[17];

  ByteCount = 0;
  AsciiBufferIndex = 0;

  Print (L"Address  : 0x%p\n", Ptr);
  Print (L"Length   : %d\n", Length);

  while (ByteCount < Length) {
    if ((ByteCount & 0x0F) == 0) {
      AsciiBuffer[AsciiBufferIndex] = '\0';
      Print (L"  %a\n%08X : ", AsciiBuffer, ByteCount);
      AsciiBufferIndex = 0;
    } else if ((ByteCount & 0x07) == 0) {
      Print (L"- ");
    }

    if ((*Ptr >= ' ') && (*Ptr < 0x7F)) {
      AsciiBuffer[AsciiBufferIndex++] = *Ptr;
    } else {
      AsciiBuffer[AsciiBufferIndex++] = '.';
    }

    Print (L"%02X ", *Ptr++);

    ByteCount++;
  }

  // Justify the final line using spaces before printing
  // the ASCII data.
  PartLineChars = (Length & 0x0F);
  if (PartLineChars != 0) {
    PartLineChars = 48 - (PartLineChars * 3);
    if ((Length & 0x0F) <= 8) {
      PartLineChars += 2;
    }
    while (PartLineChars > 0) {
      Print (L" ");
      PartLineChars--;
    }
  }

  // Print ASCII data for the final line.
  AsciiBuffer[AsciiBufferIndex] = '\0';
  Print (L"  %a\n\n", AsciiBuffer);
}

/**
  This function traces 1 byte of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint8 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  Print (Format, *Ptr);
}

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
  )
{
  Print (Format, *(UINT16*)Ptr);
}

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
  )
{
  Print (Format, *(UINT32*)Ptr);
}

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
  )
{
  // Some fields are not aligned and this causes alignment faults
  // on ARM platforms if the compiler generates LDRD instructions.
  // Perform word access so that LDRD instructions are not generated.
  UINT64 Val;

  Val = *(UINT32*)(Ptr + sizeof (UINT32));

  Val = LShiftU64(Val,32);
  Val |= (UINT64)*(UINT32*)Ptr;

  Print (Format, Val);
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2]
    );
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3]
    );
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5]
    );
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5],
    Ptr[6],
    Ptr[7]
    );
}

/**
  This function traces 12 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump12Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN       UINT8*  Ptr
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c%c%c%c%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5],
    Ptr[6],
    Ptr[7],
    Ptr[8],
    Ptr[9],
    Ptr[10],
    Ptr[11]
    );
}

/**
  This function indents and prints the ACPI table Field Name.

  @param [in] Indent      Number of spaces to add to the global table indent.
                          The global table indent is 0 by default; however
                          this value is updated on entry to the ParseAcpi()
                          by adding the indent value provided to ParseAcpi()
                          and restored back on exit.
                          Therefore the total indent in the output is
                          dependent on from where this function is called.
  @param [in] FieldName   Pointer to the Field Name.
**/
VOID
EFIAPI
PrintFieldName (
  IN UINT32         Indent,
  IN CONST CHAR16*  FieldName
)
{
  Print (
    L"%*a%-*s : ",
    gIndent + Indent,
    "",
    (OUTPUT_FIELD_COLUMN_WIDTH - gIndent - Indent),
    FieldName
    );
}

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
)
{
  UINT32  Index;
  UINT32  Offset;
  BOOLEAN HighLight;
  UINTN   OriginalAttribute;

  //
  // set local variables to suppress incorrect compiler/analyzer warnings
  //
  OriginalAttribute = 0;
  Offset = 0;

  // Increment the Indent
  gIndent += Indent;

  if (Trace && (AsciiName != NULL)){
    HighLight = GetColourHighlighting ();

    if (HighLight) {
      OriginalAttribute = gST->ConOut->Mode->Attribute;
      gST->ConOut->SetAttribute (
                     gST->ConOut,
                     EFI_TEXT_ATTR(EFI_YELLOW,
                       ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                     );
    }
    Print (
      L"%*a%-*a :\n",
      gIndent,
      "",
      (OUTPUT_FIELD_COLUMN_WIDTH - gIndent),
      AsciiName
      );
    if (HighLight) {
      gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
    }
  }

  for (Index = 0; Index < ParserItems; Index++) {
    if ((Offset + Parser[Index].Length) > Length) {

      // For fields outside the buffer length provided, reset any pointers
      // which were supposed to be updated by this function call
      if (Parser[Index].ItemPtr != NULL) {
        *Parser[Index].ItemPtr = NULL;
      }

      // We don't parse past the end of the max length specified
      continue;
    }

    if (GetConsistencyChecking () &&
        (Offset != Parser[Index].Offset)) {
      IncrementErrorCount ();
      Print (
        L"\nERROR: %a: Offset Mismatch for %s\n"
          L"CurrentOffset = %d FieldOffset = %d\n",
        AsciiName,
        Parser[Index].NameStr,
        Offset,
        Parser[Index].Offset
        );
    }

    if (Trace) {
      // if there is a Formatter function let the function handle
      // the printing else if a Format is specified in the table use
      // the Format for printing
      PrintFieldName (2, Parser[Index].NameStr);
      if (Parser[Index].PrintFormatter != NULL) {
        Parser[Index].PrintFormatter (Parser[Index].Format, Ptr);
      } else if (Parser[Index].Format != NULL) {
        switch (Parser[Index].Length) {
          case 1:
            DumpUint8 (Parser[Index].Format, Ptr);
            break;
          case 2:
            DumpUint16 (Parser[Index].Format, Ptr);
            break;
          case 4:
            DumpUint32 (Parser[Index].Format, Ptr);
            break;
          case 8:
            DumpUint64 (Parser[Index].Format, Ptr);
            break;
          default:
            Print (
              L"\nERROR: %a: CANNOT PARSE THIS FIELD, Field Length = %d\n",
              AsciiName,
              Parser[Index].Length
              );
        } // switch
      }
      // Validating only makes sense if we are tracing
      // the parsed table entries, to report by table name.
      if (GetConsistencyChecking () &&
          (Parser[Index].FieldValidator != NULL)) {
        Parser[Index].FieldValidator (Ptr, Parser[Index].Context);
      }
      Print (L"\n");
    } // if (Trace)

    if (Parser[Index].ItemPtr != NULL) {
      *Parser[Index].ItemPtr = (VOID*)Ptr;
    }

    Ptr += Parser[Index].Length;
    Offset += Parser[Index].Length;
  } // for

  // Decrement the Indent
  gIndent -= Indent;
  return Offset;
}

/**
  An array describing the ACPI Generic Address Structure.
  The GasParser array is used by the ParseAcpi function to parse and/or trace
  the GAS structure.
**/
STATIC CONST ACPI_PARSER GasParser[] = {
  {L"Address Space ID", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Register Bit Width", 1, 1, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Register Bit Offset", 1, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Access Size", 1, 3, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Address", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL}
};

/**
  This function indents and traces the GAS structure as described by the GasParser.

  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Indent  Number of spaces to indent the output.
  @param [in] Length  Length of the GAS structure buffer.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
DumpGasStruct (
  IN UINT8*        Ptr,
  IN UINT32        Indent,
  IN UINT32        Length
  )
{
  Print (L"\n");
  return ParseAcpi (
           TRUE,
           Indent,
           NULL,
           Ptr,
           Length,
           PARSER_PARAMS (GasParser)
           );
}

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
  )
{
  DumpGasStruct (Ptr, 2, sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE));
}

/**
  This function traces the ACPI header as described by the AcpiHeaderParser.

  @param [in] Ptr          Pointer to the start of the buffer.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
DumpAcpiHeader (
  IN UINT8* Ptr
  )
{
  return ParseAcpi (
           TRUE,
           0,
           "ACPI Table Header",
           Ptr,
           sizeof (EFI_ACPI_DESCRIPTION_HEADER),
           PARSER_PARAMS (AcpiHeaderParser)
           );
}

/**
  This function parses the ACPI header as described by the AcpiHeaderParser.

  This function optionally returns the signature, length and revision of the
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
  )
{
  UINT32                        BytesParsed;

  BytesParsed = ParseAcpi (
                  FALSE,
                  0,
                  NULL,
                  Ptr,
                  sizeof (EFI_ACPI_DESCRIPTION_HEADER),
                  PARSER_PARAMS (AcpiHeaderParser)
                  );

  *Signature = AcpiHdrInfo.Signature;
  *Length = AcpiHdrInfo.Length;
  *Revision = AcpiHdrInfo.Revision;

  return BytesParsed;
}
