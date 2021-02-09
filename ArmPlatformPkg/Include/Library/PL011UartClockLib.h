/** @file

  Copyright 2018 NXP
  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PL011UARTCLOCKLIB_H__
#define __PL011UARTCLOCKLIB_H__

/**

  Return baud clock frequency of  PL011.

  @return return frequency of PL011 in Hz

**/
UINT32
EFIAPI
PL011UartClockGetFreq (
  VOID
  );

#endif
