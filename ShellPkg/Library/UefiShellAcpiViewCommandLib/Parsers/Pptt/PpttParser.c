/** @file
  PPTT table parser

  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.
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
#include "DotGenerator.h"

// Local variables
STATIC CONST UINT8*  ProcessorTopologyStructureType;
STATIC CONST UINT8*  ProcessorTopologyStructureLength;

STATIC CONST UINT32* NumberOfPrivateResources;
STATIC CONST UINT32* ProcessorHierarchyParent;
STATIC CONST EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR_FLAGS* ProcStructFlags;
STATIC CONST UINT32* NextLevelOfCache;
STATIC CONST EFI_ACPI_6_3_PPTT_STRUCTURE_CACHE_ATTRIBUTES* CacheAttributes;
STATIC CONST UINT32* CacheSize;

STATIC CONST UINT8*  PpttStartPointer;

STATIC SHELL_FILE_HANDLE mDotFileHandle;

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

  {L"Flags", 4, 4, L"0x%x", NULL, (VOID**)&ProcStructFlags, NULL, NULL},
  {L"Parent", 4, 8, L"0x%x", NULL,
    (VOID**)&ProcessorHierarchyParent, NULL, NULL},
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
  {L"Next Level of Cache", 4, 8, L"0x%x", NULL,
    (VOID**)&NextLevelOfCache, NULL, NULL},
  {L"Size", 4, 12, L"0x%x", NULL, (VOID**)&CacheSize, NULL, NULL},
  {L"Number of sets", 4, 16, L"%d", NULL, NULL, ValidateCacheNumberOfSets, NULL},
  {L"Associativity", 1, 20, L"%d", NULL, NULL, ValidateCacheAssociativity, NULL},
  {L"Attributes", 1, 21, L"0x%x", NULL, (VOID**)&CacheAttributes,
    ValidateCacheAttributes, NULL},
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

  @param [in] ParseFlags   Flags describing what the parser needs to do.
  @param [in] Ptr          Pointer to the start of the Processor Hierarchy Node
                           Structure data.
  @param [in] Length       Length of the Processor Hierarchy Node Structure.
**/
STATIC
VOID
DumpProcessorHierarchyNodeStructure (
  IN UINT8  ParseFlags,
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  UINT32 Offset;
  UINT32 Index;
  CHAR16 Buffer[OUTPUT_FIELD_COLUMN_WIDTH];
  CONST UINT8* TypePtr;

  Offset = ParseAcpi (
             IS_TRACE_FLAG_SET (ParseFlags),
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

  if (IS_GRAPH_FLAG_SET (ParseFlags)) {
    if (ProcStructFlags->ProcessorIsAThread) {
      UnicodeSPrint(Buffer, sizeof (Buffer), L"Thread");
    } else if (ProcStructFlags->NodeIsALeaf) {
      UnicodeSPrint(Buffer, sizeof (Buffer), L"Core");
    } else if (ProcStructFlags->PhysicalPackage) {
      UnicodeSPrint(Buffer, sizeof (Buffer), L"Physical\\nPackage");
    } else {
      UnicodeSPrint(Buffer, sizeof (Buffer), L"Cluster");
    }

    DotAddNode (
      mDotFileHandle,
      (UINT32)(Ptr - PpttStartPointer),
      DOT_BOX_SQUARE | DOT_COLOR_BLUE | DOT_BOX_ADD_ID_TO_LABEL,
      Buffer
      );

    // Add link to parent node.
    if (*ProcessorHierarchyParent != 0) {
      DotAddLink (
        mDotFileHandle,
        (UINT32)(Ptr - PpttStartPointer),
        *ProcessorHierarchyParent,
        0x0
        );
    }
  }

  // Parse the specified number of private resource references or the Processor
  // Hierarchy Node length. Whichever is minimum.
  for (Index = 0; Index < *NumberOfPrivateResources; Index++) {
    if (IS_TRACE_FLAG_SET (ParseFlags)) {
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
    }

    if (IS_GRAPH_FLAG_SET (ParseFlags)) {
      TypePtr = PpttStartPointer + *((UINT32*)(Ptr + Offset));
      if (*TypePtr == EFI_ACPI_6_2_PPTT_TYPE_ID) {
        continue;
      }
      DotAddLink (
        mDotFileHandle,
        *((UINT32*)(Ptr + Offset)),
        (UINT32)(Ptr - PpttStartPointer),
        DOT_ARROW_RANK_REVERSE
        );
    }

    Offset += sizeof (UINT32);
  }
}

/**
  This function parses the Cache Type Structure (Type 1).

  @param [in] ParseFlags  Flags describing what the parser needs to do.
  @param [in] Ptr         Pointer to the start of the Cache Type Structure data.
  @param [in] Length      Length of the Cache Type Structure.
**/
STATIC
VOID
DumpCacheTypeStructure (
  IN UINT8  ParseFlags,
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  CHAR16 LabelBuffer[64];

  ParseAcpi (
    IS_TRACE_FLAG_SET (ParseFlags),
    2,
    "Cache Type Structure",
    Ptr,
    Length,
    PARSER_PARAMS (CacheTypeStructureParser)
    );

  if (IS_GRAPH_FLAG_SET (ParseFlags)) {
    // Create cache node

    // Start node label with type of cache
    switch (CacheAttributes->CacheType) {
      case EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA:
        UnicodeSPrint (
          LabelBuffer,
          sizeof (LabelBuffer),
          L"D-Cache\\n"
          );
        break;
      case EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION:
        UnicodeSPrint (
          LabelBuffer,
          sizeof (LabelBuffer),
          L"I-Cache\\n"
          );
        break;
      default:
        UnicodeSPrint (
          LabelBuffer,
          sizeof (LabelBuffer),
          L"Unified Cache\\n"
          );
    }

    // Add size of cache to node label
    if (((*CacheSize) & 0xfff00000) != 0) {
      UnicodeSPrint (
        LabelBuffer,
        sizeof (LabelBuffer),
        L"%s%dMiB",
        LabelBuffer,
        *CacheSize >> 20
        );
    }
    if ((*CacheSize & 0xffc00) != 0) {
      UnicodeSPrint (
        LabelBuffer,
        sizeof (LabelBuffer),
        L"%s%dkiB",
        LabelBuffer,
        (*CacheSize >> 10) & 0x3ff
        );
    }
    if ((*CacheSize & 0x3ff) != 0) {
      UnicodeSPrint (
        LabelBuffer,
        sizeof (LabelBuffer),
        L"%s%dB",
        LabelBuffer,
        *CacheSize & 0x3ff
        );
    }
    if (*CacheSize == 0) {
      UnicodeSPrint (
        LabelBuffer,
        sizeof (LabelBuffer),
        L"%s0B",
        LabelBuffer
        );
    }

    //Add node to dot file
    DotAddNode (
      mDotFileHandle,
      (UINT32)(Ptr - PpttStartPointer),
      DOT_BOX_SQUARE | DOT_COLOR_YELLOW | DOT_BOX_ADD_ID_TO_LABEL,
      LabelBuffer
      );

    if (*NextLevelOfCache != 0) {
      DotAddLink (
        mDotFileHandle,
        *NextLevelOfCache,
        (UINT32)(Ptr - PpttStartPointer),
        DOT_ARROW_RANK_REVERSE | DOT_COLOR_GRAY
        );
    }
  }
}

/**
  This function parses the ID Structure (Type 2).

  @param [in] ParseFlags         Flags describing what the parser needs to do.
  @param [in] Ptr                Pointer to the start of the ID Structure data.
  @param [in] Length             Length of the ID Structure.
**/
STATIC
VOID
DumpIDStructure (
  IN UINT8  ParseFlags,
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  ParseAcpi (
    IS_TRACE_FLAG_SET (ParseFlags),
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

  @param [in] ParseFlags         Flags describing what the parser needs to do.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiPptt (
  IN UINT8   ParseFlags,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  EFI_STATUS  Status;
  UINT32      Offset;
  UINT8*      ProcessorTopologyStructurePtr;
  CHAR16      Buffer[128];
  CHAR16      FileNameBuffer[MAX_FILE_NAME_LEN];

  if (!IS_TRACE_FLAG_SET (ParseFlags) &&
      !IS_GRAPH_FLAG_SET (ParseFlags)) {
    return;
  }

  if (IS_GRAPH_FLAG_SET (ParseFlags)) {
    Status = GetNewFileName (
               L"PPTT",
               L"dot",
               FileNameBuffer,
               sizeof (FileNameBuffer)
               );

    if (EFI_ERROR (Status)) {
      Print (
        L"Error: Could not open dot file for PPTT table:\n"
        L"Could not get a file name."
        );
      // Abandonning creation of dot graph by unsetting the flag.
      // We continue parsing in case trace is set.
      ParseFlags &= ~PARSE_FLAGS_GRAPH;
    } else {
      mDotFileHandle = DotOpenNewFile (FileNameBuffer);
      if (mDotFileHandle == NULL) {
        Print (L"ERROR: Could not open dot file for PPTT table.\n");
        // Abandonning creation of dot graph by unsetting the flag.
        // We continue parsing in case trace is set.
        ParseFlags &= ~PARSE_FLAGS_GRAPH;
      }
    }
  }

  PpttStartPointer = Ptr;

  Offset = ParseAcpi (
             IS_TRACE_FLAG_SET (ParseFlags),
             0,
             "PPTT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (PpttParser)
             );

  if (*(AcpiHdrInfo.Revision) < 2 &&
       IS_GRAPH_FLAG_SET (ParseFlags)) {
    Print (L"\nWARNING: Dot output may not be consistent for PPTT revisions < 2\n");
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"WARNING: PPTT table revision is %u.\\n" \
      L"Revisions lower than 2 might lead to incorrect labelling",
      *(AcpiHdrInfo.Revision)
      );
    DotAddNode (
      mDotFileHandle,
      0,
      DOT_COLOR_RED | DOT_BOX_SQUARE,
      Buffer
      );
  }

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

    if (IS_TRACE_FLAG_SET (ParseFlags)) {
      PrintFieldName (2, L"* Structure Offset *");
      Print (L"0x%x\n", Offset);
    }

    switch (*ProcessorTopologyStructureType) {
      case EFI_ACPI_6_2_PPTT_TYPE_PROCESSOR:
        DumpProcessorHierarchyNodeStructure (
          ParseFlags,
          ProcessorTopologyStructurePtr,
          *ProcessorTopologyStructureLength
          );
        break;
      case EFI_ACPI_6_2_PPTT_TYPE_CACHE:
        DumpCacheTypeStructure (
          ParseFlags,
          ProcessorTopologyStructurePtr,
          *ProcessorTopologyStructureLength
          );
        break;
      case EFI_ACPI_6_2_PPTT_TYPE_ID:
        DumpIDStructure (
          ParseFlags,
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

  if (IS_GRAPH_FLAG_SET (ParseFlags)) {
    DotCloseFile (mDotFileHandle);
  }
}
