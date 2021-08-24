/** @file
  MADT table parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
    - Arm Generic Interrupt Controller Architecture Specification,
      GIC architecture version 3 and version 4, issue E
    - Arm Server Base System Architecture 5.0
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewConfig.h"
#include "MadtParser.h"

// Local Variables
STATIC CONST UINT8* MadtInterruptControllerType;
STATIC CONST UINT8* MadtInterruptControllerLength;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  This function validates the System Vector Base in the GICD.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGICDSystemVectorBase (
  IN UINT8* Ptr,
  IN VOID*  Context
)
{
  if (*(UINT32*)Ptr != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: System Vector Base must be zero."
    );
  }
}

/**
  This function validates the SPE Overflow Interrupt in the GICC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateSpeOverflowInterrupt (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT16 SpeOverflowInterrupt;

  SpeOverflowInterrupt = *(UINT16*)Ptr;

  // SPE not supported by this processor
  if (SpeOverflowInterrupt == 0) {
    return;
  }

  if ((SpeOverflowInterrupt < ARM_PPI_ID_MIN) ||
      ((SpeOverflowInterrupt > ARM_PPI_ID_MAX) &&
       (SpeOverflowInterrupt < ARM_PPI_ID_EXTENDED_MIN)) ||
      (SpeOverflowInterrupt > ARM_PPI_ID_EXTENDED_MAX)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: SPE Overflow Interrupt ID of %d is not in the allowed PPI ID "
        L"ranges of %d-%d or %d-%d (for GICv3.1 or later).",
      SpeOverflowInterrupt,
      ARM_PPI_ID_MIN,
      ARM_PPI_ID_MAX,
      ARM_PPI_ID_EXTENDED_MIN,
      ARM_PPI_ID_EXTENDED_MAX
    );
  } else if (SpeOverflowInterrupt != ARM_PPI_ID_PMBIRQ) {
    IncrementWarningCount();
    Print (
      L"\nWARNING: SPE Overflow Interrupt ID of %d is not compliant with SBSA "
        L"Level 3 PPI ID assignment: %d.",
      SpeOverflowInterrupt,
      ARM_PPI_ID_PMBIRQ
    );
  }
}

/**
  An ACPI_PARSER array describing the GICC Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicCParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"CPU Interface Number", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"ACPI Processor UID", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Parking Protocol Version", 4, 16, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Performance Interrupt GSIV", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Parked Address", 8, 24, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Physical Base Address", 8, 32, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"GICV", 8, 40, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"GICH", 8, 48, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"VGIC Maintenance interrupt", 4, 56, L"0x%x", NULL, NULL, NULL, NULL},
  {L"GICR Base Address", 8, 60, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"MPIDR", 8, 68, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Processor Power Efficiency Class", 1, 76, L"0x%x", NULL, NULL, NULL,
   NULL},
  {L"Reserved", 1, 77, L"0x%x", NULL, NULL, NULL, NULL},
  {L"SPE overflow Interrupt", 2, 78, L"0x%x", NULL, NULL,
    ValidateSpeOverflowInterrupt, NULL}
};

/**
  An ACPI_PARSER array describing the GICD Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicDParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"GIC ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Physical Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"System Vector Base", 4, 16, L"0x%x", NULL, NULL,
    ValidateGICDSystemVectorBase, NULL},
  {L"GIC Version", 1, 20, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 3, 21, L"%x %x %x", Dump3Chars, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the MSI Frame Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicMSIFrameParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"MSI Frame ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Physical Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 16, L"0x%x", NULL, NULL, NULL, NULL},

  {L"SPI Count", 2, 20, L"%d", NULL, NULL, NULL, NULL},
  {L"SPI Base", 2, 22, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the GICR Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicRParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Discovery Range Base Address", 8, 4, L"0x%lx", NULL, NULL, NULL,
   NULL},
  {L"Discovery Range Length", 4, 12, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the GIC ITS Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicITSParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"GIC ITS ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Physical Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Reserved", 4, 16, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the IO APIC Structure.
**/
STATIC CONST ACPI_PARSER IoApic[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"I/O APIC ID", 1, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 1, 3, L"0x%x", NULL, NULL, NULL, NULL},
  {L"I/O APIC Address", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Global System Interrupt Base", 4, 8, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Interrupt Source Override Structure.
**/
STATIC CONST ACPI_PARSER InterruptSourceOverride[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Bus", 1, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Source", 1, 3, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Global System Interrupt", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 2, 8, L"0x%x", NULL, NULL, NULL, NULL}
};


/**
  An ACPI_PARSER array describing the Processor Local x2APIC Structure.
**/
STATIC CONST ACPI_PARSER ProcessorLocalX2Apic[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"X2APIC ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"ACPI Processor UID", 4, 12, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Local x2APIC NMI Structure.
**/
STATIC CONST ACPI_PARSER LocalX2ApicNmi[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Flags", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"ACPI Processor UID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Local x2APIC LINT#", 1, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 3, 9, L"0x%x%x%x", Dump3Chars, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the ACPI MADT Table.
**/
STATIC CONST ACPI_PARSER MadtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"Local Interrupt Controller Address", 4, 36, L"0x%x", NULL, NULL, NULL,
   NULL},
  {L"Flags", 4, 40, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the MADT Interrupt Controller Structure Header Structure.
**/
STATIC CONST ACPI_PARSER MadtInterruptControllerHeaderParser[] = {
  {NULL, 1, 0, NULL, NULL, (VOID**)&MadtInterruptControllerType, NULL, NULL},
  {L"Length", 1, 1, NULL, NULL, (VOID**)&MadtInterruptControllerLength, NULL,
   NULL},
  {L"Reserved", 2, 2, NULL, NULL, NULL, NULL, NULL}
};

/**
  This function parses the ACPI MADT table.
  When trace is enabled this function parses the MADT table and
  traces the ACPI table fields.

  This function currently parses the following Interrupt Controller
  Structures:
    - GICC
    - GICD
    - GIC MSI Frame
    - GICR
    - GIC ITS

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiMadt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT8* InterruptContollerPtr;
  UINT32 GICDCount;

  GICDCount = 0;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "MADT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (MadtParser)
             );
  InterruptContollerPtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    // Parse Interrupt Controller Structure to obtain Length.
    ParseAcpi (
      FALSE,
      0,
      NULL,
      InterruptContollerPtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (MadtInterruptControllerHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((MadtInterruptControllerType == NULL) ||
        (MadtInterruptControllerLength == NULL)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
          L"Interrupt Controller Structure header. Length = %d.\n",
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate Interrupt Controller Structure length
    if ((*MadtInterruptControllerLength == 0) ||
        ((Offset + (*MadtInterruptControllerLength)) > AcpiTableLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid Interrupt Controller Structure length. " \
          L"Length = %d. Offset = %d. AcpiTableLength = %d.\n",
        *MadtInterruptControllerLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    switch (*MadtInterruptControllerType) {
      case EFI_ACPI_6_3_GIC: {
        ParseAcpi (
          TRUE,
          2,
          "GICC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (GicCParser)
          );
        break;
      }

      case EFI_ACPI_6_3_GICD: {
        if (++GICDCount > 1) {
          IncrementErrorCount ();
          Print (
            L"ERROR: Only one GICD must be present,"
              L" GICDCount = %d\n",
            GICDCount
            );
        }
        ParseAcpi (
          TRUE,
          2,
          "GICD",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (GicDParser)
          );
        break;
      }

      case EFI_ACPI_6_3_GIC_MSI_FRAME: {
        ParseAcpi (
          TRUE,
          2,
          "GIC MSI Frame",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (GicMSIFrameParser)
          );
        break;
      }

      case EFI_ACPI_6_3_GICR: {
        ParseAcpi (
          TRUE,
          2,
          "GICR",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (GicRParser)
          );
        break;
      }

      case EFI_ACPI_6_3_GIC_ITS: {
        ParseAcpi (
          TRUE,
          2,
          "GIC ITS",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (GicITSParser)
          );
        break;
      }

      case EFI_ACPI_6_3_IO_APIC: {
        ParseAcpi (
          TRUE,
          2,
          "IO APIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (IoApic)
          );
        break;
      }

      case EFI_ACPI_6_3_INTERRUPT_SOURCE_OVERRIDE: {
        ParseAcpi (
          TRUE,
          2,
          "INTERRUPT SOURCE OVERRIDE",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (InterruptSourceOverride)
          );
        break;
      }

      case EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC: {
        ParseAcpi (
          TRUE,
          2,
          "PROCESSOR LOCAL X2APIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (ProcessorLocalX2Apic)
          );
        break;
      }

      case EFI_ACPI_6_3_LOCAL_X2APIC_NMI: {
        ParseAcpi (
          TRUE,
          2,
          "LOCAL x2APIC NMI",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (LocalX2ApicNmi)
          );
        break;
      }

      default: {
        IncrementErrorCount ();
        Print (
          L"ERROR: Unknown Interrupt Controller Structure,"
            L" Type = %d, Length = %d\n",
          *MadtInterruptControllerType,
          *MadtInterruptControllerLength
          );
      }
    } // switch

    InterruptContollerPtr += *MadtInterruptControllerLength;
    Offset += *MadtInterruptControllerLength;
  } // while
}
