/** @file
  GUID for UEFI variables that are specific to OVMF configuration.

  Copyright (C) 2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __OVMF_PLATFORM_CONFIG_H__
#define __OVMF_PLATFORM_CONFIG_H__

#define OVMF_PLATFORM_CONFIG_GUID \
{0x7235c51c, 0x0c80, 0x4cab, {0x87, 0xac, 0x3b, 0x08, 0x4a, 0x63, 0x04, 0xb1}}

extern EFI_GUID  gOvmfPlatformConfigGuid;

#endif
