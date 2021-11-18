/*++ @file
  Emu Emulation Architectural Protocol Driver as defined in UEFI/PI.
  This Timer module uses an UNIX Thread to simulate the timer-tick driven
  timer service.

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010 - 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _TIMER_H_
#define _TIMER_H_

//
// Legal timer value range in 100 ns units
//
#define TIMER_MINIMUM_VALUE  0
#define TIMER_MAXIMUM_VALUE  (0x100000000ULL - 1)

//
// Default timer value in 100 ns units (50 ms)
//
#define DEFAULT_TIMER_TICK_DURATION  500000

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
EmuTimerDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
EmuTimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  );

EFI_STATUS
EFIAPI
EmuTimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  );

EFI_STATUS
EFIAPI
EmuTimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  OUT UINT64                  *TimerPeriod
  );

EFI_STATUS
EFIAPI
EmuTimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  );

#endif
