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

typedef enum {
  IOMMU = 0,
  PCIERC,
  PLATFORM,
  UNSUPPORTED
} RimtNodeTypes;

// Local variables

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

  if (NodeType >= UNSUPPORTED) {
    IncrementErrorCount ();
    Print (L"\nERROR: Urecognized RIMT node type. Type value must be less than %d but got %d", UNSUPPORTED, NodeType);
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
  if (NodeType == IOMMU) {
    Str = "IOMMU";
  } else if (NodeType == PCIERC) {
    Str = "PCIe Root Complex";
  } else if (NodeType == PLATFORM) {
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
STATIC UINT8                                    *RimtNodeType;
STATIC UINT16                                   *RimtNodeLength;
STATIC EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE  RimtNodeHeader;
#define RIMT_PARSE_NODE_HEADER                                   \
  { L"Type",                                                     \
    sizeof(RimtNodeHeader.Type),                                 \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Type),    \
    NULL,                                                        \
    RimtNodeTypeToStr,                                           \
    (VOID**)&RimtNodeType,                                       \
    RimtValidateNodeType,                                        \
    NULL                                                         \
  },                                                             \
  { L"Revision",                                                 \
    sizeof(RimtNodeHeader.Revision),                             \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Revision),\
    L"%d",                                                       \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL                                                         \
  },                                                             \
  { L"Length",                                                   \
    sizeof(RimtNodeHeader.Length),                               \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Length),  \
    L"%d",                                                       \
    NULL,                                                        \
    (VOID**)&RimtNodeLength,                                     \
    NULL,                                                        \
    NULL                                                         \
  },                                                             \
  { L"Reserved",                                                 \
    sizeof(RimtNodeHeader.Reserved),                             \
    OFFSET_OF(EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE, Reserved),\
    L"0x%x",                                                     \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL                                                         \
  },                                                             \
  { L"Id",                                                       \
    sizeof(RimtNodeHeader.Id),                                   \
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
STATIC CONST ACPI_PARSER  RimtNodeHeaderParser[] = {
  RIMT_PARSE_NODE_HEADER
};

#define RIMT_MINIMAL_NUMBER_OF_NODES_ALLOWED  2

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
  UINT32  data; // "Number of RIMT Nodes" field is 4 byte long in spec

  data = *(UINT32 *)Ptr;
  if (data < RIMT_MINIMAL_NUMBER_OF_NODES_ALLOWED) {
    IncrementErrorCount ();
    Print (L"\nERROR: Minimal Number of RIMT nodes must be at least %d.", RIMT_MINIMAL_NUMBER_OF_NODES_ALLOWED);
  }
}

/**
  An ACPI_PARSER array describing the RIMT (RISC-V IO Mapping Table).
**/
STATIC EFI_ACPI_6_6_RIMT_STRUCTURE  RimtInfo;
STATIC CONST UINT32                 *NumberOfRimtNodes;
STATIC CONST UINT32                 *OffsetToRimtNodeArray;
STATIC CONST ACPI_PARSER            RimtParser[] = {
  PARSE_ACPI_HEADER (&RimtInfo.Header),
  {
    L"Number of RIMT Nodes",
    sizeof (RimtInfo.NumberOfRimtNodes),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_STRUCTURE, NumberOfRimtNodes),
    L"%d",
    NULL,
    (VOID **)&NumberOfRimtNodes,
    RimtValidateNumberOfNodes,
    NULL
  },
  {
    L"Offset to RIMT Node Array",
    sizeof (RimtInfo.OffsetToRimtNodeArray),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_STRUCTURE, OffsetToRimtNodeArray),
    L"0x%x",
    NULL,
    (VOID **)&OffsetToRimtNodeArray,
    NULL,
    NULL
  },
  {
    L"Reserved",
    sizeof (RimtInfo.Reserved),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_STRUCTURE, Reserved),
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
  If no format string is specified the Format must be NULL.

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

#define RIMT_GET_FLAGS_BIT_PARSER_FUNC(interpreter)  _RimtFlagsToStr ## interpreter

/**
  This macro generates function definition which traces bits in Flags field.
  If no format string is specified the Format must be NULL.

  @param [in] Parser    Optional format string for tracing the data.
  @param [in] Active    String to display if interpreted bit is active.
  @param [in] Inactive  String to display if interpreted bit is inactive.
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
  UINT64  *Data;  // RIMT specifies HardwareId field as 8-byte long
  CHAR8   Buffer[9];

  Data = (UINT64 *)Ptr;
  AsciiStrnCpyS (Buffer, sizeof (Buffer), (CHAR8 *)Data, sizeof (*Data));
  AsciiPrint (Buffer);
  Print (L" (0x%lx)", *Data);
}

/**
  This macro calls generated function which traces bits in Flags field
  defined by DECLARE_FLAG_INTERPRETER()
**/

