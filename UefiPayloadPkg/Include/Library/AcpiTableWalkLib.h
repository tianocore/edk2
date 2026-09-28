/** @file
  Locate ACPI tables by signature via a bootloader-supplied RSDP.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/Acpi.h>

/**
  Locate an ACPI table by signature via the RSDP.

  Walks the XSDT (or the RSDT if the RSDP is Revision 0 or has no
  XSDT address) referenced by Rsdp and returns the first table
  whose header signature matches Signature.

  The RSDP signature, the XSDT/RSDT signature and length, and each
  entry pointer are all validated before use.  A Length shorter than
  the common ACPI description header would underflow the entry-count
  subtraction and walk off the end of the table; that case is
  rejected.  XSDT entry pointers are read with ReadUnaligned64()
  because the 36-byte header leaves the 64-bit entry array 4-byte
  aligned.

  @param[in] Rsdp       Physical address of the ACPI RSDP, or 0.
  @param[in] Signature  4-byte ACPI table signature to find.

  @return  Pointer to the first matching table's description header,
           or NULL if Rsdp is 0, the RSDP or SDT header is invalid,
           or no table with Signature is present.
**/
EFI_ACPI_DESCRIPTION_HEADER *
EFIAPI
AcpiFindTableFromRsdp (
  IN EFI_PHYSICAL_ADDRESS  Rsdp,
  IN UINT32                Signature
  );
