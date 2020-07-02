/** @file
  PCCT table parser

  Copyright (c) 2020, Arm Limited.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
**/

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"
#include "PcctParser.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

STATIC UINT8* PccSubspaceLength;
STATIC UINT8* PccSubspaceType;

/**
  This function validates the length coded on 4 bytes of a shared memory range

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRangeLength4 (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  if (*(UINT32*)Ptr < MIN_EXT_PCC_SUBSPACE_MEM_RANGE_LEN) {
    IncrementErrorCount ();
    Print (
      L"\nError: Shared memory range length is too short.\n"
      L"Length of: %u when it should be over or equal to %u",
      *(UINT32*)Ptr,
      MIN_EXT_PCC_SUBSPACE_MEM_RANGE_LEN
      );
  }
}

/**
  This function validates the length coded on 8 bytes of a shared memory range

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRangeLength8 (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  if (*(UINT64*)Ptr <= MIN_MEMORY_RANGE_LENGTH) {
    IncrementErrorCount ();
    Print (
      L"\nError: Shared memory range length is too short.\n"
      L"Length of: %u when it should be over %u",
      *(UINT64*)Ptr,
      MIN_MEMORY_RANGE_LENGTH
      );
  }
}

/**
  This function validates address space for type 0 structure.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidatePccType0Gas (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  switch (*(UINT8*)Ptr) {
#if !(defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64))
    case EFI_ACPI_6_3_SYSTEM_IO:
#endif //if not (defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64))
    case EFI_ACPI_6_3_SYSTEM_MEMORY:
      return;
    default:
      IncrementErrorCount ();
      Print (L"\nError: Invalid address space");
  }
}

/**
  This function validates address space for structures of types other than 0.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidatePccGas (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  switch (*(UINT8*)Ptr) {
#if !(defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64))
    case EFI_ACPI_6_3_SYSTEM_IO:
#endif //if not (defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64))
    case EFI_ACPI_6_3_FUNCTIONAL_FIXED_HARDWARE:
    case EFI_ACPI_6_3_SYSTEM_MEMORY:
      return;
    default:
      IncrementErrorCount ();
      Print (L"\nError: Invalid address space");
  }
}

/**
  An ACPI_PARSER array describing the ACPI PCCT Table.
*/
STATIC CONST ACPI_PARSER PcctParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"Flags", 4, 36, NULL, NULL, NULL, NULL, NULL},
  {L"Reserved", 8, 40, NULL, NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the platform communications channel subspace
  structure header.
*/
STATIC CONST ACPI_PARSER PccSubspaceHeaderParser[] = {
  PCC_SUBSPACE_HEADER ()
  // ... Type Specific Fields ...
};

/**
  An ACPI_PARSER array describing the Generic Communications Subspace - Type 0
*/
STATIC CONST ACPI_PARSER PccSubspaceType0Parser[] = {
  PCC_SUBSPACE_HEADER (),
  {L"Reserved", 6, 2, L"0x%x%x%x%x%x%x", Dump6Chars, NULL, NULL, NULL},
  {L"Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Memory Range Length", 8, 16, L"0x%lx", NULL, NULL, ValidateRangeLength8, NULL},
  {L"Doorbell Register", 12, 24, NULL, DumpGas, NULL, ValidatePccType0Gas, NULL},
  {L"Doorbell Preserve", 8, 36, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Doorbell Write", 8, 44, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Nominal Latency", 4, 52, L"%u", NULL, NULL, NULL, NULL},
  {L"Maximum Periodic Access Rate", 4, 56, L"%u", NULL, NULL, NULL, NULL},
  {L"Minimum Request Turnaround Time", 2, 60, L"%u", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the HW-Reduced Communications Subspace
  - Type 1
*/
STATIC CONST ACPI_PARSER PccSubspaceType1Parser[] = {
  PCC_SUBSPACE_HEADER (),
  {L"Platform Interrupt", 4, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Platform Interrupt Flags", 1, 6, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 1, 7, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Memory Range Length", 8, 16, L"0x%lx", NULL, NULL, ValidateRangeLength8, NULL},
  {L"Doorbell Register", 12, 24, NULL, DumpGas, NULL,
    ValidatePccGas, NULL},
  {L"Doorbell Preserve", 8, 36, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Doorbell Write", 8, 44, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Nominal Latency", 4, 52, L"%u", NULL, NULL, NULL, NULL},
  {L"Maximum Periodic Access Rate", 4, 56, L"%u", NULL, NULL, NULL, NULL},
  {L"Minimum Request Turnaround Time", 2, 60, L"%u", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the HW-Reduced Communications Subspace
  - Type 2
*/
STATIC CONST ACPI_PARSER PccSubspaceType2Parser[] = {
  PCC_SUBSPACE_HEADER (),
  {L"Platform Interrupt", 4, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Platform Interrupt Flags", 1, 6, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 1, 7, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Memory Range Length", 8, 16, L"0x%lx", NULL, NULL, ValidateRangeLength8, NULL},
  {L"Doorbell Register", 12, 24, NULL, DumpGas, NULL,
    ValidatePccGas, NULL},
  {L"Doorbell Preserve", 8, 36, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Doorbell Write", 8, 44, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Nominal Latency", 4, 52, L"%u", NULL, NULL, NULL, NULL},
  {L"Maximum Periodic Access Rate", 4, 56, L"%u", NULL, NULL, NULL, NULL},
  {L"Minimum Request Turnaround Time", 2, 60, L"%u", NULL, NULL, NULL, NULL},
  {L"Platform Interrupt Ack Register", 12, 62, NULL, DumpGas, NULL,
    ValidatePccGas, NULL},
  {L"Platform Interrupt Ack Preserve", 8, 74, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Platform Interrupt Ack Write", 8, 82, L"0x%lx", NULL, NULL,
    NULL, NULL},
};

/**
  An ACPI_PARSER array describing the Extended PCC Subspaces - Type 3/4
*/
STATIC CONST ACPI_PARSER PccSubspaceType3Parser[] = {
  PCC_SUBSPACE_HEADER (),
  {L"Platform Interrupt", 4, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Platform Interrupt Flags", 1, 6, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 1, 7, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Memory Range Length", 4, 16, L"0x%x", NULL, NULL, ValidateRangeLength4, NULL},
  {L"Doorbell Register", 12, 20, NULL, DumpGas, NULL,
    ValidatePccGas, NULL},
  {L"Doorbell Preserve", 8, 32, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Doorbell Write", 8, 40, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Nominal Latency", 4, 48, L"%u", NULL, NULL, NULL, NULL},
  {L"Maximum Periodic Access Rate", 4, 52, L"%u", NULL, NULL, NULL, NULL},
  {L"Minimum Request Turnaround Time", 4, 56, L"%u", NULL, NULL, NULL, NULL},
  {L"Platform Interrupt Ack Register", 12, 60, NULL, DumpGas, NULL,
    ValidatePccGas, NULL},
  {L"Platform Interrupt Ack Preserve", 8, 72, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Platform Interrupt Ack Set", 8, 80, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Reserved", 8, 88, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Cmd Complete Check Reg Addr", 12, 96, NULL, DumpGas, NULL,
    ValidatePccGas, NULL},
  {L"Cmd Complete Check Mask", 8, 108, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Cmd Update Reg Addr", 12, 116, NULL, DumpGas, NULL,
    ValidatePccGas, NULL},
  {L"Cmd Update Preserve mask", 8, 128, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Cmd Update Set mask", 8, 136, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Error Status Register", 12, 144, NULL, DumpGas, NULL,
    ValidatePccGas, NULL},
  {L"Error Status Mask", 8, 156, L"0x%lx", NULL, NULL, NULL, NULL},
};

/**
  This function parses the PCC Subspace type 0.

  @param [in] Ptr     Pointer to the start of Subspace Structure.
  @param [in] Length  Length of the Subspace Structure.
**/
STATIC
VOID
DumpPccSubspaceType0 (
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Subspace Type 0",
    Ptr,
    Length,
    PARSER_PARAMS (PccSubspaceType0Parser)
    );
}

/**
  This function parses the PCC Subspace type 1.

  @param [in] Ptr     Pointer to the start of the Subspace Structure.
  @param [in] Length  Length of the Subspace Structure.
**/
STATIC
VOID
DumpPccSubspaceType1 (
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Subspace Type 1",
    Ptr,
    Length,
    PARSER_PARAMS (PccSubspaceType1Parser)
    );
}

/**
  This function parses the PCC Subspace type 2.

  @param [in] Ptr     Pointer to the start of the Subspace Structure.
  @param [in] Length  Length of the Subspace Structure.
**/
STATIC
VOID
DumpPccSubspaceType2 (
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Subspace Type 2",
    Ptr,
    Length,
    PARSER_PARAMS (PccSubspaceType2Parser)
    );
}

/**
  This function parses the PCC Subspace type 3.

  @param [in] Ptr     Pointer to the start of the Subspace Structure.
  @param [in] Length  Length of the Subspace Structure.
**/
STATIC
VOID
DumpPccSubspaceType3 (
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Subspace Type 3",
    Ptr,
    Length,
    PARSER_PARAMS (PccSubspaceType3Parser)
    );
}

/**
  This function parses the PCC Subspace type 4.

  @param [in] Ptr     Pointer to the start of the Subspace Structure.
  @param [in] Length  Length of the Subspace Structure.
**/
STATIC
VOID
DumpPccSubspaceType4 (
  IN UINT8* Ptr,
  IN UINT8  Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "Subspace Type 4",
    Ptr,
    Length,
    PARSER_PARAMS (PccSubspaceType3Parser)
    );
}

