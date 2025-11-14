/** @file
  RISC-V IO Mapping Table (RIMT) parser

  Copyright (c) 2025, Plasteli.net. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Reference(s):
    - ACPI 6.6 Specification
    - RISC-V IO Mapping Table (RIMT) Specification Version v1.0, 2025-03-31: Ratified
**/

#include <IndustryStandard/RiscVIoMappingTable.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC UINT8                                               *mRimtNodeType;
STATIC UINT16                                              *mRimtNodeLength;
STATIC EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE             mRimtNodeHeader;
STATIC EFI_ACPI_6_6_RIMT_STRUCTURE                         mRimtInfo;
STATIC CONST UINT32                                        *mNumberOfRimtNodes;
STATIC CONST UINT32                                        *mOffsetToRimtNodeArray;
STATIC EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE              mRimtIdMappingNode;
STATIC CONST UINT16                                        *mNumberOfIdMappings;
STATIC UINT8                                               *mIdMappingArrayOffset;
STATIC EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE  mRimtPcieRootComplexNode;
STATIC EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE              mRimtIommuNode;

/**
  This function validates RIMT Node type.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
RimtValidateNodeType (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  NodeType = *(UINT8 *)Ptr;

  if (NodeType >= RimtNodeUnsupported) {
    IncrementErrorCount ();
    Print (L"\nERROR: Unrecognized RIMT node type. Type value must be less than %d but got %d", RimtNodeUnsupported, NodeType);
  }
}

/**
  This function traces RIMT IOMMU Node Type field and presents in
  human-readable form.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
RimtNodeTypeToStr (
  CONST CHAR16  *Format,
  UINT8         *Ptr,
  UINT32        Length
  )
{
  CHAR8  *Str;
  UINT8  NodeType;

  NodeType = *(UINT8 *)Ptr;
  if (NodeType == RimtNodeIommu) {
    Str = "IOMMU";
  } else if (NodeType == RimtNodePcieRc) {
    Str = "PCIe Root Complex";
  } else if (NodeType == RimtNodePlatform) {
    Str = "Platform Device";
  } else {
    Str = "Unknown";
  }

  AsciiPrint (Str);
}

/**
  Helper Macro for populating the RIMT Node header in the ACPI_PARSER array.
  Every RIMT Node (IOMMU, PCIe Root Complex, PLatform Device) has this header
  common
**/
#define RIMT_PARSE_NODE_HEADER                                   \
  { L"Type",                                                     \
    sizeof(mRimtNodeHeader.Type),                                \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Type),    \
    NULL,                                                        \
    RimtNodeTypeToStr,                                           \
    (VOID**)&mRimtNodeType,                                      \
    RimtValidateNodeType,                                        \
    NULL                                                         \
  },                                                             \
  { L"Revision",                                                 \
    sizeof(mRimtNodeHeader.Revision),                            \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Revision),\
    L"%d",                                                       \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL                                                         \
  },                                                             \
  { L"Length",                                                   \
    sizeof(mRimtNodeHeader.Length),                              \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Length),  \
    L"%d",                                                       \
    NULL,                                                        \
    (VOID**)&mRimtNodeLength,                                    \
    NULL,                                                        \
    NULL                                                         \
  },                                                             \
  { L"Reserved",                                                 \
    sizeof(mRimtNodeHeader.Reserved),                            \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Reserved),\
    L"0x%x",                                                     \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL                                                         \
  },                                                             \
  { L"Id",                                                       \
    sizeof(mRimtNodeHeader.Id),                                  \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Id),      \
    L"%d",                                                       \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL                                                         \
  }                                                              \


/**
An ACPI_PARSER array describing Parser for the RIMT node header structure.
**/
STATIC CONST ACPI_PARSER  mRimtNodeHeaderParser[] = {
  RIMT_PARSE_NODE_HEADER
};

