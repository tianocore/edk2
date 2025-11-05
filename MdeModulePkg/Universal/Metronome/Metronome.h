/** @file
  Include file of Metronome driver.

Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _METRONOME_H_
#define _METRONOME_H_

#include <PiDxe.h>
#include <Protocol/Metronome.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>

/**
  Waits for the specified number of ticks.

  This function implements EFI_METRONOME_ARCH_PROTOCOL.WaitForTick().
  The WaitForTick() function waits for the number of ticks specified by
  TickNumber from a known time source in the platform.  If TickNumber of
  ticks are detected, then EFI_SUCCESS is returned.  The actual time passed
  between entry of this function and the first tick is between 0 and
  TickPeriod 100 nS units.  If you want to guarantee that at least TickPeriod
  time has elapsed, wait for two ticks.  This function waits for a hardware
  event to determine when a tick occurs.  It is possible for interrupt
  processing, or exception processing to interrupt the execution of the
  WaitForTick() function.  Depending on the hardware source for the ticks, it
  is possible for a tick to be missed.  This function cannot guarantee that
  ticks will not be missed.  If a timeout occurs waiting for the specified
  number of ticks, then EFI_TIMEOUT is returned.

  @param  This             The EFI_METRONOME_ARCH_PROTOCOL instance.
  @param  TickNumber       Number of ticks to wait.

  @retval EFI_SUCCESS      The wait for the number of ticks specified by TickNumber
                           succeeded.
  @retval EFI_TIMEOUT      A timeout occurred waiting for the specified number of ticks.

**/
EFI_STATUS
EFIAPI
WaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  );

#endif
