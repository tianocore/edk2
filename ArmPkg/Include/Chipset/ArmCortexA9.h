/** @file

  Copyright (c) 2011, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM_CORTEX_A9_H__
#define __ARM_CORTEX_A9_H__

#include <Chipset/ArmV7.h>

//
// Cortex A9 feature bit definitions
//
#define A9_FEATURE_PARITY  (1<<9)
#define A9_FEATURE_AOW     (1<<8)
#define A9_FEATURE_EXCL    (1<<7)
#define A9_FEATURE_SMP     (1<<6)
#define A9_FEATURE_FOZ     (1<<3)
#define A9_FEATURE_DPREF   (1<<2)
#define A9_FEATURE_HINT    (1<<1)
#define A9_FEATURE_FWD     (1<<0)

//
// Cortex A9 Watchdog
//
#define ARM_A9_WATCHDOG_REGION           0x600

#define ARM_A9_WATCHDOG_LOAD_REGISTER    0x20
#define ARM_A9_WATCHDOG_CONTROL_REGISTER 0x28

#define ARM_A9_WATCHDOG_WATCHDOG_MODE    (1 << 3)
#define ARM_A9_WATCHDOG_TIMER_MODE       (0 << 3)
#define ARM_A9_WATCHDOG_SINGLE_SHOT      (0 << 1)
#define ARM_A9_WATCHDOG_AUTORELOAD       (1 << 1)
#define ARM_A9_WATCHDOG_ENABLE           1

//
// SCU register offsets & masks
//
#define A9_SCU_CONTROL_OFFSET       0x0
#define A9_SCU_CONFIG_OFFSET        0x4
#define A9_SCU_INVALL_OFFSET        0xC
#define A9_SCU_FILT_START_OFFSET    0x40
#define A9_SCU_FILT_END_OFFSET      0x44
#define A9_SCU_SACR_OFFSET          0x50
#define A9_SCU_SSACR_OFFSET         0x54


UINTN
EFIAPI
ArmGetScuBaseAddress (
  VOID
  );

#endif

