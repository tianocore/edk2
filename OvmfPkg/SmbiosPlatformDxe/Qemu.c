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

/**
  Locates and extracts the QEMU SMBIOS data if present in fw_cfg

  @return                 Address of extracted QEMU SMBIOS data

**/
UINT8 *
GetQemuSmbiosTables (
  VOID
  )
{
  SMBIOS_TABLE_ENTRY_POINT QemuAnchor;
  FIRMWARE_CONFIG_ITEM     Anchor, Tables;
  UINTN                    AnchorSize, TablesSize;
  UINT8                    *QemuTables;

  if (EFI_ERROR (QemuFwCfgFindFile (
                   "etc/smbios/smbios-anchor", &Anchor, &AnchorSize)) ||
      EFI_ERROR (QemuFwCfgFindFile (
                   "etc/smbios/smbios-tables", &Tables, &TablesSize)) ||
      AnchorSize != sizeof (QemuAnchor) ||
      TablesSize == 0) {
    return NULL;
  }

  //
  // We copy the entry point structure to perform some additional checks,
  // but discard it upon return.
  //
  QemuFwCfgSelectItem (Anchor);
  QemuFwCfgReadBytes (AnchorSize, &QemuAnchor);

  if (AsciiStrnCmp ((CHAR8 *)QemuAnchor.AnchorString, "_SM_", 4) ||
      AsciiStrnCmp ((CHAR8 *)QemuAnchor.IntermediateAnchorString, "_DMI_", 5) ||
      TablesSize != QemuAnchor.TableLength) {
    return NULL;
  }

  QemuTables = AllocatePool (TablesSize);
  if (QemuTables == NULL) {
    return NULL;
  }

  QemuFwCfgSelectItem (Tables);
  QemuFwCfgReadBytes (TablesSize, QemuTables);

  return QemuTables;
}