/**
  This function validates number of RIMT nodes.

  According to RIMT spec, Chapter 2.1, system with a single IOMMU,
  should have at least two RIMT nodes

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
RimtValidateNumberOfNodes (
  UINT8   *Ptr,
  UINT32  Length,
  VOID    *Context
  )
{
  UINT32  NumNodes;

  NumNodes = *(UINT32 *)Ptr;
  if (NumNodes < RIMT_MINIMAL_NUMBER_OF_NODES_ALLOWED) {
    IncrementErrorCount ();
    Print (L"\nERROR: Minimal Number of RIMT nodes must be at least %d.", RIMT_MINIMAL_NUMBER_OF_NODES_ALLOWED);
  }
}

/**
  An ACPI_PARSER array describing the RIMT (RISC-V IO Mapping Table).
**/
STATIC CONST ACPI_PARSER  mRimtParser[] = {
  PARSE_ACPI_HEADER (&mRimtInfo.Header),
  {
    L"Number of RIMT Nodes",
    sizeof (mRimtInfo.NumberOfRimtNodes),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_STRUCTURE,  NumberOfRimtNodes),
    L"%d",
    NULL,
    (VOID **)&mNumberOfRimtNodes,
    RimtValidateNumberOfNodes,
    NULL
  },
  {
    L"Offset to RIMT Node Array",
    sizeof (mRimtInfo.OffsetToRimtNodeArray),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_STRUCTURE,  OffsetToRimtNodeArray),
    L"0x%x",
    NULL,
    (VOID **)&mOffsetToRimtNodeArray,
    NULL,
    NULL
  },
  {
    L"Reserved",
    sizeof (mRimtInfo.Reserved),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_STRUCTURE,  Reserved),
    L"0x%x",
    NULL,
    NULL,
    NULL,
    NULL
  }
};

/**
  This macro generates function definition for parser tracing
  Flags field of RIMT Node.

  @param [in] Parser  An ACPI_PARSER for tracing the data.
**/
#define RIMT_DECLARE_NODE_FLAGS_PARSER_FUNC(Parser) \
STATIC                                  \
VOID                                    \
EFIAPI                                  \
_RimtNodeFlagsParser ## Parser (        \
  CONST CHAR16  *Format,                \
  UINT8         *Ptr,                   \
  UINT32        Length                  \
  )                                     \
{                                       \
  UINT32* Flags;                        \
                                        \
  Flags = (UINT32 *)Ptr;                \
  Print (L"0x%X\n", *Flags);            \
  ParseAcpiBitFields (                  \
    TRUE,                               \
    2,                                  \
    NULL,                               \
    (UINT8 *)Flags,                     \
    sizeof(Flags),                      \
    PARSER_PARAMS (Parser)              \
    );                                  \
}                                       \

#define RIMT_GET_NODE_FLAGS_PARSER_FUNC(Parser)  _RimtNodeFlagsParser ## Parser

/**
  This macro generates function definition which traces bits in Flags field.

  @param [in] Parser    Optional format string for tracing the data.
  @param [in] Active    String to display if interpreted bit is active.
  @param [in] Inactive  String to display if interpreted bit is inactive.
**/
#define RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC(Parser, Active, Inactive) \
STATIC                                                          \
VOID                                                            \
EFIAPI                                                          \
_RimtFlagsToStr ## Parser (                                     \
   CONST CHAR16  *Format OPTIONAL,                              \
   UINT8 *Ptr,                                                  \
   UINT32 Length                                                \
   )                                                            \
{                                                               \
  CHAR8 *Str;                                                   \
                                                                \
  if ((*(UINT32 *)Ptr & 0x1) == 1)                              \
     Str = #Active ;                                            \
  else                                                          \
     Str = #Inactive ;                                          \
  AsciiPrint (Str);                                             \
}                                                               \


/**
  This macro gets generated function name which traces bits in Flags field
  defined by DECLARE_FLAG_INTERPRETER()
**/
#define RIMT_GET_FLAGS_BIT_PARSER_FUNC(interpreter)  _RimtFlagsToStr ## interpreter

