/** @file
  RISC-V IO Mapping Table (RIMT) parser

  Copyright (c) 2025, Plasteli.net. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Reference(s):
    - ACPI 6.6 Specification
    - RISC-V IO Mapping Table (RIMT) Specification Version v1.0, 2025-03-31: Ratified
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

typedef enum {
  IOMMU = 0,
  PCIERC,
  PLATFORM,
  UNSUPPORTED
} RimtNodeType;

// Local variables
STATIC EFI_ACPI_6_6_RIMT_STRUCTURE  RimtHdrInfo;

/**
  An ACPI_PARSER array describing the RISC-V IO Mapping Table.
**/
STATIC CONST ACPI_PARSER  RimtParser[] = {
  PARSE_ACPI_HEADER (&RimtHdrInfo.Header),
  { L"Number of RIMT Nodes",              4,  36, L"%d",   NULL, NULL, NULL, NULL },
  { L"Offset to RIMT Node Array",         4,  40, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",                          4,  44, L"0x%x", NULL, NULL, NULL, NULL }
};
STATIC CHAR16             *RimtNodeTypeName[] = {
  L"IOMMU",
  L"PCIe Root Complex",
  L"Platform Device",
  L"Unsupported"
};
STATIC CHAR16             *RimtNodeIommuTypeName[] = {
  L"Platform",
  L"PCIe"
};
STATIC CHAR16             *RimtNodeIommuProximityDomainTypeName[] = {
  L"Invalid",
  L"Valid"
};
STATIC CHAR16             *RimtNodeIdMappingFlag[] = {
  L"Not Required",
  L"Required"
};
#define HARDWARE_ID_STRING_SIZE  9
STATIC CHAR8  HardwareIdStr[HARDWARE_ID_STRING_SIZE];
#define  GET_INDEX(type)  (((type) >= UNSUPPORTED) ? UNSUPPORTED : (type))

/**
  This function parses the ACPI RIMT Node header.
  RIMT header is common for all RIMT node types

  @param [in] Node               Pointer to RIMT node.

 **/
STATIC
VOID
EFIAPI
PrintRimtNodeHeader (
  IN EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE  *Node
  )
{
  Print (L"\nRIMT Node Address: 0x%p\n", Node);
  Print (L"\tType                        : %s\n", RimtNodeTypeName[GET_INDEX (Node->Type)]);
  Print (L"\tRevision                    : 0x%x\n", Node->Revision);
  Print (L"\tLength                      : 0x%x\n", Node->Length);
  Print (L"\tReserved                    : 0x%x\n", Node->Reserved);
  Print (L"\tId                          : %d\n", Node->Id);
}

/**
  This function generates ASCII string from ACPI RIMT Node HardwareID
  memory.

  @param [in] HardwareId               Pointer to HardwareId binary data
  @param [out] Str                     Converted binary data to ASCII string
 **/
STATIC
VOID
EFIAPI
HardwareIdToString (
  IN  UINT64  *HardwareId,
  OUT CHAR8   *Str
  )
{
  UINT8  i;
  UINT8  Eol;

  Eol = HARDWARE_ID_STRING_SIZE - 1;
  for (i = 0; i < Eol; i++) {
    Str[i] = ((CHAR8 *)HardwareId)[i];
  }

  Str[Eol] = '\0';
}

#define GET_BIT(BIT_POS, VALUE)  ((VALUE & (1 << BIT_POS)) == 0 ? 0 : 1)

/**
  This function parses the ACPI RIMT Node of type IOMMU.

  @param [in] Node                     Pointer to RIMT node of type IOMMU
 **/
STATIC
VOID
EFIAPI
PrintRimtNodeIommu (
  IN EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE  *Node
  )
{
  PrintRimtNodeHeader ((EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE *)Node);
  HardwareIdToString (&Node->HardwareId, HardwareIdStr);
  AsciiPrint ("\tHardware Id                 : %a   (0x%lx)\n", HardwareIdStr, Node->HardwareId);
  Print (L"\tBase Address                : 0x%x\n", Node->BaseAddress);
  Print (
    L"\tFlags                       : (0x%x) %s Device | Proximity Domain %s\n",
    Node->Flags,
    RimtNodeIommuTypeName[GET_BIT (0, Node->Flags)],
    RimtNodeIommuProximityDomainTypeName[GET_BIT (1, Node->Flags)]
    );
  Print (L"\tProximity Domain            : %d\n", Node->ProximityDomain);
  Print (L"\tPCIe Segment Number         : %d\n", Node->PcieSegmentNumber);
  Print (L"\tPCIe B/D/F                  : 0x%x\n", Node->PcieBdf);
  Print (L"\tNumber of IRQ Wires         : %d\n", Node->NumberOfInterruptWires);
  Print (L"\tOffset to IRQ Wire Array    : 0x%x\n", Node->InterruptWireArrayOffset);
}

/**
  This function parses the ID Mapping Node.
  @param [in] Node                     Pointer to ID Mapping node
 **/
STATIC
VOID
EFIAPI
PrintPcieRcIdMappingNode (
  IN EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE  *IdMapping
  )
{
  Print (L"\t\tSource Id Base              : 0x%x\n", IdMapping->SourceIdBase);
  Print (L"\t\tNumber Of IDs in Range      : %d\n", IdMapping->NumberOfIDs);
  Print (L"\t\tDestination Device ID Base  : 0x%x\n", IdMapping->DestinationDeviceIdBase);
  Print (L"\t\tDestination IOMMU Offset    : 0x%x\n", IdMapping->DestinationIommuOffset);
  Print (
    L"\t\tFlags                       : (0x%x) ATS %s | PRI %s\n",
    IdMapping->Flags,
    RimtNodeIdMappingFlag[GET_BIT (0, IdMapping->Flags)],
    RimtNodeIdMappingFlag[GET_BIT (1, IdMapping->Flags)]
    );
}

/**
  This function parses ID Mapping array.
  @param [in] IdMappingArray                 Pointer to ID mappings
  @param [in] NumberOfIdMappings             Number of ID mappings
 **/
STATIC
VOID
EFIAPI
PrintRimtNodePcieRcIdMappings (
  IN EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE  *IdMappingArray,
  IN UINT16                                  NumberOfIdMappings
  )
{
  UINT16  i;

  for (i = 0; i < NumberOfIdMappings; i++) {
    Print (L"\tId Mapping [%d]:\n", i);
    PrintPcieRcIdMappingNode (&IdMappingArray[i]);
  }
}

/**
  This function parses the Root Complex Node together with corresponding ID Mapping nodes.
  @param [in] Node                     Pointer to PCIe Root Complex node
 **/
STATIC
VOID
EFIAPI
PrintRimtNodePcieRc (
  IN EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE  *Node
  )
{
  EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE  *IdMappingArray;

  IdMappingArray = (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE *)((UINTN)(Node) + (UINTN)(Node->IdMappingArrayOffset));
  PrintRimtNodeHeader ((EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE *)Node);
  Print (
    L"\tFlags                       : (0x%x) ATS %s | PRI %s\n",
    Node->Flags,
    RimtNodeIdMappingFlag[GET_BIT (0, Node->Flags)],
    RimtNodeIdMappingFlag[GET_BIT (1, Node->Flags)]
    );
  Print (L"\tReserved                    : 0x%x\n", Node->Reserved);
  Print (L"\tPCI Segment Number          : %d\n", Node->PcieSegmentNumber);
  Print (L"\tOffset to ID Mapping Array  : 0x%x\n", Node->IdMappingArrayOffset);
  Print (L"\tNumber of ID Mappings       : %d\n", Node->NumberOfIdMappings);
  PrintRimtNodePcieRcIdMappings (IdMappingArray, Node->NumberOfIdMappings);
}

/**
  TODO: This has been not tested
  This function parses the Platform Device Node and corresponding ID Mapping nodes.
  @param [in] Node                 Pointer to Platform device node
 **/
STATIC
VOID
EFIAPI
PrintRimtNodePlatformDevice (
  IN EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE  *Node
  )
{
  PrintRimtNodeHeader ((EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE *)Node);
  Print (L"\tParser not implemented for Platform Device RIMT node\n");
}

/**
  This function Iterates over RIMT Nodes and prints every node.

  @param [in] Rimt    Pointer to the start of RIMT
**/
STATIC
VOID
EFIAPI
ParseAcpiRimtNodes (
  IN EFI_ACPI_6_6_RIMT_STRUCTURE  *Rimt
  )
{
  UINT32                                   i;
  UINT32                                   NumberOfRimtNodes;
  EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE  *Node;

  Node              = (EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE *)((UINTN)Rimt + (UINTN)(Rimt->OffsetToRimtNodeArray));
  NumberOfRimtNodes = Rimt->NumberOfRimtNodes;
  Print (L"\nOffset to RIMT Node Array: 0x%x, Number of Nodes: %d\n", Rimt->OffsetToRimtNodeArray, NumberOfRimtNodes);
  for (i = 0; i < NumberOfRimtNodes; i++) {
    if (Node->Type == IOMMU) {
      PrintRimtNodeIommu ((EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE *)Node);
    } else if (Node->Type == PCIERC) {
      PrintRimtNodePcieRc ((EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE *)Node);
    } else if (Node->Type == PLATFORM) {
      /* TODO: Printing platform device nodes Not tested */
      PrintRimtNodePlatformDevice ((EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE *)Node);
    } else {
      Print (L"\nError!!!!\n");
    }

    Node = (EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE *)((UINTN)Node + (UINTN)(Node->Length));
  }
}

/**
  This function parses the ACPI RIMT table.
  When trace is enabled this function parses the RIMT table and
  traces the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiRimt (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  EFI_ACPI_6_6_RIMT_STRUCTURE  *Rimt;

  if (!Trace) {
    return;
  }

  ParseAcpi (
    Trace,
    1,
    "RIMT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (RimtParser)
    );
  Rimt = (EFI_ACPI_6_6_RIMT_STRUCTURE *)Ptr;
  ParseAcpiRimtNodes (Rimt);
}
