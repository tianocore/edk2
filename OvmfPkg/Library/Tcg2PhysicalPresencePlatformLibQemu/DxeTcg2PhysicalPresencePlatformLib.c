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

  @param[out]  Config        The config structure to fill in.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A required pointer argument was NULL.
  @retval EFI_PROTOCOL_ERROR    Invalid fw_cfg entry size.
**/
EFI_STATUS
EFIAPI
TpmPpiPlatformReadConfig (
  OUT TCG2_PHYSICAL_PRESENCE_PLATFORM_CONFIG  *Config
  )
{
  EFI_STATUS             Status;
  FIRMWARE_CONFIG_ITEM   FwCfgItem;
  UINTN                  FwCfgSize;
  QEMU_FWCFG_TPM_CONFIG  QemuConfig;

  if (Config == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = QemuFwCfgFindFile ("etc/tpm/config", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize != sizeof (QemuConfig)) {
    return EFI_PROTOCOL_ERROR;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof (QemuConfig), &QemuConfig);

  Config->PpiAddress = QemuConfig.PpiAddress;
  Config->PpiInMmio  = TRUE;

  switch (QemuConfig.TpmVersion) {
    case QEMU_TPM_VERSION_1_2:
      Config->TpmVersion = Tcg2PhysicalPresenceTpmVersion12;
      break;

    case QEMU_TPM_VERSION_2:
      Config->TpmVersion = Tcg2PhysicalPresenceTpmVersion20;
      break;

    default:
      Config->TpmVersion = Tcg2PhysicalPresenceTpmVersionUnknown;
      break;
  }

  return EFI_SUCCESS;
}
