/** @file
  HMAT table parser

  Copyright (c) 2020, Arm Limited.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019

  @par Glossary:
    - MPDA  - Memory Proximity Domain Attributes
    - SLLBI - System Locality Latency and Bandwidth Information
    - MSCI  - Memory Side Cache Information
    - Dom   - Domain
**/

#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"

// Maximum Memory Domain matrix print size.
#define MAX_MEMORY_DOMAIN_TARGET_PRINT_MATRIX    10

// Local variables
STATIC CONST UINT16*  HmatStructureType;
STATIC CONST UINT32*  HmatStructureLength;

STATIC CONST UINT32*  NumberInitiatorProximityDomain;
STATIC CONST UINT32*  NumberTargetProximityDomain;
STATIC CONST
EFI_ACPI_6_3_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO_FLAGS*
SllbiFlags;

STATIC CONST UINT8*   SllbiDataType;
STATIC CONST UINT16*  NumberSMBIOSHandles;

STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  Names of System Locality Latency Bandwidth Information (SLLBI) data types
**/
STATIC CONST CHAR16* SllbiNames[] = {
  L"Access %sLatency%s",
  L"Read %sLatency%s",
  L"Write %sLatency%s",
  L"Access %sBandwidth%s",
  L"Read %sBandwidth%s",
  L"Write %sBandwidth%s"
};

/**
  This function validates the Cache Attributes field.

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
  EFI_ACPI_6_3_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO_CACHE_ATTRIBUTES*
  Attributes;

  Attributes =
    (EFI_ACPI_6_3_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO_CACHE_ATTRIBUTES*)Ptr;

  if (Attributes->TotalCacheLevels > 0x3) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Attributes bits [3:0] have invalid value: 0x%x",
      Attributes->TotalCacheLevels
      );
  }
  if (Attributes->CacheLevel > 0x3) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Attributes bits [7:4] have invalid value: 0x%x",
      Attributes->CacheLevel
      );
  }
  if (Attributes->CacheAssociativity > 0x2) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Attributes bits [11:8] have invalid value: 0x%x",
      Attributes->CacheAssociativity
      );
  }
  if (Attributes->WritePolicy > 0x2) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Attributes bits [15:12] have invalid value: 0x%x",
      Attributes->WritePolicy
      );
  }
}

/**
  Dumps the cache attributes field

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
STATIC
VOID
EFIAPI
DumpCacheAttributes (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  )
{
  EFI_ACPI_6_3_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO_CACHE_ATTRIBUTES*
  Attributes;

  Attributes =
    (EFI_ACPI_6_3_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO_CACHE_ATTRIBUTES*)Ptr;

  Print (L"\n");
  PrintFieldName (4, L"Total Cache Levels");
  Print (L"%d\n", Attributes->TotalCacheLevels);
  PrintFieldName (4, L"Cache Level");
  Print (L"%d\n", Attributes->CacheLevel);
  PrintFieldName (4, L"Cache Associativity");
  Print (L"%d\n", Attributes->CacheAssociativity);
  PrintFieldName (4, L"Write Policy");
  Print (L"%d\n", Attributes->WritePolicy);
  PrintFieldName (4, L"Cache Line Size");
  Print (L"%d\n", Attributes->CacheLineSize);
}

/**
  An ACPI_PARSER array describing the ACPI HMAT Table.
*/
STATIC CONST ACPI_PARSER HmatParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"Reserved", 4, 36, NULL, NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the HMAT structure header.
*/
STATIC CONST ACPI_PARSER HmatStructureHeaderParser[] = {
  {L"Type", 2, 0, NULL, NULL, (VOID**)&HmatStructureType, NULL, NULL},
  {L"Reserved", 2, 2, NULL, NULL, NULL, NULL, NULL},
  {L"Length", 4, 4, NULL, NULL, (VOID**)&HmatStructureLength, NULL, NULL}
};

