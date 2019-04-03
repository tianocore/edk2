/** @file
  SRAT table parser

  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <IndustryStandard/Acpi.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local Variables
STATIC CONST UINT8* SratRAType;
STATIC CONST UINT8* SratRALength;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  This function validates the Reserved field in the SRAT table header.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateSratReserved (
  IN UINT8* Ptr,
  IN VOID*  Context
  );

/**
  This function traces the APIC Proximity Domain field.

  @param [in] Format  Format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
STATIC
VOID
EFIAPI
DumpSratApicProximity (
  IN  CONST CHAR16*  Format,
  IN  UINT8*         Ptr
  );

/**
  An ACPI_PARSER array describing the SRAT Table.
**/
STATIC CONST ACPI_PARSER SratParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"Reserved", 4, 36, L"0x%x", NULL, NULL, ValidateSratReserved, NULL},
  {L"Reserved", 8, 40, L"0x%lx", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Resource Allocation structure header.
**/
STATIC CONST ACPI_PARSER SratResourceAllocationParser[] = {
  {L"Type", 1, 0, NULL, NULL, (VOID**)&SratRAType, NULL, NULL},
  {L"Length", 1, 1, NULL, NULL, (VOID**)&SratRALength, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the GICC Affinity structure.
**/
STATIC CONST ACPI_PARSER SratGicCAffinityParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Proximity Domain", 4, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"ACPI Processor UID", 4, 6, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 10, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Clock Domain", 4, 14, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the GIC ITS Affinity structure.
**/
STATIC CONST ACPI_PARSER SratGicITSAffinityParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Proximity Domain", 4, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 6, L"0x%x", NULL, NULL, NULL, NULL},
  {L"ITS Id", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
};

/**
  An ACPI_PARSER array describing the Memory Affinity structure.
**/
STATIC CONST ACPI_PARSER SratMemAffinityParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Proximity Domain", 4, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 6, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Base Address Low", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Base Address High", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length Low", 4, 16, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length High", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 4, 24, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 28, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 8, 32, L"0x%lx", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the APIC/SAPIC Affinity structure.
**/
STATIC CONST ACPI_PARSER SratApciSapicAffinityParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Proximity Domain [7:0]", 1, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"APIC ID", 1, 3, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Local SAPIC EID", 1, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Proximity Domain [31:8]", 3, 9, L"0x%x", DumpSratApicProximity,
   NULL, NULL, NULL},
  {L"Clock Domain", 4, 12, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Processor Local x2APIC Affinity structure.
**/
STATIC CONST ACPI_PARSER SratX2ApciAffinityParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Proximity Domain", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"X2APIC ID", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Clock Domain", 4, 16, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 4, 20, L"0x%x", NULL, NULL, NULL, NULL}
};

/** This function validates the Reserved field in the SRAT table header.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateSratReserved (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  if (*(UINT32*)Ptr != 1) {
    IncrementErrorCount ();
    Print (L"\nERROR: Reserved should be 1 for backward compatibility.\n");
  }
}

/**
  This function traces the APIC Proximity Domain field.

  @param [in] Format  Format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
STATIC
VOID
EFIAPI
DumpSratApicProximity (
 IN CONST CHAR16* Format,
 IN UINT8*        Ptr
 )
{
  UINT32 ProximityDomain;

  ProximityDomain = Ptr[0] | (Ptr[1] << 8) | (Ptr[2] << 16);

  Print (Format, ProximityDomain);
}

/**
  This function parses the ACPI SRAT table.
  When trace is enabled this function parses the SRAT table and
  traces the ACPI table fields.

  This function parses the following Resource Allocation Structures:
    - Processor Local APIC/SAPIC Affinity Structure
    - Memory Affinity Structure
    - Processor Local x2APIC Affinity Structure
    - GICC Affinity Structure

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSrat (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT8* ResourcePtr;
  UINT32 GicCAffinityIndex;
  UINT32 GicITSAffinityIndex;
  UINT32 MemoryAffinityIndex;
  UINT32 ApicSapicAffinityIndex;
  UINT32 X2ApicAffinityIndex;
  CHAR8  Buffer[80];  // Used for AsciiName param of ParseAcpi

  GicCAffinityIndex = 0;
  GicITSAffinityIndex = 0;
  MemoryAffinityIndex = 0;
  ApicSapicAffinityIndex = 0;
  X2ApicAffinityIndex = 0;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "SRAT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (SratParser)
             );
  ResourcePtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    ParseAcpi (
      FALSE,
      0,
      NULL,
      ResourcePtr,
      2,  // The length is 1 byte at offset 1
      PARSER_PARAMS (SratResourceAllocationParser)
      );

    switch (*SratRAType) {
      case EFI_ACPI_6_2_GICC_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "GICC Affinity Structure [%d]",
          GicCAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratGicCAffinityParser)
          );
        break;

      case EFI_ACPI_6_2_GIC_ITS_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "GIC ITS Affinity Structure [%d]",
          GicITSAffinityIndex++
        );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratGicITSAffinityParser)
        );
        break;

      case EFI_ACPI_6_2_MEMORY_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "Memory Affinity Structure [%d]",
          MemoryAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratMemAffinityParser)
          );
        break;

      case EFI_ACPI_6_2_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "APIC/SAPIC Affinity Structure [%d]",
          ApicSapicAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratApciSapicAffinityParser)
          );
        break;

      case EFI_ACPI_6_2_PROCESSOR_LOCAL_X2APIC_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "X2APIC Affinity Structure [%d]",
          X2ApicAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratX2ApciAffinityParser)
          );
        break;

      default:
        IncrementErrorCount ();
        Print (L"ERROR: Unknown SRAT Affinity type = 0x%x\n", *SratRAType);
        break;
    }

    ResourcePtr += (*SratRALength);
    Offset += (*SratRALength);
  }
}
