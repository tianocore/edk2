/** @file
  PPTT table parser

  Copyright (c) 2019 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
    - ARM Architecture Reference Manual ARMv8 (D.a)
**/

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"
#include "PpttParser.h"

// Local variables
STATIC CONST UINT8*  ProcessorTopologyStructureType;
STATIC CONST UINT8*  ProcessorTopologyStructureLength;
STATIC CONST UINT32* NumberOfPrivateResources;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  This function validates the Cache Type Structure (Type 1) 'Number of sets'
  field.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateCacheNumberOfSets (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT32 NumberOfSets;
  NumberOfSets = *(UINT32*)Ptr;

  if (NumberOfSets == 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Cache number of sets must be greater than 0");
    return;
  }

#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (NumberOfSets > PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: When ARMv8.3-CCIDX is implemented the maximum cache number of "
        L"sets must be less than or equal to %d",
      PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX
      );
    return;
  }

  if (NumberOfSets > PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX) {
    IncrementWarningCount ();
    Print (
      L"\nWARNING: Without ARMv8.3-CCIDX, the maximum cache number of sets "
        L"must be less than or equal to %d. Ignore this message if "
        L"ARMv8.3-CCIDX is implemented",
      PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX
      );
    return;
  }
#endif

}

/**
  This function validates the Cache Type Structure (Type 1) 'Associativity'
  field.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateCacheAssociativity (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT8 Associativity;
  Associativity = *(UINT8*)Ptr;

  if (Associativity == 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Cache associativity must be greater than 0");
    return;
  }
}

/**
  This function validates the Cache Type Structure (Type 1) Line size field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateCacheLineSize (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  // Reference: ARM Architecture Reference Manual ARMv8 (D.a)
  // Section D12.2.25: CCSIDR_EL1, Current Cache Size ID Register
  //   LineSize, bits [2:0]
  //     (Log2(Number of bytes in cache line)) - 4.

  UINT16 LineSize;
  LineSize = *(UINT16*)Ptr;

  if ((LineSize < PPTT_ARM_CACHE_LINE_SIZE_MIN) ||
      (LineSize > PPTT_ARM_CACHE_LINE_SIZE_MAX)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: The cache line size must be between %d and %d bytes"
        L" on ARM Platforms.",
      PPTT_ARM_CACHE_LINE_SIZE_MIN,
      PPTT_ARM_CACHE_LINE_SIZE_MAX
      );
    return;
  }

  if ((LineSize & (LineSize - 1)) != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: The cache line size is not a power of 2.");
  }
#endif
}

/**
  This function validates the Cache Type Structure (Type 1) Attributes field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateCacheAttributes (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  // Reference: Advanced Configuration and Power Interface (ACPI) Specification
  //            Version 6.2 Errata A, September 2017
  // Table 5-153: Cache Type Structure
  UINT8 Attributes;
  Attributes = *(UINT8*)Ptr;

  if ((Attributes & 0xE0) != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Attributes bits [7:5] are reserved and must be zero.",
      Attributes
      );
    return;
  }
}

/**
  An ACPI_PARSER array describing the ACPI PPTT Table.
**/
STATIC CONST ACPI_PARSER PpttParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo)
};

/**
  An ACPI_PARSER array describing the processor topology structure header.
**/
STATIC CONST ACPI_PARSER ProcessorTopologyStructureHeaderParser[] = {
  {L"Type", 1, 0, NULL, NULL, (VOID**)&ProcessorTopologyStructureType,
   NULL, NULL},
  {L"Length", 1, 1, NULL, NULL, (VOID**)&ProcessorTopologyStructureLength,
   NULL, NULL},
  {L"Reserved", 2, 2, NULL, NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Processor Hierarchy Node Structure - Type 0.
**/
STATIC CONST ACPI_PARSER ProcessorHierarchyNodeStructureParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Flags", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Parent", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"ACPI Processor ID", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Number of private resources", 4, 16, L"%d", NULL,
   (VOID**)&NumberOfPrivateResources, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Cache Type Structure - Type 1.
**/
STATIC CONST ACPI_PARSER CacheTypeStructureParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Flags", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Next Level of Cache", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Size", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Number of sets", 4, 16, L"%d", NULL, NULL, ValidateCacheNumberOfSets, NULL},
  {L"Associativity", 1, 20, L"%d", NULL, NULL, ValidateCacheAssociativity, NULL},
  {L"Attributes", 1, 21, L"0x%x", NULL, NULL, ValidateCacheAttributes, NULL},
  {L"Line size", 2, 22, L"%d", NULL, NULL, ValidateCacheLineSize, NULL}
};

/**
  An ACPI_PARSER array describing the ID Type Structure - Type 2.
**/
STATIC CONST ACPI_PARSER IdStructureParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"VENDOR_ID", 4, 4, NULL, Dump4Chars, NULL, NULL, NULL},
  {L"LEVEL_1_ID", 8, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"LEVEL_2_ID", 8, 16, L"0x%x", NULL, NULL, NULL, NULL},
  {L"MAJOR_REV", 2, 24, L"0x%x", NULL, NULL, NULL, NULL},
  {L"MINOR_REV", 2, 26, L"0x%x", NULL, NULL, NULL, NULL},
  {L"SPIN_REV", 2, 28, L"0x%x", NULL, NULL, NULL, NULL},
};