/**
  An ACPI PARSER array describing the Memory Proximity Domain Attributes
  Structure - Type 0.
*/
STATIC CONST ACPI_PARSER MemProximityDomainAttributeParser[] = {
  {L"Type", 2, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 4, 4, L"%d", NULL, NULL, NULL, NULL},
  {L"Flags", 2, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 10, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Proximity Dom for initiator", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Proximity Dom for memory", 4, 16, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 8, 24, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Reserved", 8, 32, L"0x%lx", NULL, NULL, NULL, NULL}
};

/**
  An ACPI PARSER array describing the System Locality Latency and Bandwidth
  Information Structure - Type 1.
*/
STATIC CONST ACPI_PARSER SllbiParser[] = {
  {L"Type", 2, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 4, 4, L"%d", NULL, NULL, NULL, NULL},
  {L"Flags", 1, 8, L"0x%x", NULL, (VOID**)&SllbiFlags, NULL, NULL},
  {L"Data type", 1, 9, L"0x%x", NULL, (VOID**)&SllbiDataType, NULL, NULL},
  {L"Reserved", 2, 10, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Initiator Proximity Dom Count", 4, 12, L"%d", NULL,
    (VOID**)&NumberInitiatorProximityDomain, NULL, NULL},
  {L"Target Proximity Dom Count", 4, 16, L"%d", NULL,
    (VOID**)&NumberTargetProximityDomain, NULL, NULL},
  {L"Reserved", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Entry Base Unit", 8, 24, L"0x%lx", NULL, NULL, NULL, NULL}
  // initiator Proximity Domain list ...
  // target Proximity Domain list ...
  // Latency/Bandwidth matrix ...
};

/**
  An ACPI PARSER array describing the Memory Side Cache Information
  Structure - Type 2.
*/
STATIC CONST ACPI_PARSER MemSideCacheInfoParser[] = {
  {L"Type", 2, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 4, 4, L"%d", NULL, NULL, NULL, NULL},
  {L"Proximity Dom for memory", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Memory Side Cache Size", 8, 16, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Cache Attributes", 4, 24, NULL, DumpCacheAttributes, NULL,
    ValidateCacheAttributes, NULL},
  {L"Reserved", 2, 28, L"0x%x", NULL, NULL, NULL, NULL},
  {L"SMBIOS Handle Count", 2, 30, L"%d", NULL,
    (VOID**)&NumberSMBIOSHandles, NULL, NULL}
  // SMBIOS handles List ...
};

/**
  This function parses the Memory Proximity Domain Attributes
  Structure (Type 0).

  @param [in] Ptr     Pointer to the start of the Memory Proximity Domain
                      Attributes Structure data.
  @param [in] Length  Length of the Memory Proximity Domain Attributes
                      Structure.
**/
STATIC
VOID
DumpMpda (
  IN UINT8* Ptr,
  IN UINT32 Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Memory Proximity Domain Attributes Structure",
    Ptr,
    Length,
    PARSER_PARAMS (MemProximityDomainAttributeParser)
    );
}

/**
  This function parses the System Locality Latency and Bandwidth Information
  Structure (Type 1).

  @param [in] Ptr     Pointer to the start of the System Locality Latency and
                      Bandwidth Information Structure data.
  @param [in] Length  Length of the System Locality Latency and Bandwidth
                      Information Structure.
**/
STATIC
VOID
DumpSllbi (
  IN UINT8* Ptr,
  IN UINT32 Length
  )
{
  CONST UINT32* InitiatorProximityDomainList;
  CONST UINT32* TargetProximityDomainList;
  CONST UINT16* LatencyBandwidthMatrix;
  UINT32        Offset;
  CHAR16        Buffer[OUTPUT_FIELD_COLUMN_WIDTH];
  CHAR16        SecondBuffer[OUTPUT_FIELD_COLUMN_WIDTH];
  UINT32        RequiredTableSize;
  UINT32        Index;
  UINT32        IndexInitiator;
  UINT32        IndexTarget;
  UINT32        TargetStartOffset;

  Offset = ParseAcpi (
             TRUE,
             2,
             "System Locality Latency and Bandwidth Information Structure",
             Ptr,
             Length,
             PARSER_PARAMS (SllbiParser)
             );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((SllbiFlags == NULL)                     ||
      (SllbiDataType == NULL)                  ||
      (NumberInitiatorProximityDomain == NULL) ||
      (NumberTargetProximityDomain == NULL)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient remaining table buffer length to read the " \
        L"SLLBI structure header. Length = %d.\n",
      Length
      );
    return;
  }

  RequiredTableSize = (*NumberInitiatorProximityDomain * sizeof (UINT32)) +
                      (*NumberTargetProximityDomain * sizeof (UINT32)) +
                      (*NumberInitiatorProximityDomain *
                       *NumberTargetProximityDomain * sizeof (UINT16)) +
                      Offset;

  if (RequiredTableSize > Length) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient System Locality Latency and Bandwidth" \
      L"Information Structure length. TableLength = %d. " \
      L"RequiredTableLength = %d.\n",
      Length,
      RequiredTableSize
      );
    return;
  }

  InitiatorProximityDomainList = (UINT32*) (Ptr + Offset);
  TargetProximityDomainList = InitiatorProximityDomainList +
                              *NumberInitiatorProximityDomain;
  LatencyBandwidthMatrix = (UINT16*) (TargetProximityDomainList +
                                      *NumberTargetProximityDomain);

  // Display each element of the Initiator Proximity Domain list
  for (Index = 0; Index < *NumberInitiatorProximityDomain; Index++) {
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"Initiator Proximity Dom [%d]",
      Index
      );

    PrintFieldName (4, Buffer);
    Print (
      L"0x%x\n",
      InitiatorProximityDomainList[Index]
      );
  }

  // Display each element of the Target Proximity Domain list
  for (Index = 0; Index < *NumberTargetProximityDomain; Index++) {
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"Target Proximity Dom [%d]",
      Index
      );

    PrintFieldName (4, Buffer);
    Print (
      L"0x%x\n",
      TargetProximityDomainList[Index]
      );
  }

  // Create base name depending on Data Type in this Structure
  if (*SllbiDataType >= ARRAY_SIZE (SllbiNames)) {
    IncrementErrorCount ();
    Print (L"Error: Unkown Data Type. DataType = 0x%x.\n", *SllbiDataType);
    return;
  }
  StrCpyS (Buffer, sizeof (Buffer), SllbiNames[*SllbiDataType]);

  // Adjust base name depending on Memory Hierarchy in this Structure
  switch (SllbiFlags->MemoryHierarchy) {
    case 0:
      UnicodeSPrint (
        SecondBuffer,
        sizeof (SecondBuffer),
        Buffer,
        L"",
        L"%s"
        );
      break;
    case 1:
    case 2:
    case 3:
      UnicodeSPrint (
        SecondBuffer,
        sizeof (SecondBuffer),
        Buffer,
        L"Hit ",
        L"%s"
        );
      break;
    default:
      IncrementErrorCount ();
      Print (
        L"Error: Invalid Memory Hierarchy. MemoryHierarchy = %d.\n",
        SllbiFlags->MemoryHierarchy
        );
      return;

  } // switch

  if (*NumberTargetProximityDomain <= MAX_MEMORY_DOMAIN_TARGET_PRINT_MATRIX) {
    // Display the latency/bandwidth matrix as a matrix
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      SecondBuffer,
      L""
      );
    PrintFieldName (4, Buffer);

    Print (L"\n      Target    : X-axis (Horizontal)");
    Print (L"\n      Initiator : Y-axis (Vertical)");
    Print (L"\n         |");

    for (IndexTarget = 0;
         IndexTarget < *NumberTargetProximityDomain;
         IndexTarget++) {
      Print (L"    %2d", IndexTarget);
    }

    Print (L"\n      ---+");
    for (IndexTarget = 0;
         IndexTarget < *NumberTargetProximityDomain;
         IndexTarget++) {
      Print (L"------");
    }
    Print (L"\n");

    TargetStartOffset = 0;
    for (IndexInitiator = 0;
         IndexInitiator < *NumberInitiatorProximityDomain;
         IndexInitiator++) {
      Print (L"      %2d |", IndexInitiator);
      for (IndexTarget = 0;
           IndexTarget < *NumberTargetProximityDomain;
           IndexTarget++) {
        Print (
          L" %5d",
          LatencyBandwidthMatrix[TargetStartOffset + IndexTarget]
          );
      } // for Target
      Print (L"\n");
      TargetStartOffset += (*NumberTargetProximityDomain);
    } // for Initiator
    Print (L"\n");
  } else {
    // Display the latency/bandwidth matrix as a list
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      SecondBuffer,
      L" [%d][%d]"
      );

    TargetStartOffset = 0;
    for (IndexInitiator = 0;
         IndexInitiator < *NumberInitiatorProximityDomain;
         IndexInitiator++) {
      for (IndexTarget = 0;
           IndexTarget < *NumberTargetProximityDomain;
           IndexTarget++) {
        UnicodeSPrint (
          SecondBuffer,
          sizeof (SecondBuffer),
          Buffer,
          IndexInitiator,
          IndexTarget
          );

        PrintFieldName (4, SecondBuffer);
        Print (
          L"%d\n",
          LatencyBandwidthMatrix[TargetStartOffset + IndexTarget]
          );
      } // for Target
      TargetStartOffset += (*NumberTargetProximityDomain);
    } // for Initiator
  }
}

