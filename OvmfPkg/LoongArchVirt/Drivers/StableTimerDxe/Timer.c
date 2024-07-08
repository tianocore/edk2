/** @file
  Timer Architectural Protocol as defined in the DXE CIS

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/Cpu.h>
#include <Protocol/Timer.h>
#include <Register/LoongArch64/Csr.h>

#define DEFAULT_TIMER_TICK_DURATION  100000 // 10ms = 100000 100 ns units

//
// The current period of the timer interrupt
//
STATIC UINT64  mTimerPeriod = 0;
STATIC UINT64  mTimerTicks  = 0;

//
// The handle onto which the Timer Architectural Protocol will be installed
//
STATIC EFI_HANDLE  mTimerHandle             = NULL;
STATIC EFI_EVENT   EfiExitBootServicesEvent = (EFI_EVENT)NULL;

//
// Pointer to the CPU Architectural Protocol instance
//
STATIC EFI_CPU_ARCH_PROTOCOL  *mCpu;

//
// The notification function to call on every timer interrupt.
// A bug in the compiler prevents us from initializing this here.
//
STATIC EFI_TIMER_NOTIFY  mTimerNotifyFunction;

/**
  Sets the counter value for timer.

  @param Count    The 16-bit counter value to program into stable timer.

  @retval VOID
**/
STATIC
VOID
SetPitCount (
  IN UINT64  Count
  )
{
  if (Count <= 4) {
    return;
  }

  Count &= LOONGARCH_CSR_TMCFG_TIMEVAL;
  Count |= LOONGARCH_CSR_TMCFG_EN | LOONGARCH_CSR_TMCFG_PERIOD;
  CsrWrite (LOONGARCH_CSR_TMCFG, Count);
}

/**
  Timer Interrupt Handler.

  @param InterruptType    The type of interrupt that occurred
  @param SystemContext    A pointer to the system context when the interrupt occurred

  @retval VOID
**/
STATIC
VOID
EFIAPI
TimerInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EFI_TPL  OriginalTPL;

  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Clear interrupt.
  //
  CsrWrite (LOONGARCH_CSR_TINTCLR, 0x1);

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
STATIC
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
STATIC
EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  )
{
  UINT64  TimerCount;

  if (TimerPeriod == 0) {
    //
    // Disable timer interrupt for a TimerPeriod of 0
    //
    mCpu->DisableInterrupt (mCpu);
  } else {
    TimerCount = TimerPeriod * GetPerformanceCounterProperties (NULL, NULL) / 10000000ULL;

    if (TimerCount >= BIT48) {
      TimerCount = 0;
    }

    //
    // Program the stable timer with the new count value
    //
    mTimerTicks = TimerCount;
    SetPitCount (TimerCount);

    //
    // Enable timer interrupt
    //
    mCpu->EnableInterrupt (mCpu);
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
STATIC
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
  Disable the timer
  DXE Core will disable the timer after all the event handlers have run.

  @param[in]  Event   The Event that is being processed
  @param[in]  Context Event Context
**/
STATIC
VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  /*
   * Disable timer interrupt when exiting boot service
   */
  CsrWrite (LOONGARCH_CSR_TMCFG, 0x0);
}

/**
  This function generates a soft timer interrupt. If the platform does not support soft
  timer interrupts, then EFI_UNSUPPORTED is returned. Otherwise, EFI_SUCCESS is returned.
  If a handler has been registered through the EFI_TIMER_ARCH_PROTOCOL.RegisterHandler ()
  service, then a soft timer interrupt will be generated. If the timer interrupt is
  enabled when this service is called, then the registered handler will be invoked. The
  registered handler should not be able to distinguish a hardware-generated timer
  interrupt from a software-generated timer interrupt.

  @param This              The EFI_TIMER_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS       The soft timer interrupt was generated.
  @retval EFI_UNSUPPORTED   The platform does not support the generation of soft timer interrupts.
**/
STATIC
EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  )
{
  return EFI_UNSUPPORTED;
}

//
// The Timer Architectural Protocol that this driver produces
//
STATIC EFI_TIMER_ARCH_PROTOCOL  mTimer = {
  TimerDriverRegisterHandler,
  TimerDriverSetTimerPeriod,
  TimerDriverGetTimerPeriod,
  TimerDriverGenerateSoftInterrupt
};

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
StableTimerDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT32      TimerVector;

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
  // Calculate const frequence
  //
  DEBUG ((
    DEBUG_INFO,
    "===========Stable timer freq %d Hz=============\n",
    GetPerformanceCounterProperties (NULL, NULL)
    ));

  //
  // Install interrupt handler for Stable Timer #0 (ISA IRQ0)
  //
  TimerVector = EXCEPT_LOONGARCH_INT_TIMER;
  Status      = mCpu->RegisterInterruptHandler (mCpu, TimerVector, TimerInterruptHandler);
  ASSERT_EFI_ERROR (Status);

  //
  // Enable TI local timer interrupt
  //
  EnableLocalInterrupts (1 << EXCEPT_LOONGARCH_INT_TIMER);

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

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_NOTIFY,
                  ExitBootServicesEvent,
                  NULL,
                  &EfiExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
