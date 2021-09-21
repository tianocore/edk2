/** @file
  Header file for a generic i801 SMBUS driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#ifndef _PCI_PLATFORM_DXE_H_
#define _PCI_PLATFORM_DXE_H_
#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Pci22.h>
#include <Protocol/SmbusHc.h>
#include <Guid/BoardSettingsGuid.h>

#define BOARD_SETTINGS_OFFSET   0x1f00
#define BOARD_BOOT_OVERRIDE_OFFSET 0x1640

#define HOSTC                   0x40
#define I2C_EN_HOSTC            (1 << 2)

#endif
