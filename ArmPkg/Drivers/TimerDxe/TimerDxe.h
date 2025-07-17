/** @file
*
*  Copyright (c) 2025, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TIMER_DXE_H_
#define TIMER_DXE_H_

// Timer IntIDs are architecturally defined for GICv5
#define GICV5_ARCH_TIMER_HYP_INTID       0x2000001a
#define GICV5_ARCH_TIMER_VIRT_INTID      0x2000001b
#define GICV5_ARCH_TIMER_HYP_VIRT_INTID  0x2000001c
#define GICV5_ARCH_TIMER_SEC_INTID       0x2000001d
#define GICV5_ARCH_TIMER_INTID           0x2000001e

#endif // TIMER_DXE_H_
