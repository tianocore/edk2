/** @file
  AEST table parser

  Copyright (c) 2020 - 2026, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI for the Armv8 RAS Extensions 1.1 Platform Design Document,
      dated 28 September 2020.
      (https://developer.arm.com/documentation/den0085/0101/)
    - ACPI for the Armv8-A RAS Extension and RAS System Architecture,
      DEN0085 2.0 EAC2.
      (https://developer.arm.com/documentation/den0085/0202/)
**/

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <IndustryStandard/ArmErrorSourceTable.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  mAcpiHdrInfo;
STATIC UINT32                        *mProcessorId;
STATIC UINT8                         *mProcessorFlags;
STATIC UINT8                         *mProcessorResourceType;
STATIC CONST UINT8                   mAestProcessorFlags1   = EFI_ACPI_AEST_PROCESSOR_FLAGS_VALID_MASK;
STATIC CONST UINT8                   mAestProcessorFlags2   = EFI_ACPI_AEST_2_0_PROCESSOR_FLAGS_VALID_MASK;
STATIC CONST UINT32                  mAestInterfaceFlags1   = EFI_ACPI_AEST_INTERFACE_FLAGS_VALID_MASK;
STATIC CONST UINT32                  mAestInterfaceFlags2   = EFI_ACPI_AEST_2_0_INTERFACE_FLAGS_VALID_MASK;
STATIC CONST UINT8                   mAestInterruptFlags1   = EFI_ACPI_AEST_INTERRUPT_FLAGS_VALID_MASK;
STATIC CONST UINT8                   mAestInterruptFlags2   = EFI_ACPI_AEST_2_0_INTERRUPT_FLAGS_VALID_MASK;
STATIC CONST UINT8                   mAestInterfaceTypeMax1 = EFI_ACPI_AEST_INTERFACE_TYPE_MMIO;
STATIC CONST UINT8                   mAestInterfaceTypeMax2 = EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SINGLE_MMIO;

/**
  Validate Processor Flags.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to the valid processor flag mask.
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
  UINT8  FlagsMask;

  FlagsMask = *(CONST UINT8 *)Context;
  // If the global or shared node flag is set then the ACPI Processor ID
  // field must be set to 0 and ignored.
  if (((*Ptr & FlagsMask) != 0) && (*mProcessorId != 0)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: 'ACPI Processor ID' field must be set to 0 for global"
      L" or shared nodes."
      );
  }

  if ((*Ptr & ~FlagsMask) != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Reserved processor flag bits must be zero.");
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
  if (GicInterfaceType > EFI_ACPI_AEST_GIC_INTERFACE_TYPE_GITS) {
    IncrementErrorCount ();
    Print (L"\nError: Invalid GIC Interface type %d", GicInterfaceType);
  }
}

/**
  Validate Interface Type.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to the maximum interface type for this revision.
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
  UINT8  MaxInterfaceType;

  MaxInterfaceType = *(CONST UINT8 *)Context;
  if (*Ptr > MaxInterfaceType) {
    IncrementErrorCount ();
    Print (L"\nERROR: Invalid AEST interface type %u.", *Ptr);
  }
}

/**
  Validate the interface flag bits defined by the selected revision.

  @param [in] Ptr      Pointer to the field data.
  @param [in] Length   Length of the field.
  @param [in] Context  Pointer to the allowed UINT32 flag mask.
**/
STATIC
VOID
EFIAPI
ValidateInterfaceFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT32  Flags;

  Flags = *(UINT32 *)Ptr;
  if ((Flags & ~(*(CONST UINT32 *)Context)) != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Reserved interface flag bits must be zero.");
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
  if (*Ptr > EFI_ACPI_AEST_INTERRUPT_TYPE_ERROR_RECOVERY) {
    IncrementErrorCount ();
    Print (L"\nError: Interrupt type should be 0 or 1");
  }
}

/**
  Validate interrupt flags.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to the valid interrupt flag mask.
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
  UINT8  FlagsMask;

  FlagsMask = *(CONST UINT8 *)Context;
  if ((*Ptr & ~FlagsMask) != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Reserved interrupt flag bits must be zero.");
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
STATIC CONST ACPI_PARSER  mAestParser[] = {
  PARSE_ACPI_HEADER (&mAcpiHdrInfo)
};

/**
  AEST node types strings
**/
STATIC CONST CHAR8  *mAestNodeTypeNames[] = {
  [EFI_ACPI_AEST_2_0_NODE_TYPE_PROCESSOR] = "* Processor Error Node *",
  [EFI_ACPI_AEST_2_0_NODE_TYPE_MEMORY]    = "* Memory Controller Error Node *",
  [EFI_ACPI_AEST_2_0_NODE_TYPE_SMMU]      = "* SMMU Error Node *",
  [EFI_ACPI_AEST_2_0_NODE_TYPE_VENDOR]    = "* Vendor-Defined Error Node *",
  [EFI_ACPI_AEST_2_0_NODE_TYPE_GIC]       = "* GIC Error Node *",
  [EFI_ACPI_AEST_2_0_NODE_TYPE_PCIE]      = "* PCIe Root Complex Error Node *",
  [EFI_ACPI_AEST_2_0_NODE_TYPE_PROXY]     = "* Proxy Error Node *"
};