/**
  This function parses the Memory Side Cache Information Structure (Type 2).

  @param [in] Ptr     Pointer to the start of the Memory Side Cache Information
                      Structure data.
  @param [in] Length  Length of the Memory Side Cache Information Structure.
**/
STATIC
VOID
DumpMsci (
  IN UINT8* Ptr,
  IN UINT32 Length
  )
{
  CONST UINT16* SMBIOSHandlesList;
  CHAR16        Buffer[OUTPUT_FIELD_COLUMN_WIDTH];
  UINT32        Offset;
  UINT16        Index;

  Offset = ParseAcpi (
             TRUE,
             2,
             "Memory Side Cache Information Structure",
             Ptr,
             Length,
             PARSER_PARAMS (MemSideCacheInfoParser)
             );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if (NumberSMBIOSHandles == NULL) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient remaining table buffer length to read the " \
        L"MSCI structure header. Length = %d.\n",
      Length
      );
    return;
  }

  if ((*NumberSMBIOSHandles * sizeof (UINT16)) > (Length - Offset)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Number of SMBIOS Handles. SMBIOSHandlesCount = %d." \
      L"RemainingBufferLength = %d.\n",
      *NumberSMBIOSHandles,
      Length - Offset
      );
    return;
  }

  SMBIOSHandlesList = (UINT16*) (Ptr + Offset);

  for (Index = 0; Index < *NumberSMBIOSHandles; Index++) {
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"SMBIOS Handles [%d]",
      Index
      );

    PrintFieldName (4, Buffer);
    Print (
      L"0x%x\n",
      SMBIOSHandlesList[Index]
      );
  }
}

