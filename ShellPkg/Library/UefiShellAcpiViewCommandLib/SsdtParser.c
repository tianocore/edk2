/** @file
*
*  Copyright (c) 2016, ARM Limited. All rights reserved.
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
#include "AcpiParser.h"
#include "AcpiView.h"


/** This function parses the ACPI SSDT table.
  This function parses the SSDT table and optionally traces the ACPI
  table fields. For the SSDT table only the ACPI header fields are
  parsed and traced.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiSsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  DumpAcpiHeader (Ptr);
}
