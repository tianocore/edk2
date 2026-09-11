/** @file
  Locate ACPI tables by signature via a bootloader-supplied RSDP.

  UefiPayloadPkg has three separate places that walk the RSDP's
  XSDT/RSDT to find a table by signature: ChainloadApp (before it
  builds the payload's HOB list, and again after ExitBootServices()),
  AcpiGicPcdLib (from the RSDP handed over in the ACPI HOB), and
  UefiPayloadEntry/AcpiTable.c (deriving the ACPI board info HOB).
  Each carried its own bounds checking, and each was subtly stricter
  or laxer than the others.  This library is the single validated
  walk they now share.

  The library is BASE and depends only on BaseLib and DebugLib, so it
  links into a SEC-phase payload entry, a DXE_DRIVER NULL library and
  a UEFI_APPLICATION running under an outer firmware alike.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <IndustryStandard/Acpi.h>

#include <Library/AcpiTableWalkLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Return the first table with Signature in a validated system
  description table.

  @param[in] Sdt          The XSDT or RSDT header.
  @param[in] SdtSignature The signature Sdt must carry.
  @param[in] EntrySize    sizeof (UINT64) for XSDT, sizeof (UINT32) for RSDT.
  @param[in] Signature    The 4-byte table signature to find.

  @return  Pointer to the matching table header, or NULL.
**/
STATIC
EFI_ACPI_DESCRIPTION_HEADER *
FindInSdt (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Sdt,
  IN UINT32                       SdtSignature,
  IN UINTN                        EntrySize,
  IN UINT32                       Signature
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  *Tbl;
  UINT8                        *Entry;
  UINTN                        Count;
  UINTN                        Idx;
  UINTN                        Addr;

  //
  // Validate the SDT before deriving an entry count from its length.
  // A Length below the header size would wrap the unsigned subtraction
  // and walk the loop off the end of the table.
  //
  if ((Sdt->Signature != SdtSignature) ||
      (Sdt->Length < sizeof (EFI_ACPI_DESCRIPTION_HEADER)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: bad SDT at 0x%p: signature 0x%x, length %u\n",
      __func__,
      Sdt,
      Sdt->Signature,
      Sdt->Length
      ));
    return NULL;
  }

  Entry = (UINT8 *)(Sdt + 1);
  Count = (Sdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / EntrySize;

  for (Idx = 0; Idx < Count; Idx++) {
    if (EntrySize == sizeof (UINT64)) {
      //
      // The 36-byte common header leaves the 64-bit entry array
      // 4-byte aligned, so an aligned load may fault on a strict
      // architecture.
      //
      Addr = (UINTN)ReadUnaligned64 ((UINT64 *)Entry);
    } else {
      Addr = (UINTN)ReadUnaligned32 ((UINT32 *)Entry);
    }

    Entry += EntrySize;

    Tbl = (EFI_ACPI_DESCRIPTION_HEADER *)Addr;
    if ((Tbl != NULL) && (Tbl->Signature == Signature)) {
      return Tbl;
    }
  }

  return NULL;
}

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
  )
{
  EFI_ACPI_6_5_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rp;

  if (Rsdp == 0) {
    return NULL;
  }

  Rp = (EFI_ACPI_6_5_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)Rsdp;
  if (Rp->Signature != EFI_ACPI_6_5_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
    DEBUG ((
      DEBUG_WARN,
      "%a: RSDP at 0x%Lx has bad signature 0x%Lx\n",
      __func__,
      (UINT64)Rsdp,
      Rp->Signature
      ));
    return NULL;
  }

  //
  // ACPI 6.5 5.2.5.3: XsdtAddress is present only for Revision >= 2.
  // Prefer the XSDT when present; fall back to the RSDT otherwise.
  //
  if ((Rp->Revision >= 2) && (Rp->XsdtAddress != 0)) {
    return FindInSdt (
             (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rp->XsdtAddress,
             EFI_ACPI_6_5_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
             sizeof (UINT64),
             Signature
             );
  }

  if (Rp->RsdtAddress != 0) {
    return FindInSdt (
             (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rp->RsdtAddress,
             EFI_ACPI_6_5_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
             sizeof (UINT32),
             Signature
             );
  }

  return NULL;
}
