/** @file
  ResetSystem lib head file.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RESET_SYSTEM_ACPI_GED_H_
#define RESET_SYSTEM_ACPI_GED_H_

#include <Base.h>

typedef struct {
  UINT64    SleepControlRegAddr;
  UINT64    SleepStatusRegAddr;
  UINT64    ResetRegAddr;
  UINT8     ResetValue;
} POWER_MANAGER;

extern POWER_MANAGER  mPowerManager;
#endif // RESET_SYSTEM_ACPI_GED_H_
