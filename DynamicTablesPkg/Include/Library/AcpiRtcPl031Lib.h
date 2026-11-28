/** @file
  Interfaces for generating an SSDT describing an Arm PL031 RTC.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/Acpi.h>

/**
  Build an SSDT describing an Arm PL031 RTC.

  The caller owns the returned table and must release it with FreePool().
  Installing the table is the caller's responsibility.

  The table describes one RTC with a 4 KiB MMIO region. BaseAddress must be
  aligned to 4 KiB. A base address of zero is accepted. The caller must supply
  an address appropriate for its platform.

  @param[in]  BaseAddress  MMIO base address of the PL031 RTC.
  @param[out] Table        Generated SSDT on success. Set to NULL on failure
                          when Table is a valid pointer.

  @retval EFI_SUCCESS            The SSDT was generated successfully.
  @retval EFI_INVALID_PARAMETER  Table is NULL or BaseAddress is not 4 KiB
                                 aligned.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval EFI_NOT_FOUND          An expected AML object was not found.
  @return Other errors reported during AML processing.
**/
EFI_STATUS
EFIAPI
BuildSsdtPl031Table (
  IN  UINT64                       BaseAddress,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **Table
  );
