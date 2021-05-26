/** @file
  Find and extract QEMU SMBIOS data from fw_cfg.

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>            // ASSERT_EFI_ERROR()
#include <Library/MemoryAllocationLib.h> // AllocatePool()
#include <Library/PcdLib.h>              // PcdGetBool()
#include <Library/QemuFwCfgLib.h>        // QemuFwCfgFindFile()

#include "SmbiosPlatformDxe.h"

/**
  Locates and extracts the QEMU SMBIOS data if present in fw_cfg

  @return                 Address of extracted QEMU SMBIOS data

**/
UINT8 *
GetQemuSmbiosTables (
  VOID
  )
{
  EFI_STATUS               Status;
  FIRMWARE_CONFIG_ITEM     Tables;
  UINTN                    TablesSize;
  UINT8                    *QemuTables;

  if (!PcdGetBool (PcdQemuSmbiosValidated)) {
    return NULL;
  }

  Status = QemuFwCfgFindFile ("etc/smbios/smbios-tables", &Tables,
             &TablesSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (TablesSize > 0);

  QemuTables = AllocatePool (TablesSize);
  if (QemuTables == NULL) {
    return NULL;
  }

  QemuFwCfgSelectItem (Tables);
  QemuFwCfgReadBytes (TablesSize, QemuTables);

  return QemuTables;
}

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
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                Status;
  UINT8                     *SmbiosTables;

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
