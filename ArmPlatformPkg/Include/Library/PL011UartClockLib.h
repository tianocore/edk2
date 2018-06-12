/** @file
*
*  Copyright 2018 NXP
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
