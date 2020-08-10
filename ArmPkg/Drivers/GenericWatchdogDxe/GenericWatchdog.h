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
#define GENERIC_WDOG_OFFSET_REG_LOW           ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0x008)
#define GENERIC_WDOG_OFFSET_REG_HIGH          ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0x00C)
#define GENERIC_WDOG_COMPARE_VALUE_REG_LOW    ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0x010)
#define GENERIC_WDOG_COMPARE_VALUE_REG_HIGH   ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0x014)
#define GENERIC_WDOG_IIDR_REG                 ((UINTN)FixedPcdGet64 (PcdGenericWatchdogControlBase) + 0xFCC)

// Values of bit 0 of the Control/Status Register
#define GENERIC_WDOG_ENABLED          1
#define GENERIC_WDOG_DISABLED         0
#define GENERIC_WDOG_ARCH_REV_OFFSET 16
#define GENERIC_WDOG_ARCH_REV_MASK   0xF
#define SBSA_WDOG_WOR_WIDTH          48
#define MAX_OFFSET_REG_VAL           (((UINT64)1 << 48) - 1)

#endif  // __GENERIC_WATCHDOG_H__
