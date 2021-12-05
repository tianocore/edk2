/** @file
  SPCR table parser

  Copyright (c) 2016 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - Microsoft Serial Port Console Redirection Table
      Specification - Version 1.03 - August 10, 2015.
**/

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  This function validates the Interrupt Type.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateInterruptType (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  UINT8  InterruptType;

  InterruptType = *Ptr;

  if (InterruptType !=
      EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_GIC)
  {
    IncrementErrorCount ();
    Print (
      L"\nERROR: InterruptType = %d. This must be 8 on ARM Platforms",
      InterruptType
      );
  }

 #endif
}

/**
  This function validates the Irq.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateIrq (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  UINT8  Irq;

  Irq = *Ptr;

  if (Irq != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Irq = %d. This must be zero on ARM Platforms\n",
      Irq
      );
  }

 #endif
}

/**
  An ACPI_PARSER array describing the ACPI SPCR Table.
**/
STATIC CONST ACPI_PARSER  SpcrParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Interface Type",             1,   36, L"%d",       NULL,       NULL, NULL,                  NULL },
  { L"Reserved",                   3,   37, L"%x %x %x", Dump3Chars, NULL, NULL,                  NULL },
  { L"Base Address",               12,  40, NULL,        DumpGas,    NULL, NULL,                  NULL },
  { L"Interrupt Type",             1,   52, L"%d",       NULL,       NULL, ValidateInterruptType, NULL },
  { L"IRQ",                        1,   53, L"%d",       NULL,       NULL, ValidateIrq,           NULL },
  { L"Global System Interrupt",    4,   54, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"Baud Rate",                  1,   58, L"%d",       NULL,       NULL, NULL,                  NULL },
  { L"Parity",                     1,   59, L"%d",       NULL,       NULL, NULL,                  NULL },
  { L"Stop Bits",                  1,   60, L"%d",       NULL,       NULL, NULL,                  NULL },
  { L"Flow Control",               1,   61, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"Terminal Type",              1,   62, L"%d",       NULL,       NULL, NULL,                  NULL },
  { L"Reserved",                   1,   63, L"%x",       NULL,       NULL, NULL,                  NULL },

  { L"PCI Device ID",              2,   64, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"PCI Vendor ID",              2,   66, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"PCI Bus Number",             1,   68, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"PCI Device Number",          1,   69, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"PCI Function Number",        1,   70, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"PCI Flags",                  4,   71, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"PCI Segment",                1,   75, L"0x%x",     NULL,       NULL, NULL,                  NULL },
  { L"Reserved",                   4,   76, L"%x",       NULL,       NULL, NULL,                  NULL }
};

/**
  This function parses the ACPI SPCR table.
  When trace is enabled this function parses the SPCR table and
  traces the ACPI table fields.

  This function also performs validations of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSpcr (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  // Dump the SPCR
  ParseAcpi (
    TRUE,
    0,
    "SPCR",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (SpcrParser)
    );
}
