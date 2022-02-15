/** @file
  DSDT table parser

  Copyright (c) 2016 - 2022, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

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
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  DumpAcpiHeader (Ptr);

  // As per 19.6.29 in the version 6.4 of the ACPI spec, a revision less than 2
  // restricts integers to 32 bit width. This may not be intended, raise a
  // warning
 #if defined (MDE_CPU_AARCH64) || defined (MDE_CPU_ARM)
  if (AcpiTableRevision < 2) {
    IncrementWarningCount ();
    Print (
      L"WARNING: DSDT Table Revision less than 2. Integer width restricted to "
      L"32 bits. Table Revision = %d.\n",
      AcpiTableRevision
      );
    return;
  }

 #endif
}
