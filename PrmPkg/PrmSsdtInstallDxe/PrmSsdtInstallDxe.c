/** @file

  This file contains a sample implementation of the Platform Runtime Mechanism (PRM)
  SSDT Install library.

  Copyright (c) Microsoft Corporation
  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

#define _DBGMSGID_  "[PRMSSDTINSTALL]"

/**
  Installs the PRM SSDT.

  @param[in]  OemId                       OEM ID to be used in the SSDT installation.

  @retval EFI_SUCCESS                     The PRM SSDT was installed successfully.
  @retval EFI_INVALID_PARAMETER           The OemId pointer argument is NULL.
  @retval EFI_NOT_FOUND                   An instance of gEfiAcpiTableProtocolGuid was not found installed or
                                          the SSDT (AML RAW section) could not be found in the current FV.
  @retval EFI_OUT_OF_RESOURCES            Insufficient memory resources to install the PRM SSDT.

**/
EFI_STATUS
InstallPrmSsdt (
  IN  CONST UINT8  *OemId
  )
{
  EFI_STATUS                   Status;
  UINTN                        SsdtSize;
  UINTN                        TableKey;
  EFI_ACPI_TABLE_PROTOCOL      *AcpiTableProtocol;
  EFI_ACPI_DESCRIPTION_HEADER  *Ssdt;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __func__));

  if (OemId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTableProtocol);
  if (!EFI_ERROR (Status)) {
    //
    // Discover the SSDT
    //
    Status =  GetSectionFromFv (
                &gEfiCallerIdGuid,
                EFI_SECTION_RAW,
                0,
                (VOID **)&Ssdt,
                &SsdtSize
                );
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_INFO, "%a %a: SSDT loaded...\n", _DBGMSGID_, __func__));

    //
    // Update OEM ID in the SSDT
    //
    CopyMem (&Ssdt->OemId, OemId, sizeof (Ssdt->OemId));

    //
    // Publish the SSDT. Table is re-checksummed.
    //
    TableKey = 0;
    Status   = AcpiTableProtocol->InstallAcpiTable (
                                    AcpiTableProtocol,
                                    Ssdt,
                                    SsdtSize,
                                    &TableKey
                                    );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  The entry point for this module.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Others         An error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
PrmSsdtInstallEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = InstallPrmSsdt ((UINT8 *)PcdGetPtr (PcdAcpiDefaultOemId));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
