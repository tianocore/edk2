/** @file
  WSMT table parser

  Copyright (c) 2024, Arm Limited. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - Windows SMM Security Mitigation Table spec, version 1.0
**/

#include <Library/UefiLib.h>
#include <IndustryStandard/WindowsSmmSecurityMitigationTable.h>
#include "AcpiParser.h"

STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  This function validates the WSMT Protection flag.

  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.

**/
STATIC
VOID
EFIAPI
ValidateWsmtProtectionFlag (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT32  ProtectionFlag;

  ProtectionFlag = *(UINT32 *)Ptr;

  if ((ProtectionFlag & EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION) \
      == EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION)
  {
    if ((ProtectionFlag & EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS) \
        != EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS)
    {
      IncrementErrorCount ();
      Print (L"ERROR: COMM_BUFFER_NESTED_PTR_PROTECTION is set but FIXED_COMM_BUFFERS is not set.\n");
    }
  }
}

/**
  This function validates the reserved bits in the WSMT Protection flag.

  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateReserved (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT32  ProtectionFlag;

  ProtectionFlag = *(UINT32 *)Ptr;

  if ((ProtectionFlag & 0xFFFFFFF8) != 0) {
    IncrementErrorCount ();
    Print (L"ERROR: Reserved bits are not zero.\n");
  }
}

/**
  An ACPI_PARSER array describing the WSMT Protection flag .
**/
STATIC CONST ACPI_PARSER  WsmtProtectionFlagParser[] = {
  { L"FIXED_COMM_BUFFERS ",                1,  0, L"0x%x", NULL, NULL, NULL,             NULL },
  { L"COMM_BUFFER_NESTED_PTR_PROTECTION ", 1,  1, L"0x%x", NULL, NULL, NULL,             NULL },
  { L"SYSTEM_RESOURCE_PROTECTION ",        1,  2, L"0x%x", NULL, NULL, NULL,             NULL },
  { L"Reserved ",                          29, 3, L"0x%x", NULL, NULL, ValidateReserved, NULL },
};

/**
  This function prints WSMT Protection flag.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpWsmtProtectionFlag (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (WsmtProtectionFlagParser)
    );
}

/**
  An ACPI_PARSER array describing the ACPI WSMT Table.
**/
STATIC CONST ACPI_PARSER  WsmtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Protection Flag",            4,36, NULL, DumpWsmtProtectionFlag, NULL, ValidateWsmtProtectionFlag, NULL }
};

/**
  This function parses the ACPI WSMT table.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiWsmt (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  ParseAcpi (
    Trace,
    0,
    "WSMT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (WsmtParser)
    );
}
