/** @file
  Xen ARM ACPI Platform Driver using Xen ARM multiboot protocol

  Copyright (C) 2016, Linaro Ltd. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FdtClient.h>

#include <IndustryStandard/Acpi.h>

/**
  Get the address of Xen ACPI Root System Description Pointer (RSDP)
  structure.

  @param  RsdpStructurePtr   Return pointer to RSDP structure

  @return EFI_SUCCESS        Find Xen RSDP structure successfully.
  @return EFI_NOT_FOUND      Don't find Xen RSDP structure.
  @return EFI_ABORTED        Find Xen RSDP structure, but it's not integrated.

**/
STATIC
EFI_STATUS
EFIAPI
GetXenArmAcpiRsdp (
  OUT   EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER   **RsdpPtr
  )
{
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER   *RsdpStructurePtr;
  EFI_STATUS                                     Status;
  FDT_CLIENT_PROTOCOL                            *FdtClient;
  CONST UINT64                                   *Reg;
  UINT32                                         RegSize;
  UINTN                                          AddressCells, SizeCells;
  UINT64                                         RegBase;
  UINT8                                          Sum;

  RsdpStructurePtr = NULL;
  FdtClient = NULL;
  //
  // Get the RSDP structure address from DeviceTree
  //
  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeReg (FdtClient, "xen,guest-acpi",
                        (CONST VOID **)&Reg, &AddressCells, &SizeCells,
                        &RegSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_WARN, "%a: No 'xen,guest-acpi' compatible DT node found\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  ASSERT (AddressCells == 2);
  ASSERT (SizeCells == 2);
  ASSERT (RegSize == 2 * sizeof (UINT64));

  RegBase = SwapBytes64(Reg[0]);
  RsdpStructurePtr =
    (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)RegBase;

  if (RsdpStructurePtr && RsdpStructurePtr->Revision >= 2) {
    Sum = CalculateSum8 ((CONST UINT8 *)RsdpStructurePtr,
            sizeof (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER));
    if (Sum != 0) {
      return EFI_ABORTED;
    }
  }

  *RsdpPtr = RsdpStructurePtr;
  return EFI_SUCCESS;
}

/**
  Get Xen Acpi tables from the RSDP structure. And installs Xen ACPI tables
  into the RSDT/XSDT using InstallAcpiTable. Some signature of the installed
  ACPI tables are: FACP, APIC, GTDT, DSDT.

  @param  AcpiProtocol           Protocol instance pointer.

  @return EFI_SUCCESS            The table was successfully inserted.
  @return EFI_INVALID_PARAMETER  Either AcpiTableBuffer is NULL, TableHandle is
                                 NULL, or AcpiTableBufferSize and the size
                                 field embedded in the ACPI table pointed to
                                 by AcpiTableBuffer are not in sync.
  @return EFI_OUT_OF_RESOURCES   Insufficient resources exist to complete the request.

**/
STATIC
EFI_STATUS
EFIAPI
InstallXenArmTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol
  )
{
  EFI_STATUS                                       Status;
  UINTN                                            TableHandle;
  VOID                                             *CurrentTableEntry;
  UINTN                                            CurrentTablePointer;
  EFI_ACPI_DESCRIPTION_HEADER                      *CurrentTable;
  UINTN                                            Index;
  UINTN                                            NumberOfTableEntries;
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER     *XenAcpiRsdpStructurePtr;
  EFI_ACPI_DESCRIPTION_HEADER                      *Xsdt;
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE        *FadtTable;
  EFI_ACPI_DESCRIPTION_HEADER                      *DsdtTable;

  XenAcpiRsdpStructurePtr = NULL;
  FadtTable   = NULL;
  DsdtTable   = NULL;
  TableHandle = 0;
  NumberOfTableEntries = 0;

  //
  // Try to find Xen ARM ACPI tables
  //
  Status = GetXenArmAcpiRsdp (&XenAcpiRsdpStructurePtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "%a: No RSDP table found\n", __FUNCTION__));
    return Status;
  }

  //
  // If XSDT table is find, just install its tables.
  //
  if (XenAcpiRsdpStructurePtr->XsdtAddress) {
    //
    // Retrieve the addresses of XSDT and
    // calculate the number of its table entries.
    //
    Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN)
             XenAcpiRsdpStructurePtr->XsdtAddress;
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
      CurrentTableEntry = (VOID *) ((UINT8 *) Xsdt +
                            sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
                            Index * sizeof (UINT64));
      CurrentTablePointer = (UINTN) *(UINT64 *)CurrentTableEntry;
      CurrentTable = (EFI_ACPI_DESCRIPTION_HEADER *) CurrentTablePointer;

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
        return Status;
      }

      //
      // Get the FACS and DSDT table address from the table FADT
      //
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "FACP", 4)) {
        FadtTable = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)
                      (UINTN) CurrentTablePointer;
        DsdtTable  = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) FadtTable->Dsdt;
      }
    }
  }

  //
  // Install DSDT table.
  //
  Status = AcpiProtocol->InstallAcpiTable (
             AcpiProtocol,
             DsdtTable,
             DsdtTable->Length,
             &TableHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_ACPI_TABLE_PROTOCOL *
FindAcpiTableProtocol (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_TABLE_PROTOCOL *AcpiTable;

  AcpiTable = NULL;
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID**)&AcpiTable
                  );
  ASSERT_EFI_ERROR (Status);
  return AcpiTable;
}

/**
  Entrypoint of Xen ARM Acpi Platform driver.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

**/

EFI_STATUS
EFIAPI
XenAcpiPlatformEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                         Status;

  Status = InstallXenArmTables (FindAcpiTableProtocol ());
  return Status;
}
