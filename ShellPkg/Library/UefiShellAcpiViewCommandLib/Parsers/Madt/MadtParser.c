/** @file
  MADT table parser

  Copyright (c) 2016 - 2024, Arm Limited. All rights reserved.
  Copyright (c) 2022, AMD Incorporated. All rights reserved.
  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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
STATIC CONST UINT8                   *MadtInterruptControllerType;
STATIC CONST UINT8                   *MadtInterruptControllerLength;
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;
STATIC BOOLEAN                       mMadtHasRiscVAplic;

/**
  This function validates the System Vector Base in the GICD.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGICDSystemVectorBase (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*(UINT32 *)Ptr != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: System Vector Base must be zero."
      );
  }
}

/**
  This function validates the SPE Overflow Interrupt in the GICC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateSpeOverflowInterrupt (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT16  SpeOverflowInterrupt;

  SpeOverflowInterrupt = *(UINT16 *)Ptr;

  // SPE not supported by this processor
  if (SpeOverflowInterrupt == 0) {
    return;
  }

  if ((SpeOverflowInterrupt < ARM_PPI_ID_MIN) ||
      ((SpeOverflowInterrupt > ARM_PPI_ID_MAX) &&
       (SpeOverflowInterrupt < ARM_PPI_ID_EXTENDED_MIN)) ||
      (SpeOverflowInterrupt > ARM_PPI_ID_EXTENDED_MAX))
  {
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
    IncrementWarningCount ();
    Print (
      L"\nWARNING: SPE Overflow Interrupt ID of %d is not compliant with SBSA "
      L"Level 3 PPI ID assignment: %d.",
      SpeOverflowInterrupt,
      ARM_PPI_ID_PMBIRQ
      );
  }
}

/**
  This function validates the TRBE Interrupt in the GICC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateTrbeInterrupt (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT16  TrbeInterrupt;

  TrbeInterrupt = *(UINT16 *)Ptr;

  // SPE not supported by this processor
  if (TrbeInterrupt == 0) {
    return;
  }

  if ((TrbeInterrupt < ARM_PPI_ID_MIN) ||
      ((TrbeInterrupt > ARM_PPI_ID_MAX) &&
       (TrbeInterrupt < ARM_PPI_ID_EXTENDED_MIN)) ||
      (TrbeInterrupt > ARM_PPI_ID_EXTENDED_MAX))
  {
    IncrementErrorCount ();
    Print (
      L"\nERROR: TRBE Interrupt ID of %d is not in the allowed PPI ID "
      L"ranges of %d-%d or %d-%d (for GICv3.1 or later).",
      TrbeInterrupt,
      ARM_PPI_ID_MIN,
      ARM_PPI_ID_MAX,
      ARM_PPI_ID_EXTENDED_MIN,
      ARM_PPI_ID_EXTENDED_MAX
      );
  }
}

/**
  This function dumps the GICC Flags fields.
  Format string is 2 fields separated by a \0 mapping to 0 or > 0 of
  the buffer field bit.

  @param [in] Format Format string that is the list of strings to
                     map values to.
  @param [in] Ptr    Pointer to the start of the buffer.
  @param [in] Length Length of the field.
**/
STATIC
VOID
EFIAPI
DumpValue (
  IN CONST CHAR16  *Format,
  IN UINT8         *Ptr,
  IN UINT32        Length OPTIONAL
  )
{
  UINT32        Value;
  UINTN         Len;
  CONST CHAR16  *Format_Alt;

  Len        = StrLen (Format);
  Format_Alt = Format + Len + 1;
  Value      = *(UINT32 *)Ptr;

  Print (L"%s", Value ? Format : Format_Alt);
}

STATIC CONST ACPI_PARSER  GICCFlagParser[] = {
  { L"Enabled",                      1,  0, L"%d",                              NULL,      NULL, NULL, NULL },
  { L"Performance Inter. Mode",      1,  1, L"Level Triggered\0Edge Triggered", DumpValue, NULL, NULL, NULL },
  { L"VGIC Maintenance Inter. Mode", 1,  2, L"Level Triggered\0Edge Triggered", DumpValue, NULL, NULL, NULL },
  { L"Online Capable",               1,  3, L"%d",                              NULL,      NULL, NULL, NULL },
  { L"Reserved",                     28, 4, L"%d",                              NULL,      NULL, NULL, NULL }
};

