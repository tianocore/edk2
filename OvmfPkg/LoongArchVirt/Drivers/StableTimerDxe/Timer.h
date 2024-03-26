/** @file
  Private data structures

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TIMER_H_
#define TIMER_H_

#include <Protocol/Timer.h>

#define DEFAULT_TIMER_TICK_DURATION  100000 // 10ms = 100000 100 ns units

//
// The current period of the timer interrupt
//
volatile UINT64  mTimerPeriod = 0;
volatile UINT64  mTimerTicks  = 0;

/**
  This function adjusts the period of timer interrupts to the value specified
  by TimerPeriod.  If the timer period is updated, then the selected timer
  period is stored in EFI_TIMER.TimerPeriod, and EFI_SUCCESS is returned.  If
  the timer hardware is not programmable, then EFI_UNSUPPORTED is returned.
  If an error occurs while attempting to update the timer period, then the
  timer hardware will be put back in its state prior to this call, and
  EFI_DEVICE_ERROR is returned.  If TimerPeriod is 0, then the timer interrupt
  is disabled.  This is not the same as disabling the CPU's interrupts.
  Instead, it must either turn off the timer hardware, or it must adjust the
  interrupt controller so that a CPU interrupt is not generated when the timer
  interrupt fires.

  @param This            The EFI_TIMER_ARCH_PROTOCOL instance.
  @param NotifyFunction  The rate to program the timer interrupt in 100 nS units.  If
                         the timer hardware is not programmable, then EFI_UNSUPPORTED is
                         returned.  If the timer is programmable, then the timer period
                         will be rounded up to the nearest timer period that is supported
                         by the timer hardware.  If TimerPeriod is set to 0, then the
                         timer interrupts will be disabled.

  @retval        EFI_SUCCESS       The timer period was changed.
  @retval        EFI_UNSUPPORTED   The platform cannot change the period of the timer interrupt.
  @retval        EFI_DEVICE_ERROR  The timer period could not be changed due to a device error.
**/
EFI_STATUS
EFIAPI
TimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  );

/**
  This function adjusts the period of timer interrupts to the value specified
  by TimerPeriod.  If the timer period is updated, then the selected timer
  period is stored in EFI_TIMER.TimerPeriod, and EFI_SUCCESS is returned.  If
  the timer hardware is not programmable, then EFI_UNSUPPORTED is returned.
  If an error occurs while attempting to update the timer period, then the
  timer hardware will be put back in its state prior to this call, and
  EFI_DEVICE_ERROR is returned.  If TimerPeriod is 0, then the timer interrupt
  is disabled.  This is not the same as disabling the CPU's interrupts.
  Instead, it must either turn off the timer hardware, or it must adjust the
  interrupt controller so that a CPU interrupt is not generated when the timer
  interrupt fires.

  @param This            The EFI_TIMER_ARCH_PROTOCOL instance.
  @param TimerPeriod     The rate to program the timer interrupt in 100 nS units.  If
                         the timer hardware is not programmable, then EFI_UNSUPPORTED is
                         returned.  If the timer is programmable, then the timer period
                         will be rounded up to the nearest timer period that is supported
                         by the timer hardware.  If TimerPeriod is set to 0, then the
                         timer interrupts will be disabled.

  @retval        EFI_SUCCESS       The timer period was changed.
  @retval        EFI_UNSUPPORTED   The platform cannot change the period of the timer interrupt.
  @retval        EFI_DEVICE_ERROR  The timer period could not be changed due to a device error.
**/
EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  );

/**
  This function retrieves the period of timer interrupts in 100 ns units,
  returns that value in TimerPeriod, and returns EFI_SUCCESS.  If TimerPeriod
  is NULL, then EFI_INVALID_PARAMETER is returned.  If a TimerPeriod of 0 is
  returned, then the timer is currently disabled.

  @param This            The EFI_TIMER_ARCH_PROTOCOL instance.
  @param TimerPeriod     A pointer to the timer period to retrieve in 100 ns units.  If
                         0 is returned, then the timer is currently disabled.

  @retval EFI_SUCCESS            The timer period was returned in TimerPeriod.
  @retval EFI_INVALID_PARAMETER  TimerPeriod is NULL.
**/
EFI_STATUS
EFIAPI
TimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  OUT UINT64                  *TimerPeriod
  );

/**
  This function generates a soft timer interrupt. If the platform does not support soft
  timer interrupts, then EFI_UNSUPPORTED is returned. Otherwise, EFI_SUCCESS is returned.
  If a handler has been registered through the EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()
  service, then a soft timer interrupt will be generated. If the timer interrupt is
  enabled when this service is called, then the registered handler will be invoked. The
  registered handler should not be able to distinguish a hardware-generated timer
  interrupt from a software-generated timer interrupt.

  @param This              The EFI_TIMER_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS       The soft timer interrupt was generated.
  @retval EFI_UNSUPPORTED   The platform does not support the generation of soft timer interrupts.
**/
EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  );

#endif // TIMER_H_
