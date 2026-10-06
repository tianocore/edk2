/** @file
  Implement Goldfish RealTimeClock runtime services via RTC Lib.

  Copyright (C) 2007 Google, Inc.
  Copyright (C) 2017 Imagination Technologies Ltd.
  Copyright (C) 2026 ZTE Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

// Goldfish Registers
#define GOLDFISH_RTC_TIME_LOW         0x00
#define GOLDFISH_RTC_TIME_HIGH        0x04
#define GOLDFISH_RTC_ALARM_LOW        0x08
#define GOLDFISH_RTC_ALARM_HIGH       0x0c
#define GOLDFISH_RTC_IRQ_ENABLED      0x10
#define GOLDFISH_RTC_CLEAR_ALARM      0x14
#define GOLDFISH_RTC_ALARM_STATUS     0x18
#define GOLDFISH_RTC_CLEAR_INTERRUPT  0x1c

#define NANO_SEC  1000000000ULL

#define GOLDFISH_COUNTS_PER_SECOND  1
#define GOLDFISH_ACCURACY           50000000
