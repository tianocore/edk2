/** @file
  OVMF ACPI Platform Driver

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <OvmfPlatforms.h> // CLOUDHV_DEVICE_ID
#include <ConfidentialComputingGuestAttr.h>

#include <Library/AcpiPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "AcpiPlatform.h"

/**
  Effective entrypoint of Acpi Platform driver.

  @param  AcpiTable
  Pointer to the EFI ACPI Table Protocol instance.

  @retval EFI_SUCCESS           ACPI tables installed successfully.
  @retval EFI_INVALID_PARAMETER AcpiTable was NULL.
  @retval EFI_LOAD_ERROR        ACPI tables failed to load.
  @retval EFI_OUT_OF_RESOURCES  Out of memory while installing tables.
**/
EFI_STATUS
EFIAPI
InstallAcpiTables (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_STATUS  Status;
  UINT16      HostBridgeDevId;
  
  if (AcpiTable == NULL) {
    DEBUG ((DEBUG_ERROR, "InstallAcpiTables: AcpiTable is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  HostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);

  if (HostBridgeDevId == CLOUDHV_DEVICE_ID) {
    if (CC_GUEST_IS_TDX (PcdGet64 (PcdConfidentialComputingGuestAttr))) {
      Status = InstallCloudHvTablesTdx (AcpiTable);
    } else {
      Status = InstallCloudHvTables (AcpiTable);
    }
  } else {
    Status = InstallQemuFwCfgTables (AcpiTable);
  }

  return Status;
}
