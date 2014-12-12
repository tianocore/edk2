/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD
*  License which accompanies this distribution.  The full text of the license
*  may be found at http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/ArmGenericTimerCounterLib.h>

#include <Protocol/WatchdogTimer.h>
#include <Protocol/HardwareInterrupt.h>

#include "GenericWatchdog.h"

// The number of 100ns periods (the unit of time passed to these functions)
// in a second
#define TIME_UNITS_PER_SECOND 10000000

// Tick frequency of the generic timer that is the basis of the generic watchdog
UINTN mTimerFrequencyHz = 0;

// In cases where the compare register was set manually, information about
// how long the watchdog was asked to wait cannot be retrieved from hardware.
// It is therefore stored here. 0 means the timer is not running.
UINT64 mNumTimerTicks = 0;

EFI_HARDWARE_INTERRUPT_PROTOCOL *mInterruptProtocol;

EFI_STATUS
WatchdogWriteOffsetRegister (
  UINT32  Value
  )
{
  return MmioWrite32 (GENERIC_WDOG_OFFSET_REG, Value);
}

EFI_STATUS
WatchdogWriteCompareRegister (
  UINT64  Value
  )
{
  return MmioWrite64 (GENERIC_WDOG_COMPARE_VALUE_REG, Value);
}

EFI_STATUS
WatchdogEnable (
  VOID
  )
{
  return MmioWrite32 (GENERIC_WDOG_CONTROL_STATUS_REG, GENERIC_WDOG_ENABLED);
}

EFI_STATUS
WatchdogDisable (
  VOID
  )
{
  return MmioWrite32 (GENERIC_WDOG_CONTROL_STATUS_REG, GENERIC_WDOG_DISABLED);
}

/**
    On exiting boot services we must make sure the Watchdog Timer
    is stopped.
**/
VOID
EFIAPI
WatchdogExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  WatchdogDisable ();
  mNumTimerTicks = 0;
}