/**
  This function prints Hardwareld field.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
RimtHardwareIdToStr (
  CONST CHAR16  *Format,
  UINT8         *Ptr,
  UINT32        Length
  )
{
  UINT64  *HardwareId;  // RIMT specifies HardwareId field as 8-byte long
  CHAR8   Buffer[9];

  HardwareId = (UINT64 *)Ptr;
  AsciiStrnCpyS (Buffer, sizeof (Buffer), (CHAR8 *)HardwareId, sizeof (*HardwareId));
  AsciiPrint (Buffer);
  Print (L" (0x%lx)", *HardwareId);
}

RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (IommuType, "PCIe", "Platform");
RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (ProximityDomain, "Valid", "Invalid");
STATIC CONST ACPI_PARSER  RimtIommuFlagParser[] = {
  {
    L"Device Type",
    RIMT_IOMMU_FLAGS_TYPE_BIT_COUNT,
    RIMT_IOMMU_FLAGS_TYPE_BIT_OFFSET,
    NULL,
    RIMT_GET_FLAGS_BIT_PARSER_FUNC (IommuType),
    NULL, NULL, NULL
  },
  {
    L"Proximity Domain",
    RIMT_IOMMU_FLAGS_PROXIMITY_DOMAIN_BIT_COUNT,
    RIMT_IOMMU_FLAGS_PROXIMITY_DOMAIN_BIT_OFFSET,
    NULL,
    RIMT_GET_FLAGS_BIT_PARSER_FUNC (ProximityDomain),
    NULL, NULL, NULL
  },
  {
    L"Reserved",
    30,
    2,
    L"0x%x",
    NULL,
    NULL, NULL, NULL
  }
};

RIMT_DECLARE_NODE_FLAGS_PARSER_FUNC (RimtIommuFlagParser);

/**
An ACPI_PARSER array describing the IOMMU Node.
**/
STATIC CONST ACPI_PARSER  RimtIommuNodeParser[] = {
  RIMT_PARSE_NODE_HEADER,
  {
    L"Hardware ID",
    sizeof (mRimtIommuNode.HardwareId),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    HardwareId),
    NULL,
    RimtHardwareIdToStr,
    NULL,
    NULL,
    NULL
  },
  {
    L"Base Address",
    sizeof (mRimtIommuNode.BaseAddress),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    BaseAddress),
    L"0x%lx",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"Flags",
    sizeof (mRimtIommuNode.Flags),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    Flags),
    NULL,
    RIMT_GET_NODE_FLAGS_PARSER_FUNC (RimtIommuFlagParser),
    NULL,
    NULL,
    NULL
  },
  {
    L"Proximity Domain",
    sizeof (mRimtIommuNode.ProximityDomain),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    ProximityDomain),
    L"%d",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"PCIe Segment number",
    sizeof (mRimtIommuNode.PcieSegmentNumber),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    PcieSegmentNumber),
    L"0x%x",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"PCIe B/D/F",
    sizeof (mRimtIommuNode.PcieBdf),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    PcieBdf),
    L"%d",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"Number of interrupt wires",
    sizeof (mRimtIommuNode.NumberOfInterruptWires),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    NumberOfInterruptWires),
    L"0x%x",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"Interrupt wire array offset",
    sizeof (mRimtIommuNode.InterruptWireArrayOffset),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    InterruptWireArrayOffset),
    L"0x%x",
    NULL,
    NULL,
    NULL,
    NULL
  }
};

/**
  This function parses the RIMT IOMMU node.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
**/
STATIC
VOID
EFIAPI
RimtParseIommuNode (
  IN UINT8   *Ptr,
  IN UINT16  Length,
  IN UINT32  Index
  )
{
  CHAR8  Buffer[32];

  AsciiSPrint (
    Buffer,
    sizeof (Buffer),
    "RIMT Node [%d]",
    Index
    );
  ParseAcpi (
    TRUE,
    2,
    Buffer,
    Ptr,
    Length,
    PARSER_PARAMS (RimtIommuNodeParser)
    );
}

RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (IdMappingAtsSupport, "Required", "Not Required");
RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (IdMappingPriSupport, "Required", "Not Required");
STATIC CONST ACPI_PARSER  RimtIdMappingFlagParser[] = {
  {
    L"ATS",
    RIMT_ID_MAPPING_FLAGS_ATS_BIT_COUNT,
    RIMT_ID_MAPPING_FLAGS_ATS_BIT_OFFSET,
    NULL,
    RIMT_GET_FLAGS_BIT_PARSER_FUNC (IdMappingAtsSupport),
    NULL, NULL, NULL
  },
  {
    L"PRI",
    RIMT_ID_MAPPING_FLAGS_PRI_BIT_COUNT,
    RIMT_ID_MAPPING_FLAGS_PRI_BIT_OFFSET,
    NULL,
    RIMT_GET_FLAGS_BIT_PARSER_FUNC (IdMappingPriSupport),
    NULL, NULL, NULL
  },
  {
    L"Reserved",
    30,
    2,
    L"0x%x",
    NULL,
    NULL, NULL, NULL
  }
};
RIMT_DECLARE_NODE_FLAGS_PARSER_FUNC (RimtIdMappingFlagParser);

