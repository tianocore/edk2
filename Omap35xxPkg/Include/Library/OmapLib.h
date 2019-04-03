/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __OMAPLIB_H__
#define __OMAPLIB_H__

UINT32
EFIAPI
GpioBase (
  IN  UINTN Port
  );

UINT32
EFIAPI
TimerBase (
  IN  UINTN Timer
  );

UINTN
EFIAPI
InterruptVectorForTimer (
  IN  UINTN TImer
  );

UINT32
EFIAPI
UartBase (
  IN  UINTN Uart
  );


#endif // __OMAPLIB_H__

