/** @file

  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
  Copyright (c) 2018, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/HardwareInterrupt.h>
#include <Protocol/WatchdogTimer.h>

#include "SP805Watchdog.h"

STATIC EFI_EVENT                        mEfiExitBootServicesEvent;
STATIC EFI_HARDWARE_INTERRUPT_PROTOCOL  *mInterrupt;
STATIC EFI_WATCHDOG_TIMER_NOTIFY        mWatchdogNotify;
STATIC UINT32                           mTimerPeriod;

/**
  Make sure the SP805 registers are unlocked for writing.

  Note: The SP805 Watchdog Timer supports locking of its registers,
  i.e. it inhibits all writes to avoid rogue software accidentally
  corrupting their contents.
**/
STATIC
VOID
SP805Unlock (
  VOID
  )
{
  if (MmioRead32 (SP805_WDOG_LOCK_REG) == SP805_WDOG_LOCK_IS_LOCKED) {
    MmioWrite32 (SP805_WDOG_LOCK_REG, SP805_WDOG_SPECIAL_UNLOCK_CODE);
  }
}

/**
  Make sure the SP805 registers are locked and can not be overwritten.

  Note: The SP805 Watchdog Timer supports locking of its registers,
  i.e. it inhibits all writes to avoid rogue software accidentally
  corrupting their contents.
**/
STATIC
VOID
SP805Lock (
  VOID
  )
{
  if (MmioRead32 (SP805_WDOG_LOCK_REG) == SP805_WDOG_LOCK_IS_UNLOCKED) {
    // To lock it, just write in any number (except the special unlock code).
    MmioWrite32 (SP805_WDOG_LOCK_REG, SP805_WDOG_LOCK_IS_LOCKED);
  }
}

STATIC
VOID
EFIAPI
SP805InterruptHandler (
  IN  HARDWARE_INTERRUPT_SOURCE  Source,
  IN  EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  SP805Unlock ();
  MmioWrite32 (SP805_WDOG_INT_CLR_REG, 0); // write of any value clears the irq
  SP805Lock ();

  mInterrupt->EndOfInterrupt (mInterrupt, Source);

  //
  // The notify function should be called with the elapsed number of ticks
  // since the watchdog was armed, which should exceed the timer period.
  // We don't actually know the elapsed number of ticks, so let's return
  // the timer period plus 1.
  //
  if (mWatchdogNotify != NULL) {
    mWatchdogNotify (mTimerPeriod + 1);
  }

  gRT->ResetSystem (EfiResetCold, EFI_TIMEOUT, 0, NULL);
}

/**
  Stop the SP805 watchdog timer from counting down by disabling interrupts.
**/
STATIC
VOID
SP805Stop (
  VOID
  )
{
  // Disable interrupts
  if ((MmioRead32 (SP805_WDOG_CONTROL_REG) & SP805_WDOG_CTRL_INTEN) != 0) {
    MmioAnd32 (SP805_WDOG_CONTROL_REG, ~SP805_WDOG_CTRL_INTEN);
  }
}

/**
  Starts the SP805 counting down by enabling interrupts.
  The count down will start from the value stored in the Load register,
  not from the value where it was previously stopped.
**/
STATIC
VOID
SP805Start (
  VOID
  )
{
  // Enable interrupts
  if ((MmioRead32 (SP805_WDOG_CONTROL_REG) & SP805_WDOG_CTRL_INTEN) == 0) {
    MmioOr32 (SP805_WDOG_CONTROL_REG, SP805_WDOG_CTRL_INTEN);
  }
}

/**
    On exiting boot services we must make sure the SP805 Watchdog Timer
    is stopped.
**/
STATIC
VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  SP805Unlock ();
  SP805Stop ();
  SP805Lock ();
}

