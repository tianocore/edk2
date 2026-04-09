/** @file
  OVMF ACPI Platform Driver

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <OvmfPlatforms.h> // CLOUDHV_DEVICE_ID
#include <ConfidentialComputingGuestAttr.h>

#include <Library/AcpiPlatformLib.h>

#include "AcpiPlatform.h"

/**
  Effective entrypoint of Acpi Platform driver.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
EFIAPI
InstallAcpiTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_STATUS  Status;
  UINT16      HostBridgeDevId;

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
