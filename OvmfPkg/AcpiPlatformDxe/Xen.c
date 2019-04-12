/** @file
  OVMF ACPI Xen support

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2012, Bei Guan <gbtju85@gmail.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/ 

#include "AcpiPlatform.h"
#include <Library/HobLib.h>
#include <Guid/XenInfo.h>
#include <Library/BaseLib.h>

#define XEN_ACPI_PHYSICAL_ADDRESS         0x000EA020
#define XEN_BIOS_PHYSICAL_END             0x000FFFFF

EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *XenAcpiRsdpStructurePtr = NULL;

/**
  This function detects if OVMF is running on Xen.

**/
BOOLEAN
XenDetected (
  VOID
  )
{
  EFI_HOB_GUID_TYPE         *GuidHob;

  //
  // See if a XenInfo HOB is available
  //
  GuidHob = GetFirstGuidHob (&gEfiXenInfoGuid);
  if (GuidHob == NULL) {
    return FALSE;
  }

  return TRUE;
}

/**
  Get the address of Xen ACPI Root System Description Pointer (RSDP)
  structure.

  @param  RsdpStructurePtr   Return pointer to RSDP structure

  @return EFI_SUCCESS        Find Xen RSDP structure successfully.
  @return EFI_NOT_FOUND      Don't find Xen RSDP structure.
  @return EFI_ABORTED        Find Xen RSDP structure, but it's not integrated.

**/
EFI_STATUS
EFIAPI
GetXenAcpiRsdp (
  OUT   EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER   **RsdpPtr
  )
{
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER   *RsdpStructurePtr;
  UINT8                                          *XenAcpiPtr;
  UINT8                                          Sum;

  //
  // Detect the RSDP structure
  //
  for (XenAcpiPtr = (UINT8*)(UINTN) XEN_ACPI_PHYSICAL_ADDRESS;
       XenAcpiPtr < (UINT8*)(UINTN) XEN_BIOS_PHYSICAL_END;
       XenAcpiPtr += 0x10) {

    RsdpStructurePtr = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)
                         (UINTN) XenAcpiPtr;

    if (!AsciiStrnCmp ((CHAR8 *) &RsdpStructurePtr->Signature, "RSD PTR ", 8)) {
      //
      // RSDP ACPI 1.0 checksum for 1.0/2.0/3.0 table.
      // This is only the first 20 bytes of the structure
      //
      Sum = CalculateSum8 (
              (CONST UINT8 *)RsdpStructurePtr,
              sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER)
              );
      if (Sum != 0) {
        return EFI_ABORTED;
      }

      if (RsdpStructurePtr->Revision >= 2) {
        //
        // RSDP ACPI 2.0/3.0 checksum, this is the entire table
        //
        Sum = CalculateSum8 (
                (CONST UINT8 *)RsdpStructurePtr,
                sizeof (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER)
                );
        if (Sum != 0) {
          return EFI_ABORTED;
        }
      }
      *RsdpPtr = RsdpStructurePtr;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get Xen Acpi tables from the RSDP structure. And installs Xen ACPI tables
  into the RSDT/XSDT using InstallAcpiTable. Some signature of the installed
  ACPI tables are: FACP, APIC, HPET, WAET, SSDT, FACS, DSDT.

  @param  AcpiProtocol           Protocol instance pointer.

  @return EFI_SUCCESS            The table was successfully inserted.
  @return EFI_INVALID_PARAMETER  Either AcpiTableBuffer is NULL, TableHandle is
                                 NULL, or AcpiTableBufferSize and the size
                                 field embedded in the ACPI table pointed to
                                 by AcpiTableBuffer are not in sync.
  @return EFI_OUT_OF_RESOURCES   Insufficient resources exist to complete the request.

**/
EFI_STATUS
EFIAPI
InstallXenTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol
  )
{
  EFI_STATUS                                       Status;
  UINTN                                            TableHandle;

  EFI_ACPI_DESCRIPTION_HEADER                      *Rsdt;
  EFI_ACPI_DESCRIPTION_HEADER                      *Xsdt;
  VOID                                             *CurrentTableEntry;
  UINTN                                            CurrentTablePointer;
  EFI_ACPI_DESCRIPTION_HEADER                      *CurrentTable;
  UINTN                                            Index;
  UINTN                                            NumberOfTableEntries;
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE        *Fadt2Table;
  EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE        *Fadt1Table;
  EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE     *Facs2Table;
  EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE     *Facs1Table;
  EFI_ACPI_DESCRIPTION_HEADER                      *DsdtTable;

  Fadt2Table  = NULL;
  Fadt1Table  = NULL;
  Facs2Table  = NULL;
  Facs1Table  = NULL;
  DsdtTable   = NULL;
  TableHandle = 0;
  NumberOfTableEntries = 0;

  //
  // Try to find Xen ACPI tables
  //
  Status = GetXenAcpiRsdp (&XenAcpiRsdpStructurePtr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If XSDT table is find, just install its tables. 
  // Otherwise, try to find and install the RSDT tables.
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
      Status = InstallAcpiTable (
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
        Fadt2Table = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)
                       (UINTN) CurrentTablePointer;
        Facs2Table = (EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)
                       (UINTN) Fadt2Table->FirmwareCtrl;
        DsdtTable  = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Fadt2Table->Dsdt;
      }
    }
  }
  else if (XenAcpiRsdpStructurePtr->RsdtAddress) {
    //
    // Retrieve the addresses of RSDT and
    // calculate the number of its table entries.
    //
    Rsdt = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN)
             XenAcpiRsdpStructurePtr->RsdtAddress;
    NumberOfTableEntries = (Rsdt->Length -
                             sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / 
                             sizeof (UINT32);

    //
    // Install ACPI tables found in XSDT.
    //
    for (Index = 0; Index < NumberOfTableEntries; Index++) {
      //
      // Get the table entry from RSDT
      //
      CurrentTableEntry = (UINT32 *) ((UINT8 *) Rsdt +
                            sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
                            Index * sizeof (UINT32));
      CurrentTablePointer = *(UINT32 *)CurrentTableEntry;
      CurrentTable = (EFI_ACPI_DESCRIPTION_HEADER *) CurrentTablePointer;

      //
      // Install the RSDT tables
      //
      Status = InstallAcpiTable (
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
        Fadt1Table = (EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE *)
                       (UINTN) CurrentTablePointer;
        Facs1Table = (EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)
                       (UINTN) Fadt1Table->FirmwareCtrl;
        DsdtTable  = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Fadt1Table->Dsdt;
      }
    }
  }

  //
  // Install the FACS table.
  //
  if (Fadt2Table) {
    //
    // FACS 2.0
    //
    Status = InstallAcpiTable (
               AcpiProtocol,
               Facs2Table,
               Facs2Table->Length,
               &TableHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  else if (Fadt1Table) {
    //
    // FACS 1.0
    //
    Status = InstallAcpiTable (
               AcpiProtocol,
               Facs1Table,
               Facs1Table->Length,
               &TableHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
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

  Status = InstallAcpiTable (
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

