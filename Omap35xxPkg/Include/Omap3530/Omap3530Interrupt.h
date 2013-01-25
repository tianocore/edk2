/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OMAP3530INTERRUPT_H__
#define __OMAP3530INTERRUPT_H__

#define INTERRUPT_BASE (0x48200000)

#define INT_NROF_VECTORS      (96)
#define MAX_VECTOR            (INT_NROF_VECTORS - 1)
#define INTCPS_SYSCONFIG      (INTERRUPT_BASE + 0x0010)
#define INTCPS_SYSSTATUS      (INTERRUPT_BASE + 0x0014)
#define INTCPS_SIR_IRQ        (INTERRUPT_BASE + 0x0040)
#define INTCPS_SIR_IFQ        (INTERRUPT_BASE + 0x0044)
#define INTCPS_CONTROL        (INTERRUPT_BASE + 0x0048)
#define INTCPS_PROTECTION     (INTERRUPT_BASE + 0x004C)
#define INTCPS_IDLE           (INTERRUPT_BASE + 0x0050)
#define INTCPS_IRQ_PRIORITY   (INTERRUPT_BASE + 0x0060)
#define INTCPS_FIQ_PRIORITY   (INTERRUPT_BASE + 0x0064)
#define INTCPS_THRESHOLD      (INTERRUPT_BASE + 0x0068)
#define INTCPS_ITR(n)         (INTERRUPT_BASE + 0x0080 + (0x20 * (n)))
#define INTCPS_MIR(n)         (INTERRUPT_BASE + 0x0084 + (0x20 * (n)))
#define INTCPS_MIR_CLEAR(n)   (INTERRUPT_BASE + 0x0088 + (0x20 * (n)))
#define INTCPS_MIR_SET(n)     (INTERRUPT_BASE + 0x008C + (0x20 * (n)))
#define INTCPS_ISR_SET(n)     (INTERRUPT_BASE + 0x0090 + (0x20 * (n)))
#define INTCPS_ISR_CLEAR(n)   (INTERRUPT_BASE + 0x0094 + (0x20 * (n)))
#define INTCPS_PENDING_IRQ(n) (INTERRUPT_BASE + 0x0098 + (0x20 * (n)))
#define INTCPS_PENDING_FIQ(n) (INTERRUPT_BASE + 0x009C + (0x20 * (n)))
#define INTCPS_ILR(m)         (INTERRUPT_BASE + 0x0100 + (0x04 * (m)))

#define INTCPS_ILR_FIQ            BIT0
#define INTCPS_SIR_IRQ_MASK       (0x7F)
#define INTCPS_CONTROL_NEWIRQAGR  BIT0
#define INTCPS_CONTROL_NEWFIQAGR  BIT1

#endif // __OMAP3530INTERRUPT_H__