#define RIMT_FLAGS_IOMMU_TYPE_BIT_OFFSET              0
#define RIMT_FLAGS_IOMMU_TYPE_BITS                    1
#define RIMT_FLAGS_IOMMU_PROXIMITY_DOMAIN_BIT_OFFSET  1
#define RIMT_FLAGS_IOMMU_PROXIMITY_DOMAIN_BITS        1

RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (IommuType, "PCIe", "Platform");
RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (ProximityDomain, "Valid", "Invalid");
STATIC CONST ACPI_PARSER  RimtIommuFlagParser[] = {
  {
    L"Device Type",
    RIMT_FLAGS_IOMMU_TYPE_BITS,
    RIMT_FLAGS_IOMMU_TYPE_BIT_OFFSET,
    NULL,
    RIMT_GET_FLAGS_BIT_PARSER_FUNC (IommuType),
    NULL, NULL, NULL
  },
  {
    L"Proximity Domain",
    RIMT_FLAGS_IOMMU_PROXIMITY_DOMAIN_BITS,
    RIMT_FLAGS_IOMMU_PROXIMITY_DOMAIN_BIT_OFFSET,
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

STATIC EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE  RimtIommuNode;
RIMT_DECLARE_NODE_FLAGS_PARSER_FUNC (RimtIommuFlagParser);

/**
An ACPI_PARSER array describing the IOMMU Node.
**/
STATIC CONST ACPI_PARSER  RimtIommuNodeParser[] = {
  RIMT_PARSE_NODE_HEADER,
  {
    L"Hardware ID",
    sizeof (RimtIommuNode.HardwareId),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    HardwareId),
    NULL,
    RimtHardwareIdToStr,
    NULL,
    NULL,
    NULL
  },
  {
    L"Base Address",
    sizeof (RimtIommuNode.BaseAddress),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    BaseAddress),
    L"0x%lx",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"Flags",
    sizeof (RimtIommuNode.Flags),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    Flags),
    NULL,
    RIMT_GET_NODE_FLAGS_PARSER_FUNC (RimtIommuFlagParser),
    NULL,
    NULL,
    NULL
  },
  {
    L"Proximity Domain",
    sizeof (RimtIommuNode.ProximityDomain),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    ProximityDomain),
    L"%d",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"PCIe Segment number",
    sizeof (RimtIommuNode.PcieSegmentNumber),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    PcieSegmentNumber),
    L"0x%x",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"PCIe B/D/F",
    sizeof (RimtIommuNode.PcieBdf),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    PcieBdf),
    L"%d",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"Number of interrupt wires",
    sizeof (RimtIommuNode.NumberOfInterruptWires),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE,    NumberOfInterruptWires),
    L"0x%x",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"Interrupt wire array offset",
    sizeof (RimtIommuNode.InterruptWireArrayOffset),
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

#define RIMT_ID_MAPPING_FLAGS_ATS_BIT_OFFSET  0
#define RIMT_ID_MAPPING_FLAGS_ATS_BIT_COUNT   1
#define RIMT_ID_MAPPING_FLAGS_PRI_BIT_OFFSET  1
#define RIMT_ID_MAPPING_FLAGS_PRI_BIT_COUNT   1
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

STATIC EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE  RimtIdMappingNode;

