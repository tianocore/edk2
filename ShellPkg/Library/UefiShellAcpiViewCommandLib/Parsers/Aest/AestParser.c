/** @file
  AEST table parser

  Copyright (c) 2020 - 2024, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI for the Armv8 RAS Extensions 1.1 Platform Design Document,
      dated 28 September 2020.
      (https://developer.arm.com/documentation/den0085/0101/)
**/

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <IndustryStandard/ArmErrorSourceTable.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;
STATIC UINT8                         *AestNodeType;
STATIC UINT16                        *AestNodeLength;
STATIC UINT32                        *NodeDataOffset;
STATIC UINT32                        *NodeInterfaceOffset;
STATIC UINT32                        *NodeInterruptArrayOffset;
STATIC UINT32                        *NodeInterruptCount;
STATIC UINT32                        *ProcessorId;
STATIC UINT8                         *ProcessorFlags;
STATIC UINT8                         *ProcessorResourceType;

/**
  Validate Processor Flags.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateProcessorFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  // If the global or shared node flag is set then the ACPI Processor ID
  // field must be set to 0 and ignored.
  if (((*Ptr & 0x3) != 0) && (*ProcessorId != 0)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: 'ACPI Processor ID' field must be set to 0 for global"
      L" or shared nodes."
      );
  }
}

/**
  Validate GIC Interface Type.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGicInterfaceType (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT32  GicInterfaceType;

  GicInterfaceType = *(UINT32 *)Ptr;
  if (GicInterfaceType > 3) {
    IncrementErrorCount ();
    Print (L"\nError: Invalid GIC Interface type %d", GicInterfaceType);
  }
}

/**
  Validate Interface Type.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateInterfaceType (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*Ptr > 1) {
    IncrementErrorCount ();
    Print (L"\nError: Interface type should be 0 or 1");
  }
}

/**
  Validate Interrupt Type.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateInterruptType (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*Ptr > 1) {
    IncrementErrorCount ();
    Print (L"\nError: Interrupt type should be 0 or 1");
  }
}

/**
  Validate interrupt flags.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateInterruptFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if ((*Ptr & 0xfe) != 0) {
    IncrementErrorCount ();
    Print (L"\nError: Reserved Flag bits not set to 0");
  }
}

/**
  Dumps 16 bytes of data.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
VOID
EFIAPI
DumpVendorSpecificData (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  Print (
    L"%02X %02X %02X %02X %02X %02X %02X %02X\n",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5],
    Ptr[6],
    Ptr[7]
    );

  Print (
    L"%*a   %02X %02X %02X %02X %02X %02X %02X %02X",
    OUTPUT_FIELD_COLUMN_WIDTH,
    "",
    Ptr[8],
    Ptr[9],
    Ptr[10],
    Ptr[11],
    Ptr[12],
    Ptr[13],
    Ptr[14],
    Ptr[15]
    );
}

/**
  An ACPI_PARSER array describing the ACPI AEST Table.
**/
STATIC CONST ACPI_PARSER  AestParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo)
};

/**
  An ACPI_PARSER array describing the AEST Node Header.
**/
STATIC CONST ACPI_PARSER  AestNodeHeaderParser[] = {
  { L"Type",                           1, 0,  L"%d",    NULL, (VOID **)&AestNodeType,   NULL, NULL },
  { L"Length",                         2, 1,  L"%d",    NULL, (VOID **)&AestNodeLength, NULL, NULL },
  { L"Reserved",                       1, 3,  L"0x%x",  NULL, NULL,                     NULL, NULL },
  { L"Node Data Offset",               4, 4,  L"%d",    NULL, (VOID **)&NodeDataOffset, NULL, NULL },
  { L"Node Interface Offset",          4, 8,  L"%d",    NULL,
    (VOID **)&NodeInterfaceOffset, NULL, NULL },
  { L"Node Interrupt Array Offset",    4, 12, L"%d",    NULL,
    (VOID **)&NodeInterruptArrayOffset, NULL, NULL },
  { L"Node Interrupt Count",           4, 16, L"%d",    NULL,
    (VOID **)&NodeInterruptCount, NULL, NULL },
  { L"Timestamp Rate",                 8, 20, L"%ld",   NULL, NULL,                     NULL, NULL },
  { L"Reserved1",                      8, 28, L"0x%lx", NULL, NULL,                     NULL, NULL },
  { L"Error Injection Countdown Rate", 8, 36, L"%ld",   NULL, NULL,                     NULL, NULL }
  // Node specific data...
  // Node interface...
  // Node interrupt array...
};