/**
  AEST node interface bitmap names.
**/
STATIC CONST CHAR16  *mAestBitmapNames[] = {
  L"Error Records Implemented",
  L"Group Status Reporting Supported",
  L"Addressing Mode"
};

/**
  An ACPI_PARSER array describing the AEST Node Header.
**/
STATIC CONST ACPI_PARSER  mAestNodeHeaderParser[] = {
  { L"Type",                           1, 0,  L"%d",    NULL, NULL, NULL,             NULL },
  { L"Length",                         2, 1,  L"%d",    NULL, NULL, NULL,             NULL },
  { L"Reserved",                       1, 3,  L"0x%x",  NULL, NULL, ValidateReserved, NULL },
  { L"Node Data Offset",               4, 4,  L"0x%x",  NULL, NULL, NULL,             NULL },
  { L"Node Interface Offset",          4, 8,  L"0x%x",  NULL, NULL, NULL,             NULL },
  { L"Node Interrupt Array Offset",    4, 12, L"0x%x",  NULL, NULL, NULL,             NULL },
  { L"Node Interrupt Count",           4, 16, L"%d",    NULL, NULL, NULL,             NULL },
  { L"Timestamp Rate",                 8, 20, L"%ld",   NULL, NULL, NULL,             NULL },
  { L"Reserved1",                      8, 28, L"0x%lx", NULL, NULL, ValidateReserved, NULL },
  { L"Error Injection Countdown Rate", 8, 36, L"%ld",   NULL, NULL, NULL,             NULL }
  // Node specific data...
  // Node interface...
  // Node interrupt array...
};

/**
  An ACPI_PARSER array describing the AEST 2.0+ Node Header
**/
STATIC CONST ACPI_PARSER  mAestNodeHeaderParser2[] = {
  { L"Type",                           1, 0,  L"%d",   NULL, NULL, NULL,             NULL },
  { L"Length",                         2, 1,  L"%d",   NULL, NULL, NULL,             NULL },
  { L"Reserved",                       1, 3,  L"0x%x", NULL, NULL, ValidateReserved, NULL },
  { L"Node Data Offset",               4, 4,  L"0x%x", NULL, NULL, NULL,             NULL },
  { L"Node Interface Offset",          4, 8,  L"0x%x", NULL, NULL, NULL,             NULL },
  { L"Node Interrupt Array Offset",    4, 12, L"0x%x", NULL, NULL, NULL,             NULL },
  { L"Node Interrupt Count",           4, 16, L"%d",   NULL, NULL, NULL,             NULL },
  { L"Timestamp Rate",                 8, 20, L"%ld",  NULL, NULL, NULL,             NULL },
  { L"Timebase Offset",                8, 28, L"%ld",  NULL, NULL, NULL,             NULL },
  { L"Error Injection Countdown Rate", 8, 36, L"%ld",  NULL, NULL, NULL,             NULL }
};

/**
  An ACPI_PARSER array describing the Processor error node specific data.
**/
STATIC CONST ACPI_PARSER  mAestProcessorStructure[] = {
  { L"ACPI Processor ID",                  4, 0, L"0x%x",  NULL, (VOID **)&mProcessorId,           NULL, NULL },
  { L"Resource Type",                      1, 4, L"%d",    NULL, (VOID **)&mProcessorResourceType, NULL,
    NULL },
  { L"Reserved",                           1, 5, L"0x%x",  NULL, NULL,
    ValidateReserved, NULL },
  { L"Flags",                              1, 6, L"0x%x",  NULL, (VOID **)&mProcessorFlags,
    ValidateProcessorFlags, (VOID *)&mAestProcessorFlags1 },
  { L"Revision",                           1, 7, L"%d",    NULL, NULL,                             NULL, NULL },
  { L"Processor Affinity Level Indicator", 8, 8, L"0x%lx", NULL, NULL,                             NULL,
    NULL },
  // Resource specific data...
};

