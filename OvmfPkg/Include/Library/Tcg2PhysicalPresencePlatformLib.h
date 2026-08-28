/** @file
  Returns the platform specific Physical Presence configuration.

  Copyright (c) 2020, 9elements GmbH. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __TCG2_PHYSICAL_PRESENCE_PLATFORM_LIB_H__
#define __TCG2_PHYSICAL_PRESENCE_PLATFORM_LIB_H__

#include <Base.h>

typedef enum {
  Tcg2PhysicalPresenceTpmVersionUnknown,
  Tcg2PhysicalPresenceTpmVersion12,
  Tcg2PhysicalPresenceTpmVersion20
} TCG2_PHYSICAL_PRESENCE_TPM_VERSION;

typedef struct {
  EFI_PHYSICAL_ADDRESS                  PpiAddress;
  TCG2_PHYSICAL_PRESENCE_TPM_VERSION    TpmVersion;
  BOOLEAN                               PpiInMmio;
} TCG2_PHYSICAL_PRESENCE_PLATFORM_CONFIG;

/**
  Reads the platform specific Physical Presence configuration.

  @param[out]  Config        The config structure to fill in.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A required pointer argument was NULL.
  @retval EFI_PROTOCOL_ERROR    Invalid config entry size.
**/
EFI_STATUS
EFIAPI
TpmPpiPlatformReadConfig (
  OUT TCG2_PHYSICAL_PRESENCE_PLATFORM_CONFIG  *Config
  );

#endif