/**
  This function parses the ACPI PCCT table including its sub-structures of type
  0 through 4.
  When trace is enabled this function parses the PCCT table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiPcct (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT8* PccSubspacePtr;
  UINT32 SubspaceCount;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "PCCT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (PcctParser)
             );

  PccSubspacePtr = Ptr + Offset;

  SubspaceCount = 0;
  while (Offset < AcpiTableLength) {
    // Parse common structure header to obtain Type and Length.
    ParseAcpi (
      FALSE,
      0,
      NULL,
      PccSubspacePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (PccSubspaceHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((PccSubspaceType == NULL) ||
        (PccSubspaceLength == NULL)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
          L"structure header. Length = %u.\n",
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate Structure length
    if ((*PccSubspaceLength == 0) ||
        ((Offset + (*PccSubspaceLength)) > AcpiTableLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid Structure length. " \
          L"Length = %u. Offset = %u. AcpiTableLength = %u.\n",
        *PccSubspaceLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    switch (*PccSubspaceType) {
      case EFI_ACPI_6_3_PCCT_SUBSPACE_TYPE_GENERIC:
        DumpPccSubspaceType0 (
          PccSubspacePtr,
          *PccSubspaceLength
          );
        break;
      case EFI_ACPI_6_3_PCCT_SUBSPACE_TYPE_1_HW_REDUCED_COMMUNICATIONS:
        DumpPccSubspaceType1 (
          PccSubspacePtr,
          *PccSubspaceLength
          );
        break;
      case EFI_ACPI_6_3_PCCT_SUBSPACE_TYPE_2_HW_REDUCED_COMMUNICATIONS:
        DumpPccSubspaceType2 (
          PccSubspacePtr,
          *PccSubspaceLength
          );
        break;
      case EFI_ACPI_6_3_PCCT_SUBSPACE_TYPE_3_EXTENDED_PCC:
        DumpPccSubspaceType3 (
          PccSubspacePtr,
          *PccSubspaceLength
          );
        break;
      case EFI_ACPI_6_3_PCCT_SUBSPACE_TYPE_4_EXTENDED_PCC:
        DumpPccSubspaceType4 (
          PccSubspacePtr,
          *PccSubspaceLength
          );
        break;
      default:
        IncrementErrorCount ();
        Print (
          L"ERROR: Unknown processor topology structure:"
            L" Type = %u, Length = %u\n",
          PccSubspaceType,
          *PccSubspaceLength
          );
    }

    PccSubspacePtr += *PccSubspaceLength;
    Offset += *PccSubspaceLength;
    SubspaceCount++;
  } // while
  if (SubspaceCount > MAX_PCC_SUBSPACES) {
    IncrementErrorCount ();
    Print (L"ERROR: Too many subspaces.");
  }
}
