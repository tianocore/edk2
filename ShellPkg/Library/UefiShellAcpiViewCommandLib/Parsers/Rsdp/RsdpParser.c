/** @file
  RSDP table parser

  Copyright (c) 2016 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
**/

#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local Variables
STATIC CONST UINT64  *XsdtAddress;

/**
  This function validates the RSDT Address.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRsdtAddress (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  // Reference: Server Base Boot Requirements System Software on ARM Platforms
  // Section: 4.2.1.1 RSDP
  // Root System Description Pointer (RSDP), ACPI ? 5.2.5.
  //   - Within the RSDP, the RsdtAddress field must be null (zero) and the
  //     XsdtAddresss MUST be a valid, non-null, 64-bit value.
  UINT32  RsdtAddr;

  RsdtAddr = *(UINT32 *)Ptr;

  if (RsdtAddr != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Rsdt Address = 0x%p. This must be NULL on ARM Platforms.",
      RsdtAddr
      );
  }

 #endif
}

/**
  This function validates the XSDT Address.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateXsdtAddress (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  // Reference: Server Base Boot Requirements System Software on ARM Platforms
  // Section: 4.2.1.1 RSDP
  // Root System Description Pointer (RSDP), ACPI ? 5.2.5.
  //   - Within the RSDP, the RsdtAddress field must be null (zero) and the
  //     XsdtAddresss MUST be a valid, non-null, 64-bit value.
  UINT64  XsdtAddr;

  XsdtAddr = *(UINT64 *)Ptr;

  if (XsdtAddr == 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Xsdt Address = 0x%p. This must not be NULL on ARM Platforms.",
      XsdtAddr
      );
  }

 #endif
}

/**
  An array describing the ACPI RSDP Table.
**/
STATIC CONST ACPI_PARSER  RsdpParser[] = {
  { L"Signature",         8, 0,  NULL,        Dump8Chars, NULL,                  NULL,                NULL },
  { L"Checksum",          1, 8,  L"0x%x",     NULL,       NULL,                  NULL,                NULL },
  { L"Oem ID",            6, 9,  NULL,        Dump6Chars, NULL,                  NULL,                NULL },
  { L"Revision",          1, 15, L"%d",       NULL,       NULL,                  NULL,                NULL },
  { L"RSDT Address",      4, 16, L"0x%x",     NULL,       NULL,                  ValidateRsdtAddress, NULL },
  { L"Length",            4, 20, L"%d",       NULL,       NULL,                  NULL,                NULL },
  { L"XSDT Address",      8, 24, L"0x%lx",    NULL,       (VOID **)&XsdtAddress,
    ValidateXsdtAddress, NULL },
  { L"Extended Checksum", 1, 32, L"0x%x",     NULL,       NULL,                  NULL,                NULL },
  { L"Reserved",          3, 33, L"%x %x %x", Dump3Chars, NULL,                  NULL,                NULL }
};

/**
  This function parses the ACPI RSDP table.

  This function invokes the parser for the XSDT table.
  * Note - This function does not support parsing of RSDT table.

  This function also performs a RAW dump of the ACPI table and
  validates the checksum.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiRsdp (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  if (Trace) {
    DumpRaw (Ptr, AcpiTableLength);
    VerifyChecksum (TRUE, Ptr, AcpiTableLength);
  }

  ParseAcpi (
    Trace,
    0,
    "RSDP",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (RsdpParser)
    );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if (XsdtAddress == NULL) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient table length. AcpiTableLength = %d." \
      L"RSDP parsing aborted.\n",
      AcpiTableLength
      );
    return;
  }

  // This code currently supports parsing of XSDT table only
  // and does not parse the RSDT table. Platforms provide the
  // RSDT to enable compatibility with ACPI 1.0 operating systems.
  // Therefore the RSDT should not be used on ARM platforms.
  if ((*XsdtAddress) == 0) {
    IncrementErrorCount ();
    Print (L"ERROR: XSDT Pointer is not set. RSDP parsing aborted.\n");
    return;
  }

  ProcessAcpiTable ((UINT8 *)(UINTN)(*XsdtAddress));
}
