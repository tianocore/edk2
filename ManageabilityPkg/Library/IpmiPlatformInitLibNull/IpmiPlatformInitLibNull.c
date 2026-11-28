/** @file
  No-op implementation of explicit IPMI platform initialization.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/IpmiPlatformInitLib.h>

/**
  Perform no platform-specific preparation for the IPMI provider.

  @retval EFI_SUCCESS  No platform preparation is required.
**/
EFI_STATUS
EFIAPI
IpmiPlatformInitialize (
  VOID
  )
{
  return EFI_SUCCESS;
}