/**
  An ACPI_PARSER array describing the Processor error node specific data.
**/
STATIC CONST ACPI_PARSER  AestProcessorStructure[] = {
  { L"ACPI Processor ID",                  4, 0, L"0x%x",  NULL, (VOID **)&ProcessorId,           NULL, NULL },
  { L"Resource Type",                      1, 4, L"%d",    NULL, (VOID **)&ProcessorResourceType, NULL,
    NULL },
  { L"Reserved",                           1, 5, L"0x%x",  NULL, NULL,                            NULL, NULL },
  { L"Flags",                              1, 6, L"0x%x",  NULL, (VOID **)&ProcessorFlags,
    ValidateProcessorFlags, NULL },
  { L"Revision",                           1, 7, L"%d",    NULL, NULL,                            NULL, NULL },
  { L"Processor Affinity Level Indicator", 8, 8, L"0x%lx", NULL, NULL,                            NULL,
    NULL },
  // Resource specific data...
};

/**
  An ACPI_PARSER array describing the processor cache resource substructure.
**/
STATIC CONST ACPI_PARSER  AestProcessorCacheResourceSubstructure[] = {
  { L"Cache reference ID", 4, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",           4, 4, L"%d",   NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the processor TLB resource substructure.
**/
STATIC CONST ACPI_PARSER  AestProcessorTlbResourceSubstructure[] = {
  { L"TLB reference ID", 4, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",         4, 4, L"%d",   NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the processor generic resource substructure.
**/
STATIC CONST ACPI_PARSER  AestProcessorGenericResourceSubstructure[] = {
  { L"Vendor-defined data", 4, 0, L"%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the memory controller structure.
**/
STATIC CONST ACPI_PARSER  AestMemoryControllerStructure[] = {
  { L"Proximity Domain", 4, 0, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the SMMU structure.
**/
STATIC CONST ACPI_PARSER  AestSmmuStructure[] = {
  { L"IORT Node reference ID",    4, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"SubComponent reference ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the vendor-defined structure.
**/
STATIC CONST ACPI_PARSER  AestVendorDefinedStructure[] = {
  { L"Hardware ID",          4,  0, L"0x%x", NULL,                   NULL, NULL, NULL },
  { L"Unique ID",            4,  4, L"0x%x", NULL,                   NULL, NULL, NULL },
  { L"Vendor-specific data", 16, 8, NULL,    DumpVendorSpecificData, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the GIC structure.
**/
STATIC CONST ACPI_PARSER  AestGicStructure[] = {
  { L"GIC Interface Type",         4, 0, L"0x%x", NULL, NULL, ValidateGicInterfaceType,
    NULL },
  { L"GIC Interface reference ID", 4, 4, L"0x%x", NULL, NULL, NULL,                    NULL}
};

/**
  An ACPI_PARSER array describing the node interface.
**/
STATIC CONST ACPI_PARSER  AestNodeInterface[] = {
  { L"Interface Type",            1, 0,  L"%d",       NULL,       NULL, ValidateInterfaceType, NULL },
  { L"Reserved",                  3, 1,  L"%x %x %x", Dump3Chars, NULL, NULL,                  NULL },
  { L"Flags",                     4, 4,  L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"Base Address",              8, 8,  L"0x%lx",    NULL,       NULL, NULL,                  NULL },
  { L"Start Error Record Index",  4, 16, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"Number of Error Records",   4, 20, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"Error Records Implemented", 8, 24, L"0x%lx",    NULL,       NULL, NULL,                  NULL },
  { L"Error Records Support",     8, 32, L"0x%lx",    NULL,       NULL, NULL,                  NULL },
  { L"Addressing mode",           8, 40, L"0x%lx",    NULL,       NULL, NULL,                  NULL }
};

/**
  An ACPI_PARSER array describing the node interrupts.
**/
STATIC CONST ACPI_PARSER  AestNodeInterrupt[] = {
  { L"Interrupt Type",  1, 0, L"%d",       NULL,       NULL, ValidateInterruptType,  NULL },
  { L"Reserved",        2, 1, L"0x%x",     NULL,       NULL, NULL,                   NULL },
  { L"Interrupt Flags", 1, 3, L"0x%x",     NULL,       NULL, ValidateInterruptFlags, NULL },
  { L"Interrupt GSIV",  4, 4, L"0x%x",     NULL,       NULL, NULL,                   NULL },
  { L"ID",              1, 8, L"0x%x",     NULL,       NULL, NULL,                   NULL },
  { L"Reserved1",       3, 9, L"%x %x %x", Dump3Chars, NULL, NULL,                   NULL }
};

/**
  Parses the Processor Error Node structure along with its resource
  specific data.

  @param [in] Ptr     Pointer to the start of the Processor node.
  @param [in] Length  Maximum length of the Processor node.
**/
STATIC
VOID
DumpProcessorNode (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  UINT32  Offset;

  Offset = ParseAcpi (
             TRUE,
             2,
             "Processor",
             Ptr,
             Length,
             PARSER_PARAMS (AestProcessorStructure)
             );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((ProcessorId == NULL)           ||
      (ProcessorResourceType == NULL) ||
      (ProcessorFlags == NULL))
  {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient Processor Error Node length. Length = %d.\n",
      Length
      );
    return;
  }

  switch (*ProcessorResourceType) {
    case EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_CACHE:
      ParseAcpi (
        TRUE,
        2,
        "Cache Resource",
        Ptr + Offset,
        Length - Offset,
        PARSER_PARAMS (AestProcessorCacheResourceSubstructure)
        );
      break;
    case EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_TLB:
      ParseAcpi (
        TRUE,
        2,
        "TLB Resource",
        Ptr + Offset,
        Length - Offset,
        PARSER_PARAMS (AestProcessorTlbResourceSubstructure)
        );
      break;
    case EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_GENERIC:
      ParseAcpi (
        TRUE,
        2,
        "Generic Resource",
        Ptr + Offset,
        Length - Offset,
        PARSER_PARAMS (AestProcessorGenericResourceSubstructure)
        );
      break;
    default:
      IncrementErrorCount ();
      Print (L"ERROR: Invalid Processor Resource Type.");
      return;
  } // switch
}

/**
  Parses the Memory Controller node.

  @param [in] Ptr     Pointer to the start of the Memory Controller node.
  @param [in] Length  Maximum length of the Memory Controller node.
**/
STATIC
VOID
DumpMemoryControllerNode (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Memory Controller",
    Ptr,
    Length,
    PARSER_PARAMS (AestMemoryControllerStructure)
    );
}

/**
  Parses the SMMU node.

  @param [in] Ptr     Pointer to the start of the SMMU node.
  @param [in] Length  Maximum length of the SMMU node.
**/
STATIC
VOID
DumpSmmuNode (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "SMMU",
    Ptr,
    Length,
    PARSER_PARAMS (AestSmmuStructure)
    );
}

/**
  Parses the Vendor-defined structure.

  @param [in] Ptr     Pointer to the start of the Vendor-defined node.
  @param [in] Length  Maximum length of the Vendor-defined node.
**/
STATIC
VOID
DumpVendorDefinedNode (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Vendor-defined",
    Ptr,
    Length,
    PARSER_PARAMS (AestVendorDefinedStructure)
    );
}

/**
  Parses the GIC node.

  @param [in] Ptr     Pointer to the start of the GIC node.
  @param [in] Length  Maximum length of the GIC node.
**/
STATIC
VOID
DumpGicNode (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "GIC",
    Ptr,
    Length,
    PARSER_PARAMS (AestGicStructure)
    );
}

/**
  Parses the Node Interface structure.

  @param [in] Ptr     Pointer to the start of the Node Interface Structure.
  @param [in] Length  Maximum length of the Node Interface Structure.
**/
STATIC
VOID
DumpNodeInterface (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Node Interface",
    Ptr,
    Length,
    PARSER_PARAMS (AestNodeInterface)
    );
}

/**
  Parses the Node Interrupts Structure.

  @param [in] Ptr             Pointer to the start of the Node Interrupt array.
  @param [in] Length          Maximum length of the Node Interrupt array.
  @param [in] InterruptCount  Number if interrupts in the Node Interrupts array.
**/
STATIC
VOID
DumpNodeInterrupts (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN UINT32  InterruptCount
  )
{
  UINT32  Offset;
  UINT32  Index;
  CHAR8   Buffer[64];

  if (Length < (InterruptCount * sizeof (EFI_ACPI_AEST_INTERRUPT_STRUCT))) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Node not long enough for Interrupt Array.\n" \
      L"       Length left = %d, Required = %d, Interrupt Count = %d\n",
      Length,
      (InterruptCount * sizeof (EFI_ACPI_AEST_INTERRUPT_STRUCT)),
      InterruptCount
      );
    return;
  }

  Offset = 0;
  for (Index = 0; Index < InterruptCount; Index++) {
    AsciiSPrint (
      Buffer,
      sizeof (Buffer),
      "Node Interrupt [%d]",
      Index
      );

    Offset += ParseAcpi (
                TRUE,
                4,
                Buffer,
                Ptr + Offset,
                Length - Offset,
                PARSER_PARAMS (AestNodeInterrupt)
                );
  } // for
}

/**
  Parses a single AEST Node Structure.

  @param [in] Ptr                   Pointer to the start of the Node.
  @param [in] Length                Maximum length of the Node.
  @param [in] NodeType              AEST node type.
  @param [in] DataOffset            Offset to the node data.
  @param [in] InterfaceOffset       Offset to the node interface data.
  @param [in] InterruptArrayOffset  Offset to the node interrupt array.
  @param [in] InterruptCount        Number of interrupts.
**/
STATIC
VOID
DumpAestNodeStructure (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN UINT8   NodeType,
  IN UINT32  DataOffset,
  IN UINT32  InterfaceOffset,
  IN UINT32  InterruptArrayOffset,
  IN UINT32  InterruptCount
  )
{
  UINT32  Offset;
  UINT32  RemainingLength;
  UINT8   *NodeDataPtr;

  Offset = ParseAcpi (
             TRUE,
             2,
             "Node Structure",
             Ptr,
             Length,
             PARSER_PARAMS (AestNodeHeaderParser)
             );

  if ((Offset > DataOffset) || (DataOffset > Length)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Node Data Offset: %d.\n" \
      L"       It should be between %d and %d.\n",
      DataOffset,
      Offset,
      Length
      );
  }

  if ((Offset > InterfaceOffset) || (InterfaceOffset > Length)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Node Interface Offset: %d.\n" \
      L"       It should be between %d and %d.\n",
      InterfaceOffset,
      Offset,
      Length
      );
  }

  if ((Offset > InterruptArrayOffset) || (InterruptArrayOffset > Length)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Node Interrupt Array Offset: %d.\n" \
      L"       It should be between %d and %d.\n",
      InterruptArrayOffset,
      Offset,
      Length
      );
  }

  // Parse Node Data Field.
  NodeDataPtr     = Ptr + DataOffset;
  RemainingLength = Length - DataOffset;
  switch (NodeType) {
    case EFI_ACPI_AEST_NODE_TYPE_PROCESSOR:
      DumpProcessorNode (NodeDataPtr, RemainingLength);
      break;
    case EFI_ACPI_AEST_NODE_TYPE_MEMORY:
      DumpMemoryControllerNode (NodeDataPtr, RemainingLength);
      break;
    case EFI_ACPI_AEST_NODE_TYPE_SMMU:
      DumpSmmuNode (NodeDataPtr, RemainingLength);
      break;
    case EFI_ACPI_AEST_NODE_TYPE_VENDOR_DEFINED:
      DumpVendorDefinedNode (NodeDataPtr, RemainingLength);
      break;
    case EFI_ACPI_AEST_NODE_TYPE_GIC:
      DumpGicNode (NodeDataPtr, RemainingLength);
      break;
    default:
      IncrementErrorCount ();
      Print (L"ERROR: Invalid Error Node Type.\n");
      return;
  } // switch

  // Parse the Interface Field.
  DumpNodeInterface (
    Ptr + InterfaceOffset,
    Length - InterfaceOffset
    );

  // Parse the Node Interrupt Array.
  DumpNodeInterrupts (
    Ptr + InterruptArrayOffset,
    Length - InterruptArrayOffset,
    InterruptCount
    );

  return;
}

/**
  This function parses the ACPI AEST table.
  When trace is enabled this function parses the AEST table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiAest (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;
  UINT8   *NodePtr;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "AEST",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (AestParser)
             );

  while (Offset < AcpiTableLength) {
    NodePtr = Ptr + Offset;

    ParseAcpi (
      FALSE,
      0,
      NULL,
      NodePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (AestNodeHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((AestNodeType == NULL)             ||
        (AestNodeLength == NULL)           ||
        (NodeDataOffset == NULL)           ||
        (NodeInterfaceOffset == NULL)      ||
        (NodeInterruptArrayOffset == NULL) ||
        (NodeInterruptCount == NULL))
    {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient length left for Node Structure.\n" \
        L"       Length left = %d.\n",
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate AEST Node length
    if ((*AestNodeLength == 0) ||
        ((Offset + (*AestNodeLength)) > AcpiTableLength))
    {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid AEST Node length. " \
        L"Length = %d. Offset = %d. AcpiTableLength = %d.\n",
        *AestNodeLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    DumpAestNodeStructure (
      NodePtr,
      *AestNodeLength,
      *AestNodeType,
      *NodeDataOffset,
      *NodeInterfaceOffset,
      *NodeInterruptArrayOffset,
      *NodeInterruptCount
      );

    Offset += *AestNodeLength;
  } // while
}
