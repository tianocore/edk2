/** @file
  Returns the platform specific configuration for the QEMU PPI.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

Copyright (C) 2018, Red Hat, Inc.
Copyright (c) 2018, IBM Corporation. All rights reserved.<BR>
Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <IndustryStandard/QemuTpm.h>

#include <Library/QemuFwCfgLib.h>
#include <Library/Tcg2PhysicalPresencePlatformLib.h>

/**
  Reads QEMU PPI config from fw_cfg.

  @param[out]  The Config structure to read to.
  @param[out]  The PPIinMMIO is True when the PPI is in MMIO memory space

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_PROTOCOL_ERROR    Invalid fw_cfg entry size.
**/
EFI_STATUS
TpmPPIPlatformReadConfig (
  OUT QEMU_FWCFG_TPM_CONFIG *Config,
  OUT BOOLEAN               *PPIinMMIO
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;

  Status = QemuFwCfgFindFile ("etc/tpm/config", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize != sizeof (*Config)) {
    return EFI_PROTOCOL_ERROR;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof (*Config), Config);

  *PPIinMMIO = TRUE;

  return EFI_SUCCESS;
}
