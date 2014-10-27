/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD
*  License which accompanies this distribution.  The full text of the license
*  may be found at http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/
#ifndef __GENERIC_WATCHDOG_H__
#define __GENERIC_WATCHDOG_H__

// Refresh Frame:
#define GENERIC_WDOG_REFRESH_REG              ((UINT32)FixedPcdGet32 (PcdGenericWatchdogRefreshBase) + 0x000)

// Control Frame:
#define GENERIC_WDOG_CONTROL_STATUS_REG       ((UINT32)FixedPcdGet32 (PcdGenericWatchdogControlBase) + 0x000)
#define GENERIC_WDOG_OFFSET_REG               ((UINT32)FixedPcdGet32 (PcdGenericWatchdogControlBase) + 0x008)
#define GENERIC_WDOG_COMPARE_VALUE_REG        ((UINT32)FixedPcdGet32 (PcdGenericWatchdogControlBase) + 0x010)

// Values of bit 0 of the Control/Status Register
#define GENERIC_WDOG_ENABLED          1
#define GENERIC_WDOG_DISABLED         0

#endif  // __GENERIC_WATCHDOG_H__
