/** @file
  Timer Architectural Protocol as defined in the DXE CIS

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2019, Citrix Systems, Inc.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LocalApicTimerDxe.h"

//
// The handle onto which the Timer Architectural Protocol will be installed
//
EFI_HANDLE  mTimerHandle = NULL;

//
// The Timer Architectural Protocol that this driver produces
//
EFI_TIMER_ARCH_PROTOCOL  mTimer = {
  TimerDriverRegisterHandler,
  TimerDriverSetTimerPeriod,
  TimerDriverGetTimerPeriod,
  TimerDriverGenerateSoftInterrupt
};

//
// Pointer to the CPU Architectural Protocol instance
//
EFI_CPU_ARCH_PROTOCOL  *mCpu;

//
// The notification function to call on every timer interrupt.
// A bug in the compiler prevents us from initializing this here.
//
EFI_TIMER_NOTIFY  mTimerNotifyFunction;

//
// The current period of the timer interrupt
//
volatile UINT64  mTimerPeriod = 0;

//
// Worker Functions
//

/**
  Interrupt Handler.

  @param InterruptType    The type of interrupt that occurred
  @param SystemContext    A pointer to the system context when the interrupt occurred
**/
VOID
EFIAPI
TimerInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EFI_TPL  OriginalTPL;

  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  SendApicEoi ();

  if (mTimerNotifyFunction != NULL) {
    //
    // @bug : This does not handle missed timer interrupts
    //
    mTimerNotifyFunction (mTimerPeriod);
  }

  gBS->RestoreTPL (OriginalTPL);
}

/**

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


  @param This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param NotifyFunction   The function to call when a timer interrupt fires.  This
                          function executes at TPL_HIGH_LEVEL.  The DXE Core will
                          register a handler for the timer interrupt, so it can know
                          how much time has passed.  This information is used to
                          signal timer based events.  NULL will unregister the handler.

  @retval        EFI_SUCCESS            The timer handler was registered.
  @retval        EFI_UNSUPPORTED        The platform does not support timer interrupts.
  @retval        EFI_ALREADY_STARTED    NotifyFunction is not NULL, and a handler is already
                                        registered.
  @retval        EFI_INVALID_PARAMETER  NotifyFunction is NULL, and a handler was not
                                        previously registered.
  @retval        EFI_DEVICE_ERROR       The timer handler could not be registered.

**/
EFI_STATUS
EFIAPI
TimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  )
{
  //
  // Check for invalid parameters
  //
  if ((NotifyFunction == NULL) && (mTimerNotifyFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NotifyFunction != NULL) && (mTimerNotifyFunction != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  mTimerNotifyFunction = NotifyFunction;

  return EFI_SUCCESS;
}

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
  )
{
  UINT64  TimerCount;
  UINT32  TimerFrequency;
  UINT32  DivideValue = 1;

  if (TimerPeriod == 0) {
    //
    // Disable timer interrupt for a TimerPeriod of 0
    //
    DisableApicTimerInterrupt ();
  } else {
    TimerFrequency = PcdGet32 (PcdFSBClock) / (UINT32)DivideValue;

    //
    // Convert TimerPeriod into local APIC counts
    //
    // TimerPeriod is in 100ns
    // TimerPeriod/10000000 will be in seconds.
    TimerCount = DivU64x32 (
                   MultU64x32 (TimerPeriod, TimerFrequency),
                   10000000
                   );

    // Check for overflow
    if (TimerCount > MAX_UINT32) {
      TimerCount = MAX_UINT32;
      /* TimerPeriod = (MAX_UINT32 / TimerFrequency) * 10000000; */
      TimerPeriod = 429496730;
    }

    //
    // Program the timer with the new count value
    //
    InitializeApicTimer (DivideValue, (UINT32)TimerCount, TRUE, LOCAL_APIC_TIMER_VECTOR);

    //
    // Enable timer interrupt
    //
    EnableApicTimerInterrupt ();
  }

  //
  // Save the new timer period
  //
  mTimerPeriod = TimerPeriod;

  return EFI_SUCCESS;
}

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
  )
{
  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerPeriod = mTimerPeriod;

  return EFI_SUCCESS;
}

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
  )
{
  EFI_TPL  OriginalTPL;

  if (GetApicTimerInterruptState ()) {
    //
    // Invoke the registered handler
    //
    OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

    if (mTimerNotifyFunction != NULL) {
      //
      // @bug : This does not handle missed timer interrupts
      //
      mTimerNotifyFunction (mTimerPeriod);
    }

    gBS->RestoreTPL (OriginalTPL);
  } else {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Initialize the Timer Architectural Protocol driver

  @param ImageHandle     ImageHandle of the loaded driver
  @param SystemTable     Pointer to the System Table

  @retval EFI_SUCCESS            Timer Architectural Protocol created
  @retval EFI_OUT_OF_RESOURCES   Not enough resources available to initialize driver.
  @retval EFI_DEVICE_ERROR       A device error occurred attempting to initialize the driver.

**/
EFI_STATUS
EFIAPI
TimerDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Initialize the pointer to our notify function.
  //
  mTimerNotifyFunction = NULL;

  //
  // Make sure the Timer Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiTimerArchProtocolGuid);

  //
  // Find the CPU architectural protocol.
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
  ASSERT_EFI_ERROR (Status);

  //
  // Force the timer to be disabled
  //
  Status = TimerDriverSetTimerPeriod (&mTimer, 0);
  ASSERT_EFI_ERROR (Status);

  //
  // Install interrupt handler for Local APIC Timer
  //
  Status = mCpu->RegisterInterruptHandler (
                   mCpu,
                   LOCAL_APIC_TIMER_VECTOR,
                   TimerInterruptHandler
                   );
  ASSERT_EFI_ERROR (Status);

  //
  // Force the timer to be enabled at its default period
  //
  Status = TimerDriverSetTimerPeriod (&mTimer, DEFAULT_TIMER_TICK_DURATION);
  ASSERT_EFI_ERROR (Status);

  //
  // Install the Timer Architectural Protocol onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mTimerHandle,
                  &gEfiTimerArchProtocolGuid,
                  &mTimer,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