/**
  This function dumps the GICC Flags fields.
  Format string is unused.

  @param [in] Format Unused
  @param [in] Ptr    Pointer to the start of the buffer.
  @param [in] Length Length of the field.
**/
STATIC
VOID
EFIAPI
DumpGicCFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length OPTIONAL
  )
{
  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (GICCFlagParser)
    );
}

/**
  An ACPI_PARSER array describing the GICC Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER  GicCParser[] = {
  { L"Type",                             1, 0,  L"0x%x",  NULL,          NULL, NULL, NULL },
  { L"Length",                           1, 1,  L"%d",    NULL,          NULL, NULL, NULL },
  { L"Reserved",                         2, 2,  L"0x%x",  NULL,          NULL, NULL, NULL },

  { L"CPU Interface Number",             4, 4,  L"0x%x",  NULL,          NULL, NULL, NULL },
  { L"ACPI Processor UID",               4, 8,  L"0x%x",  NULL,          NULL, NULL, NULL },
  { L"Flags",                            4, 12, NULL,     DumpGicCFlags, NULL, NULL, NULL },
  { L"Parking Protocol Version",         4, 16, L"0x%x",  NULL,          NULL, NULL, NULL },

  { L"Performance Interrupt GSIV",       4, 20, L"0x%x",  NULL,          NULL, NULL, NULL },
  { L"Parked Address",                   8, 24, L"0x%lx", NULL,          NULL, NULL, NULL },
  { L"Physical Base Address",            8, 32, L"0x%lx", NULL,          NULL, NULL, NULL },
  { L"GICV",                             8, 40, L"0x%lx", NULL,          NULL, NULL, NULL },
  { L"GICH",                             8, 48, L"0x%lx", NULL,          NULL, NULL, NULL },
  { L"VGIC Maintenance interrupt",       4, 56, L"0x%x",  NULL,          NULL, NULL, NULL },
  { L"GICR Base Address",                8, 60, L"0x%lx", NULL,          NULL, NULL, NULL },
  { L"MPIDR",                            8, 68, L"0x%lx", NULL,          NULL, NULL, NULL },
  { L"Processor Power Efficiency Class", 1, 76, L"0x%x",  NULL,          NULL, NULL, NULL },
  { L"Reserved",                         1, 77, L"0x%x",  NULL,          NULL, NULL, NULL },
  { L"SPE overflow Interrupt",           2, 78, L"0x%x",  NULL,          NULL,
    ValidateSpeOverflowInterrupt, NULL },
  { L"TRBE Interrupt",                   2, 80, L"0x%x",  NULL,          NULL,
    ValidateTrbeInterrupt, NULL }
};

/**
  An ACPI_PARSER array describing the GICD Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER  GicDParser[] = {
  { L"Type",                  1, 0,  L"0x%x",     NULL,       NULL, NULL, NULL },
  { L"Length",                1, 1,  L"%d",       NULL,       NULL, NULL, NULL },
  { L"Reserved",              2, 2,  L"0x%x",     NULL,       NULL, NULL, NULL },

  { L"GIC ID",                4, 4,  L"0x%x",     NULL,       NULL, NULL, NULL },
  { L"Physical Base Address", 8, 8,  L"0x%lx",    NULL,       NULL, NULL, NULL },
  { L"System Vector Base",    4, 16, L"0x%x",     NULL,       NULL,
    ValidateGICDSystemVectorBase, NULL },
  { L"GIC Version",           1, 20, L"%d",       NULL,       NULL, NULL, NULL },
  { L"Reserved",              3, 21, L"%x %x %x", Dump3Chars, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the MSI Frame Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER  GicMSIFrameParser[] = {
  { L"Type",                  1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",                1, 1,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Reserved",              2, 2,  L"0x%x",  NULL, NULL, NULL, NULL },

  { L"MSI Frame ID",          4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Physical Base Address", 8, 8,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Flags",                 4, 16, L"0x%x",  NULL, NULL, NULL, NULL },

  { L"SPI Count",             2, 20, L"%d",    NULL, NULL, NULL, NULL },
  { L"SPI Base",              2, 22, L"0x%x",  NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the GICR Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER  GicRParser[] = {
  { L"Type",                         1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",                       1, 1,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Reserved",                     2, 2,  L"0x%x",  NULL, NULL, NULL, NULL },

  { L"Discovery Range Base Address", 8, 4,  L"0x%lx", NULL, NULL, NULL,
    NULL },
  { L"Discovery Range Length",       4, 12, L"0x%x",  NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the GIC ITS Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER  GicITSParser[] = {
  { L"Type",                  1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",                1, 1,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Reserved",              2, 2,  L"0x%x",  NULL, NULL, NULL, NULL },

  { L"GIC ITS ID",            4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Physical Base Address", 8, 8,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Reserved",              4, 16, L"0x%x",  NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the IO APIC Structure.
**/
STATIC CONST ACPI_PARSER  IoApic[] = {
  { L"Type",                         1, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Length",                       1, 1, L"%d",   NULL, NULL, NULL, NULL },
  { L"I/O APIC ID",                  1, 2, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",                     1, 3, L"0x%x", NULL, NULL, NULL, NULL },
  { L"I/O APIC Address",             4, 4, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Global System Interrupt Base", 4, 8, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Interrupt Source Override Structure.
**/
STATIC CONST ACPI_PARSER  InterruptSourceOverride[] = {
  { L"Type",                    1, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Length",                  1, 1, L"%d",   NULL, NULL, NULL, NULL },
  { L"Bus",                     1, 2, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Source",                  1, 3, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Global System Interrupt", 4, 4, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Flags",                   2, 8, L"0x%x", NULL, NULL, NULL, NULL }
};

STATIC CONST ACPI_PARSER  LocalApicFlags[] = {
  { L"Enabled",        1,  0, L"%d", NULL, NULL, NULL, NULL },
  { L"Online Capable", 1,  1, L"%d", NULL, NULL, NULL, NULL },
  { L"Reserved",       30, 2, L"%d", NULL, NULL, NULL, NULL }
};

/**
  This function traces Bit Flags fields.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
VOID
EFIAPI
DumpLocalApicBitFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (LocalApicFlags)
    );
}

/**
   An ACPI_PARSER array describing the Processor Local APIC Structure.
 **/
STATIC CONST ACPI_PARSER  ProcessorLocalApic[] = {
  { L"Type",               1, 0, L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"Length",             1, 1, L"%d",   NULL,                  NULL, NULL, NULL },

  { L"ACPI Processor UID", 1, 2, L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"APIC ID",            1, 3, L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"Flags",              4, 4, NULL,    DumpLocalApicBitFlags, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Processor Local x2APIC Structure.
**/
STATIC CONST ACPI_PARSER  ProcessorLocalX2Apic[] = {
  { L"Type",               1, 0,  L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"Length",             1, 1,  L"%d",   NULL,                  NULL, NULL, NULL },
  { L"Reserved",           2, 2,  L"0x%x", NULL,                  NULL, NULL, NULL },

  { L"X2APIC ID",          4, 4,  L"0x%x", NULL,                  NULL, NULL, NULL },
  { L"Flags",              4, 8,  NULL,    DumpLocalApicBitFlags, NULL, NULL, NULL },
  { L"ACPI Processor UID", 4, 12, L"0x%x", NULL,                  NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Local x2APIC NMI Structure.
**/
STATIC CONST ACPI_PARSER  LocalX2ApicNmi[] = {
  { L"Type",               1, 0, L"0x%x",     NULL,       NULL, NULL, NULL },
  { L"Length",             1, 1, L"%d",       NULL,       NULL, NULL, NULL },
  { L"Flags",              2, 2, L"0x%x",     NULL,       NULL, NULL, NULL },

  { L"ACPI Processor UID", 4, 4, L"0x%x",     NULL,       NULL, NULL, NULL },
  { L"Local x2APIC LINT#", 1, 8, L"0x%x",     NULL,       NULL, NULL, NULL },
  { L"Reserved",           3, 9, L"0x%x%x%x", Dump3Chars, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Core Pragrammable Interrupt Controller (CORE PIC) Structure.
**/
STATIC CONST ACPI_PARSER  CorePic[] = {
  { L"Type",                  1, 0,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"Length",                1, 1,  L"%d",   NULL, NULL, NULL, NULL },
  { L"Version",               1, 2,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"ACPI Processor ID",     4, 3,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"Physical Processor ID", 4, 7,  L"0x%x", NULL, NULL, NULL, NULL },
  { L"Flags",                 4, 11, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Leagcy I/O Programmable Interrupt Controller (LIO PIC) Structure.
**/
STATIC CONST ACPI_PARSER  LegacyIoPic[] = {
  { L"Type",                   1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",                 1, 1,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Version",                1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Base Address",           8, 3,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Size",                   2, 11, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Cascade Vector",         2, 13, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Cascade vector mapping", 8, 15, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the HyperTransport Programmable Interrupt Controller (HT PIC) Structure.
**/
STATIC CONST ACPI_PARSER  HyperTransportPic[] = {
  { L"Type",           1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",         1, 1,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Version",        1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Base Address",   8, 3,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Size",           2, 11, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Cascade Vector", 8, 13, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Extend I/0 Programmable Interrupt Controller (EIO PIC) Structure.
**/
STATIC CONST ACPI_PARSER  ExtendIoPic[] = {
  { L"Type",           1, 0, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",         1, 1, L"%d",    NULL, NULL, NULL, NULL },
  { L"Version",        1, 2, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Cascade Vector", 1, 3, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Node",           1, 4, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Node Map",       8, 5, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the MSI Programmable Interrupt Controller (MSI PIC) Structure.
**/
STATIC CONST ACPI_PARSER  MsiPic[] = {
  { L"Type",            1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",          1, 1,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Version",         1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Message Address", 8, 3,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Start",           4, 11, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Count",           4, 15, L"0x%x",  NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Bridge I/O Programmable Interrupt Controller (BIO PIC) Structure.
**/
STATIC CONST ACPI_PARSER  BridgeIoPic[] = {
  { L"Type",         1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",       1, 1,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Version",      1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Base Address", 8, 3,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Size",         2, 11, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Hardware ID",  2, 13, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"GSI base",     2, 15, L"0x%x",  NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the LPC Programmable Interrupt Controller (LPC PIC) Structure.
**/
STATIC CONST ACPI_PARSER  LpcPic[] = {
  { L"Type",           1, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",         1, 1,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Version",        1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Base Address",   8, 3,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Size",           2, 11, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Cascade vector", 2, 13, L"0x%x",  NULL, NULL, NULL, NULL }
};

/**
  This function validates the RISC-V INTC Flags.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateIntcFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT32  Flags;

  Flags = *(UINT32 *)Ptr;
  if (((Flags & EFI_ACPI_6_6_RINTC_FLAG_ENABLE) != 0) &&
      ((Flags & EFI_ACPI_6_6_RINTC_FLAG_ONLINE_CAPABLE) != 0))
  {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V INTC Online Capable flag must be zero when "
      L"Enabled flag is set."
      );
  }
}

STATIC CONST ACPI_PARSER  RiscVIntcFlags[] = {
  { L"Enabled",        1,  0, L"%d", NULL, NULL, NULL,                 NULL },
  { L"Online Capable", 1,  1, L"%d", NULL, NULL, NULL,                 NULL },
  { L"Reserved",       30, 2, L"%d", NULL, NULL, ValidateReservedBits, NULL }
};

/**
  This function traces RISC-V INTC Flags fields.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpRiscVIntcFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (RiscVIntcFlags)
    );
}

/**
  An ACPI_PARSER array describing the RISC-V INTC Structure.
**/
STATIC CONST ACPI_PARSER  RiscVIntc[] = {
  { L"Type",                             1, 0,  L"0x%x",  NULL,               NULL, NULL, NULL },
  { L"Length",                           1, 1,  L"%d",    NULL,               NULL, NULL, NULL },
  { L"Version",                          1, 2,  L"0x%x",  NULL,               NULL, NULL, NULL },
  { L"Reserved",                         1, 3,  NULL,     DumpReserved,
    NULL, ValidateReserved, NULL },
  { L"Flags",                            4, 4,  NULL,     DumpRiscVIntcFlags,
    NULL, ValidateIntcFlags, NULL },
  { L"Hart ID",                          8, 8,  L"0x%lx", NULL,               NULL, NULL, NULL },
  { L"ACPI Processor UID",               4, 16, L"0x%x",  NULL,               NULL, NULL, NULL },
  { L"External Interrupt Controller ID", 4, 20, L"0x%x",  NULL,               NULL, NULL, NULL },
  { L"IMSIC Base Address",               8, 24, L"0x%lx", NULL,               NULL, NULL, NULL },
  { L"IMSIC Size",                       4, 32, L"0x%x",  NULL,               NULL, NULL, NULL }
};

/**
  This function validates the number of supervisor interrupt identities
  supported by the RISC-V IMSIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateImsicNumSupervisorIds (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT16  NumIds;

  NumIds = *(UINT16 *)Ptr;
  if ((NumIds < IMSIC_MIN_NUM_IDS) ||
      (NumIds > IMSIC_MAX_NUM_IDS) ||
      (((NumIds + 1) % 64) != 0))
  {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V IMSIC Number of Supervisor Interrupt Identities "
      L"must be between %d and %d, and one less than a multiple of 64. "
      L"Value = %d.",
      IMSIC_MIN_NUM_IDS,
      IMSIC_MAX_NUM_IDS,
      NumIds
      );
  }
}

/**
  This function validates the number of guest interrupt identities supported
  by the RISC-V IMSIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateImsicNumGuestIds (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT16  NumGuestIds;

  NumGuestIds = *(UINT16 *)Ptr;
  if ((NumGuestIds != 0) &&
      ((NumGuestIds < IMSIC_MIN_NUM_GUEST_IDS) ||
       (NumGuestIds > IMSIC_MAX_NUM_GUEST_IDS) ||
       (((NumGuestIds + 1) % 64) != 0)))
  {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V IMSIC Number of Guest Interrupt Identities "
      L"must be 0, or between %d and %d and one less than a multiple "
      L"of 64. Value = %d.",
      IMSIC_MIN_NUM_GUEST_IDS,
      IMSIC_MAX_NUM_GUEST_IDS,
      NumGuestIds
      );
  }
}

/**
  This function validates an 8-bit RISC-V IMSIC field against a min/max range.

  @param [in] NameStr   String describing the ACPI table field.
  @param [in] Value     Field value.
  @param [in] MinValue  Minimum valid value.
  @param [in] MaxValue  Maximum valid value.
**/
STATIC
VOID
EFIAPI
ValidateImsicUint8Range (
  IN CONST CHAR16  *NameStr,
  IN UINT8         Value,
  IN UINT8         MinValue,
  IN UINT8         MaxValue
  )
{
  if ((Value < MinValue) ||
      (Value > MaxValue))
  {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V IMSIC %s must be between %d and %d. Value = %d.",
      NameStr,
      MinValue,
      MaxValue,
      Value
      );
  }
}

/**
  This function validates the guest index bits supported by the RISC-V IMSIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateImsicGuestIndexBits (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  ValidateImsicUint8Range (
    L"Guest Index Bits",
    *Ptr,
    IMSIC_MIN_GUEST_INDEX_BITS,
    IMSIC_MAX_GUEST_INDEX_BITS
    );
}

/**
  This function validates the hart index bits supported by the RISC-V IMSIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateImsicHartIndexBits (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  ValidateImsicUint8Range (
    L"Hart Index Bits",
    *Ptr,
    IMSIC_MIN_HART_INDEX_BITS,
    IMSIC_MAX_HART_INDEX_BITS
    );
}

/**
  This function validates the group index bits supported by the RISC-V IMSIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateImsicGroupIndexBits (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  ValidateImsicUint8Range (
    L"Group Index Bits",
    *Ptr,
    IMSIC_MIN_GROUP_INDEX_BITS,
    IMSIC_MAX_GROUP_INDEX_BITS
    );
}

/**
  This function validates the group index shift supported by the RISC-V IMSIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateImsicGroupIndexShift (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8         GroupIndexShift;
  UINT8         MinValue;
  CONST CHAR16  *AplicMessage;

  GroupIndexShift = *Ptr;
  if (mMadtHasRiscVAplic) {
    MinValue     = IMSIC_MIN_GROUP_INDEX_SHIFT_WITH_APLIC;
    AplicMessage = L" when an APLIC structure is present";
  } else {
    MinValue     = IMSIC_MIN_GROUP_INDEX_SHIFT;
    AplicMessage = L"";
  }

  if ((GroupIndexShift < MinValue) ||
      (GroupIndexShift > IMSIC_MAX_GROUP_INDEX_SHIFT))
  {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V IMSIC Group Index Shift must be between %d "
      L"and %d%s. Value = %d.",
      MinValue,
      IMSIC_MAX_GROUP_INDEX_SHIFT,
      AplicMessage,
      GroupIndexShift
      );
  }
}

STATIC CONST ACPI_PARSER  RiscVImsicFlags[] = {
  { L"Reserved", 32, 0, L"%d", NULL, NULL, ValidateReservedBits, NULL }
};

/**
  This function traces RISC-V IMSIC Flags fields.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpRiscVImsicFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (RiscVImsicFlags)
    );
}

/**
  An ACPI_PARSER array describing the RISC-V IMSIC Structure.
**/
STATIC CONST ACPI_PARSER  RiscVImsic[] = {
  { L"Type",                            1, 0,  L"0x%x", NULL,                NULL, NULL, NULL },
  { L"Length",                          1, 1,  L"%d",   NULL,                NULL, NULL, NULL },
  { L"Version",                         1, 2,  L"0x%x", NULL,                NULL, NULL, NULL },
  { L"Reserved",                        1, 3,  NULL,    DumpReserved,
    NULL, ValidateReserved, NULL },
  { L"Flags",                           4, 4,  NULL,    DumpRiscVImsicFlags,
    NULL, NULL, NULL },
  { L"No. of Supervisor Interrupt Ids", 2, 8,  L"%d",   NULL,                NULL,
    ValidateImsicNumSupervisorIds, NULL },
  { L"No. of Guest Interrupt Ids",      2, 10, L"%d",   NULL,                NULL,
    ValidateImsicNumGuestIds, NULL },
  { L"Guest Index Bits",                1, 12, L"%d",   NULL,                NULL,
    ValidateImsicGuestIndexBits, NULL },
  { L"Hart Index Bits",                 1, 13, L"%d",   NULL,                NULL,
    ValidateImsicHartIndexBits, NULL },
  { L"Group Index Bits",                1, 14, L"%d",   NULL,                NULL,
    ValidateImsicGroupIndexBits, NULL },
  { L"Group Index Shift",               1, 15, L"%d",   NULL,                NULL,
    ValidateImsicGroupIndexShift, NULL }
};

/**
  This function validates a Hardware ID supported by a RISC-V interrupt
  controller.

  @param [in] Ptr            Pointer to the start of the field data.
  @param [in] ControllerName Name of the interrupt controller.
**/
STATIC
VOID
EFIAPI
ValidateRiscVHardwareId (
  IN UINT8         *Ptr,
  IN CONST CHAR16  *ControllerName
  )
{
  if (Ptr == NULL) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V %s Hardware ID pointer is NULL.",
      ControllerName
      );
    return;
  }

  if (!IsAcpiId ((CONST CHAR8 *)Ptr)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V %s Hardware ID must be a valid ACPI ID "
      L"in the form NNNN####, where N is an uppercase letter or a "
      L"digit ('0'-'9') and # is an uppercase hexadecimal digit. "
      L"Value = %c%c%c%c%c%c%c%c.",
      ControllerName,
      Ptr[0],
      Ptr[1],
      Ptr[2],
      Ptr[3],
      Ptr[4],
      Ptr[5],
      Ptr[6],
      Ptr[7]
      );
  }
}

/**
  This function validates the Hardware ID supported by the RISC-V APLIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRiscVAplicHardwareId (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  ValidateRiscVHardwareId (Ptr, L"APLIC");
}

/**
  This function validates the number of external interrupts supported by the
  RISC-V APLIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRiscVAplicNumSources (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT16  NumSources;

  NumSources = *(UINT16 *)Ptr;
  if ((NumSources == 0) || (NumSources > APLIC_MAX_EXT_INT_SOURCES)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V APLIC Total External Interrupt Sources Supported "
      L"must be between 1 and %d. Value = %d.",
      APLIC_MAX_EXT_INT_SOURCES,
      NumSources
      );
  }
}

STATIC CONST ACPI_PARSER  RiscVAplicFlags[] = {
  { L"Reserved", 32, 0, L"%d", NULL, NULL, ValidateReservedBits, NULL }
};

/**
  This function traces RISC-V APLIC Flags fields.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpRiscVAplicFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (RiscVAplicFlags)
    );
}

/**
  An ACPI_PARSER array describing the RISC-V APLIC Structure.
**/
STATIC CONST ACPI_PARSER  RiscVAplic[] = {
  { L"Type",                          1, 0,  L"0x%x",  NULL,                NULL, NULL, NULL },
  { L"Length",                        1, 1,  L"%d",    NULL,                NULL, NULL, NULL },
  { L"Version",                       1, 2,  L"0x%x",  NULL,                NULL, NULL, NULL },
  { L"APLIC ID",                      1, 3,  L"0x%x",  NULL,                NULL, NULL, NULL },
  { L"Flags",                         4, 4,  NULL,     DumpRiscVAplicFlags,
    NULL, NULL, NULL },
  { L"Hardware ID",                   8, 8,  NULL,     Dump8Chars,
    NULL, ValidateRiscVAplicHardwareId, NULL },
  { L"Number of IDCs",                2, 16, L"%d",    NULL,                NULL, NULL, NULL },
  { L"External Interrupts Supported", 2, 18, L"%d",    NULL,                NULL,
    ValidateRiscVAplicNumSources, NULL },
  { L"Global System Interrupt Base",  4, 20, L"0x%x",  NULL,                NULL, NULL, NULL },
  { L"APLIC Address",                 8, 24, L"0x%lx", NULL,                NULL, NULL, NULL },
  { L"APLIC Size",                    4, 32, L"0x%x",  NULL,                NULL, NULL, NULL }
};

/**
  This function validates the Hardware ID supported by the RISC-V PLIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRiscVPlicHardwareId (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  ValidateRiscVHardwareId (Ptr, L"PLIC");
}

/**
  This function validates the number of external interrupts supported by the
  RISC-V PLIC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRiscVPlicNumSources (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT16  NumSources;

  NumSources = *(UINT16 *)Ptr;
  if ((NumSources == 0) || (NumSources > PLIC_MAX_EXT_INT_SOURCES)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: RISC-V PLIC Total External Interrupt Sources Supported "
      L"must be between 1 and %d. Value = %d.",
      PLIC_MAX_EXT_INT_SOURCES,
      NumSources
      );
  }
}

STATIC CONST ACPI_PARSER  RiscVPlicFlags[] = {
  { L"Reserved", 32, 0, L"%d", NULL, NULL, ValidateReservedBits, NULL }
};

/**
  This function traces RISC-V PLIC Flags fields.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpRiscVPlicFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (RiscVPlicFlags)
    );
}

/**
  An ACPI_PARSER array describing the RISC-V PLIC Structure.
**/
STATIC CONST ACPI_PARSER  RiscVPlic[] = {
  { L"Type",                          1, 0,  L"0x%x",  NULL,               NULL, NULL, NULL },
  { L"Length",                        1, 1,  L"%d",    NULL,               NULL, NULL, NULL },
  { L"Version",                       1, 2,  L"0x%x",  NULL,               NULL, NULL, NULL },
  { L"PLIC ID",                       1, 3,  L"0x%x",  NULL,               NULL, NULL, NULL },
  { L"Hardware ID",                   8, 4,  NULL,     Dump8Chars,
    NULL, ValidateRiscVPlicHardwareId, NULL },
  { L"External Interrupts Supported", 2, 12, L"%d",    NULL,               NULL,
    ValidateRiscVPlicNumSources, NULL },
  { L"Max Priority",                  2, 14, L"%d",    NULL,               NULL, NULL, NULL },
  { L"Flags",                         4, 16, NULL,     DumpRiscVPlicFlags,
    NULL, NULL, NULL },
  { L"PLIC Size",                     4, 20, L"0x%x",  NULL,               NULL, NULL, NULL },
  { L"PLIC Address",                  8, 24, L"0x%lx", NULL,               NULL, NULL, NULL },
  { L"Global System Interrupt Base",  4, 32, L"0x%x",  NULL,               NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI MADT Table.
**/
STATIC CONST ACPI_PARSER  MadtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Local Interrupt Controller Address",4,         36, L"0x%x", NULL, NULL, NULL,
    NULL },
  { L"Flags",                      4,         40, L"0x%x", NULL, NULL, NULL,NULL}
};

/**
  An ACPI_PARSER array describing the MADT Interrupt Controller Structure Header Structure.
**/
STATIC CONST ACPI_PARSER  MadtInterruptControllerHeaderParser[] = {
  { NULL,        1, 0, NULL, NULL, (VOID **)&MadtInterruptControllerType,   NULL, NULL },
  { L"Length",   1, 1, NULL, NULL, (VOID **)&MadtInterruptControllerLength, NULL,
    NULL },
  { L"Reserved", 2, 2, NULL, NULL, NULL,                                    NULL, NULL }
};

/**
  Check whether a MADT interrupt controller structure type is present.

  @param [in] Ptr     Pointer to the start of the interrupt controller list.
  @param [in] Length  Length of the interrupt controller list.
  @param [in] Type    Interrupt controller structure type.

  @retval TRUE   The interrupt controller structure type is present.
  @retval FALSE  The interrupt controller structure type is not present.
**/
STATIC
BOOLEAN
MadtHasInterruptController (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN UINT8   Type
  )
{
  UINT32  Offset;
  UINT8   InterruptControllerLength;

  if (Ptr == NULL) {
    return FALSE;
  }

  Offset = 0;
  while (Offset < Length) {
    if ((Length - Offset) < 2) {
      return FALSE;
    }

    if (Ptr[Offset] == Type) {
      return TRUE;
    }

    InterruptControllerLength = Ptr[Offset + 1];
    if ((InterruptControllerLength == 0) ||
        (InterruptControllerLength > (Length - Offset)))
    {
      return FALSE;
    }

    Offset += InterruptControllerLength;
  }

  return FALSE;
}

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
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;
  UINT8   *InterruptContollerPtr;
  UINT32  GICDCount;

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
  mMadtHasRiscVAplic    = MadtHasInterruptController (
                            InterruptContollerPtr,
                            AcpiTableLength - Offset,
                            EFI_ACPI_6_6_APLIC
                            );

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
        (MadtInterruptControllerLength == NULL))
    {
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
        ((Offset + (*MadtInterruptControllerLength)) > AcpiTableLength))
    {
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
      case EFI_ACPI_6_3_GIC:
      {
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

      case EFI_ACPI_6_3_GICD:
      {
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

      case EFI_ACPI_6_3_GIC_MSI_FRAME:
      {
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

      case EFI_ACPI_6_3_GICR:
      {
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

      case EFI_ACPI_6_3_GIC_ITS:
      {
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

      case EFI_ACPI_6_3_IO_APIC:
      {
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

      case EFI_ACPI_6_3_INTERRUPT_SOURCE_OVERRIDE:
      {
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
      case EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "PROCESSOR LOCAL APIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (ProcessorLocalApic)
          );
        break;
      }
      case EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC:
      {
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

      case EFI_ACPI_6_3_LOCAL_X2APIC_NMI:
      {
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

      case EFI_ACPI_6_5_CORE_PIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "CORE PIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (CorePic)
          );
        break;
      }

      case EFI_ACPI_6_5_LIO_PIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "LIO PIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (LegacyIoPic)
          );
        break;
      }

      case EFI_ACPI_6_5_HT_PIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "HT PIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (HyperTransportPic)
          );
        break;
      }

      case EFI_ACPI_6_5_EIO_PIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "EIO PIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (ExtendIoPic)
          );
        break;
      }

      case EFI_ACPI_6_5_MSI_PIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "MSI PIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (MsiPic)
          );
        break;
      }

      case EFI_ACPI_6_5_BIO_PIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "BIO PIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (BridgeIoPic)
          );
        break;
      }

      case EFI_ACPI_6_5_LPC_PIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "LPC PIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (LpcPic)
          );
        break;
      }

      case EFI_ACPI_6_6_RINTC:
      {
        ParseAcpi (
          TRUE,
          2,
          "RISC-V INTC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (RiscVIntc)
          );
        break;
      }

      case EFI_ACPI_6_6_IMSIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "RISC-V IMSIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (RiscVImsic)
          );
        break;
      }

      case EFI_ACPI_6_6_APLIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "RISC-V APLIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (RiscVAplic)
          );
        break;
      }

      case EFI_ACPI_6_6_PLIC:
      {
        ParseAcpi (
          TRUE,
          2,
          "RISC-V PLIC",
          InterruptContollerPtr,
          *MadtInterruptControllerLength,
          PARSER_PARAMS (RiscVPlic)
          );
        break;
      }

      default:
      {
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
    Offset                += *MadtInterruptControllerLength;
  } // while
}
