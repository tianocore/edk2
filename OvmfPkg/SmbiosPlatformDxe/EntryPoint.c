/** @file
  Find and extract SMBIOS data.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemoryAllocationLib.h> // FreePool()

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

  Status = EFI_NOT_FOUND;
  //
  // Add QEMU SMBIOS data if found
  //
  SmbiosTables = GetQemuSmbiosTables ();
  if (SmbiosTables != NULL) {
    Status = InstallAllStructures (SmbiosTables);
    FreePool (SmbiosTables);
  }

  return Status;
}
