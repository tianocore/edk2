/** @file
  GUID for UEFI variables that are specific to OVMF configuration.

  Copyright (C) 2014, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OVMF_PLATFORM_CONFIG_H__
#define __OVMF_PLATFORM_CONFIG_H__

#define OVMF_PLATFORM_CONFIG_GUID \
{0x7235c51c, 0x0c80, 0x4cab, {0x87, 0xac, 0x3b, 0x08, 0x4a, 0x63, 0x04, 0xb1}}

extern EFI_GUID gOvmfPlatformConfigGuid;

#endif