/**
  This function parses the ACPI HMAT table.
  When trace is enabled this function parses the HMAT table and
  traces the ACPI table fields.

  This function parses the following HMAT structures:
    - Memory Proximity Domain Attributes Structure (Type 0)
    - System Locality Latency and Bandwidth Info Structure (Type 1)
    - Memory Side Cache Info structure (Type 2)

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiHmat (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT8* HmatStructurePtr;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             Trace,
             0,
             "HMAT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (HmatParser)
             );

  HmatStructurePtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    // Parse HMAT Structure Header to obtain Type and Length.
    ParseAcpi (
      FALSE,
      0,
      NULL,
      HmatStructurePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (HmatStructureHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((HmatStructureType == NULL) ||
        (HmatStructureLength == NULL)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
          L"HMAT structure header. Length = %d.\n",
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate HMAT Structure length.
    if ((*HmatStructureLength == 0) ||
        ((Offset + (*HmatStructureLength)) > AcpiTableLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid HMAT Structure length. " \
          L"Length = %d. Offset = %d. AcpiTableLength = %d.\n",
        *HmatStructureLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    switch (*HmatStructureType) {
      case EFI_ACPI_6_3_HMAT_TYPE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES:
        DumpMpda (
          HmatStructurePtr,
          *HmatStructureLength
          );
        break;
      case EFI_ACPI_6_3_HMAT_TYPE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO:
        DumpSllbi (
          HmatStructurePtr,
          *HmatStructureLength
          );
        break;
      case EFI_ACPI_6_3_HMAT_TYPE_MEMORY_SIDE_CACHE_INFO:
         DumpMsci (
          HmatStructurePtr,
          *HmatStructureLength
          );
        break;
      default:
        IncrementErrorCount ();
        Print (
          L"ERROR: Unknown HMAT structure:"
            L" Type = %d, Length = %d\n",
          *HmatStructureType,
          *HmatStructureLength
          );
        break;
    } // switch

    HmatStructurePtr += *HmatStructureLength;
    Offset += *HmatStructureLength;
  } // while
}
