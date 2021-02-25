/** @file

  Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __PL031_REAL_TIME_CLOCK_H__
#define __PL031_REAL_TIME_CLOCK_H__

// PL031 Registers
#define PL031_RTC_DR_DATA_REGISTER                      0x000
#define PL031_RTC_MR_MATCH_REGISTER                     0x004
#define PL031_RTC_LR_LOAD_REGISTER                      0x008
#define PL031_RTC_CR_CONTROL_REGISTER                   0x00C
#define PL031_RTC_IMSC_IRQ_MASK_SET_CLEAR_REGISTER      0x010
#define PL031_RTC_RIS_RAW_IRQ_STATUS_REGISTER           0x014
#define PL031_RTC_MIS_MASKED_IRQ_STATUS_REGISTER        0x018
#define PL031_RTC_ICR_IRQ_CLEAR_REGISTER                0x01C
#define PL031_RTC_PERIPH_ID0                            0xFE0
#define PL031_RTC_PERIPH_ID1                            0xFE4
#define PL031_RTC_PERIPH_ID2                            0xFE8
#define PL031_RTC_PERIPH_ID3                            0xFEC
#define PL031_RTC_PCELL_ID0                             0xFF0
#define PL031_RTC_PCELL_ID1                             0xFF4
#define PL031_RTC_PCELL_ID2                             0xFF8
#define PL031_RTC_PCELL_ID3                             0xFFC

// PL031 Values
#define PL031_RTC_ENABLED                               0x00000001
#define PL031_SET_IRQ_MASK                              0x00000001
#define PL031_IRQ_TRIGGERED                             0x00000001
#define PL031_CLEAR_IRQ                                 0x00000001

#define PL031_COUNTS_PER_SECOND                         1

#endif