/**
An ACPI_PARSER array describing the ID Mapping Node.
**/
STATIC CONST ACPI_PARSER  RimtIdMappingNodeParser[] = {
  {
    L"Source ID Base",
    sizeof (RimtIdMappingNode.SourceIdBase),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, SourceIdBase),
    L"0x%x",
    NULL,
    NULL, NULL, NULL
  },
  {
    L"Number of IDs",
    sizeof (RimtIdMappingNode.NumberOfIDs),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, NumberOfIDs),
    L"%d",
    NULL,
    NULL, NULL, NULL
  },
  {
    L"Destination Device ID Base",
    sizeof (RimtIdMappingNode.DestinationDeviceIdBase),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, DestinationDeviceIdBase),
    L"%d",
    NULL,
    NULL, NULL, NULL
  },
  {
    L"Destination IOMMU Offset",
    sizeof (RimtIdMappingNode.DestinationIommuOffset),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE, DestinationIommuOffset),
    L"0x%x",
    NULL,
    NULL, NULL, NULL
  },
  {
    L"Flags",
    sizeof (RimtIdMappingNode.Flags),
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
  CHAR8   Buffer[32]; // Used for AsciiName param of ParseAcpi

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

#define RIMT_PCIERC_FLAGS_ATS_OFFSET  0
#define RIMT_PCIERC_FLAGS_ATS_BITS    1
#define RIMT_PCIERC_FLAGS_PRI_OFFSET  1
#define RIMT_PCIERC_FLAGS_PRI_BITS    1
RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (PcieRcAtsSupport, "Supported", "Not Supported");
RIMT_DECLARE_FLAGS_BIT_PARSER_FUNC (PcieRcPriSupport, "Supported", "Not Supported");
STATIC CONST ACPI_PARSER  RimtPcieRcFlagParser[] = {
  {
    L"ATS",
    RIMT_PCIERC_FLAGS_ATS_BITS, RIMT_PCIERC_FLAGS_ATS_OFFSET,
    NULL,
    RIMT_GET_FLAGS_BIT_PARSER_FUNC (PcieRcAtsSupport),
    NULL, NULL, NULL
  },
  {
    L"PRI",
    RIMT_PCIERC_FLAGS_PRI_BITS, RIMT_PCIERC_FLAGS_PRI_OFFSET,
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

STATIC CONST UINT16                                        *NumberOfIdMappings;
STATIC UINT8                                               *IdMappingArrayOffset;
STATIC EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE  RimtPcieRootComplexNode;

/**
  An ACPI_PARSER array describing the Pcie Root Complex Node.
**/
STATIC CONST ACPI_PARSER  RimtPcieRcNodeParser[] = {
  RIMT_PARSE_NODE_HEADER,
  {
    L"Flags",
    sizeof (RimtPcieRootComplexNode.Flags),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,Flags),
    NULL,
    RIMT_GET_NODE_FLAGS_PARSER_FUNC (RimtPcieRcFlagParser),
    NULL,
    NULL,
    NULL
  },
  {
    L"Reserved",
    sizeof (RimtPcieRootComplexNode.Reserved),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,Reserved),
    L"0x%lx",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"PCIe Segment Number",
    sizeof (RimtPcieRootComplexNode.PcieSegmentNumber),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,PcieSegmentNumber),
    L"%d",
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    L"Id Mapping Array Offset",
    sizeof (RimtPcieRootComplexNode.IdMappingArrayOffset),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,IdMappingArrayOffset),
    L"0x%x",
    NULL,
    (VOID **)&IdMappingArrayOffset,
    NULL,
    NULL
  },
  {
    L"Number Of Id Mappings",
    sizeof (RimtPcieRootComplexNode.NumberOfIdMappings),
    OFFSET_OF (EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE,NumberOfIdMappings),
    L"%d",
    NULL,
    (VOID **)&NumberOfIdMappings,
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
  IdMappingArray = PcieRootComplexNode + *IdMappingArrayOffset;
  RimtParseIdMappingArray (
    IdMappingArray,
    Length - *IdMappingArrayOffset,
    *NumberOfIdMappings
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
    PARSER_PARAMS (RimtParser)
    );
  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((NumberOfRimtNodes == NULL) ||
      (OffsetToRimtNodeArray == NULL))
  {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient table length. AcpiTableLength = %d.\n",
      AcpiTableLength
      );
    return;
  }

  Offset  = *OffsetToRimtNodeArray;
  NodePtr =  Rimt + Offset;
  Index   = 0;
  while ((Index < *NumberOfRimtNodes) &&
         (Offset < AcpiTableLength))
  {
    // Parse the RIMT Node Header
    ParseAcpi (
      FALSE,
      0,
      "RIMT Node Header",
      NodePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (RimtNodeHeaderParser)
      );
    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((RimtNodeType == NULL)        ||
        (RimtNodeLength == NULL))
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
    if ((*RimtNodeLength == 0) ||
        ((Offset + (*RimtNodeLength)) > AcpiTableLength))
    {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid RIMT Node length. " \
        L"Length = %d. Offset = %d. AcpiTableLength = %d.\n",
        *RimtNodeLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    switch (*RimtNodeType) {
      case IOMMU:
        RimtParseIommuNode (
          NodePtr,
          *RimtNodeLength,
          Index
          );
        break;
      case PCIERC:
        RimtParsePcieRcNode (
          NodePtr,
          *RimtNodeLength,
          Index
          );
        break;
      case PLATFORM:
        RimtParsePlatformDeviceNode (
          NodePtr,
          *RimtNodeLength,
          Index
          );
        break;
      default:
        IncrementErrorCount ();
        Print (L"ERROR: Unsupported RIMT Node type = %d\n", *RimtNodeType);
    } // switch

    Index++;
    NodePtr += (*RimtNodeLength);
    Offset  += (*RimtNodeLength);
  }
}