/**
  This function parses the Processor Hierarchy Node Structure (Type 0).

  @param [in] Ptr     Pointer to the start of the Processor Hierarchy Node
                      Structure data.
  @param [in] Length  Length of the Processor Hierarchy Node Structure.
**/
STATIC
VOID
DumpProcessorHierarchyNodeStructure (
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  UINT32 Offset;
  UINT32 Index;
  CHAR16 Buffer[OUTPUT_FIELD_COLUMN_WIDTH];

  Offset = ParseAcpi (
             TRUE,
             2,
             "Processor Hierarchy Node Structure",
             Ptr,
             Length,
             PARSER_PARAMS (ProcessorHierarchyNodeStructureParser)
             );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if (NumberOfPrivateResources == NULL) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient Processor Hierarchy Node length. Length = %d.\n",
      Length
      );
    return;
  }

  // Make sure the Private Resource array lies inside this structure
  if (Offset + (*NumberOfPrivateResources * sizeof (UINT32)) > Length) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Number of Private Resources. " \
        L"PrivateResourceCount = %d. RemainingBufferLength = %d. " \
        L"Parsing of this structure aborted.\n",
      *NumberOfPrivateResources,
      Length - Offset
      );
    return;
  }

  Index = 0;

  // Parse the specified number of private resource references or the Processor
  // Hierarchy Node length. Whichever is minimum.
  while (Index < *NumberOfPrivateResources) {
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"Private resources [%d]",
      Index
      );

    PrintFieldName (4, Buffer);
    Print (
      L"0x%x\n",
      *((UINT32*)(Ptr + Offset))
      );

    Offset += sizeof (UINT32);
    Index++;
  }
}

/**
  This function parses the Cache Type Structure (Type 1).

  @param [in] Ptr     Pointer to the start of the Cache Type Structure data.
  @param [in] Length  Length of the Cache Type Structure.
**/
STATIC
VOID
DumpCacheTypeStructure (
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Cache Type Structure",
    Ptr,
    Length,
    PARSER_PARAMS (CacheTypeStructureParser)
    );
}

/**
  This function parses the ID Structure (Type 2).

  @param [in] Ptr     Pointer to the start of the ID Structure data.
  @param [in] Length  Length of the ID Structure.
**/
STATIC
VOID
DumpIDStructure (
  IN UINT8* Ptr,
  IN UINT8 Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "ID Structure",
    Ptr,
    Length,
    PARSER_PARAMS (IdStructureParser)
    );
}

/**
  This function parses the ACPI PPTT table.
  When trace is enabled this function parses the PPTT table and
  traces the ACPI table fields.

  This function parses the following processor topology structures:
    - Processor hierarchy node structure (Type 0)
    - Cache Type Structure (Type 1)
    - ID structure (Type 2)

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiPptt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT8* ProcessorTopologyStructurePtr;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "PPTT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (PpttParser)
             );

  ProcessorTopologyStructurePtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    // Parse Processor Hierarchy Node Structure to obtain Type and Length.
    ParseAcpi (
      FALSE,
      0,
      NULL,
      ProcessorTopologyStructurePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (ProcessorTopologyStructureHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((ProcessorTopologyStructureType == NULL) ||
        (ProcessorTopologyStructureLength == NULL)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
          L"processor topology structure header. Length = %d.\n",
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate Processor Topology Structure length
    if ((*ProcessorTopologyStructureLength == 0) ||
        ((Offset + (*ProcessorTopologyStructureLength)) > AcpiTableLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid Processor Topology Structure length. " \
          L"Length = %d. Offset = %d. AcpiTableLength = %d.\n",
        *ProcessorTopologyStructureLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    PrintFieldName (2, L"* Structure Offset *");
    Print (L"0x%x\n", Offset);

    switch (*ProcessorTopologyStructureType) {
      case EFI_ACPI_6_2_PPTT_TYPE_PROCESSOR:
        DumpProcessorHierarchyNodeStructure (
          ProcessorTopologyStructurePtr,
          *ProcessorTopologyStructureLength
          );
        break;
      case EFI_ACPI_6_2_PPTT_TYPE_CACHE:
        DumpCacheTypeStructure (
          ProcessorTopologyStructurePtr,
          *ProcessorTopologyStructureLength
          );
        break;
      case EFI_ACPI_6_2_PPTT_TYPE_ID:
        DumpIDStructure (
          ProcessorTopologyStructurePtr,
          *ProcessorTopologyStructureLength
          );
        break;
      default:
        IncrementErrorCount ();
        Print (
          L"ERROR: Unknown processor topology structure:"
            L" Type = %d, Length = %d\n",
          *ProcessorTopologyStructureType,
          *ProcessorTopologyStructureLength
          );
    }

    ProcessorTopologyStructurePtr += *ProcessorTopologyStructureLength;
    Offset += *ProcessorTopologyStructureLength;
  } // while
}