/**
  An ACPI_PARSER array describing the Processor error node specific data.
  AEST 2.0+ deprecates the processor affinity level indicator and requires
  Revision field to be set to zero.
**/
STATIC CONST ACPI_PARSER  mAestProcessorStructure2[] = {
  { L"ACPI Processor ID",                  4, 0, L"0x%x",  NULL, (VOID **)&mProcessorId,
    NULL, NULL },
  { L"Resource Type",                      1, 4, L"%d",    NULL, (VOID **)&mProcessorResourceType,
    NULL, NULL },
  { L"Reserved",                           1, 5, L"0x%x",  NULL, NULL,
    ValidateReserved, NULL },
  { L"Flags",                              1, 6, L"0x%x",  NULL, (VOID **)&mProcessorFlags,
    ValidateProcessorFlags, (VOID *)&mAestProcessorFlags2 },
  { L"Revision",                           1, 7, L"%d",    NULL, NULL,
    ValidateReserved, NULL },
  { L"Processor Affinity Level Indicator", 8, 8, L"0x%lx", NULL, NULL,
    ValidateReserved, NULL }
};

/**
  An ACPI_PARSER array describing the processor cache resource substructure.
**/
STATIC CONST ACPI_PARSER  mAestProcessorCacheResourceSubstructure[] = {
  { L"Cache reference ID", 4, 0, L"0x%x", NULL, NULL, NULL,             NULL },
  { L"Reserved",           4, 4, L"%d",   NULL, NULL, ValidateReserved, NULL }
};

/**
  An ACPI_PARSER array describing the processor TLB resource substructure.
**/
STATIC CONST ACPI_PARSER  mAestProcessorTlbResourceSubstructure[] = {
  { L"TLB reference ID", 4, 0, L"0x%x", NULL, NULL, NULL,             NULL },
  { L"Reserved",         4, 4, L"%d",   NULL, NULL, ValidateReserved, NULL }
};

