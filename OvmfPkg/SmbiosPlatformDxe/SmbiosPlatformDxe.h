/** @file
  This driver installs SMBIOS information for OVMF

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_PLATFORM_DXE_H_
#define SMBIOS_PLATFORM_DXE_H_

/**
  Install all structures from the given SMBIOS structures block

  @param  TableAddress         SMBIOS tables starting address

**/
EFI_STATUS
InstallAllStructures (
  IN UINT8                     *TableAddress
  );

#endif
