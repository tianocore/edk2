/*++ @file
  Emu Emulation Timer Architectural Protocol Driver as defined in DXE CIS

  This Timer module uses an Emu Thread to simulate the timer-tick driven
  timer service.  In the future, the Thread creation should possibly be
  abstracted by the CPU architectural protocol

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010 - 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "PiDxe.h"
#include <Protocol/Timer.h>
#include <Protocol/Cpu.h>
#include "Timer.h"
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/EmuThunkLib.h>

//
// Pointer to the CPU Architectural Protocol instance
//
EFI_CPU_ARCH_PROTOCOL   *mCpu;

//
// The Timer Architectural Protocol that this driver produces
//
EFI_TIMER_ARCH_PROTOCOL mTimer = {
  EmuTimerDriverRegisterHandler,
  EmuTimerDriverSetTimerPeriod,
  EmuTimerDriverGetTimerPeriod,
  EmuTimerDriverGenerateSoftInterrupt
};

//
// The notification function to call on every timer interrupt
//
EFI_TIMER_NOTIFY        mTimerNotifyFunction = NULL;

//
// The current period of the timer interrupt
//
UINT64                  mTimerPeriodMs;


VOID
EFIAPI
TimerCallback (UINT64 DeltaMs)
{
  EFI_TPL           OriginalTPL;
  EFI_TIMER_NOTIFY  CallbackFunction;


  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  if (OriginalTPL < TPL_HIGH_LEVEL) {
    CallbackFunction = mTimerNotifyFunction;

    //
    // Only invoke the callback function if a Non-NULL handler has been
    // registered. Assume all other handlers are legal.
    //
    if (CallbackFunction != NULL) {
      CallbackFunction (MultU64x32 (DeltaMs, 10000));
    }
  }

  gBS->RestoreTPL (OriginalTPL);

}

EFI_STATUS
EFIAPI
EmuTimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL           *This,
  IN EFI_TIMER_NOTIFY                  NotifyFunction
  )
/*++

Routine Description:

  This function registers the handler NotifyFunction so it is called every time
  the timer interrupt fires.  It also passes the amount of time since the last
  handler call to the NotifyFunction.  If NotifyFunction is NULL, then the
  handler is unregistered.  If the handler is registered, then EFI_SUCCESS is
  returned.  If the CPU does not support registering a timer interrupt handler,
  then EFI_UNSUPPORTED is returned.  If an attempt is made to register a handler
  when a handler is already registered, then EFI_ALREADY_STARTED is returned.
  If an attempt is made to unregister a handler when a handler is not registered,
  then EFI_INVALID_PARAMETER is returned.  If an error occurs attempting to
  register the NotifyFunction with the timer interrupt, then EFI_DEVICE_ERROR
  is returned.

Arguments:

  This           - The EFI_TIMER_ARCH_PROTOCOL instance.

  NotifyFunction - The function to call when a timer interrupt fires.  This
                   function executes at TPL_HIGH_LEVEL.  The DXE Core will
                   register a handler for the timer interrupt, so it can know
                   how much time has passed.  This information is used to
                   signal timer based events.  NULL will unregister the handler.

Returns:

  EFI_SUCCESS           - The timer handler was registered.

  EFI_UNSUPPORTED       - The platform does not support timer interrupts.

  EFI_ALREADY_STARTED   - NotifyFunction is not NULL, and a handler is already
                          registered.

  EFI_INVALID_PARAMETER - NotifyFunction is NULL, and a handler was not
                          previously registered.

  EFI_DEVICE_ERROR      - The timer handler could not be registered.

**/
{
  //
  // Check for invalid parameters
  //
  if (NotifyFunction == NULL && mTimerNotifyFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (NotifyFunction != NULL && mTimerNotifyFunction != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (NotifyFunction == NULL) {
    /* Disable timer.  */
    gEmuThunk->SetTimer (0, TimerCallback);
  } else if (mTimerNotifyFunction == NULL) {
    /* Enable Timer.  */
    gEmuThunk->SetTimer (mTimerPeriodMs, TimerCallback);
  }
  mTimerNotifyFunction = NotifyFunction;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EmuTimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  )
/*++

Routine Description:

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

Arguments:

  This        - The EFI_TIMER_ARCH_PROTOCOL instance.

  TimerPeriod - The rate to program the timer interrupt in 100 nS units.  If
                the timer hardware is not programmable, then EFI_UNSUPPORTED is
                returned.  If the timer is programmable, then the timer period
                will be rounded up to the nearest timer period that is supported
                by the timer hardware.  If TimerPeriod is set to 0, then the
                timer interrupts will be disabled.

Returns:

  EFI_SUCCESS      - The timer period was changed.

  EFI_UNSUPPORTED  - The platform cannot change the period of the timer interrupt.

  EFI_DEVICE_ERROR - The timer period could not be changed due to a device error.

**/
{

  //
  // If TimerPeriod is 0, then the timer thread should be canceled
  // If the TimerPeriod is valid, then create and/or adjust the period of the timer thread
  //
  if (TimerPeriod == 0
      || ((TimerPeriod > TIMER_MINIMUM_VALUE)
    && (TimerPeriod < TIMER_MAXIMUM_VALUE))) {
    mTimerPeriodMs = DivU64x32 (TimerPeriod + 5000, 10000);

    gEmuThunk->SetTimer (mTimerPeriodMs, TimerCallback);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EmuTimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL            *This,
  OUT UINT64                            *TimerPeriod
  )
/*++

Routine Description:

  This function retrieves the period of timer interrupts in 100 ns units,
  returns that value in TimerPeriod, and returns EFI_SUCCESS.  If TimerPeriod
  is NULL, then EFI_INVALID_PARAMETER is returned.  If a TimerPeriod of 0 is
  returned, then the timer is currently disabled.

Arguments:

  This        - The EFI_TIMER_ARCH_PROTOCOL instance.

  TimerPeriod - A pointer to the timer period to retrieve in 100 ns units.  If
                0 is returned, then the timer is currently disabled.

Returns:

  EFI_SUCCESS           - The timer period was returned in TimerPeriod.

  EFI_INVALID_PARAMETER - TimerPeriod is NULL.

**/
{
  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerPeriod = MultU64x32 (mTimerPeriodMs, 10000);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EmuTimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  )
/*++

Routine Description:

  This function generates a soft timer interrupt. If the platform does not support soft
  timer interrupts, then EFI_UNSUPPORTED is returned. Otherwise, EFI_SUCCESS is returned.
  If a handler has been registered through the EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()
  service, then a soft timer interrupt will be generated. If the timer interrupt is
  enabled when this service is called, then the registered handler will be invoked. The
  registered handler should not be able to distinguish a hardware-generated timer
  interrupt from a software-generated timer interrupt.

Arguments:

  This  -  The EFI_TIMER_ARCH_PROTOCOL instance.

Returns:

  EFI_SUCCESS       - The soft timer interrupt was generated.

  EFI_UNSUPPORTED   - The platform does not support the generation of soft timer interrupts.

**/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EmuTimerDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  Initialize the Timer Architectural Protocol driver

Arguments:

  ImageHandle - ImageHandle of the loaded driver

  SystemTable - Pointer to the System Table

Returns:

  EFI_SUCCESS           - Timer Architectural Protocol created

  EFI_OUT_OF_RESOURCES  - Not enough resources available to initialize driver.

  EFI_DEVICE_ERROR      - A device error occurred attempting to initialize the driver.

**/
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  //
  // Make sure the Timer Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiTimerArchProtocolGuid);

  //
  // Get the CPU Architectural Protocol instance
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (void *)&mCpu);
  ASSERT_EFI_ERROR (Status);

  //
  // Start the timer thread at the default timer period
  //
  Status = mTimer.SetTimerPeriod (&mTimer, DEFAULT_TIMER_TICK_DURATION);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install the Timer Architectural Protocol onto a new handle
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiTimerArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mTimer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }


  return EFI_SUCCESS;
}
