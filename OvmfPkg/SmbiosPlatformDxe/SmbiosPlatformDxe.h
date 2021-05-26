/** @file
  This driver installs SMBIOS information for OVMF

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_PLATFORM_DXE_H_
#define SMBIOS_PLATFORM_DXE_H_

/**
  Locates and extracts the QEMU SMBIOS table data if present in fw_cfg

  @return             Address of extracted QEMU SMBIOS data

**/
UINT8 *
GetQemuSmbiosTables (
  VOID
  );

#endif
