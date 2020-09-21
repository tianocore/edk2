/** @file
  Returns the platform specific Physical Presence configuration.

  Copyright (C) 2020 9elements GmbH

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __TCG2_PHYSICAL_PRESENCE_PLATFORM_LIB_H__
#define __TCG2_PHYSICAL_PRESENCE_PLATFORM_LIB_H__

#include <IndustryStandard/QemuTpm.h>

/**
  Reads the platform specific Physical Presence configuration.

  @param[out]  The Config structure to read to.
  @param[out]  The PPIinMMIO is True when the PPI is in MMIO memory space

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_PROTOCOL_ERROR    Invalid fw_cfg entry size.
**/
EFI_STATUS
TpmPPIPlatformReadConfig (
  OUT QEMU_FWCFG_TPM_CONFIG *Config,
  OUT BOOLEAN               *PPIinMMIO
  );

#endif
