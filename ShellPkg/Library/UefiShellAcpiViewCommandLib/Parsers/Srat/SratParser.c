/** @file
  SRAT table parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
**/

#include <IndustryStandard/Acpi.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewConfig.h"

// Local Variables
STATIC CONST UINT8                   *SratRAType;
STATIC CONST UINT8                   *SratRALength;
STATIC CONST UINT8                   *SratDeviceHandleType;
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  This function validates the Reserved field in the SRAT table header.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateSratReserved (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
  if (*(UINT32 *)Ptr != 1) {
    IncrementErrorCount ();
    Print (L"\nERROR: Reserved should be 1 for backward compatibility.\n");
  }
}

/**
  This function validates the Device Handle Type field in the Generic Initiator
  Affinity Structure.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateSratDeviceHandleType (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
  UINT8  DeviceHandleType;

  DeviceHandleType = *Ptr;

  if (DeviceHandleType > EFI_ACPI_6_3_PCI_DEVICE_HANDLE) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Invalid Device Handle Type: %d. Must be between 0 and %d.",
      DeviceHandleType,
      EFI_ACPI_6_3_PCI_DEVICE_HANDLE
      );
  }
}

/**
  This function traces the PCI BDF Number field inside Device Handle - PCI

  @param [in] Format  Format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
STATIC
VOID
EFIAPI
DumpSratPciBdfNumber (
  IN CONST CHAR16  *Format,
  IN UINT8         *Ptr
  )
{
  CHAR16  Buffer[OUTPUT_FIELD_COLUMN_WIDTH];

  Print (L"\n");

  /*
    The PCI BDF Number subfields are printed in the order specified in the ACPI
    specification. The format of the 16-bit PCI BDF Number field is as follows:

    +-----+------+------+
    |DEV  | FUNC | BUS  |
    +-----+------+------+
    |15:11| 10:8 |  7:0 |
    +-----+------+------+
  */

  // Print PCI Bus Number (Bits 7:0 of Byte 2)
  UnicodeSPrint (
    Buffer,
    sizeof (Buffer),
    L"PCI Bus Number"
    );
  PrintFieldName (4, Buffer);
  Print (
    L"0x%x\n",
    *Ptr
    );

  Ptr++;

  // Print PCI Device Number (Bits 7:3 of Byte 3)
  UnicodeSPrint (
    Buffer,
    sizeof (Buffer),
    L"PCI Device Number"
    );
  PrintFieldName (4, Buffer);
  Print (
    L"0x%x\n",
    (*Ptr & (BIT7 | BIT6 | BIT5 | BIT4 | BIT3)) >> 3
    );

  // PCI Function Number (Bits 2:0 of Byte 3)
  UnicodeSPrint (
    Buffer,
    sizeof (Buffer),
    L"PCI Function Number"
    );
  PrintFieldName (4, Buffer);
  Print (
    L"0x%x\n",
    *Ptr & (BIT2 | BIT1 | BIT0)
    );
}