/**
An ACPI_PARSER array describing the ID Mapping Node.
**/
STATIC CONST ACPI_PARSER  RimtIdMappingNodeParser[] = {
  {
    L"Source ID Base",
    sizeof (mRimtIdMappingNode.SourceIdBase),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, SourceIdBase),
    L"0x%x",
    NULL,
    NULL, NULL, NULL
  },
  {
    L"Number of IDs",
    sizeof (mRimtIdMappingNode.NumberOfIDs),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, NumberOfIDs),
    L"%d",
    NULL,
    NULL, NULL, NULL
  },
  {
    L"Destination Device ID Base",
    sizeof (mRimtIdMappingNode.DestinationDeviceIdBase),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, DestinationDeviceIdBase),
    L"%d",
    NULL,
    NULL, NULL, NULL
  },
  {
    L"Destination IOMMU Offset",
    sizeof (mRimtIdMappingNode.DestinationIommuOffset),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, DestinationIommuOffset),
    L"0x%x",
    NULL,
    NULL, NULL, NULL
  },
  {
    L"Flags",
    sizeof (mRimtIdMappingNode.Flags),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, Flags),
    NULL,
    RIMT_GET_NODE_FLAGS_PARSER_FUNC (RimtIdMappingFlagParser),
    NULL, NULL, NULL
  },
};

/**
  This function parses the RIMT ID Mapping node.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
  @retval                    Number of bytes parsed.
**/
STATIC
VOID
EFIAPI
RimtParseIdMappingArray (
  IN UINT8   *Node,
  IN UINT16  Length,
  IN UINT32  NumberOfIdMappings
  )
{
  UINT32  Index;
  UINT32  Offset;
  CHAR8   Buffer[32];

  Index  = 0;
  Offset = 0;

  while ((Index < NumberOfIdMappings) &&
         (Offset < Length))
  {
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
                Node + Offset,
                Length - Offset,
                PARSER_PARAMS (RimtIdMappingNodeParser)
                );
    ++Index;
  }
}

RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (PcieRcAtsSupport, "Supported", "Not Supported");
RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (PcieRcPriSupport, "Supported", "Not Supported");
STATIC CONST ACPI_PARSER  RimtPcieRcFlagParser[] = {
  {
    L"ATS",
    RIMT_PCIERC_FLAGS_ATS_BIT_COUNT, RIMT_PCIERC_FLAGS_ATS_BIT_OFFSET,
    NULL,
    RIMT_GET_FLAGS_BIT_PARSER_FUNC (PcieRcAtsSupport),
    NULL, NULL, NULL
  },
  {
    L"PRI",
    RIMT_PCIERC_FLAGS_PRI_BIT_COUNT, RIMT_PCIERC_FLAGS_PRI_BIT_OFFSET,
    NULL,
    RIMT_GET_FLAGS_BIT_PARSER_FUNC (PcieRcPriSupport),
    NULL, NULL, NULL
  },
  {
    L"Reserved",
    30,
    2,
    L"0x%x",
    NULL,
    NULL, NULL, NULL
  }
};
RIMT_DECLARE_NODE_FLAGS_PARSER_FUNC (RimtPcieRcFlagParser);

/**
  An ACPI_PARSER array describing the Pcie Root Complex Node.
**/
STATIC CONST ACPI_PARSER  RimtPcieRcNodeParser[] = {
  RIMT_PARSE_NODE_HEADER,
  {
    L"Flags",
    sizeof (mRimtPcieRootComplexNode.Flags),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,Flags),
    NULL,
    RIMT_GET_NODE_FLAGS_PARSER_FUNC (RimtPcieRcFlagParser),
    NULL,
    NULL,
    NULL
  },
  {
    L"Reserved",
    sizeof (mRimtPcieRootComplexNode.Reserved),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,Reserved),
    L"0x%lx",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"PCIe Segment Number",
    sizeof (mRimtPcieRootComplexNode.PcieSegmentNumber),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,PcieSegmentNumber),
    L"%d",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"Id Mapping Array Offset",
    sizeof (mRimtPcieRootComplexNode.IdMappingArrayOffset),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,IdMappingArrayOffset),
    L"0x%x",
    NULL,
    (VOID **)&mIdMappingArrayOffset,
    NULL,
    NULL
  },
  {
    L"Number Of Id Mappings",
    sizeof (mRimtPcieRootComplexNode.NumberOfIdMappings),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,NumberOfIdMappings),
    L"%d",
    NULL,
    (VOID **)&mNumberOfIdMappings,
    NULL,
    NULL
  },
};

