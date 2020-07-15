/** @file
  IORT table parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - IO Remapping Table, Platform Design Document, Revision D, March 2018
**/

#include <IndustryStandard/IoRemappingTable.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewConfig.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

STATIC CONST UINT32* IortNodeCount;
STATIC CONST UINT32* IortNodeOffset;

STATIC CONST UINT8*  IortNodeType;
STATIC CONST UINT16* IortNodeLength;
STATIC CONST UINT32* IortIdMappingCount;
STATIC CONST UINT32* IortIdMappingOffset;

STATIC CONST UINT32* InterruptContextCount;
STATIC CONST UINT32* InterruptContextOffset;
STATIC CONST UINT32* PmuInterruptCount;
STATIC CONST UINT32* PmuInterruptOffset;

STATIC CONST UINT32* ItsCount;

/**
  This function validates the ID Mapping array count for the ITS node.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateItsIdMappingCount (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  if (*(UINT32*)Ptr != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: IORT ID Mapping count must be zero.");
  }
}

/**
  This function validates the ID Mapping array count for the Performance
  Monitoring Counter Group (PMCG) node.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidatePmcgIdMappingCount (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  if (*(UINT32*)Ptr > 1) {
    IncrementErrorCount ();
    Print (L"\nERROR: IORT ID Mapping count must not be greater than 1.");
  }
}

/**
  This function validates the ID Mapping array offset for the ITS node.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateItsIdArrayReference (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  if (*(UINT32*)Ptr != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: IORT ID Mapping offset must be zero.");
  }
}

/**
  Helper Macro for populating the IORT Node header in the ACPI_PARSER array.

  @param [out] ValidateIdMappingCount    Optional pointer to a function for
                                         validating the ID Mapping count.
  @param [out] ValidateIdArrayReference  Optional pointer to a function for
                                         validating the ID Array reference.
**/
#define PARSE_IORT_NODE_HEADER(ValidateIdMappingCount,                   \
                               ValidateIdArrayReference)                 \
  { L"Type", 1, 0, L"%d", NULL, (VOID**)&IortNodeType, NULL, NULL },     \
  { L"Length", 2, 1, L"%d", NULL, (VOID**)&IortNodeLength, NULL, NULL }, \
  { L"Revision", 1, 3, L"%d", NULL, NULL, NULL, NULL },                  \
  { L"Reserved", 4, 4, L"0x%x", NULL, NULL, NULL, NULL },                \
  { L"Number of ID mappings", 4, 8, L"%d", NULL,                         \
    (VOID**)&IortIdMappingCount, ValidateIdMappingCount, NULL },         \
  { L"Reference to ID Array", 4, 12, L"0x%x", NULL,                      \
    (VOID**)&IortIdMappingOffset, ValidateIdArrayReference, NULL }

