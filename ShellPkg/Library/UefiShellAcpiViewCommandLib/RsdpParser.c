/** @file
*
*  Copyright (c) 2016 - 2017, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiTableParser.h"

/// The RSDP table signature is "RSD PTR " (8 bytes)
/// However The signature for ACPI tables is 4 bytes.
/// To work around this oddity define a signature type
/// that allows us to process the log options.
#define RSDP_TABLE_INFO  SIGNATURE_32('R', 'S', 'D', 'P')

// Local Variables
STATIC CONST UINT64* XsdtAddress;
STATIC CONST UINT32* RsdpLength;


/** This function validates the RSDT Address.

  @params [in] Ptr     Pointer to the start of the field data.
  @params [in] Context Pointer to context specific information e.g. this
                       could be a pointer to the ACPI table header.
**/
STATIC
VOID
ValidateRsdtAddress (
  IN UINT8* Ptr,
  IN VOID*  Context
  );

/** This function validates the XSDT Address.

  @params [in] Ptr     Pointer to the start of the field data.
  @params [in] Context Pointer to context specific information e.g. this
                       could be a pointer to the ACPI table header.
**/
STATIC
VOID
ValidateXsdtAddress (
  IN UINT8* Ptr,
  IN VOID*  Context
  );

/** An array describing the ACPI RSDP Table.

**/
STATIC CONST ACPI_PARSER RsdpParser[] = {
  {L"Signature", 8, 0, NULL, Dump8Chars, NULL, NULL, NULL},
  {L"Checksum", 1, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Oem ID", 6, 9, NULL, Dump6Chars, NULL, NULL, NULL},
  {L"Revision", 1, 15, L"%d", NULL, NULL, NULL, NULL},
  {L"RSDT Address", 4, 16, L"0x%x", NULL, NULL, ValidateRsdtAddress, NULL},
  {L"Length", 4, 20, L"%d", NULL, (VOID**)&RsdpLength, NULL, NULL},
  {L"XSDT Address", 8, 24, L"0x%lx", NULL, (VOID**)&XsdtAddress,
   ValidateXsdtAddress, NULL},
  {L"Extended Checksum", 1, 32, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 3, 33, L"%x %x %x", Dump3Chars, NULL, NULL, NULL}
};

/** This function validates the RSDT Address.

  @params [in] Ptr     Pointer to the start of the field data.
  @params [in] Context Pointer to context specific information e.g. this
                       could be a pointer to the ACPI table header.
**/
STATIC
VOID
ValidateRsdtAddress (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  // Reference: Server Base Boot Requirements System Software on ARM Platforms
  // Section: 4.2.1.1 RSDP
  // Root System Description Pointer (RSDP), ACPI ? 5.2.5.
  //   - Within the RSDP, the RsdtAddress field must be null (zero) and the
  //     XsdtAddresss MUST be a valid, non-null, 64-bit value.
  UINT32 RsdtAddr;
  RsdtAddr = *(UINT32*)Ptr;
  if (0 != RsdtAddr) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Rsdt Address = 0x%p. This must be NULL on ARM Platforms.",
      RsdtAddr
      );
  }
#endif
}

/** This function validates the XSDT Address.

  @params [in] Ptr     Pointer to the start of the field data.
  @params [in] Context Pointer to context specific information e.g. this
                       could be a pointer to the ACPI table header.
**/
STATIC
VOID
ValidateXsdtAddress (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  // Reference: Server Base Boot Requirements System Software on ARM Platforms
  // Section: 4.2.1.1 RSDP
  // Root System Description Pointer (RSDP), ACPI ? 5.2.5.
  //   - Within the RSDP, the RsdtAddress field must be null (zero) and the
  //     XsdtAddresss MUST be a valid, non-null, 64-bit value.
  UINT64 XsdtAddr;
  XsdtAddr = *(UINT64*)Ptr;
  if (0 == XsdtAddr) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Xsdt Address = 0x%p. This must not be NULL on ARM Platforms.",
      XsdtAddr
      );
  }
#endif
}


/** This function parses the ACPI RSDP table.
  This function parses the RSDP table and optionally traces the ACPI
  table fields. ProcessTableReportOptions() is called to determine if
  the ACPI fields should be traced.

  This function invokes the parser for the XSDT table.
  * Note - This function does not support parsing of RSDT table.

  This function also performs a RAW dump of the ACPI table and
  validates the checksum.

  @params [in] Ptr                Pointer to the start of the buffer.

  @retval EFI_SUCCESS             Success.
  @retval EFI_NOT_FOUND           Valid XSDT pointer not found.
**/
EFI_STATUS
ParseRsdp (
  IN UINT8* Ptr
  )
{
  BOOLEAN  Trace;

  // The length is 4 bytes starting at offset 20
  ParseAcpi (FALSE, 0, "RSDP", Ptr, 24, PARSER_PARAMS (RsdpParser));

  Trace = ProcessTableReportOptions (RSDP_TABLE_INFO, Ptr, *RsdpLength);

  if (Trace) {
    DumpRaw (Ptr, *RsdpLength);
    VerifyChecksum (TRUE, Ptr, *RsdpLength);
  }

  ParseAcpi (Trace, 0, "RSDP", Ptr, *RsdpLength, PARSER_PARAMS (RsdpParser));

  // This code currently supports parsing of XSDT table only
  // and does not parse the RSDT table. Platforms provide the
  // RSDT to enable compatibility with ACPI 1.0 operating systems.
  // Therefore the RSDT should not be used on ARM platforms.
  if (0 == (*XsdtAddress)) {
    IncrementErrorCount ();
    Print (L"ERROR: XSDT Pointer is not set.\n");
    return EFI_NOT_FOUND;
  }

  ProcessAcpiTable ((UINT8*)(UINTN)(*XsdtAddress));
  return EFI_SUCCESS;
}
