/** @file
  AGDI table parser

  Copyright (c) 2025, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI for the Arm Components 1.2 EAC1 Platform Design Document,
      dated July 2025.
      (https://developer.arm.com/documentation/den0093/1-2eac1/)
**/

#include <IndustryStandard/ArmAgdiTable.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"

STATIC ACPI_DESCRIPTION_HEADER_INFO  mAcpiHdrInfo;

/**
  Validate Signaling Mode Value.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateSignalingFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*Ptr >= ArmAgdiSignalingModeInval) {
    IncrementErrorCount ();
    Print (L"\nERROR: Signaling Flags must be either 0, 1 or 2");
  }
}

/**
  Validate that the Reserved field has expected value.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateResField (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*Ptr != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Reserved bits in Flags must be 0");
  }
}

/**
  An ACPI_PARSER array describing the AGDI Signaling Mode Flags field.
**/
STATIC CONST ACPI_PARSER  AgdiSignalingModeFlags[] = {
  { L"Signaling Mode", 2, 0, L"%u", NULL, NULL, ValidateSignalingFlags, NULL },
  { L"Reserved",       6, 2, L"%u", NULL, NULL, ValidateResField,       NULL },
};

/**
  This function dumps the signaling mode flags field.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpSignalingFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%x\n", *Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    1,
    PARSER_PARAMS (AgdiSignalingModeFlags)
    );
}

/**
  An ACPI_PARSER array describing the ACPI AGDI Table.
**/
STATIC CONST ACPI_PARSER  AgdiParser[] = {
  PARSE_ACPI_HEADER (&mAcpiHdrInfo),
  { L"Signaling mode Flags",        1,  36, NULL,              DumpSignalingFlags, NULL, NULL, NULL },
  { L"Reserved",                    3,  37, L"0x%x 0x%x 0x%x", Dump3Chars,         NULL, NULL, NULL },
  { L"Sdei Event Number",           4,  40, L"0x%x",           NULL,               NULL, NULL, NULL },
  { L"GSIV",                        4,  44, L"0x%x",           NULL,               NULL, NULL, NULL },
};

/**
  This function parses the ACPI AGDI table.
  When trace is enabled this function parses the AGDI table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiAgdi (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  // Parse ACPI Header + RASF "fixed" fields
  ParseAcpi (
    Trace,
    0,
    "AGDI",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (AgdiParser)
    );
}
