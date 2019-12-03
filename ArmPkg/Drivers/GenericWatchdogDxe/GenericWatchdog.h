/** @file
*
*  Copyright (c) 2013-2017, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/
#ifndef __GENERIC_WATCHDOG_H__
#define __GENERIC_WATCHDOG_H__

// Refresh Frame:
#define GENERIC_WDOG_REFRESH_REG              ((UINTN)FixedPcdGet64 (PcdGenericWatchdogRefreshBase) + 0x000)

// Control Frame:
#define GENERIC_WDOG_CONTROL_STATUS_REG       ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0x000)
#define GENERIC_WDOG_OFFSET_REG               ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0x008)
#define GENERIC_WDOG_COMPARE_VALUE_REG_LOW    ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0x010)
#define GENERIC_WDOG_COMPARE_VALUE_REG_HIGH   ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0x014)

// Values of bit 0 of the Control/Status Register
#define GENERIC_WDOG_ENABLED          1
#define GENERIC_WDOG_DISABLED         0

#endif  // __GENERIC_WATCHDOG_H__
