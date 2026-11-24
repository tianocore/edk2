/** @file
  Generate an SSDT describing an Arm PL031 RTC.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm PrimeCell Real Time Clock (PL031) Technical Reference Manual,
    ARM DDI 0224.
    https://support.arm.com/documentation/ddi0224/latest
**/

#include <Library/AcpiRtcPl031Lib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/MemoryAllocationLib.h>

#include "AcpiRtcPl031LibInternal.h"

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  ssdtpl031template_aml_code[];

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
  )
{
  EFI_STATUS                   Status;
  EFI_STATUS                   CleanupStatus;
  AML_ROOT_NODE_HANDLE         RootNode;
  AML_NODE_HANDLE              BaseAddressNode;
  EFI_ACPI_DESCRIPTION_HEADER  *NewTable;

  if (Table == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  // Require the PL031 base address to be 4 KiB aligned.
  if ((BaseAddress & (PL031_RTC_REGION_SIZE - 1)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  RootNode = NULL;
  NewTable = NULL;

  Status = AmlParseDefinitionBlock (
             (CONST EFI_ACPI_DESCRIPTION_HEADER *)ssdtpl031template_aml_code,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlFindNode (
             RootNode,
             "\\_SB_.RTC0.RBAS",
             &BaseAddressNode
             );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Status = AmlNameOpUpdateInteger (
             (AML_OBJECT_NODE_HANDLE)BaseAddressNode,
             BaseAddress
             );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Status = AmlSerializeDefinitionBlock (
             RootNode,
             &NewTable
             );

Cleanup:
  CleanupStatus = AmlDeleteTree (RootNode);
  if (!EFI_ERROR (Status)) {
    Status = CleanupStatus;
  }

  if (EFI_ERROR (Status)) {
    if (NewTable != NULL) {
      FreePool (NewTable);
    }

    return Status;
  }

  *Table = NewTable;
  return EFI_SUCCESS;
}
