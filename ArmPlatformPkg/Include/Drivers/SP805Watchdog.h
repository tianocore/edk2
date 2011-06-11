/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/


#ifndef __SP805_WATCHDOG_H__
#define __SP805_WATCHDOG_H__

#include <Base.h>
#include <ArmPlatform.h>

// SP805 Watchdog Registers
#define SP805_WDOG_LOAD_REG             (SP805_WDOG_BASE + 0x000)
#define SP805_WDOG_CURRENT_REG          (SP805_WDOG_BASE + 0x004)
#define SP805_WDOG_CONTROL_REG          (SP805_WDOG_BASE + 0x008)
#define SP805_WDOG_INT_CLR_REG          (SP805_WDOG_BASE + 0x00C)
#define SP805_WDOG_RAW_INT_STS_REG      (SP805_WDOG_BASE + 0x010)
#define SP805_WDOG_MSK_INT_STS_REG      (SP805_WDOG_BASE + 0x014)
#define SP805_WDOG_LOCK_REG             (SP805_WDOG_BASE + 0xC00)

#define SP805_WDOG_PERIPH_ID0           (SP805_WDOG_BASE + 0xFE0)
#define SP805_WDOG_PERIPH_ID1           (SP805_WDOG_BASE + 0xFE4)
#define SP805_WDOG_PERIPH_ID2           (SP805_WDOG_BASE + 0xFE8)
#define SP805_WDOG_PERIPH_ID3           (SP805_WDOG_BASE + 0xFEC)

#define SP805_WDOG_PCELL_ID0            (SP805_WDOG_BASE + 0xFF0)
#define SP805_WDOG_PCELL_ID1            (SP805_WDOG_BASE + 0xFF4)
#define SP805_WDOG_PCELL_ID2            (SP805_WDOG_BASE + 0xFF8)
#define SP805_WDOG_PCELL_ID3            (SP805_WDOG_BASE + 0xFFC)

// Timer control register bit definitions
#define SP805_WDOG_CTRL_INTEN           BIT0
#define SP805_WDOG_CTRL_RESEN           BIT1
#define SP805_WDOG_RAW_INT_STS_WDOGRIS  BIT0
#define SP805_WDOG_MSK_INT_STS_WDOGMIS  BIT0

#define SP805_WDOG_LOCK_IS_UNLOCKED     0x00000000
#define SP805_WDOG_LOCK_IS_LOCKED       0x00000001
#define SP805_WDOG_SPECIAL_UNLOCK_CODE  0x1ACCE551

VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
);

EFI_STATUS
EFIAPI
SP805SetTimerPeriod (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  IN UINT64                                   TimerPeriod   // In 100ns units
);

EFI_STATUS
EFIAPI
SP805GetTimerPeriod (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                                  *TimerPeriod
);

EFI_STATUS
EFIAPI
SP805RegisterHandler (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  IN EFI_WATCHDOG_TIMER_NOTIFY                NotifyFunction
);

EFI_STATUS
SP805Initialize (
  VOID
);

EFI_STATUS
EFIAPI
SP805InstallProtocol (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
);


#endif  // __SP805_WATCHDOG_H__
