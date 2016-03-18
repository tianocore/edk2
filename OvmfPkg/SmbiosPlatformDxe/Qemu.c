/** @file
  Find and extract QEMU SMBIOS data from fw_cfg.

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.   The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "SmbiosPlatformDxe.h"
#include <Library/QemuFwCfgLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

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