/**
  An ACPI_PARSER array describing the Device Handle - ACPI
**/
STATIC CONST ACPI_PARSER  SratDeviceHandleAcpiParser[] = {
  { L"ACPI_HID", 8, 0,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"ACPI_UID", 4, 8,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved", 4, 12, L"0x%x",  NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Device Handle - PCI
**/
STATIC CONST ACPI_PARSER  SratDeviceHandlePciParser[] = {
  { L"PCI Segment",    2,  0, L"0x%x",                                    NULL,                 NULL, NULL, NULL },
  { L"PCI BDF Number", 2,  2, NULL,                                       DumpSratPciBdfNumber, NULL, NULL, NULL },
  { L"Reserved",       12, 4, L"%x %x %x %x - %x %x %x %x - %x %x %x %x", Dump12Chars,
    NULL, NULL, NULL }
};

/**
  This function traces the Device Handle field inside Generic Initiator
  Affinity Structure.

  @param [in] Format  Format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
STATIC
VOID
EFIAPI
DumpSratDeviceHandle (
  IN CONST CHAR16  *Format,
  IN UINT8         *Ptr
  )
{
  if (SratDeviceHandleType == NULL) {
    IncrementErrorCount ();
    Print (L"\nERROR: Device Handle Type read incorrectly.\n");
    return;
  }

  Print (L"\n");

  if (*SratDeviceHandleType == EFI_ACPI_6_3_ACPI_DEVICE_HANDLE) {
    ParseAcpi (
      TRUE,
      2,
      NULL,
      Ptr,
      sizeof (EFI_ACPI_6_3_DEVICE_HANDLE_ACPI),
      PARSER_PARAMS (SratDeviceHandleAcpiParser)
      );
  } else if (*SratDeviceHandleType == EFI_ACPI_6_3_PCI_DEVICE_HANDLE) {
    ParseAcpi (
      TRUE,
      2,
      NULL,
      Ptr,
      sizeof (EFI_ACPI_6_3_DEVICE_HANDLE_PCI),
      PARSER_PARAMS (SratDeviceHandlePciParser)
      );
  }
}

/**
  This function traces the APIC Proximity Domain field.

  @param [in] Format  Format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
STATIC
VOID
EFIAPI
DumpSratApicProximity (
  IN CONST CHAR16  *Format,
  IN UINT8         *Ptr
  )
{
  UINT32  ProximityDomain;

  ProximityDomain = Ptr[0] | (Ptr[1] << 8) | (Ptr[2] << 16);

  Print (Format, ProximityDomain);
}

/**
  An ACPI_PARSER array describing the SRAT Table.
**/
STATIC CONST ACPI_PARSER  SratParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Reserved",                   4,  36, L"0x%x",  NULL, NULL, ValidateSratReserved, NULL },
  { L"Reserved",                   8,  40, L"0x%lx", NULL, NULL, NULL,                 NULL }
};

/**
  An ACPI_PARSER array describing the Resource Allocation structure header.
**/
STATIC CONST ACPI_PARSER  SratResourceAllocationParser[] = {
  { L"Type",   1, 0, NULL, NULL, (VOID **)&SratRAType,   NULL, NULL },
  { L"Length", 1, 1, NULL, NULL, (VOID **)&SratRALength, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the GICC Affinity structure.
**/
STATIC CONST ACPI_PARSER  SratGicCAffinityParser[] = {
  { L"Type",               1, 0,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"Length",             1, 1,  L"0x%x", NULL, NULL, NULL, NULL },

  { L"Proximity Domain",   4, 2,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"ACPI Processor UID", 4, 6,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"Flags",              4, 10, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Clock Domain",       4, 14, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the GIC ITS Affinity structure.
**/
STATIC CONST ACPI_PARSER  SratGicITSAffinityParser[] = {
  { L"Type",             1, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Length",           1, 1, L"0x%x", NULL, NULL, NULL, NULL },

  { L"Proximity Domain", 4, 2, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",         2, 6, L"0x%x", NULL, NULL, NULL, NULL },
  { L"ITS Id",           4, 8, L"0x%x", NULL, NULL, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the Generic Initiator Affinity Structure
**/
STATIC CONST ACPI_PARSER  SratGenericInitiatorAffinityParser[] = {
  { L"Type",               1,  0,  L"0x%x", NULL,                 NULL,                           NULL, NULL },
  { L"Length",             1,  1,  L"0x%x", NULL,                 NULL,                           NULL, NULL },

  { L"Reserved",           1,  2,  L"0x%x", NULL,                 NULL,                           NULL, NULL },
  { L"Device Handle Type", 1,  3,  L"%d",   NULL,                 (VOID **)&SratDeviceHandleType,
    ValidateSratDeviceHandleType, NULL },
  { L"Proximity Domain",   4,  4,  L"0x%x", NULL,                 NULL,                           NULL, NULL },
  { L"Device Handle",      16, 8,  L"%s",   DumpSratDeviceHandle, NULL,                           NULL, NULL },
  { L"Flags",              4,  24, L"0x%x", NULL,                 NULL,                           NULL, NULL },
  { L"Reserved",           4,  28, L"0x%x", NULL,                 NULL,                           NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Memory Affinity structure.
**/
STATIC CONST ACPI_PARSER  SratMemAffinityParser[] = {
  { L"Type",              1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",            1, 1,  L"0x%x",  NULL, NULL, NULL, NULL },

  { L"Proximity Domain",  4, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",          2, 6,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Base Address Low",  4, 8,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Base Address High", 4, 12, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length Low",        4, 16, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length High",       4, 20, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",          4, 24, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Flags",             4, 28, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",          8, 32, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the APIC/SAPIC Affinity structure.
**/
STATIC CONST ACPI_PARSER  SratApciSapicAffinityParser[] = {
  { L"Type",                    1, 0,  L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"Length",                  1, 1,  L"0x%x", NULL,                  NULL, NULL, NULL },

  { L"Proximity Domain [7:0]",  1, 2,  L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"APIC ID",                 1, 3,  L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"Flags",                   4, 4,  L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"Local SAPIC EID",         1, 8,  L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"Proximity Domain [31:8]", 3, 9,  L"0x%x", DumpSratApicProximity,
    NULL, NULL, NULL },
  { L"Clock Domain",            4, 12, L"0x%x", NULL,                  NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Processor Local x2APIC Affinity structure.
**/
STATIC CONST ACPI_PARSER  SratX2ApciAffinityParser[] = {
  { L"Type",             1, 0,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"Length",           1, 1,  L"0x%x", NULL, NULL, NULL, NULL },

  { L"Reserved",         2, 2,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"Proximity Domain", 4, 4,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"X2APIC ID",        4, 8,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"Flags",            4, 12, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Clock Domain",     4, 16, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",         4, 20, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  This function parses the ACPI SRAT table.
  When trace is enabled this function parses the SRAT table and
  traces the ACPI table fields.

  This function parses the following Resource Allocation Structures:
    - Processor Local APIC/SAPIC Affinity Structure
    - Memory Affinity Structure
    - Processor Local x2APIC Affinity Structure
    - GICC Affinity Structure

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSrat (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;
  UINT8   *ResourcePtr;
  UINT32  GicCAffinityIndex;
  UINT32  GicITSAffinityIndex;
  UINT32  GenericInitiatorAffinityIndex;
  UINT32  MemoryAffinityIndex;
  UINT32  ApicSapicAffinityIndex;
  UINT32  X2ApicAffinityIndex;
  CHAR8   Buffer[80]; // Used for AsciiName param of ParseAcpi

  GicCAffinityIndex             = 0;
  GicITSAffinityIndex           = 0;
  GenericInitiatorAffinityIndex = 0;
  MemoryAffinityIndex           = 0;
  ApicSapicAffinityIndex        = 0;
  X2ApicAffinityIndex           = 0;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "SRAT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (SratParser)
             );

  ResourcePtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    ParseAcpi (
      FALSE,
      0,
      NULL,
      ResourcePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (SratResourceAllocationParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((SratRAType == NULL) ||
        (SratRALength == NULL))
    {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
        L"Static Resource Allocation structure header. Length = %d.\n",
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate Static Resource Allocation Structure length
    if ((*SratRALength == 0) ||
        ((Offset + (*SratRALength)) > AcpiTableLength))
    {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid Static Resource Allocation Structure length. " \
        L"Length = %d. Offset = %d. AcpiTableLength = %d.\n",
        *SratRALength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    switch (*SratRAType) {
      case EFI_ACPI_6_3_GICC_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "GICC Affinity Structure [%d]",
          GicCAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratGicCAffinityParser)
          );
        break;

      case EFI_ACPI_6_3_GIC_ITS_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "GIC ITS Affinity Structure [%d]",
          GicITSAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratGicITSAffinityParser)
          );
        break;

      case EFI_ACPI_6_3_GENERIC_INITIATOR_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "Generic Initiator Affinity Structure [%d]",
          GenericInitiatorAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratGenericInitiatorAffinityParser)
          );
        break;

      case EFI_ACPI_6_3_MEMORY_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "Memory Affinity Structure [%d]",
          MemoryAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratMemAffinityParser)
          );
        break;

      case EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "APIC/SAPIC Affinity Structure [%d]",
          ApicSapicAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratApciSapicAffinityParser)
          );
        break;

      case EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC_AFFINITY:
        AsciiSPrint (
          Buffer,
          sizeof (Buffer),
          "X2APIC Affinity Structure [%d]",
          X2ApicAffinityIndex++
          );
        ParseAcpi (
          TRUE,
          2,
          Buffer,
          ResourcePtr,
          *SratRALength,
          PARSER_PARAMS (SratX2ApciAffinityParser)
          );
        break;

      default:
        IncrementErrorCount ();
        Print (L"ERROR: Unknown SRAT Affinity type = 0x%x\n", *SratRAType);
        break;
    }

    ResourcePtr += (*SratRALength);
    Offset      += (*SratRALength);
  }
}
