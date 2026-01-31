/** @file
  Returns the platform specific Physical Presence configuration.

  Copyright (c) 2020, 9elements GmbH. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __TCG2_PHYSICAL_PRESENCE_PLATFORM_LIB_H__
#define __TCG2_PHYSICAL_PRESENCE_PLATFORM_LIB_H__

#include <IndustryStandard/QemuTpm.h>

/**
  Reads the platform specific Physical Presence configuration.

  @param[out]  Config        The config structure to fill in.
  @param[out]  PpiInMmio     TRUE when the PPI lives in MMIO space.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A required pointer argument was NULL.
  @retval EFI_PROTOCOL_ERROR    Invalid config entry size.
**/
EFI_STATUS
EFIAPI
TpmPpiPlatformReadConfig (
  OUT QEMU_FWCFG_TPM_CONFIG  *Config,
  OUT BOOLEAN                *PpiInMmio
  );

#endif