/**
  This function registers the handler NotifyFunction so it is called every time
  the watchdog timer expires.  It also passes the amount of time since the last
  handler call to the NotifyFunction.
  If NotifyFunction is not NULL and a handler is not already registered,
  then the new handler is registered and EFI_SUCCESS is returned.
  If NotifyFunction is NULL, and a handler is already registered,
  then that handler is unregistered.
  If an attempt is made to register a handler when a handler is already registered,
  then EFI_ALREADY_STARTED is returned.
  If an attempt is made to unregister a handler when a handler is not registered,
  then EFI_INVALID_PARAMETER is returned.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  NotifyFunction   The function to call when a timer interrupt fires. This
                           function executes at TPL_HIGH_LEVEL. The DXE Core will
                           register a handler for the timer interrupt, so it can know
                           how much time has passed. This information is used to
                           signal timer based events. NULL will unregister the handler.

  @retval EFI_SUCCESS           The watchdog timer handler was registered.
  @retval EFI_ALREADY_STARTED   NotifyFunction is not NULL, and a handler is already
                                registered.
  @retval EFI_INVALID_PARAMETER NotifyFunction is NULL, and a handler was not
                                previously registered.

**/
STATIC
EFI_STATUS
EFIAPI
SP805RegisterHandler (
  IN EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_WATCHDOG_TIMER_NOTIFY         NotifyFunction
  )
{
  if ((mWatchdogNotify == NULL) && (NotifyFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((mWatchdogNotify != NULL) && (NotifyFunction != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  mWatchdogNotify = NotifyFunction;
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

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod      The rate to program the timer interrupt in 100 nS units. If
                           the timer hardware is not programmable, then EFI_UNSUPPORTED is
                           returned. If the timer is programmable, then the timer period
                           will be rounded up to the nearest timer period that is supported
                           by the timer hardware. If TimerPeriod is set to 0, then the
                           timer interrupts will be disabled.


  @retval EFI_SUCCESS           The timer period was changed.
  @retval EFI_UNSUPPORTED       The platform cannot change the period of the timer interrupt.
  @retval EFI_DEVICE_ERROR      The timer period could not be changed due to a device error.

**/
STATIC
EFI_STATUS
EFIAPI
SP805SetTimerPeriod (
  IN EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                            TimerPeriod          // In 100ns units
  )
{
  EFI_STATUS  Status;
  UINT64      Ticks64bit;

  SP805Unlock ();

  Status = EFI_SUCCESS;

  if (TimerPeriod == 0) {
    // This is a watchdog stop request
    SP805Stop ();
  } else {
    // Calculate the Watchdog ticks required for a delay of (TimerTicks * 100) nanoseconds
    // The SP805 will count down to zero and generate an interrupt.
    //
    // WatchdogTicks = ((TimerPeriod * 100 * SP805_CLOCK_FREQUENCY) / 1GHz);
    //
    // i.e.:
    //
    // WatchdogTicks = (TimerPeriod * SP805_CLOCK_FREQUENCY) / 10 MHz ;

    Ticks64bit = MultU64x32 (TimerPeriod, PcdGet32 (PcdSP805WatchdogClockFrequencyInHz));
    Ticks64bit = DivU64x32 (Ticks64bit, 10 * 1000 * 1000);

    // The registers in the SP805 are only 32 bits
    if (Ticks64bit > MAX_UINT32) {
      // We could load the watchdog with the maximum supported value but
      // if a smaller value was requested, this could have the watchdog
      // triggering before it was intended.
      // Better generate an error to let the caller know.
      Status = EFI_DEVICE_ERROR;
      goto EXIT;
    }

    // Update the watchdog with a 32-bit value.
    MmioWrite32 (SP805_WDOG_LOAD_REG, (UINT32)Ticks64bit);

    // Start the watchdog
    SP805Start ();
  }

  mTimerPeriod = TimerPeriod;

EXIT:
  // Ensure the watchdog is locked before exiting.
  SP805Lock ();
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  This function retrieves the period of timer interrupts in 100 ns units,
  returns that value in TimerPeriod, and returns EFI_SUCCESS.  If TimerPeriod
  is NULL, then EFI_INVALID_PARAMETER is returned.  If a TimerPeriod of 0 is
  returned, then the timer is currently disabled.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod      A pointer to the timer period to retrieve in 100 ns units. If
                           0 is returned, then the timer is currently disabled.


  @retval EFI_SUCCESS           The timer period was returned in TimerPeriod.
  @retval EFI_INVALID_PARAMETER TimerPeriod is NULL.

**/
STATIC
EFI_STATUS
EFIAPI
SP805GetTimerPeriod (
  IN EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *This,
  OUT UINT64                           *TimerPeriod
  )
{
  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerPeriod = mTimerPeriod;
  return EFI_SUCCESS;
}

/**
  Interface structure for the Watchdog Architectural Protocol.

  @par Protocol Description:
  This protocol provides a service to set the amount of time to wait
  before firing the watchdog timer, and it also provides a service to
  register a handler that is invoked when the watchdog timer fires.

  @par When the watchdog timer fires, control will be passed to a handler
  if one has been registered.  If no handler has been registered,
  or the registered handler returns, then the system will be
  reset by calling the Runtime Service ResetSystem().

  @param RegisterHandler
  Registers a handler that will be called each time the
  watchdogtimer interrupt fires.  TimerPeriod defines the minimum
  time between timer interrupts, so TimerPeriod will also
  be the minimum time between calls to the registered
  handler.
  NOTE: If the watchdog resets the system in hardware, then
        this function will not have any chance of executing.

  @param SetTimerPeriod
  Sets the period of the timer interrupt in 100 nS units.
  This function is optional, and may return EFI_UNSUPPORTED.
  If this function is supported, then the timer period will
  be rounded up to the nearest supported timer period.

  @param GetTimerPeriod
  Retrieves the period of the timer interrupt in 100 nS units.

**/
STATIC EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  mWatchdogTimer = {
  SP805RegisterHandler,
  SP805SetTimerPeriod,
  SP805GetTimerPeriod
};

/**
  Initialize the state information for the Watchdog Timer Architectural Protocol.

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
EFIAPI
SP805Initialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  // Find the interrupt controller protocol.  ASSERT if not found.
  Status = gBS->LocateProtocol (
                  &gHardwareInterruptProtocolGuid,
                  NULL,
                  (VOID **)&mInterrupt
                  );
  ASSERT_EFI_ERROR (Status);

  // Unlock access to the SP805 registers
  SP805Unlock ();

  // Stop the watchdog from triggering unexpectedly
  SP805Stop ();

  // Set the watchdog to reset the board when triggered
  // This is a last resort in case the interrupt handler fails
  if ((MmioRead32 (SP805_WDOG_CONTROL_REG) & SP805_WDOG_CTRL_RESEN) == 0) {
    MmioOr32 (SP805_WDOG_CONTROL_REG, SP805_WDOG_CTRL_RESEN);
  }

  // Clear any pending interrupts
  MmioWrite32 (SP805_WDOG_INT_CLR_REG, 0); // write of any value clears the irq

  // Prohibit any rogue access to SP805 registers
  SP805Lock ();

  if (PcdGet32 (PcdSP805WatchdogInterrupt) > 0) {
    Status = mInterrupt->RegisterInterruptSource (
                           mInterrupt,
                           PcdGet32 (PcdSP805WatchdogInterrupt),
                           SP805InterruptHandler
                           );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: failed to register watchdog interrupt - %r\n",
        __FUNCTION__,
        Status
        ));
      return Status;
    }
  } else {
    DEBUG ((
      DEBUG_WARN,
      "%a: no interrupt specified, running in RESET mode only\n",
      __FUNCTION__
      ));
  }

  //
  // Make sure the Watchdog Timer Architectural Protocol has not been installed in the system yet.
  // This will avoid conflicts with the universal watchdog
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiWatchdogTimerArchProtocolGuid);

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_NOTIFY,
                  ExitBootServicesEvent,
                  NULL,
                  &mEfiExitBootServicesEvent
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // Install the Timer Architectural Protocol onto a new handle
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiWatchdogTimerArchProtocolGuid,
                  &mWatchdogTimer,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

EXIT:
  ASSERT_EFI_ERROR (Status);
  return Status;
}
