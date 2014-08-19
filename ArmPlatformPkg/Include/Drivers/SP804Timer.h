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


#ifndef _SP804_TIMER_H__
#define _SP804_TIMER_H__

// SP804 Timer constants
// Note: The SP804 Timer module comprises two timers, Timer_0 and Timer_1
//       These timers are identical and all their registers have an offset of 0x20
//       i.e. SP804_TIMER_0_LOAD_REG = 0x00 and SP804_TIMER_1_LOAD_REG = 0x20
//       Therefore, define all registers only once and adjust the base addresses by 0x20
#define SP804_TIMER_LOAD_REG            0x00
#define SP804_TIMER_CURRENT_REG         0x04
#define SP804_TIMER_CONTROL_REG         0x08
#define SP804_TIMER_INT_CLR_REG         0x0C
#define SP804_TIMER_RAW_INT_STS_REG     0x10
#define SP804_TIMER_MSK_INT_STS_REG     0x14
#define SP804_TIMER_BG_LOAD_REG         0x18

// Timer control register bit definitions
#define SP804_TIMER_CTRL_ONESHOT        BIT0
#define SP804_TIMER_CTRL_32BIT          BIT1
#define SP804_TIMER_CTRL_PRESCALE_MASK  (BIT3|BIT2)
#define SP804_PRESCALE_DIV_1            0
#define SP804_PRESCALE_DIV_16           BIT2
#define SP804_PRESCALE_DIV_256          BIT3
#define SP804_TIMER_CTRL_INT_ENABLE     BIT5
#define SP804_TIMER_CTRL_PERIODIC       BIT6
#define SP804_TIMER_CTRL_ENABLE         BIT7

// Other SP804 Timer definitions
#define SP804_MAX_TICKS                 0xFFFFFFFF

// SP810 System Controller constants
#define SP810_SYS_CTRL_REG              0x00
#define SP810_SYS_CTRL_TIMER0_TIMCLK    BIT15 // 0=REFCLK, 1=TIMCLK
#define SP810_SYS_CTRL_TIMER0_EN        BIT16
#define SP810_SYS_CTRL_TIMER1_TIMCLK    BIT17 // 0=REFCLK, 1=TIMCLK
#define SP810_SYS_CTRL_TIMER1_EN        BIT18
#define SP810_SYS_CTRL_TIMER2_TIMCLK    BIT19 // 0=REFCLK, 1=TIMCLK
#define SP810_SYS_CTRL_TIMER2_EN        BIT20
#define SP810_SYS_CTRL_TIMER3_TIMCLK    BIT21 // 0=REFCLK, 1=TIMCLK
#define SP810_SYS_CTRL_TIMER3_EN        BIT22

#endif
