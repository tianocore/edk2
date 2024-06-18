/** @file
  Implement EFI RealTimeClock runtime services via RTC Lib.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LS_REAL_TIME_CLOCK_H_
#define LS_REAL_TIME_CLOCK_H_

#define TOY_WRITE0_REG  0x24
#define TOY_WRITE1_REG  0x28
#define TOY_READ0_REG   0x2c
#define TOY_READ1_REG   0x30
#define RTC_CTRL_REG    0x40

/* TOY Enable bits */
#define RTC_ENABLE_BIT  (1UL << 13)
#define TOY_ENABLE_BIT  (1UL << 11)
#define OSC_ENABLE_BIT  (1UL << 8)

/*
 * shift bits and filed mask
 */
#define TOY_MON_MASK   0x3f
#define TOY_DAY_MASK   0x1f
#define TOY_HOUR_MASK  0x1f
#define TOY_MIN_MASK   0x3f
#define TOY_SEC_MASK   0x3f
#define TOY_MSEC_MASK  0xf

#define TOY_MON_SHIFT   26
#define TOY_DAY_SHIFT   21
#define TOY_HOUR_SHIFT  16
#define TOY_MIN_SHIFT   10
#define TOY_SEC_SHIFT   4

#endif // LS_REAL_TIME_CLOCK_H_