/**
  An ACPI_PARSER array describing the processor generic resource substructure.
**/
STATIC CONST ACPI_PARSER  mAestProcessorGenericResourceSubstructure[] = {
  { L"Vendor-defined data", 4, 0, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the memory controller structure.
**/
STATIC CONST ACPI_PARSER  mAestMemoryControllerStructure[] = {
  { L"Proximity Domain", 4, 0, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the SMMU structure.
**/
STATIC CONST ACPI_PARSER  mAestSmmuStructure[] = {
  { L"IORT Node reference ID",    4, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"SubComponent reference ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the vendor-defined structure.
**/
STATIC CONST ACPI_PARSER  mAestVendorDefinedStructure[] = {
  { L"Hardware ID",          4,  0, L"0x%x", NULL,                   NULL, NULL, NULL },
  { L"Unique ID",            4,  4, L"0x%x", NULL,                   NULL, NULL, NULL },
  { L"Vendor-specific data", 16, 8, NULL,    DumpVendorSpecificData, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the vendor-defined structure for AEST 2.0+.
  AEST 2.0+ expands the vendor hardware ID from four to eight bytes.
**/
STATIC CONST ACPI_PARSER  mAestVendorDefinedStructure2[] = {
  { L"Hardware ID",          8,  0,  NULL,    Dump8Chars,             NULL, NULL, NULL },
  { L"Unique ID",            4,  8,  L"0x%x", NULL,                   NULL, NULL, NULL },
  { L"Vendor-specific data", 16, 12, NULL,    DumpVendorSpecificData, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the GIC structure.
**/
STATIC CONST ACPI_PARSER  mAestGicStructure[] = {
  { L"GIC Interface Type",         4, 0, L"0x%x", NULL, NULL, ValidateGicInterfaceType,
    NULL },
  { L"GIC Interface reference ID", 4, 4, L"0x%x", NULL, NULL, NULL,                    NULL}
};

STATIC CONST ACPI_PARSER  mAestPcieStructure[] = {
  { L"IORT Node reference ID", 4, 0, L"0x%x", NULL, NULL, NULL, NULL }
};

STATIC CONST ACPI_PARSER  mAestProxyStructure[] = {
  { L"Proxied Node Address", 8, 0, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the node interface.
**/
STATIC CONST ACPI_PARSER  mAestNodeInterface[] = {
  { L"Interface Type",            1, 0,  L"%d",       NULL,       NULL, ValidateInterfaceType,
    (VOID *)&mAestInterfaceTypeMax1 },
  { L"Reserved",                  3, 1,  L"%x %x %x", Dump3Chars, NULL, ValidateReserved,      NULL  },
  { L"Flags",                     4, 4,  L"0x%x",     NULL,       NULL, ValidateInterfaceFlags,
    (VOID *)&mAestInterfaceFlags1 },
  { L"Base Address",              8, 8,  L"0x%lx",    NULL,       NULL, NULL,                  NULL  },
  { L"Start Error Record Index",  4, 16, L"0x%x",     NULL,       NULL, NULL,                  NULL  },
  { L"Number of Error Records",   4, 20, L"0x%x",     NULL,       NULL, NULL,                  NULL  },
  { L"Error Records Implemented", 8, 24, L"0x%lx",    NULL,       NULL, NULL,                  NULL  },
  { L"Error Records Support",     8, 32, L"0x%lx",    NULL,       NULL, NULL,                  NULL  },
  { L"Addressing mode",           8, 40, L"0x%lx",    NULL,       NULL, NULL,                  NULL  }
};

/**
  ACPI_PARSER arrays describing AEST 2.0+ interface fields before and after the variable bitmaps.
**/
STATIC CONST ACPI_PARSER  mAestNodeInterface2Prefix[] = {
  { L"Interface Type",           1, 0,  L"%d",    NULL, NULL, ValidateInterfaceType,
    (VOID *)&mAestInterfaceTypeMax2 },
  { L"Group Format",             1, 1,  L"%d",    NULL, NULL, NULL,                  NULL  },
  { L"Reserved",                 2, 2,  L"0x%x",  NULL, NULL, ValidateReserved,      NULL  },
  { L"Flags",                    4, 4,  L"0x%x",  NULL, NULL, ValidateInterfaceFlags,
    (VOID *)&mAestInterfaceFlags2 },
  { L"Base Address",             8, 8,  L"0x%lx", NULL, NULL, NULL,                  NULL  },
  { L"Start Error Record Index", 4, 16, L"0x%x",  NULL, NULL, NULL,                  NULL  },
  { L"Number of Error Records",  4, 20, L"0x%x",  NULL, NULL, NULL,                  NULL  }
};

STATIC CONST ACPI_PARSER  mAestNodeInterface2Suffix[] = {
  { L"Arm Error Node Device UID",      4, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Processor Affinity",             4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Error Group Registers Base",     8, 8,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Fault Injection Registers Base", 8, 16, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Interrupt Configuration Base",   8, 24, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the node interrupts.
**/
STATIC CONST ACPI_PARSER  mAestNodeInterrupt[] = {
  { L"Interrupt Type",  1, 0, L"%d",       NULL,       NULL, ValidateInterruptType,  NULL },
  { L"Reserved",        2, 1, L"0x%x",     NULL,       NULL, ValidateReserved,       NULL },
  { L"Interrupt Flags", 1, 3, L"0x%x",     NULL,       NULL, ValidateInterruptFlags,
    (VOID *)&mAestInterruptFlags1 },
  { L"Interrupt GSIV",  4, 4, L"0x%x",     NULL,       NULL, NULL,                   NULL },
  { L"ID",              1, 8, L"0x%x",     NULL,       NULL, NULL,                   NULL },
  { L"Reserved1",       3, 9, L"%x %x %x", Dump3Chars, NULL, ValidateReserved,       NULL }
};

STATIC CONST ACPI_PARSER  mAestNodeInterrupt2[] = {
  { L"Interrupt Type",  1, 0, L"%d",   NULL, NULL, ValidateInterruptType,  NULL },
  { L"Reserved",        2, 1, L"0x%x", NULL, NULL, ValidateReserved,       NULL },
  { L"Interrupt Flags", 1, 3, L"0x%x", NULL, NULL, ValidateInterruptFlags,
    (VOID *)&mAestInterruptFlags2 },
  { L"Interrupt GSIV",  4, 4, L"0x%x", NULL, NULL, NULL,                   NULL },
  { L"Reserved",        4, 8, L"0x%x", NULL, NULL, ValidateReserved,       NULL }
};

/**
  Parses the Processor Error Node structure along with its resource
  specific data.

  @param [in] Ptr     Pointer to the start of the Processor node.
  @param [in] Length  Maximum length of the Processor node.
  @param [in] Revision  AEST table revision.
**/
STATIC
VOID
DumpProcessorNode (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN UINT8   Revision
  )
{
  UINT32  Offset;

  if (Revision == EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION) {
    Offset = ParseAcpi (
               TRUE,
               4,
               "Processor Node Specific Data",
               Ptr,
               Length,
               PARSER_PARAMS (mAestProcessorStructure)
               );
  } else {
    Offset = ParseAcpi (
               TRUE,
               4,
               "Processor Node Specific Data",
               Ptr,
               Length,
               PARSER_PARAMS (mAestProcessorStructure2)
               );
  }

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((mProcessorId == NULL)           ||
      (mProcessorResourceType == NULL) ||
      (mProcessorFlags == NULL))
  {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient Processor Error Node length. Length = %d.\n",
      Length
      );
    return;
  }

  switch (*mProcessorResourceType) {
    case EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_CACHE:
      ParseAcpi (
        TRUE,
        4,
        "Cache Resource",
        Ptr + Offset,
        Length - Offset,
        PARSER_PARAMS (mAestProcessorCacheResourceSubstructure)
        );
      break;
    case EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_TLB:
      ParseAcpi (
        TRUE,
        4,
        "TLB Resource",
        Ptr + Offset,
        Length - Offset,
        PARSER_PARAMS (mAestProcessorTlbResourceSubstructure)
        );
      break;
    case EFI_ACPI_AEST_PROCESSOR_RESOURCE_TYPE_GENERIC:
      ParseAcpi (
        TRUE,
        4,
        "Generic Resource",
        Ptr + Offset,
        Length - Offset,
        PARSER_PARAMS (mAestProcessorGenericResourceSubstructure)
        );
      break;
    default:
      IncrementErrorCount ();
      Print (L"ERROR: Invalid Processor Resource Type.\n");
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
    4,
    "Memory Controller Node Specific Data",
    Ptr,
    Length,
    PARSER_PARAMS (mAestMemoryControllerStructure)
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
    4,
    "SMMU Node Specific Data",
    Ptr,
    Length,
    PARSER_PARAMS (mAestSmmuStructure)
    );
}

/**
  Parses the Vendor-defined structure.

  @param [in] Ptr       Pointer to the start of the Vendor-defined node.
  @param [in] Length    Maximum length of the Vendor-defined node.
  @param [in] Revision  AEST table revision.
**/
STATIC
VOID
DumpVendorDefinedNode (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN UINT8   Revision
  )
{
  if (Revision == EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION) {
    ParseAcpi (
      TRUE,
      4,
      "Vendor-Defined Node Specific Data",
      Ptr,
      Length,
      PARSER_PARAMS (mAestVendorDefinedStructure)
      );
  } else {
    ParseAcpi (
      TRUE,
      4,
      "Vendor-Defined Node Specific Data",
      Ptr,
      Length,
      PARSER_PARAMS (mAestVendorDefinedStructure2)
      );
  }
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
    4,
    "GIC Node Specific Data",
    Ptr,
    Length,
    PARSER_PARAMS (mAestGicStructure)
    );
}

/**
  Parse the PCIe Root Complex node.

  @param [in] Ptr     Pointer to node-specific data.
  @param [in] Length  Length of node-specific data.
**/
STATIC
VOID
DumpPcieNode (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  ParseAcpi (
    TRUE,
    4,
    "PCIe Root Complex Node Specific Data",
    Ptr,
    Length,
    PARSER_PARAMS (mAestPcieStructure)
    );
}

/**
  Parse the AEST Proxy node.

  @param [in] Ptr     Pointer to node-specific data.
  @param [in] Length  Length of node-specific data.
**/
STATIC
VOID
DumpProxyNode (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  ParseAcpi (
    TRUE,
    4,
    "Proxy Node Specific Data",
    Ptr,
    Length,
    PARSER_PARAMS (mAestProxyStructure)
    );
}

/**
  Print and validate the three AEST 2.0+ interface bitmaps.

  @param [in] Ptr           Start of the first bitmap.
  @param [in] Length        Length of the buffer.
  @param [in] WordCount     Number of UINT64 words in each bitmap.
  @param [in] MaxRecords    Maximum records in this group format.
  @param [in] StartIndex    First record belonging to this node.
  @param [in] RecordCount   Number of records belonging to this node.
  @param [in] SingleRecord  TRUE for the single-record MMIO view.
**/
STATIC
VOID
DumpAestBitmaps (
  IN UINT8    *Ptr,
  IN UINT32   Length,
  IN UINT32   WordCount,
  IN UINT32   MaxRecords,
  IN UINT32   StartIndex,
  IN UINT32   RecordCount,
  IN BOOLEAN  SingleRecord
  )
{
  UINT32   BitmapIndex;
  UINT32   WordIndex;
  UINT32   BitIndex;
  UINT32   RecordIndex;
  UINT64   Word;
  BOOLEAN  RangeValid;
  BOOLEAN  InvalidBit;

  if (WordCount > Length / (ARRAY_SIZE (mAestBitmapNames) * sizeof (UINT64))) {
    IncrementErrorCount ();
    Print (L"ERROR: Node interface bitmaps extend beyond available length.\n");
    return;
  }

  RangeValid = (StartIndex < MaxRecords) &&
               (RecordCount <= MaxRecords - StartIndex);
  if (!RangeValid) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Error-record start %u and count %u exceed group capacity %u.\n",
      StartIndex,
      RecordCount,
      MaxRecords
      );
  }

  for (BitmapIndex = 0; BitmapIndex < ARRAY_SIZE (mAestBitmapNames); BitmapIndex++) {
    PrintFieldName (6, mAestBitmapNames[BitmapIndex]);
    Print (L"\n");
    for (WordIndex = 0; WordIndex < WordCount; WordIndex++) {
      Word = *(UINT64 *)(Ptr + ((BitmapIndex * WordCount + WordIndex) * sizeof (Word)));
      Print (
        L"        Bits %3u:%3u: 0x%016lx\n",
        WordIndex * 64 + 63,
        WordIndex * 64,
        Word
        );

      if (SingleRecord && (BitmapIndex == 0) && (Word != 0)) {
        IncrementErrorCount ();
        Print (L"ERROR: Single-record MMIO implemented bitmap must be zero.\n");
      }

      if (!RangeValid) {
        continue;
      }

      InvalidBit = FALSE;
      for (BitIndex = 0; BitIndex < 64; BitIndex++) {
        RecordIndex = WordIndex * 64 + BitIndex;
        if (((RecordIndex < StartIndex) ||
             (RecordIndex >= StartIndex + RecordCount)) &&
            ((Word & LShiftU64 (1, BitIndex)) != 0))
        {
          InvalidBit = TRUE;
          break;
        }
      }

      if (InvalidBit) {
        IncrementErrorCount ();
        Print (
          L"ERROR: %s word %u sets a bit outside this node's records.\n",
          mAestBitmapNames[BitmapIndex],
          WordIndex
          );
      }
    }
  }
}

/**
  Parse a AEST 2.0+ variable-length node interface.

  @param [in] Ptr     Start of the interface.
  @param [in] Length  Length of the buffer.

  @retval TRUE   The interface fits in the available length.
  @retval FALSE  The interface is truncated or has an invalid group format.
**/
STATIC
BOOLEAN
DumpNodeInterface2 (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  CONST EFI_ACPI_AEST_2_0_INTERFACE_PREFIX  *Prefix;
  UINT32                                    BitmapBytes;
  UINT32                                    WordCount;
  UINT32                                    MaxRecords;

  if (Length < sizeof (EFI_ACPI_AEST_2_0_INTERFACE_PREFIX)) {
    IncrementErrorCount ();
    Print (L"ERROR: Truncated AEST 2.0+ interface prefix.\n");
    return FALSE;
  }

  Prefix = (CONST EFI_ACPI_AEST_2_0_INTERFACE_PREFIX *)Ptr;
  switch (Prefix->GroupFormat) {
    case EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB:
      WordCount  = EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB_BITMAP_WORD_COUNT;
      MaxRecords = EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB_MAX_ERROR_RECORDS;
      break;
    case EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB:
      WordCount  = EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB_BITMAP_WORD_COUNT;
      MaxRecords = EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB_MAX_ERROR_RECORDS;
      break;
    case EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB:
      WordCount  = EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB_BITMAP_WORD_COUNT;
      MaxRecords = EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB_MAX_ERROR_RECORDS;
      break;
    default:
      IncrementErrorCount ();
      Print (L"ERROR: Invalid AEST 2.0+ group format %u.\n", Prefix->GroupFormat);
      return FALSE;
  }

  BitmapBytes = ARRAY_SIZE (mAestBitmapNames) * WordCount * sizeof (UINT64);
  if ((BitmapBytes > Length - sizeof (*Prefix)) ||
      (sizeof (EFI_ACPI_AEST_2_0_INTERFACE_SUFFIX) >
       Length - sizeof (*Prefix) - BitmapBytes))
  {
    IncrementErrorCount ();
    Print (L"ERROR: Node interface extends beyond node length.\n");
    return FALSE;
  }

  ParseAcpi (
    TRUE,
    4,
    "Node Interface",
    Ptr,
    Length,
    PARSER_PARAMS (mAestNodeInterface2Prefix)
    );

  if ((Prefix->Type == EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SINGLE_MMIO) &&
      ((Prefix->GroupFormat != EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB) ||
       (Prefix->StartErrorRecordIndex != 0) ||
       (Prefix->NumberErrorRecords != 1)))
  {
    IncrementErrorCount ();
    Print (
      L"ERROR: Single-record MMIO requires group format 0, start index 0, "
      L"and one error record.\n"
      );
  }

  DumpAestBitmaps (
    Ptr + sizeof (*Prefix),
    Length - sizeof (*Prefix),
    WordCount,
    MaxRecords,
    Prefix->StartErrorRecordIndex,
    Prefix->NumberErrorRecords,
    Prefix->Type == EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SINGLE_MMIO
    );

  ParseAcpi (
    TRUE,
    4,
    NULL,
    Ptr + sizeof (*Prefix) + BitmapBytes,
    Length - sizeof (*Prefix) - BitmapBytes,
    PARSER_PARAMS (mAestNodeInterface2Suffix)
    );

  return TRUE;
}

/**
  Parses the Node Interface structure.

  @param [in] Ptr       Pointer to the start of the Node Interface Structure.
  @param [in] Length    Length of the buffer.
  @param [in] Revision  AEST table revision.

  @retval TRUE   The interface fits in the available length.
  @retval FALSE  The interface is truncated or has an invalid group format.
**/
STATIC
BOOLEAN
DumpNodeInterface (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN UINT8   Revision
  )
{
  if (Revision == EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION) {
    if (Length < sizeof (EFI_ACPI_AEST_INTERFACE_STRUCT)) {
      IncrementErrorCount ();
      Print (L"ERROR: Node interface extends beyond node length.\n");
      return FALSE;
    }

    ParseAcpi (
      TRUE,
      4,
      "Node Interface",
      Ptr,
      Length,
      PARSER_PARAMS (mAestNodeInterface)
      );
    return TRUE;
  }

  return DumpNodeInterface2 (Ptr, Length);
}

/**
  Parses the Node Interrupts Structure.

  @param [in] Ptr             Pointer to the start of the Node Interrupt array.
  @param [in] Length          Length of the buffer.
  @param [in] InterruptCount  Number of interrupts in the Node Interrupts array.
  @param [in] Revision        AEST table revision.

  @retval TRUE   All interrupt records fit in the buffer.
  @retval FALSE  The interrupt count exceeds the available buffer.
**/
STATIC
BOOLEAN
DumpNodeInterrupts (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN UINT32  InterruptCount,
  IN UINT8   Revision
  )
{
  UINT32  Index;
  UINT32  Offset;
  UINT32  RecordLength;
  CHAR8   Buffer[64];

  if (Revision == EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION) {
    RecordLength = sizeof (EFI_ACPI_AEST_INTERRUPT_STRUCT);
  } else {
    RecordLength = sizeof (EFI_ACPI_AEST_2_0_INTERRUPT);
  }

  if (InterruptCount > Length / RecordLength) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Node interrupt count %u exceeds space for %u entries.\n",
      InterruptCount,
      Length / RecordLength
      );
    return FALSE;
  }

  for (Index = 0; Index < InterruptCount; Index++) {
    Offset = Index * RecordLength;
    AsciiSPrint (Buffer, sizeof (Buffer), "Node Interrupt [%u]", Index);
    if (Revision == EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION) {
      ParseAcpi (
        TRUE,
        4,
        Buffer,
        Ptr + Offset,
        Length - Offset,
        PARSER_PARAMS (mAestNodeInterrupt)
        );
    } else {
      ParseAcpi (
        TRUE,
        4,
        Buffer,
        Ptr + Offset,
        Length - Offset,
        PARSER_PARAMS (mAestNodeInterrupt2)
        );
    }
  } // for

  return TRUE;
}

/**
  Parses a single AEST Node Structure.

  @param [in] Ptr       Pointer to the start of the Node.
  @param [in] Length    Length of the buffer.
  @param [in] Revision  AEST table revision.
**/
STATIC
VOID
DumpAestNodeStructure (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN UINT8   Revision
  )
{
  CONST EFI_ACPI_AEST_NODE_STRUCT  *Node;
  CONST CHAR8                      *NodeName;
  UINT32                           RemainingLength;
  UINT8                            *NodeDataPtr;

  if (Length < sizeof (EFI_ACPI_AEST_NODE_STRUCT)) {
    IncrementErrorCount ();
    Print (L"ERROR: Truncated AEST node header.\n");
    return;
  }

  Node = (CONST EFI_ACPI_AEST_NODE_STRUCT *)Ptr;

  NodeName = "Node Structure";
  if (Node->Type < ARRAY_SIZE (mAestNodeTypeNames)) {
    NodeName = mAestNodeTypeNames[Node->Type];
  }

  if (Revision == EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION) {
    ParseAcpi (
      TRUE,
      2,
      NodeName,
      Ptr,
      Length,
      PARSER_PARAMS (mAestNodeHeaderParser)
      );
  } else {
    ParseAcpi (
      TRUE,
      2,
      NodeName,
      Ptr,
      Length,
      PARSER_PARAMS (mAestNodeHeaderParser2)
      );
  }

  if ((Node->DataOffset < sizeof (EFI_ACPI_AEST_NODE_STRUCT)) ||
      (Node->DataOffset > Length))
  {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Node Data Offset: 0x%lx.\n"
      L"       It should be between 0x%lx and 0x%lx.\n",
      Node->DataOffset,
      sizeof (EFI_ACPI_AEST_NODE_STRUCT),
      Length
      );
    return;
  }

  if ((Node->InterfaceOffset < sizeof (EFI_ACPI_AEST_NODE_STRUCT)) ||
      (Node->InterfaceOffset > Length))
  {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Node Interface Offset: 0x%lx.\n"
      L"       It should be between 0x%lx and 0x%lx.\n",
      Node->InterfaceOffset,
      sizeof (EFI_ACPI_AEST_NODE_STRUCT),
      Length
      );
    return;
  }

  if ((Node->InterruptArrayCount != 0) &&
      ((Node->InterruptArrayOffset < sizeof (EFI_ACPI_AEST_NODE_STRUCT)) ||
       (Node->InterruptArrayOffset > Length)))
  {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Node Interrupt Array Offset: 0x%lx.\n"
      L"       It should be between 0x%lx and 0x%lx.\n",
      Node->InterruptArrayOffset,
      sizeof (EFI_ACPI_AEST_NODE_STRUCT),
      Length
      );
    return;
  }

  // Parse Node Data Field.
  NodeDataPtr     = Ptr + Node->DataOffset;
  RemainingLength = Length - Node->DataOffset;
  switch (Node->Type) {
    case EFI_ACPI_AEST_NODE_TYPE_PROCESSOR:
      DumpProcessorNode (NodeDataPtr, RemainingLength, Revision);
      break;
    case EFI_ACPI_AEST_NODE_TYPE_MEMORY:
      DumpMemoryControllerNode (NodeDataPtr, RemainingLength);
      break;
    case EFI_ACPI_AEST_NODE_TYPE_SMMU:
      DumpSmmuNode (NodeDataPtr, RemainingLength);
      break;
    case EFI_ACPI_AEST_NODE_TYPE_VENDOR_DEFINED:
      DumpVendorDefinedNode (NodeDataPtr, RemainingLength, Revision);
      break;
    case EFI_ACPI_AEST_NODE_TYPE_GIC:
      DumpGicNode (NodeDataPtr, RemainingLength);
      break;
    case EFI_ACPI_AEST_2_0_NODE_TYPE_PCIE:
      if (Revision == EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION) {
        IncrementErrorCount ();
        Print (L"ERROR: PCIe node type is reserved in AEST revision 1.\n");
        return;
      }

      DumpPcieNode (NodeDataPtr, RemainingLength);
      break;
    case EFI_ACPI_AEST_2_0_NODE_TYPE_PROXY:
      if (Revision == EFI_ACPI_ARM_ERROR_SOURCE_TABLE_REVISION) {
        IncrementErrorCount ();
        Print (L"ERROR: Proxy node type is reserved in AEST revision 1.\n");
        return;
      }

      DumpProxyNode (NodeDataPtr, RemainingLength);
      break;
    default:
      IncrementErrorCount ();
      Print (L"ERROR: Invalid AEST node type %u.\n", Node->Type);
      return;
  } // switch

  // Parse the Interface Field.
  RemainingLength = Length - Node->InterfaceOffset;
  if (!DumpNodeInterface (
         Ptr + Node->InterfaceOffset,
         RemainingLength,
         Revision
         ))
  {
    return;
  }

  // Parse the Node Interrupt Array.
  if (Node->InterruptArrayCount != 0) {
    RemainingLength = Length - Node->InterruptArrayOffset;
    DumpNodeInterrupts (
      Ptr + Node->InterruptArrayOffset,
      RemainingLength,
      Node->InterruptArrayCount,
      Revision
      );
  }
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
  CONST EFI_ACPI_AEST_NODE_STRUCT  *Node;
  UINT32                           Offset;
  UINT32                           RemainingLength;

  if (!Trace) {
    return;
  }

  if ((Ptr == NULL) || (AcpiTableLength < sizeof (EFI_ACPI_DESCRIPTION_HEADER))) {
    IncrementErrorCount ();
    Print (L"ERROR: AEST table does not contain a complete ACPI header.\n");
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "AEST",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (mAestParser)
             );

  while (Offset < AcpiTableLength) {
    RemainingLength = AcpiTableLength - Offset;

    // Check that the fixed node header is complete before reading its fields.
    if (RemainingLength < sizeof (*Node)) {
      IncrementErrorCount ();
      Print (L"ERROR: Truncated AEST node header at table offset %u.\n", Offset);
      return;
    }

    Node = (CONST EFI_ACPI_AEST_NODE_STRUCT *)(Ptr + Offset);

    // Validate AEST Node length
    if ((Node->Length < sizeof (*Node)) || (Node->Length > RemainingLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid AEST node length %u at table offset %u, %u bytes remain.\n",
        Node->Length,
        Offset,
        RemainingLength
        );
      return;
    }

    DumpAestNodeStructure (Ptr + Offset, Node->Length, AcpiTableRevision);
    Offset += Node->Length;
  } // while
}
