/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

Module Name:

  Timer.h

Abstract:

  Driver implementing the EFI 2.0 timer protocol using the ARM SP804 timer.

--*/

#ifndef _TIMER_SP804_H__
#define _TIMER_SP804_H__



// EB board constants
#define EB_SP810_CTRL_BASE            0x10001000
#define EB_SP804_TIMER0_BASE          0x10011000
#define EB_SP804_TIMER1_BASE          0x10011020
#define EB_SP804_TIMER2_BASE          0x10012000
#define EB_SP804_TIMER3_BASE          0x10012020

#define EB_TIMER01_INTERRUPT_NUM      36
#define EB_TIMER23_INTERRUPT_NUM      37

// SP804 Timer constants
#define SP804_TIMER_LOAD_REG          0x00
#define SP804_TIMER_CURRENT_REG       0x04
#define SP804_TIMER_CONTROL_REG       0x08
#define SP804_TIMER_INT_CLR_REG       0x0C
#define SP804_TIMER_RAW_INT_STS_REG   0x10
#define SP804_TIMER_MSK_INT_STS_REG   0x14
#define SP804_TIMER_BG_LOAD_REG       0x18

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

// default timer tick period - 1ms, or 10000 units of 100ns
//#define TIMER_DEFAULT_PERIOD            10000

// default timer tick period - 50ms, or 500000 units of 100ns
#define TIMER_DEFAULT_PERIOD            500000

// default timer tick period - 500ms, or 5000000 units of 100ns
//#define TIMER_DEFAULT_PERIOD            5000000

#endif

