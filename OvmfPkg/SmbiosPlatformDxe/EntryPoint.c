/** @file
  Find and extract SMBIOS data.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemoryAllocationLib.h> // FreePool()
#include <OvmfPlatforms.h>               // CLOUDHV_DEVICE_ID

#include "SmbiosPlatformDxe.h"

/**
  Installs SMBIOS information for OVMF

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @retval EFI_SUCCESS    Smbios data successfully installed
  @retval Other          Smbios data was not installed

**/
EFI_STATUS
EFIAPI
SmbiosTablePublishEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT8       *SmbiosTables;
  UINT16      HostBridgeDevId;

  Status = EFI_NOT_FOUND;
  //
  // Add SMBIOS data if found
  //
  HostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);
  if (HostBridgeDevId == CLOUDHV_DEVICE_ID) {
    SmbiosTables = GetCloudHvSmbiosTables ();
    if (SmbiosTables != NULL) {
      Status = InstallAllStructures (SmbiosTables);
    }
  } else {
    SmbiosTables = GetQemuSmbiosTables ();
    if (SmbiosTables != NULL) {
      Status = InstallAllStructures (SmbiosTables);
      FreePool (SmbiosTables);
    }
  }

  //
  // If Status still reads EFI_NOT_FOUND, either there are no drivers
  // implementing EFI_SMBIOS_PROTOCOL, or none of the above codepaths ran. In
  // the second case, call InstallAllStructures with NULL so that the default
  // Type 0 (BIOS Information) structure is installed. In the first case,
  // calling InstallAllStructures is harmless because it'll fail immediately.
  //
  if (Status == EFI_NOT_FOUND) {
    Status = InstallAllStructures (NULL);
  }

  return Status;
}
