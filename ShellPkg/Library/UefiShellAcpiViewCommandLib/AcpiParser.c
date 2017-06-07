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

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"

STATIC UINT32 gIndent = 0;

/** This function prints a GUID to STDOUT.

  @params [in] Guid    Pointer to a GUID to print.

  @retval EFI_SUCCESS             The GUID was printed.
  @retval EFI_INVALID_PARAMETER   The input was NULL.
**/
EFI_STATUS
PrintGuid (
  IN EFI_GUID* Guid
  )
{
  if (Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Print (
    L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
    Guid->Data1,
    Guid->Data2,
    Guid->Data3,
    Guid->Data4[0],
    Guid->Data4[1],
    Guid->Data4[2],
    Guid->Data4[3],
    Guid->Data4[4],
    Guid->Data4[5],
    Guid->Data4[6],
    Guid->Data4[7]
    );
  return EFI_SUCCESS;
}

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
  )
{
  UINTN ByteCount;
  UINT8 Checksum;
  UINTN OriginalAttribute;

  ByteCount = 0;
  Checksum = 0;

  while (ByteCount < Length) {
    Checksum += *(Ptr++);
    ByteCount++;
  }

  if (Log) {
    OriginalAttribute = gST->ConOut->Mode->Attribute;
    if (0 == Checksum) {
      if (GetColourHighlighting ()) {
        gST->ConOut->SetAttribute (
                       gST->ConOut,
                       EFI_TEXT_ATTR (EFI_GREEN,
                         ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                       );
      }
      Print (L"\n\nTable Checksum : OK\n\n");
    } else {
      IncrementErrorCount ();
      if (GetColourHighlighting ()) {
        gST->ConOut->SetAttribute (
                       gST->ConOut,
                       EFI_TEXT_ATTR (EFI_RED,
                         ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                       );
      }
      Print (L"\n\nTable Checksum : FAILED (0x%X)\n\n", Checksum);
    }
    if (GetColourHighlighting ()) {
      gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
    }
  }

  return (0 == Checksum);
}


/** This function performs a raw data dump of the ACPI table.

  @params [in] Ptr     Pointer to the start of the table buffer.
  @params [in] Length  The length of the buffer.

**/
VOID
DumpRaw (
  IN UINT8* Ptr,
  IN UINT32 Length
  )
{
  UINTN ByteCount = 0;
  UINTN PartLineChars;
  UINTN AsciiBufferIndex = 0;
  CHAR8 AsciiBuffer[17];

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
  Print (L"  %a", AsciiBuffer);
}



/** This function traces 1 byte of datum as specified in the
    format string.

  @params [in] Format  The format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
DumpUint8 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  Print (Format, *Ptr);
}


/** This function traces 2 bytes of data as specified in the
    format string.

  @params [in] Format  The format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
DumpUint16 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  Print (Format, *(UINT16*)Ptr);
}

/** This function traces 4 bytes of data as specified in the
    format string.

  @params [in] Format  The format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/

VOID
DumpUint32 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  Print (Format, *(UINT32*)Ptr);
}

/** This function traces 8 bytes of data as specified by the
    format string.

  @params [in] Format  The format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.

**/
VOID
DumpUint64 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  UINT64 Val;
  // Some fields are not aligned and this causes alignment faults
  // on ARM platforms if the compiler generates LDRD instructions.
  // Perform word access so that LDRD instructions are not generated.
  Val = *(UINT32*)(Ptr + sizeof (UINT32));
  Val <<= 32;
  Val |= *(UINT32*)Ptr;

  Print (Format, Val);
}


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
  )
{
  Print (
    (NULL != Format) ? Format : L"%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2]
    );
}

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
  )
{
  Print (
    (NULL != Format) ? Format : L"%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3]
    );
}

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
  )
{
  Print (
    (NULL != Format) ? Format : L"%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5]
    );
}

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
  )
{
  Print (
    (NULL != Format) ? Format : L"%c%c%c%c%c%c%c%c",
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
)
{
  UINT32  Index;
  UINT32  Offset = 0;

  // Increment the Indent
  gIndent += Indent;

  if ((Trace) && (NULL != AsciiName)){
    BOOLEAN HighLight = GetColourHighlighting ();
    UINTN   OriginalAttribute;

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
      // We don't parse past the end of the max length specified
      break;
    }

    if (Offset != Parser[Index].Offset) {
      IncrementErrorCount ();
      Print (
        L"\nERROR: %a: Offset Mismatch for %s\n"
          "CurrentOffset = %d FieldOffset = %d\n",
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
      if (NULL != Parser[Index].PrintFormatter) {
        Parser[Index].PrintFormatter (Parser[Index].Format, Ptr);
      } else if (NULL != Parser[Index].Format) {
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

        // Validating only makes sense if we are Tracing
        // the parsed table entries, to report by table name.
        if (NULL != Parser[Index].FieldValidator) {
          Parser[Index].FieldValidator (Ptr, Parser[Index].Context);
        }
      }
      Print (L"\n");
    } // if (Trace)


    if (NULL != Parser[Index].ItemPtr) {
      *Parser[Index].ItemPtr = (VOID*)Ptr;
    }

    Ptr += Parser[Index].Length;
    Offset += Parser[Index].Length;
  } // for

  // Decrement the Indent
  gIndent -= Indent;
  return Offset;
}


/** An array describing the ACPI Generic Address Structure.
  The GasParser array is used by the ParseAcpi function to parse and/or trace
  the GAS structure.
**/

STATIC CONST ACPI_PARSER GasParser[] = {
  {L"Address Space ID", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Register Bit Width", 1, 1, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Register Bit Offset", 1, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Address Size", 1, 3, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Address", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL}
};

/** This function indents and traces the GAS structure as described
    by the GasParser.

  @params [in] Ptr     Pointer to the start of the buffer.
  @params [in] Indent  Number of spaces to indent the output.
**/

VOID
DumpGasStruct (
  IN UINT8*        Ptr,
  IN UINT32        Indent
  )
{
  Print (L"\n");
  ParseAcpi (
    TRUE,
    Indent,
    NULL,
    Ptr,
    GAS_LENGTH,
    PARSER_PARAMS (GasParser)
    );
}


/** This function traces the GAS structure as described by the GasParser.

  @params [in] Format  Optional format string for tracing the data.
  @params [in] Ptr     Pointer to the start of the buffer.
**/

VOID
DumpGas (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  )
{
  DumpGasStruct (Ptr, 2);
}

/** This function traces the ACPI header as described by the AcpiHeaderParser.

  @params [in] Ptr          Pointer to the start of the buffer.

  @retval Number of bytes parsed.
**/
UINT32
DumpAcpiHeader (
  IN UINT8* Ptr
  )
{
  ACPI_PARSER AcpiHeaderParser[] = {
    PARSE_ACPI_HEADER (NULL, NULL, NULL)
  };

  return ParseAcpi (
           TRUE,
           0,
           "ACPI Table Header",
           Ptr,
           ACPI_DESCRIPTION_HEADER_LENGTH,
           PARSER_PARAMS (AcpiHeaderParser)
           );
}

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
  OUT CONST UINT32** Signature,
  OUT CONST UINT32** Length,
  OUT CONST UINT8**  Revision
  )
{
  ACPI_PARSER AcpiHeaderParser[] = {
    PARSE_ACPI_HEADER (Signature, Length, Revision)
  };

  return ParseAcpi (
             FALSE,
             0,
             NULL,
             Ptr,
             ACPI_DESCRIPTION_HEADER_LENGTH,
             PARSER_PARAMS (AcpiHeaderParser)
             );
}
