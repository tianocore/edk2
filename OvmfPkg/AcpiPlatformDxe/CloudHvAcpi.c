/** @file
  OVMF ACPI Cloud Hypervisor support

  Copyright (c) 2021, Intel Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/CloudHv.h> // CLOUDHV_RSDP_ADDRESS
#include <Library/BaseLib.h>          // CpuDeadLoop()
#include <Library/DebugLib.h>         // DEBUG()

#include "AcpiPlatform.h"

// Get the ACPI tables from EBDA start
EFI_STATUS
EFIAPI
InstallCloudHvTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol
  )
{
  EFI_STATUS  Status;
  UINTN       TableHandle;

  EFI_ACPI_DESCRIPTION_HEADER                *Xsdt;
  VOID                                       *CurrentTableEntry;
  UINTN                                      CurrentTablePointer;
  EFI_ACPI_DESCRIPTION_HEADER                *CurrentTable;
  UINTN                                      Index;
  UINTN                                      NumberOfTableEntries;
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE  *Fadt2Table;
  EFI_ACPI_DESCRIPTION_HEADER                *DsdtTable;

  Fadt2Table           = NULL;
  DsdtTable            = NULL;
  TableHandle          = 0;
  NumberOfTableEntries = 0;
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *AcpiRsdpStructurePtr = (VOID *)CLOUDHV_RSDP_ADDRESS;

  // If XSDT table is found, just install its tables.
  // Otherwise, try to find and install the RSDT tables.
  //
  if (AcpiRsdpStructurePtr->XsdtAddress) {
    //
    // Retrieve the addresses of XSDT and
    // calculate the number of its table entries.
    //
    Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)
           AcpiRsdpStructurePtr->XsdtAddress;
    NumberOfTableEntries = (Xsdt->Length -
                            sizeof (EFI_ACPI_DESCRIPTION_HEADER)) /
                           sizeof (UINT64);

    //
    // Install ACPI tables found in XSDT.
    //
    for (Index = 0; Index < NumberOfTableEntries; Index++) {
      //
      // Get the table entry from XSDT
      //
      CurrentTableEntry = (VOID *)((UINT8 *)Xsdt +
                                   sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
                                   Index * sizeof (UINT64));
      CurrentTablePointer = (UINTN)*(UINT64 *)CurrentTableEntry;
      CurrentTable        = (EFI_ACPI_DESCRIPTION_HEADER *)CurrentTablePointer;

      //
      // Install the XSDT tables
      //
      Status = AcpiProtocol->InstallAcpiTable (
                               AcpiProtocol,
                               CurrentTable,
                               CurrentTable->Length,
                               &TableHandle
                               );

      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      //
      // Get the X-DSDT table address from the table FADT
      //
      if (!AsciiStrnCmp ((CHAR8 *)&CurrentTable->Signature, "FACP", 4)) {
        Fadt2Table = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)
                     (UINTN)CurrentTablePointer;
        DsdtTable = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Fadt2Table->XDsdt;
      }
    }
  } else {
    return EFI_NOT_FOUND;
  }

  //
  // Install DSDT table. If we reached this point without finding the DSDT,
  // then we're out of sync with the hypervisor, and cannot continue.
  //
  if (DsdtTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: no DSDT found\n", __FUNCTION__));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           DsdtTable,
                           DsdtTable->Length,
                           &TableHandle
                           );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return EFI_SUCCESS;
}
