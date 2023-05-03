/** @file
  Copyright (c) 2023, Corvin Köhne <corvink@FreeBSD.org>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

/**
  Searches and returns the address of the ACPI Root System Description Pointer (RSDP) in system memory.

  @param  StartAddress        Start address of search range.
  @param  EndAddress          End address of search range.
  @param  RsdpPtr             Return pointer to RSDP.

  @retval EFI_SUCCESS         RSDP successfully found.
  @retval EFI_NOT_FOUND       Couldn't find RSDP.
  @retval EFI_ABORTED         Invalid RSDP found.
**/
EFI_STATUS
EFIAPI
GetAcpiRsdpFromMemory (
  IN UINT64                                         StartAddress,
  IN UINT64                                         EndAddress,
  OUT EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  **RsdpPtr
  );
