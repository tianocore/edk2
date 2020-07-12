/** @file
  'acpiview' logging and output facility

  Copyright (c) 2020, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiViewLog.h"
#include "AcpiViewConfig.h"
#include "AcpiParser.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

static CHAR16 mOutputBuffer [MAX_OUTPUT_SIZE] = { 0 };

// String descriptions of error types
static const CHAR16* mErrorTypeDesc [ACPI_ERROR_MAX] = {
  L"Not an error",        ///< ACPI_ERROR_NONE
  L"Generic",             ///< ACPI_ERROR_GENERIC
  L"Checksum",            ///< ACPI_ERROR_CSUM
  L"Parsing",             ///< ACPI_ERROR_PARSE
  L"Length",              ///< ACPI_ERROR_LENGTH
  L"Value",               ///< ACPI_ERROR_VALUE
  L"Cross-check",         ///< ACPI_ERROR_CROSS
};

// Publicly accessible error and warning counters.
UINT32   mTableErrorCount;
UINT32   mTableWarningCount;
UINT32   gIndent;

/**
  Change the attributes of the standard output console
  to change the colour of the text according to the given
  severity of a log message.

  @param[in] Severity          The severity of the log message that is being
                               annotated with changed colour text.
  @param[in] OriginalAttribute The current attributes of ConOut that will
                               be modified.
**/
static
VOID
EFIAPI
ApplyColor (
  IN ACPI_LOG_SEVERITY Severity,
  IN UINTN             OriginalAttribute
  )
{
  if (!mConfig.ColourHighlighting) {
    return;
  }

  // Strip the foreground colour
  UINTN NewAttribute = OriginalAttribute & 0xF0;

  // Add specific foreground colour based on severity
  switch (Severity) {
  case ACPI_DEBUG:
    NewAttribute |= EFI_DARKGRAY;
    break;
  case ACPI_HIGHLIGHT:
    NewAttribute |= EFI_LIGHTBLUE;
    break;
  case ACPI_GOOD:
    NewAttribute |= EFI_GREEN;
    break;
  case ACPI_ITEM:
  case ACPI_WARN:
    NewAttribute |= EFI_YELLOW;
    break;
  case ACPI_BAD:
  case ACPI_ERROR:
  case ACPI_FATAL:
    NewAttribute |= EFI_RED;
    break;
  case ACPI_INFO:
  default:
    NewAttribute |= OriginalAttribute;
    break;
  }

  gST->ConOut->SetAttribute (gST->ConOut, NewAttribute);
}

/**
  Restore ConOut text attributes.

  @param[in] OriginalAttribute The attribute set that will be restored.
**/
static
VOID
EFIAPI
RestoreColor(
  IN UINTN OriginalAttribute
  )
{
  gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
}

/**
  Formats and prints an ascii string to screen.

  @param[in] Format String that will be formatted and printed.
  @param[in] Marker The marker for variable parameters to be formatted.
**/
static
VOID
EFIAPI
AcpiViewVSOutput (
  IN const CHAR16  *Format,
  IN VA_LIST       Marker
  )
{
  UnicodeVSPrint (mOutputBuffer, sizeof(mOutputBuffer), Format, Marker);
  gST->ConOut->OutputString (gST->ConOut, mOutputBuffer);
}

/**
  Formats and prints and ascii string to screen.

  @param[in] Format String that will be formatted and printed.
  @param[in] ...    A variable number of parameters that will be formatted.
**/
VOID
EFIAPI
AcpiViewOutput (
  IN const CHAR16 *Format,
  IN ...
  )
{
  VA_LIST Marker;
  VA_START (Marker, Format);

  AcpiViewVSOutput (Format, Marker);

  VA_END (Marker);
}


/**
  Prints the base file name given a full file path.

  @param[in] FullName Fully qualified file path
**/
VOID
EFIAPI
PrintFileName (
  IN const CHAR8* FullName
  )
{
  const CHAR8* Cursor;
  UINTN        Size;

  Cursor = FullName;
  Size = 0;

  // Find the end point of the string.
  while (*Cursor && Cursor < FullName + MAX_OUTPUT_SIZE)
    Cursor++;

  // Find the rightmost path separator.
  while (*Cursor != '\\' && *Cursor != '/' && Cursor > FullName) {
    Cursor--;
    Size++;
  }

  // Print base file name.
  AcpiViewOutput (L"%.*a", Size - 1, Cursor + 1);
}

