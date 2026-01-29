/** @file
  MCFG table parser

  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - PCI Firmware Specification - Revision 3.2, January 26, 2015.
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI MCFG Table.
**/
STATIC CONST ACPI_PARSER  McfgParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Reserved",                   8,36, L"0x%lx", NULL, NULL, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the PCI configuration Space Base Address structure.
**/
STATIC CONST ACPI_PARSER  PciCfgSpaceBaseAddrParser[] = {
  { L"Base Address",          8, 0,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"PCI Segment Group No.", 2, 8,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Start Bus No.",         1, 10, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"End Bus No.",           1, 11, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",              4, 12, L"0x%x",  NULL, NULL, NULL, NULL }
};

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
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;
  UINT32  PciCfgOffset;
  UINT8   *PciCfgSpacePtr;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "MCFG",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (McfgParser)
             );

  PciCfgSpacePtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    PciCfgOffset = ParseAcpi (
                     TRUE,
                     2,
                     "PCI Configuration Space",
                     PciCfgSpacePtr,
                     (AcpiTableLength - Offset),
                     PARSER_PARAMS (PciCfgSpaceBaseAddrParser)
                     );
    PciCfgSpacePtr += PciCfgOffset;
    Offset         += PciCfgOffset;
  }
}
