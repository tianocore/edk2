/** @file
  SSDT table parser

  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

/**
  This function parses the ACPI SSDT table.
  When trace is enabled this function parses the SSDT table and
  traces the ACPI table fields.
  For the SSDT table only the ACPI header fields are
  parsed and traced.

  @param [in] ParseFlags         Flags describing what the parser needs to do.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSsdt (
  IN UINT8   ParseFlags,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  if (!IS_TRACE_FLAG_SET (ParseFlags)) {
    return;
  }

  DumpAcpiHeader (Ptr);
}
