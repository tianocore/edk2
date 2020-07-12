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
#include "AcpiViewLog.h"
#include "PpttParser.h"
#include "AcpiViewLog.h"

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
  UINT32 CacheNumberOfSets;

  CacheNumberOfSets = *(UINT32*) Ptr;
  AssertConstraint (L"ACPI", CacheNumberOfSets != 0);

#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (AssertConstraint (
        L"ARMv8.3-CCIDX",
        CacheNumberOfSets <= PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX)) {
    return;
  }

  WarnConstraint (
    L"No-ARMv8.3-CCIDX", CacheNumberOfSets <= PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX);
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
  UINT8 CacheAssociativity;

  CacheAssociativity = *Ptr;
  AssertConstraint (L"ACPI", CacheAssociativity != 0);
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

  UINT16 CacheLineSize;

  CacheLineSize = *(UINT16 *) Ptr;
  AssertConstraint (
    L"ARM",
    (CacheLineSize >= PPTT_ARM_CACHE_LINE_SIZE_MIN &&
     CacheLineSize <= PPTT_ARM_CACHE_LINE_SIZE_MAX));

  AssertConstraint (L"ARM", BitFieldCountOnes32 (CacheLineSize, 0, 15) == 1);
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

  Attributes = *(UINT8 *) Ptr;
  AssertConstraint (L"ACPI", BitFieldCountOnes32 (Attributes, 5, 7) == 0);
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
  if(NumberOfPrivateResources == NULL) {
    AcpiError (ACPI_ERROR_PARSE, L"Failed to parse processor hierarchy");
    return;
  }

  // Parse the specified number of private resource references or the Processor
  // Hierarchy Node length. Whichever is minimum.
  for (Index = 0; Index < *NumberOfPrivateResources; Index++) {
    if (AssertMemberIntegrity (Offset, sizeof (UINT32), Ptr, Length)) {
      return;
    }

    PrintFieldName (4, L"Private resources [%d]", Index);
    AcpiInfo (L"0x%x", *(UINT32 *) (Ptr + Offset));

    Offset += sizeof (UINT32);
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

  while (Offset < AcpiTableLength) {
    // Parse Processor Hierarchy Node Structure to obtain Type and Length.
    ParseAcpi (
      FALSE,
      0,
      NULL,
      Ptr + Offset,
      AcpiTableLength - Offset,
      PARSER_PARAMS (ProcessorTopologyStructureHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((ProcessorTopologyStructureType == NULL) ||
        (ProcessorTopologyStructureLength == NULL)) {
      AcpiError (ACPI_ERROR_PARSE, L"Failed to parse processor topology");
      return;
    }

    // Validate Processor Topology Structure length
    if (AssertMemberIntegrity (
          Offset, *ProcessorTopologyStructureLength, Ptr, AcpiTableLength)) {
      return;
    }

    PrintFieldName (2, L"* Structure Offset *");
    AcpiInfo (L"0x%x", Offset);

    switch (*ProcessorTopologyStructureType) {
      case EFI_ACPI_6_2_PPTT_TYPE_PROCESSOR:
        DumpProcessorHierarchyNodeStructure (
          Ptr + Offset,
          *ProcessorTopologyStructureLength
          );
        break;
      case EFI_ACPI_6_2_PPTT_TYPE_CACHE:
        DumpCacheTypeStructure (
          Ptr + Offset,
          *ProcessorTopologyStructureLength
          );
        break;
      case EFI_ACPI_6_2_PPTT_TYPE_ID:
        DumpIDStructure (
          Ptr + Offset,
          *ProcessorTopologyStructureLength
          );
        break;
      default:
        AcpiError (ACPI_ERROR_VALUE, L"Unknown processor topology structure");
    }

    Offset += *ProcessorTopologyStructureLength;
  } // while
}
