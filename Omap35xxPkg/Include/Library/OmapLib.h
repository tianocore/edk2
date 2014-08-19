/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

