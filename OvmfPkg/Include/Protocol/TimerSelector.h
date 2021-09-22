/** @file

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TIMER_SELECTOR_H_
#define TIMER_SELECTOR_H_

typedef enum {
  TimerSelector8254,
  TimerSelectorLocalApic,
} TIMER_SELECTOR;

#endif