/**
  This function parses the RIMT Pcie Root Complex node.

  This function parses also Id Mappings related to this PCIe Root Complex

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
**/
STATIC
VOID
EFIAPI
RimtParsePcieRcNode (
  IN UINT8   *Ptr,
  IN UINT16  Length,
  IN UINT32  Index
  )
{
  UINT8  *IdMappingArray;
  UINT8  *PcieRootComplexNode;
  CHAR8  Buffer[32];

  PcieRootComplexNode = Ptr;
  AsciiSPrint (
    Buffer,
    sizeof (Buffer),
    "RIMT Node [%d]",
    Index
    );

  ParseAcpi (
    TRUE,
    2,
    Buffer,
    PcieRootComplexNode,
    Length,
    PARSER_PARAMS (RimtPcieRcNodeParser)
    );
  IdMappingArray = PcieRootComplexNode + *mIdMappingArrayOffset;
  RimtParseIdMappingArray (
    IdMappingArray,
    Length - *mIdMappingArrayOffset,
    *mNumberOfIdMappings
    );
}

/**
  This function parses the RIMT Platform Device node.

  TODO: Add parsers when PLatform Device node generator in Qemu is ready.

  @param [in] Ptr            Pointer to the start of the buffer.
  @param [in] Length         Length of the buffer.
**/
STATIC
VOID
EFIAPI
RimtParsePlatformDeviceNode (
  IN UINT8   *Ptr,
  IN UINT16  Length,
  IN UINT32  Index
  )
{
  Print (L"WARNING: Platform type of RIMT Node not supported yet.\n");
}

/**
  This function parses the ACPI RIMT table.
  When trace is enabled this function parses the RIMT table and
  traces the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Rimt               Pointer to the start of the RIMT buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiRimt (
  IN BOOLEAN  Trace,
  IN UINT8    *Rimt,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT8   *NodePtr;
  UINT32  Offset;
  UINT32  Index;

  if (!Trace) {
    return;
  }

  ParseAcpi (
    Trace,
    0,
    "RIMT",
    Rimt,
    AcpiTableLength,
    PARSER_PARAMS (mRimtParser)
    );
  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((mNumberOfRimtNodes == NULL) ||
      (mOffsetToRimtNodeArray == NULL))
  {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient table length. AcpiTableLength = %d.\n",
      AcpiTableLength
      );
    return;
  }

  Offset  = *mOffsetToRimtNodeArray;
  NodePtr =  Rimt + Offset;
  Index   = 0;
  while ((Index < *mNumberOfRimtNodes) &&
         (Offset < AcpiTableLength))
  {
    // Parse the RIMT Node Header
    ParseAcpi (
      FALSE,
      0,
      "RIMT Node Header",
      NodePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (mRimtNodeHeaderParser)
      );
    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((mRimtNodeType == NULL)        ||
        (mRimtNodeLength == NULL))
    {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
        L"RIMT node header. Length = %d.\n",
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate RIMT Node length
    if ((*mRimtNodeLength == 0) ||
        ((Offset + (*mRimtNodeLength)) > AcpiTableLength))
    {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid RIMT Node length. " \
        L"Length = %d. Offset = %d. AcpiTableLength = %d.\n",
        *mRimtNodeLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    switch (*mRimtNodeType) {
      case RimtNodeIommu:
        RimtParseIommuNode (
          NodePtr,
          *mRimtNodeLength,
          Index
          );
        break;
      case RimtNodePcieRc:
        RimtParsePcieRcNode (
          NodePtr,
          *mRimtNodeLength,
          Index
          );
        break;
      case RimtNodePlatform:
        RimtParsePlatformDeviceNode (
          NodePtr,
          *mRimtNodeLength,
          Index
          );
        break;
      default:
        IncrementErrorCount ();
        Print (L"ERROR: Unsupported RIMT Node type = %d\n", *mRimtNodeType);
    } // switch

    Index++;
    NodePtr += (*mRimtNodeLength);
    Offset  += (*mRimtNodeLength);
  }
}