/**
  AcpiView output and logging function. Will log the event to
  configured output (currently screen) and annotate with colour
  and extra metadata.

  @param[in] FileName      The full filename of the source file where this
                           event occured.
  @param[in] FunctionName  The name of the function where this event occured.
  @param[in] LineNumber    The line number in the source code where this event
                           occured.
  @param[in] Severity      The severity of the event that occured.
  @param[in] Format        The format of the string describing the event.
  @param[in] ...           The variable number of parameters that will format the
                           string supplied in Format.
**/
VOID
EFIAPI
AcpiViewLog (
  IN const CHAR8       *FileName,
  IN const CHAR8       *FunctionName,
  IN UINTN             LineNumber,
  IN ACPI_ERROR_TYPE   Error,
  IN ACPI_LOG_SEVERITY Severity,
  IN const CHAR16      *Format,
  ...
  )
{
  VA_LIST         Marker;
  UINTN           OriginalAttribute;

  OriginalAttribute = gST->ConOut->Mode->Attribute;
  ApplyColor (Severity, OriginalAttribute);
  VA_START (Marker, Format);

  switch (Severity) {
  case ACPI_FATAL:
    AcpiViewOutput (L"FATAL ");
    break;
  case ACPI_ERROR:
    AcpiViewOutput (L"ERROR[%s] ", mErrorTypeDesc[Error]);
    mTableErrorCount++;
    break;
  case ACPI_WARN:
    AcpiViewOutput (L"WARN ");
    mTableWarningCount++;
    break;
  default:
    break;
  }

  if (Severity >= ACPI_WARN) {
    AcpiViewOutput (L"(");
    PrintFileName (FileName);
    AcpiViewOutput (L":%d) ", LineNumber);
  }

  AcpiViewVSOutput (Format, Marker);
  AcpiViewOutput (L"\n");

  VA_END (Marker);
  RestoreColor (OriginalAttribute);
}

/**
  Check that a buffer has not been overrun by a member. Can be invoked
  using the BufferOverrun macro that fills in local source metadata
  (first three parameters) for logging purposes.

  @param[in] FileName        Source file where this invocation is made
  @param[in] FunctionName    Name of the local symbol
  @param[in] LineNumber      Source line number of the local call
  @param[in] ItemDescription User friendly name for the member being checked
  @param[in] Position        Memory address of the member
  @param[in] Length          Length of the member
  @param[in] Buffer          Memory address of the buffer where member resides
  @param[in] BufferSize      Size of the buffer where member resides

  @retval TRUE               Buffer was overrun
  @retval FALSE              Buffer is safe
**/
BOOLEAN
EFIAPI
MemberIntegrityInternal (
  IN const CHAR8  *FileName,
  IN const CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN const CHAR8 *ItemDescription,
  IN UINTN        Offset,
  IN UINTN        Length,
  IN VOID         *Buffer,
  IN UINTN        BufferSize
  )
{
  if (Length == 0) {
    AcpiViewLog (
      FileName,
      FunctionName,
      LineNumber,
      ACPI_ERROR_LENGTH,
      ACPI_ERROR,
      L"%a at %p in buffer %p+%x has zero size!",
      ItemDescription,
      (UINT8 *)Buffer + Offset,
      Buffer,
      BufferSize);
    return TRUE;
  }

  if (Offset + Length > BufferSize) {
    AcpiViewLog (
      FileName,
      FunctionName,
      LineNumber,
      ACPI_ERROR_LENGTH,
      ACPI_ERROR,
      L"%a %p+%x overruns buffer %p+%x",
      ItemDescription,
      (UINT8 *) Buffer + Offset,
      Length,
      Buffer,
      BufferSize);
  }

  return (Offset + Length > BufferSize);
}

/**
  Checks that a boolean constraint evaluates correctly. Can be invoked
  using the CheckConstraint macro that fills in the source code metadata.

  @param[in] FileName        Source file where this invocation is made
  @param[in] FunctionName    Name of the local symbol
  @param[in] LineNumber      Source line number of the local call
  @param[in] ConstraintText  The Source code of the constraint
  @param[in] Specification   The specification that imposes the constraint
  @param[in] Constraint      The actual constraint
  @

  @retval TRUE               Constraint is violated
  @retval FALSE              Constraint is not violated
**/
BOOLEAN
EFIAPI
CheckConstraintInternal (
  IN const CHAR8       *FileName,
  IN const CHAR8       *FunctionName,
  IN UINTN             LineNumber,
  IN const CHAR8       *ConstraintText,
  IN const CHAR16      *Specification,
  IN BOOLEAN           Constraint,
  IN ACPI_LOG_SEVERITY Severity
)
{
  if (!Constraint) {
    AcpiViewLog (
      FileName,
      FunctionName,
      LineNumber,
      Severity == ACPI_ERROR ? ACPI_ERROR_VALUE : ACPI_ERROR_NONE,
      Severity,
      L"(%a) Constraint was violated: %a",
      Specification,
      ConstraintText);
  }

  // Return TRUE if constraint was violated
  return !Constraint;
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
  @param [in] FieldName   Pointer to the format string for field name.
  @param [in] ...         Variable List parameters to format.
**/
VOID
EFIAPI
PrintFieldName (
  IN UINT32         Indent,
  IN CONST CHAR16*  FieldNameFormat,
  ...
  )
{
  VA_LIST Marker;
  CHAR16 Buffer[64];

  VA_START(Marker, FieldNameFormat);
  UnicodeVSPrint(Buffer, sizeof(Buffer), FieldNameFormat, Marker);
  VA_END(Marker);

  AcpiViewOutput (
    L"%*a%-*s : ",
    gIndent + Indent,
    "",
    (OUTPUT_FIELD_COLUMN_WIDTH - gIndent - Indent),
    Buffer
    );
}
