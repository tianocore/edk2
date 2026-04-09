/** @file
  APMT table parser

  Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
  Copyright (c) 2017 - 2018, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/ArmPerformanceMonitoringUnitTable.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;
STATIC CONST UINT16                  *NodeLength;

/**
  An ACPI_PARSER array describing the ACPI APMT Table.
**/
STATIC CONST ACPI_PARSER  ApmtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo)
};

/**
  An ACPI_PARSER array describing the ACPI Arm PMU Node.
**/
STATIC CONST ACPI_PARSER  ArmPmuNodeParser[] = {
  { L"Length",                   2, 0,  L"0x%x",  NULL, (VOID **)&NodeLength, NULL, NULL },
  { L"Node flags",               1, 2,  L"0x%x",  NULL, NULL,                 NULL, NULL },
  { L"Node type",                1, 3,  L"0x%x",  NULL, NULL,                 NULL, NULL },
  { L"Identifier",               4, 4,  L"0x%x",  NULL, NULL,                 NULL, NULL },
  { L"Node Instance primary",    8, 8,  L"0x%lx", NULL, NULL,                 NULL, NULL },
  { L"Node Instance secondary",  4, 16, L"0x%x",  NULL, NULL,                 NULL, NULL },
  { L"Base address 0",           8, 20, L"0x%lx", NULL, NULL,                 NULL, NULL },
  { L"Base address 1",           8, 28, L"0x%lx", NULL, NULL,                 NULL, NULL },
  { L"Overflow interrupt",       4, 36, L"0x%x",  NULL, NULL,                 NULL, NULL },
  { L"Reserved1",                4, 40, L"0x%x",  NULL, NULL,                 NULL, NULL },
  { L"Overflow interrupt flags", 4, 44, L"0x%x",  NULL, NULL,                 NULL, NULL },
  { L"Processor affinity",       4, 48, L"0x%x",  NULL, NULL,                 NULL, NULL },
  { L"Implementation ID",        4, 52, L"0x%x",  NULL, NULL,                 NULL, NULL }
};

/**
  This function parses the ACPI APMT table.
  When trace is enabled this function parses the APMT table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiApmt (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;

  if (!Trace) {
    return;
  }

  ParseAcpi (
    Trace,
    0,
    "APMT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (ApmtParser)
    );
  Offset = sizeof (EFI_ACPI_DESCRIPTION_HEADER);

  while (Offset < AcpiTableLength) {
    ParseAcpi (
      Trace,
      2,
      "Arm PMU node",
      Ptr + Offset,
      (AcpiTableLength - Offset),
      PARSER_PARAMS (ArmPmuNodeParser)
      );
    if (NodeLength == NULL) {
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
        L"Node structure. Length = %d.\n",
        (AcpiTableLength - Offset)
        );
      IncrementErrorCount ();
      break;
    }

    Offset += *NodeLength;
  }
}