/**
  An ACPI_PARSER array describing the ACPI IORT Table
**/
STATIC CONST ACPI_PARSER IortParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"Number of IORT Nodes", 4, 36, L"%d", NULL,
   (VOID**)&IortNodeCount, NULL, NULL},
  {L"Offset to Array of IORT Nodes", 4, 40, L"0x%x", NULL,
   (VOID**)&IortNodeOffset, NULL, NULL},
  {L"Reserved", 4, 44, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the IORT node header structure.
**/
STATIC CONST ACPI_PARSER IortNodeHeaderParser[] = {
  PARSE_IORT_NODE_HEADER (NULL, NULL)
};

/**
  An ACPI_PARSER array describing the IORT SMMUv1/2 node.
**/
STATIC CONST ACPI_PARSER IortNodeSmmuV1V2Parser[] = {
  PARSE_IORT_NODE_HEADER (NULL, NULL),
  {L"Base Address", 8, 16, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Span", 8, 24, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Model", 4, 32, L"%d", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 36, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reference to Global Interrupt Array", 4, 40, L"0x%x", NULL, NULL, NULL,
   NULL},
  {L"Number of context interrupts", 4, 44, L"%d", NULL,
   (VOID**)&InterruptContextCount, NULL, NULL},
  {L"Reference to Context Interrupt Array", 4, 48, L"0x%x", NULL,
   (VOID**)&InterruptContextOffset, NULL, NULL},
  {L"Number of PMU Interrupts", 4, 52, L"%d", NULL,
   (VOID**)&PmuInterruptCount, NULL, NULL},
  {L"Reference to PMU Interrupt Array", 4, 56, L"0x%x", NULL,
   (VOID**)&PmuInterruptOffset, NULL, NULL},

  // Interrupt Array
  {L"SMMU_NSgIrpt", 4, 60, L"0x%x", NULL, NULL, NULL, NULL},
  {L"SMMU_NSgIrpt interrupt flags", 4, 64, L"0x%x", NULL, NULL, NULL, NULL},
  {L"SMMU_NSgCfgIrpt", 4, 68, L"0x%x", NULL, NULL, NULL, NULL},
  {L"SMMU_NSgCfgIrpt interrupt flags", 4, 72, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the SMMUv1/2 Node Interrupt Array.
**/
STATIC CONST ACPI_PARSER InterruptArrayParser[] = {
  {L"Interrupt GSIV", 4, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 4, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the IORT ID Mapping.
**/
STATIC CONST ACPI_PARSER IortNodeIdMappingParser[] = {
  {L"Input base", 4, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Number of IDs", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Output base", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Output reference", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 16, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the IORT SMMUv3 node.
**/
STATIC CONST ACPI_PARSER IortNodeSmmuV3Parser[] = {
  PARSE_IORT_NODE_HEADER (NULL, NULL),
  {L"Base Address", 8, 16, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 24, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 4, 28, L"0x%x", NULL, NULL, NULL, NULL},
  {L"VATOS Address", 8, 32, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Model", 4, 40, L"%d", NULL, NULL, NULL, NULL},
  {L"Event", 4, 44, L"0x%x", NULL, NULL, NULL, NULL},
  {L"PRI", 4, 48, L"0x%x", NULL, NULL, NULL, NULL},
  {L"GERR", 4, 52, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Sync", 4, 56, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Proximity domain", 4, 60, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Device ID mapping index", 4, 64, L"%d", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the IORT ITS node.
**/
STATIC CONST ACPI_PARSER IortNodeItsParser[] = {
  PARSE_IORT_NODE_HEADER (
    ValidateItsIdMappingCount,
    ValidateItsIdArrayReference
    ),
  {L"Number of ITSs", 4, 16, L"%d", NULL, (VOID**)&ItsCount, NULL}
};

/**
  An ACPI_PARSER array describing the ITS ID.
**/
STATIC CONST ACPI_PARSER ItsIdParser[] = {
  { L"GIC ITS Identifier", 4, 0, L"%d", NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the IORT Names Component node.
**/
STATIC CONST ACPI_PARSER IortNodeNamedComponentParser[] = {
  PARSE_IORT_NODE_HEADER (NULL, NULL),
  {L"Node Flags", 4, 16, L"%d", NULL, NULL, NULL, NULL},
  {L"Memory access properties", 8, 20, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Device memory address size limit", 1, 28, L"%d", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the IORT Root Complex node.
**/
STATIC CONST ACPI_PARSER IortNodeRootComplexParser[] = {
  PARSE_IORT_NODE_HEADER (NULL, NULL),
  {L"Memory access properties", 8, 16, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"ATS Attribute", 4, 24, L"0x%x", NULL, NULL, NULL, NULL},
  {L"PCI Segment number", 4, 28, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Memory access size limit", 1, 32, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 3, 33, L"%x %x %x", Dump3Chars, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the IORT PMCG node.
**/
STATIC CONST ACPI_PARSER IortNodePmcgParser[] = {
  PARSE_IORT_NODE_HEADER (ValidatePmcgIdMappingCount, NULL),
  {L"Page 0 Base Address", 8, 16, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Overflow interrupt GSIV", 4, 24, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Node reference", 4, 28, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Page 1 Base Address", 8, 32, L"0x%lx", NULL, NULL, NULL, NULL}
};

/**
  This function parses the IORT Node Id Mapping array.

  @param [in] Ptr            Pointer to the start of the ID mapping array.
  @param [in] Length         Length of the buffer.
  @param [in] MappingCount   The ID Mapping count.
**/
STATIC
VOID
DumpIortNodeIdMappings (
  IN UINT8* Ptr,
  IN UINT32 Length,
  IN UINT32 MappingCount
  )
{
  UINT32 Index;
  UINT32 Offset;
  CHAR8  Buffer[40];  // Used for AsciiName param of ParseAcpi

  Index = 0;
  Offset = 0;

  while ((Index < MappingCount) &&
         (Offset < Length)) {
    AsciiSPrint (
      Buffer,
      sizeof (Buffer),
      "ID Mapping [%d]",
      Index
      );
    Offset += ParseAcpi (
                TRUE,
                4,
                Buffer,
                Ptr + Offset,
                Length - Offset,
                PARSER_PARAMS (IortNodeIdMappingParser)
                );
    Index++;
  }
}

/**
  This function parses the IORT SMMUv1/2 node.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
  @param [in] MappingCount   The ID Mapping count.
  @param [in] MappingOffset  The offset of the ID Mapping array
                             from the start of the IORT table.
**/
STATIC
VOID
DumpIortNodeSmmuV1V2 (
  IN UINT8* Ptr,
  IN UINT16 Length,
  IN UINT32 MappingCount,
  IN UINT32 MappingOffset
  )
{
  UINT32 Index;
  UINT32 Offset;
  CHAR8  Buffer[50];  // Used for AsciiName param of ParseAcpi

  ParseAcpi (
    TRUE,
    2,
    "SMMUv1 or SMMUv2 Node",
    Ptr,
    Length,
    PARSER_PARAMS (IortNodeSmmuV1V2Parser)
    );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((InterruptContextCount == NULL)   ||
      (InterruptContextOffset == NULL)  ||
      (PmuInterruptCount == NULL)       ||
      (PmuInterruptOffset == NULL)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient SMMUv1/2 node length. Length = %d\n",
      Length
      );
    return;
  }

  Offset = *InterruptContextOffset;
  Index = 0;

  while ((Index < *InterruptContextCount) &&
         (Offset < Length)) {
    AsciiSPrint (
      Buffer,
      sizeof (Buffer),
      "Context Interrupts Array [%d]",
      Index
      );
    Offset += ParseAcpi (
                TRUE,
                4,
                Buffer,
                Ptr + Offset,
                Length - Offset,
                PARSER_PARAMS (InterruptArrayParser)
                );
    Index++;
  }

  Offset = *PmuInterruptOffset;
  Index = 0;

  while ((Index < *PmuInterruptCount) &&
         (Offset < Length)) {
    AsciiSPrint (
      Buffer,
      sizeof (Buffer),
      "PMU Interrupts Array [%d]",
      Index
      );
    Offset += ParseAcpi (
                TRUE,
                4,
                Buffer,
                Ptr + Offset,
                Length - Offset,
                PARSER_PARAMS (InterruptArrayParser)
                );
    Index++;
  }

  DumpIortNodeIdMappings (
    Ptr + MappingOffset,
    Length - MappingOffset,
    MappingCount
    );
}

/**
  This function parses the IORT SMMUv3 node.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
  @param [in] MappingCount   The ID Mapping count.
  @param [in] MappingOffset  The offset of the ID Mapping array
                             from the start of the IORT table.
**/
STATIC
VOID
DumpIortNodeSmmuV3 (
  IN UINT8* Ptr,
  IN UINT16 Length,
  IN UINT32 MappingCount,
  IN UINT32 MappingOffset
  )
{
  ParseAcpi (
    TRUE,
    2,
    "SMMUV3 Node",
    Ptr,
    Length,
    PARSER_PARAMS (IortNodeSmmuV3Parser)
    );

  DumpIortNodeIdMappings (
    Ptr + MappingOffset,
    Length - MappingOffset,
    MappingCount
    );
}

/**
  This function parses the IORT ITS node.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
**/
STATIC
VOID
DumpIortNodeIts (
  IN UINT8* Ptr,
  IN UINT16 Length
  )
{
  UINT32 Offset;
  UINT32 Index;
  CHAR8  Buffer[80];  // Used for AsciiName param of ParseAcpi

  Offset = ParseAcpi (
            TRUE,
            2,
            "ITS Node",
            Ptr,
            Length,
            PARSER_PARAMS (IortNodeItsParser)
            );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if (ItsCount == NULL) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient ITS group length. Length = %d.\n",
      Length
      );
    return;
  }

  Index = 0;

  while ((Index < *ItsCount) &&
         (Offset < Length)) {
    AsciiSPrint (
      Buffer,
      sizeof (Buffer),
      "GIC ITS Identifier Array [%d]",
      Index
      );
    Offset += ParseAcpi (
                TRUE,
                4,
                Buffer,
                Ptr + Offset,
                Length - Offset,
                PARSER_PARAMS (ItsIdParser)
                );
    Index++;
  }

  // Note: ITS does not have the ID Mappings Array

}

/**
  This function parses the IORT Named Component node.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
  @param [in] MappingCount   The ID Mapping count.
  @param [in] MappingOffset  The offset of the ID Mapping array
                             from the start of the IORT table.
**/
STATIC
VOID
DumpIortNodeNamedComponent (
  IN UINT8* Ptr,
  IN UINT16 Length,
  IN UINT32 MappingCount,
  IN UINT32 MappingOffset
  )
{
  UINT32 Offset;

  Offset = ParseAcpi (
             TRUE,
             2,
             "Named Component Node",
             Ptr,
             Length,
             PARSER_PARAMS (IortNodeNamedComponentParser)
             );

  // Estimate the Device Name length
  PrintFieldName (2, L"Device Object Name");

  while ((*(Ptr + Offset) != 0) &&
         (Offset < Length)) {
    Print (L"%c", *(Ptr + Offset));
    Offset++;
  }
  Print (L"\n");

  DumpIortNodeIdMappings (
    Ptr + MappingOffset,
    Length - MappingOffset,
    MappingCount
    );
}

/**
  This function parses the IORT Root Complex node.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
  @param [in] MappingCount   The ID Mapping count.
  @param [in] MappingOffset  The offset of the ID Mapping array
                             from the start of the IORT table.
**/
STATIC
VOID
DumpIortNodeRootComplex (
  IN UINT8* Ptr,
  IN UINT16 Length,
  IN UINT32 MappingCount,
  IN UINT32 MappingOffset
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Root Complex Node",
    Ptr,
    Length,
    PARSER_PARAMS (IortNodeRootComplexParser)
    );

  DumpIortNodeIdMappings (
    Ptr + MappingOffset,
    Length - MappingOffset,
    MappingCount
    );
}

/**
  This function parses the IORT PMCG node.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
  @param [in] MappingCount   The ID Mapping count.
  @param [in] MappingOffset  The offset of the ID Mapping array
                             from the start of the IORT table.
**/
STATIC
VOID
DumpIortNodePmcg (
  IN UINT8* Ptr,
  IN UINT16 Length,
  IN UINT32 MappingCount,
  IN UINT32 MappingOffset
)
{
  ParseAcpi (
    TRUE,
    2,
    "PMCG Node",
    Ptr,
    Length,
    PARSER_PARAMS (IortNodePmcgParser)
    );

  DumpIortNodeIdMappings (
    Ptr + MappingOffset,
    Length - MappingOffset,
    MappingCount
    );
}

/**
  This function parses the ACPI IORT table.
  When trace is enabled this function parses the IORT table and traces the ACPI fields.

  This function also parses the following nodes:
    - ITS Group
    - Named Component
    - Root Complex
    - SMMUv1/2
    - SMMUv3
    - PMCG

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiIort (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT32 Index;
  UINT8* NodePtr;

  if (!Trace) {
    return;
  }

  ParseAcpi (
    TRUE,
    0,
    "IORT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (IortParser)
    );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((IortNodeCount == NULL) ||
      (IortNodeOffset == NULL)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient table length. AcpiTableLength = %d.\n",
      AcpiTableLength
      );
    return;
  }

  Offset = *IortNodeOffset;
  NodePtr = Ptr + Offset;
  Index = 0;

  // Parse the specified number of IORT nodes or the IORT table buffer length.
  // Whichever is minimum.
  while ((Index++ < *IortNodeCount) &&
         (Offset < AcpiTableLength)) {
    // Parse the IORT Node Header
    ParseAcpi (
      FALSE,
      0,
      "IORT Node Header",
      NodePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (IortNodeHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((IortNodeType == NULL)        ||
        (IortNodeLength == NULL)      ||
        (IortIdMappingCount == NULL)  ||
        (IortIdMappingOffset == NULL)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
          L"IORT node header. Length = %d.\n",
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate IORT Node length
    if ((*IortNodeLength == 0) ||
        ((Offset + (*IortNodeLength)) > AcpiTableLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid IORT Node length. " \
          L"Length = %d. Offset = %d. AcpiTableLength = %d.\n",
        *IortNodeLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    PrintFieldName (2, L"* Node Offset *");
    Print (L"0x%x\n", Offset);

    switch (*IortNodeType) {
      case EFI_ACPI_IORT_TYPE_ITS_GROUP:
        DumpIortNodeIts (
          NodePtr,
          *IortNodeLength
          );
        break;
      case EFI_ACPI_IORT_TYPE_NAMED_COMP:
        DumpIortNodeNamedComponent (
          NodePtr,
          *IortNodeLength,
          *IortIdMappingCount,
          *IortIdMappingOffset
          );
        break;
      case EFI_ACPI_IORT_TYPE_ROOT_COMPLEX:
        DumpIortNodeRootComplex (
          NodePtr,
          *IortNodeLength,
          *IortIdMappingCount,
          *IortIdMappingOffset
          );
        break;
      case EFI_ACPI_IORT_TYPE_SMMUv1v2:
        DumpIortNodeSmmuV1V2 (
          NodePtr,
          *IortNodeLength,
          *IortIdMappingCount,
          *IortIdMappingOffset
          );
        break;
      case EFI_ACPI_IORT_TYPE_SMMUv3:
        DumpIortNodeSmmuV3 (
          NodePtr,
          *IortNodeLength,
          *IortIdMappingCount,
          *IortIdMappingOffset
          );
        break;
      case EFI_ACPI_IORT_TYPE_PMCG:
        DumpIortNodePmcg (
          NodePtr,
          *IortNodeLength,
          *IortIdMappingCount,
          *IortIdMappingOffset
        );
        break;

      default:
        IncrementErrorCount ();
        Print (L"ERROR: Unsupported IORT Node type = %d\n", *IortNodeType);
    } // switch

    NodePtr += (*IortNodeLength);
    Offset += (*IortNodeLength);
  } // while
}
