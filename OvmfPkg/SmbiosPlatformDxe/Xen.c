/** @file
  This driver installs SMBIOS information for OVMF on Xen

  Copyright (C) 2021, Red Hat, Inc.
  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"
#include "XenSmbiosPlatformDxe.h"

/**
  Installs SMBIOS information for OVMF on Xen

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @retval EFI_SUCCESS    Smbios data successfully installed
  @retval Other          Smbios data was not installed

**/
EFI_STATUS
EFIAPI
XenSmbiosTablePublishEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  SMBIOS_TABLE_ENTRY_POINT  *EntryPointStructure;
  UINT8                     *SmbiosTables;

  Status = EFI_NOT_FOUND;
  //
  // Add Xen SMBIOS data if found
  //
  EntryPointStructure = GetXenSmbiosTables ();
  if (EntryPointStructure != NULL) {
    SmbiosTables = (UINT8 *)(UINTN)EntryPointStructure->TableAddress;
    if (SmbiosTables != NULL) {
      Status = InstallAllStructures (SmbiosTables);
    }
  }

  return Status;
}