/*
  This function is called when the watchdog's first signal (WS0) goes high.
  It uses the ResetSystem Runtime Service to reset the board.
*/
VOID
EFIAPI
WatchdogInterruptHandler (
  IN  HARDWARE_INTERRUPT_SOURCE   Source,
  IN  EFI_SYSTEM_CONTEXT          SystemContext
  )
{
  STATIC CONST CHAR16      ResetString[] = L"The generic watchdog timer ran out.";

  WatchdogDisable ();

  mInterruptProtocol->EndOfInterrupt (mInterruptProtocol, Source);

  gRT->ResetSystem (
         EfiResetCold,
         EFI_TIMEOUT,
         StrSize (ResetString),
         &ResetString
         );

  // If we got here then the reset didn't work
  ASSERT (FALSE);
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
  @param  NotifyFunction   The function to call when a timer interrupt fires.
                           This function executes at TPL_HIGH_LEVEL. The DXE
                           Core will register a handler for the timer interrupt,
                           so it can know how much time has passed. This
                           information is used to signal timer based events.
                           NULL will unregister the handler.

  @retval EFI_SUCCESS           The watchdog timer handler was registered.
  @retval EFI_ALREADY_STARTED   NotifyFunction is not NULL, and a handler is already
                                registered.
  @retval EFI_INVALID_PARAMETER NotifyFunction is NULL, and a handler was not
                                previously registered.

**/
EFI_STATUS
EFIAPI
WatchdogRegisterHandler (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  IN EFI_WATCHDOG_TIMER_NOTIFY                NotifyFunction
  )
{
  // ERROR: This function is not supported.
  // The watchdog will reset the board
  return EFI_UNSUPPORTED;
}

/**
  This function sets the amount of time to wait before firing the watchdog
  timer to TimerPeriod 100 nS units.  If TimerPeriod is 0, then the watchdog
  timer is disabled.

  @param  This             The EFI_WATCHDOG_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod      The amount of time in 100 nS units to wait before the watchdog
                           timer is fired. If TimerPeriod is zero, then the watchdog
                           timer is disabled.

  @retval EFI_SUCCESS           The watchdog timer has been programmed to fire in Time
                                100 nS units.
  @retval EFI_DEVICE_ERROR      A watchdog timer could not be programmed due to a device
                                error.

**/
EFI_STATUS
EFIAPI
WatchdogSetTimerPeriod (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  IN UINT64                                   TimerPeriod   // In 100ns units
  )
{
  UINTN       SystemCount;
  EFI_STATUS  Status;

  // if TimerPerdiod is 0, this is a request to stop the watchdog.
  if (TimerPeriod == 0) {
    mNumTimerTicks = 0;
    return WatchdogDisable ();
  }

  // Work out how many timer ticks will equate to TimerPeriod
  mNumTimerTicks = (mTimerFrequencyHz * TimerPeriod) / TIME_UNITS_PER_SECOND;

  //
  // If the number of required ticks is greater than the max number the
  // watchdog's offset register (WOR) can hold, we need to manually compute and
  // set the compare register (WCV)
  //
  if (mNumTimerTicks > MAX_UINT32) {
    //
    // We need to enable the watchdog *before* writing to the compare register,
    // because enabling the watchdog causes an "explicit refresh", which
    // clobbers the compare register (WCV). In order to make sure this doesn't
    // trigger an interrupt, set the offset to max.
    //
    Status = WatchdogWriteOffsetRegister (MAX_UINT32);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    WatchdogEnable ();
    SystemCount = ArmGenericTimerGetSystemCount ();
    Status      = WatchdogWriteCompareRegister (SystemCount + mNumTimerTicks);
  } else {
    Status = WatchdogWriteOffsetRegister ((UINT32)mNumTimerTicks);
    WatchdogEnable ();
  }

  return Status;
}

/**
  This function retrieves the period of timer interrupts in 100 ns units,
  returns that value in TimerPeriod, and returns EFI_SUCCESS.  If TimerPeriod
  is NULL, then EFI_INVALID_PARAMETER is returned.  If a TimerPeriod of 0 is
  returned, then the timer is currently disabled.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod      A pointer to the timer period to retrieve in 100
                           ns units. If 0 is returned, then the timer is
                           currently disabled.


  @retval EFI_SUCCESS           The timer period was returned in TimerPeriod.
  @retval EFI_INVALID_PARAMETER TimerPeriod is NULL.

**/
EFI_STATUS
EFIAPI
WatchdogGetTimerPeriod (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                                  *TimerPeriod
  )
{
  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerPeriod = ((TIME_UNITS_PER_SECOND / mTimerFrequencyHz) * mNumTimerTicks);

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
EFI_WATCHDOG_TIMER_ARCH_PROTOCOL    gWatchdogTimer = {
  (EFI_WATCHDOG_TIMER_REGISTER_HANDLER) WatchdogRegisterHandler,
  (EFI_WATCHDOG_TIMER_SET_TIMER_PERIOD) WatchdogSetTimerPeriod,
  (EFI_WATCHDOG_TIMER_GET_TIMER_PERIOD) WatchdogGetTimerPeriod
};

EFI_EVENT                           EfiExitBootServicesEvent = (EFI_EVENT)NULL;

EFI_STATUS
EFIAPI
GenericWatchdogEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      Handle;

  //
  // Make sure the Watchdog Timer Architectural Protocol has not been installed
  // in the system yet.
  // This will avoid conflicts with the universal watchdog
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiWatchdogTimerArchProtocolGuid);

  mTimerFrequencyHz = ArmGenericTimerGetTimerFreq ();
  ASSERT (mTimerFrequencyHz != 0);

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY,
                  WatchdogExitBootServicesEvent, NULL, &EfiExitBootServicesEvent
                  );
  if (!EFI_ERROR (Status)) {
    // Install interrupt handler
    Status = gBS->LocateProtocol (
                    &gHardwareInterruptProtocolGuid,
                    NULL,
                    (VOID **)&mInterruptProtocol
                    );
    if (!EFI_ERROR (Status)) {
      Status = mInterruptProtocol->RegisterInterruptSource (
                                    mInterruptProtocol,
                                    FixedPcdGet32 (PcdGenericWatchdogEl2IntrNum),
                                    WatchdogInterruptHandler
                                    );
      if (!EFI_ERROR (Status)) {
        // Install the Timer Architectural Protocol onto a new handle
        Handle = NULL;
        Status = gBS->InstallMultipleProtocolInterfaces (
                        &Handle,
                        &gEfiWatchdogTimerArchProtocolGuid, &gWatchdogTimer,
                        NULL
                        );
      }
    }
  }

  if (EFI_ERROR (Status)) {
    // The watchdog failed to initialize
    ASSERT (FALSE);
  }

  mNumTimerTicks = 0;
  WatchdogDisable ();

  return Status;
}
