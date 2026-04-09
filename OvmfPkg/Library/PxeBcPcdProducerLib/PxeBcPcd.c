/** @file
  Configure some PCDs dynamically for
  "NetworkPkg/UefiPxeBcDxe/UefiPxeBcDxe.inf", from QEMU's fw_cfg.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/PcdLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>

RETURN_STATUS
EFIAPI
SetPxeBcPcds (
  VOID
  )
{
  BOOLEAN        FwCfgBool;
  RETURN_STATUS  PcdStatus;

  if (!RETURN_ERROR (
         QemuFwCfgParseBool (
           "opt/org.tianocore/IPv4PXESupport",
           &FwCfgBool
           )
         ))
  {
    PcdStatus = PcdSet8S (PcdIPv4PXESupport, FwCfgBool);
    if (RETURN_ERROR (PcdStatus)) {
      return PcdStatus;
    }
  }

  if (!RETURN_ERROR (
         QemuFwCfgParseBool (
           "opt/org.tianocore/IPv6PXESupport",
           &FwCfgBool
           )
         ))
  {
    PcdStatus = PcdSet8S (PcdIPv6PXESupport, FwCfgBool);
    if (RETURN_ERROR (PcdStatus)) {
      return PcdStatus;
    }
  }

  return RETURN_SUCCESS;
}
