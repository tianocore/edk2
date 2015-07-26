/** @file
*  An "early" DXE driver that parses well-known fw-cfg files into dynamic PCDs
*  that control other (universal) DXE drivers.
*
*  Copyright (C) 2015, Red Hat, Inc.
*  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR
*  IMPLIED.
*
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <IndustryStandard/SmBios.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>


/**
  Set the SMBIOS entry point version for the generic SmbiosDxe driver.
**/
STATIC
VOID
SmbiosVersionInitialization (
  VOID
  )
{
  FIRMWARE_CONFIG_ITEM     Anchor;
  UINTN                    AnchorSize;
  SMBIOS_TABLE_ENTRY_POINT QemuAnchor;
  UINT16                   SmbiosVersion;

  if (RETURN_ERROR (QemuFwCfgFindFile ("etc/smbios/smbios-anchor", &Anchor,
                      &AnchorSize)) ||
      AnchorSize != sizeof QemuAnchor) {
    return;
  }

  QemuFwCfgSelectItem (Anchor);
  QemuFwCfgReadBytes (AnchorSize, &QemuAnchor);
  if (CompareMem (QemuAnchor.AnchorString, "_SM_", 4) != 0 ||
      CompareMem (QemuAnchor.IntermediateAnchorString, "_DMI_", 5) != 0) {
    return;
  }

  SmbiosVersion = (UINT16)(QemuAnchor.MajorVersion << 8 |
                           QemuAnchor.MinorVersion);
  DEBUG ((EFI_D_INFO, "%a: SMBIOS version from QEMU: 0x%04x\n", __FUNCTION__,
    SmbiosVersion));
  PcdSet16 (PcdSmbiosVersion, SmbiosVersion);
}

EFI_STATUS
EFIAPI
ParseQemuFwCfgToPcd (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  SmbiosVersionInitialization ();
  return EFI_SUCCESS;
}
