/** @file
  Install Acpi tables for Cloud Hypervisor

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Acpi63.h>
#include <Protocol/AcpiTable.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>

/**
   Find Acpi table Protocol and return it

   @return AcpiTable  Protocol, which is used to handle Acpi Table, on SUCCESS or NULL on FAILURE.

**/
STATIC
EFI_ACPI_TABLE_PROTOCOL *
FindAcpiTableProtocol (
  VOID
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  ASSERT_EFI_ERROR (Status);
  return AcpiTable;
}

/** Install Acpi tables for Cloud Hypervisor

  @param [in]  AcpiProtocol  Acpi Protocol which is used to install Acpi tables

  @return EFI_SUCCESS            The table was successfully inserted.
  @return EFI_INVALID_PARAMETER  Either AcpiProtocol, AcpiTablePtr or DsdtPtr is NULL
                                 and the size field embedded in the ACPI table pointed
                                 by AcpiTablePtr or DsdtPtr are not in sync.
  @return EFI_OUT_OF_RESOURCES   Insufficient resources exist to complete the request.
  @return EFI_NOT_FOUND          DSDT table not found.
**/
EFI_STATUS
EFIAPI
InstallCloudHvAcpiTables (
  IN     EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol
  )
{
  UINTN       InstalledKey;
  UINTN       TableSize;
  UINTN       AcpiTableLength;
  UINT64      RsdpPtr;
  UINT64      XsdtPtr;
  UINT64      TableOffset;
  UINT64      AcpiTablePtr;
  UINT64      *DsdtPtr;
  EFI_STATUS  Status;

  if (AcpiProtocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RsdpPtr         = PcdGet64 (PcdCloudHvAcpiRsdpBaseAddress);
  XsdtPtr         = ((EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER *)RsdpPtr)->XsdtAddress;
  AcpiTableLength = ((EFI_ACPI_COMMON_HEADER *)XsdtPtr)->Length;
  TableOffset     = sizeof (EFI_ACPI_DESCRIPTION_HEADER);
  DsdtPtr         = NULL;

  while (TableOffset < AcpiTableLength) {
    AcpiTablePtr = *(UINT64 *)(XsdtPtr + TableOffset);
    TableSize    = ((EFI_ACPI_COMMON_HEADER *)AcpiTablePtr)->Length;

    //
    // Install ACPI tables from XSDT
    //
    Status = AcpiProtocol->InstallAcpiTable (
                             AcpiProtocol,
                             (VOID *)AcpiTablePtr,
                             TableSize,
                             &InstalledKey
                             );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Get DSDT from FADT
    //
    if ((DsdtPtr == NULL) &&
        (EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE ==
         ((EFI_ACPI_COMMON_HEADER *)AcpiTablePtr)->Signature))
    {
      DsdtPtr = (UINT64 *)((EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE *)AcpiTablePtr)->XDsdt;
    }

    TableOffset += sizeof (UINT64);
  } // while

  if (DsdtPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: no DSDT found\n", __func__));
    return EFI_NOT_FOUND;
  }

  //
  // Install DSDT table
  //
  TableSize = ((EFI_ACPI_COMMON_HEADER *)DsdtPtr)->Length;
  Status    = AcpiProtocol->InstallAcpiTable (
                              AcpiProtocol,
                              DsdtPtr,
                              TableSize,
                              &InstalledKey
                              );

  return Status;
}

/** Entry point for Cloud Hypervisor Platform Dxe

  @param [in]  ImageHandle  Handle for this image.
  @param [in]  SystemTable  Pointer to the EFI system table.

  @return EFI_SUCCESS            The table was successfully inserted.
  @return EFI_INVALID_PARAMETER  Either AcpiProtocol, AcpiTablePtr or DsdtPtr is NULL
                                 and the size field embedded in the ACPI table pointed to
                                 by AcpiTablePtr or DsdtPtr are not in sync.
  @return EFI_OUT_OF_RESOURCES   Insufficient resources exist to complete the request.
  @return EFI_NOT_FOUND          DSDT table not found
**/
EFI_STATUS
EFIAPI
CloudHvAcpiPlatformEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = InstallCloudHvAcpiTables (FindAcpiTableProtocol ());

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to install Acpi table: %r\n",
      __func__,
      Status
      ));
    CpuDeadLoop ();
  }

  return EFI_SUCCESS;
}
