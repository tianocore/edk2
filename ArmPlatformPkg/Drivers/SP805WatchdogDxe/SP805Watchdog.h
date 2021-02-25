/** @file

  Copyright (c) 2011-2012, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __SP805_WATCHDOG_H__
#define __SP805_WATCHDOG_H__

// SP805 Watchdog Registers
#define SP805_WDOG_LOAD_REG             ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0x000)
#define SP805_WDOG_CURRENT_REG          ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0x004)
#define SP805_WDOG_CONTROL_REG          ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0x008)
#define SP805_WDOG_INT_CLR_REG          ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0x00C)
#define SP805_WDOG_RAW_INT_STS_REG      ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0x010)
#define SP805_WDOG_MSK_INT_STS_REG      ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0x014)
#define SP805_WDOG_LOCK_REG             ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xC00)

#define SP805_WDOG_PERIPH_ID0           ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xFE0)
#define SP805_WDOG_PERIPH_ID1           ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xFE4)
#define SP805_WDOG_PERIPH_ID2           ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xFE8)
#define SP805_WDOG_PERIPH_ID3           ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xFEC)

#define SP805_WDOG_PCELL_ID0            ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xFF0)
#define SP805_WDOG_PCELL_ID1            ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xFF4)
#define SP805_WDOG_PCELL_ID2            ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xFF8)
#define SP805_WDOG_PCELL_ID3            ((UINT32)PcdGet32 (PcdSP805WatchdogBase) + 0xFFC)

// Timer control register bit definitions
#define SP805_WDOG_CTRL_INTEN           BIT0
#define SP805_WDOG_CTRL_RESEN           BIT1
#define SP805_WDOG_RAW_INT_STS_WDOGRIS  BIT0
#define SP805_WDOG_MSK_INT_STS_WDOGMIS  BIT0

#define SP805_WDOG_LOCK_IS_UNLOCKED     0x00000000
#define SP805_WDOG_LOCK_IS_LOCKED       0x00000001
#define SP805_WDOG_SPECIAL_UNLOCK_CODE  0x1ACCE551

#endif  // __SP805_WATCHDOG_H__
