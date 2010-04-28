/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Timer.c

Abstract:

  NT Emulation Timer Architectural Protocol Driver as defined in DXE CIS

  This Timer module uses an NT Thread to simulate the timer-tick driven
  timer service.  In the future, the Thread creation should possibly be 
  abstracted by the CPU architectural protocol

**/

#include "Timer.h"

//
// Pointer to the CPU Architectural Protocol instance
//
EFI_CPU_ARCH_PROTOCOL   *mCpu;

//
// The Timer Architectural Protocol that this driver produces
//
EFI_TIMER_ARCH_PROTOCOL mTimer = {
  WinNtTimerDriverRegisterHandler,
  WinNtTimerDriverSetTimerPeriod,
  WinNtTimerDriverGetTimerPeriod,
  WinNtTimerDriverGenerateSoftInterrupt
};

//
// Define a global that we can use to shut down the NT timer thread when
// the timer is canceled.
//
BOOLEAN                 mCancelTimerThread = FALSE;

//
// The notification function to call on every timer interrupt
//
EFI_TIMER_NOTIFY        mTimerNotifyFunction = NULL;

//
// The current period of the timer interrupt
//
UINT64                  mTimerPeriod;

//
// The thread handle for this driver
//
HANDLE                  mNtMainThreadHandle;

//
// The timer value from the last timer interrupt
//
UINT32                  mNtLastTick;

//
// Critical section used to update varibles shared between the main thread and
// the timer interrupt thread.
//
CRITICAL_SECTION        mNtCriticalSection;

//
// Worker Functions
//
UINT                    mMMTimerThreadID = 0;

VOID
CALLBACK
MMTimerThread (
  UINT  wTimerID,
  UINT  msg,
  DWORD dwUser,
  DWORD dw1,
  DWORD dw2
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  wTimerID  - TODO: add argument description
  msg       - TODO: add argument description
  dwUser    - TODO: add argument description
  dw1       - TODO: add argument description
  dw2       - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  EFI_TPL           OriginalTPL;
  UINT32            CurrentTick;
  UINT32            Delta;
  EFI_TIMER_NOTIFY  CallbackFunction;
  BOOLEAN           InterruptState;

  if (!mCancelTimerThread) {
  
    //
    // Suspend the main thread until we are done.
    // Enter the critical section before suspending
    // and leave the critical section after resuming
    // to avoid deadlock between main and timer thread.
    //
    gWinNt->EnterCriticalSection (&mNtCriticalSection);
    gWinNt->SuspendThread (mNtMainThreadHandle);

    //
    // If the timer thread is being canceled, then bail immediately.
    // We check again here because there's a small window of time from when
    // this thread was kicked off and when we suspended the main thread above.
    //
    if (mCancelTimerThread) {
      gWinNt->ResumeThread (mNtMainThreadHandle);
      gWinNt->LeaveCriticalSection (&mNtCriticalSection);
      gWinNt->timeKillEvent (wTimerID);
      mMMTimerThreadID = 0;
      return ;
    }

    mCpu->GetInterruptState (mCpu, &InterruptState);
    while (!InterruptState) {
      //
      //  Resume the main thread
      //
      gWinNt->ResumeThread (mNtMainThreadHandle);
      gWinNt->LeaveCriticalSection (&mNtCriticalSection);

      //
      //  Wait for interrupts to be enabled.
      //
      mCpu->GetInterruptState (mCpu, &InterruptState);
      while (!InterruptState) {
        gWinNt->Sleep (1);
        mCpu->GetInterruptState (mCpu, &InterruptState);
      }
       
      //
      //  Suspend the main thread until we are done
      //
      gWinNt->EnterCriticalSection (&mNtCriticalSection);
      gWinNt->SuspendThread (mNtMainThreadHandle);
      mCpu->GetInterruptState (mCpu, &InterruptState);
    }

    //
    //  Get the current system tick
    //
    CurrentTick = gWinNt->GetTickCount ();
    Delta       = CurrentTick - mNtLastTick;
    mNtLastTick = CurrentTick;

    //
    //  If delay was more then 1 second, ignore it (probably debugging case)
    //
    if (Delta < 1000) {

      OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

      //
      //  Inform the firmware of an "timer interrupt".  The time
      //  expired since the last call is 10,000 times the number
      //  of ms.  (or 100ns units)
      //
      CallbackFunction = mTimerNotifyFunction;

      //
      // Only invoke the callback function if a Non-NULL handler has been
      // registered. Assume all other handlers are legal.
      //
      if (CallbackFunction != NULL) {
        CallbackFunction ((UINT64) (Delta * 10000));
      }

      gBS->RestoreTPL (OriginalTPL);

    }

    //
    //  Resume the main thread
    //
    gWinNt->ResumeThread (mNtMainThreadHandle);
    gWinNt->LeaveCriticalSection (&mNtCriticalSection);
  } else {
    gWinNt->timeKillEvent (wTimerID);
    mMMTimerThreadID = 0;
  }

}

UINT
CreateNtTimer (
  VOID
  )
/*++

Routine Description:

   It is used to emulate a platform 
  timer-driver interrupt handler.  

Returns:

  Timer ID

--*/
// TODO: function comment is missing 'Arguments:'
{
  UINT32  SleepCount;

  //
  //  Set our thread priority higher than the "main" thread.
  //
  gWinNt->SetThreadPriority (
            gWinNt->GetCurrentThread (),
            THREAD_PRIORITY_HIGHEST
            );

  //
  //  Calc the appropriate interval
  //
  gWinNt->EnterCriticalSection (&mNtCriticalSection);
  SleepCount = (UINT32) (mTimerPeriod + 5000) / 10000;
  gWinNt->LeaveCriticalSection (&mNtCriticalSection);

  return gWinNt->timeSetEvent (
                  SleepCount,
                  0,
                  MMTimerThread,
                  (DWORD_PTR) NULL,
                  TIME_PERIODIC | TIME_KILL_SYNCHRONOUS | TIME_CALLBACK_FUNCTION
                  );

}

EFI_STATUS
EFIAPI
WinNtTimerDriverRegisterHandler (
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

--*/
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

  //
  // Use Critical Section to update the notification function that is
  // used from the timer interrupt thread.
  //
  gWinNt->EnterCriticalSection (&mNtCriticalSection);

  mTimerNotifyFunction = NotifyFunction;

  gWinNt->LeaveCriticalSection (&mNtCriticalSection);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtTimerDriverSetTimerPeriod (
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

--*/
{

  //
  // If TimerPeriod is 0, then the timer thread should be canceled
  //
  if (TimerPeriod == 0) {
    //
    // Cancel the timer thread
    //
    gWinNt->EnterCriticalSection (&mNtCriticalSection);

    mCancelTimerThread = TRUE;

    gWinNt->LeaveCriticalSection (&mNtCriticalSection);

    //
    // Wait for the timer thread to exit
    //

    if (mMMTimerThreadID) {
      gWinNt->timeKillEvent (mMMTimerThreadID);
    }

    mMMTimerThreadID = 0;

    //
    // Update the timer period
    //
    gWinNt->EnterCriticalSection (&mNtCriticalSection);

    mTimerPeriod = TimerPeriod;

    gWinNt->LeaveCriticalSection (&mNtCriticalSection);

    //
    // NULL out the thread handle so it will be re-created if the timer is enabled again
    //

  } else if ((TimerPeriod > TIMER_MINIMUM_VALUE) && (TimerPeriod < TIMER_MAXIMUM_VALUE)) {
    //
    // If the TimerPeriod is valid, then create and/or adjust the period of the timer thread
    //
    gWinNt->EnterCriticalSection (&mNtCriticalSection);

    mTimerPeriod        = TimerPeriod;

    mCancelTimerThread  = FALSE;

    gWinNt->LeaveCriticalSection (&mNtCriticalSection);

    //
    //  Get the starting tick location if we are just starting the timer thread
    //
    mNtLastTick = gWinNt->GetTickCount ();

    if (mMMTimerThreadID) {
      gWinNt->timeKillEvent (mMMTimerThreadID);
    }

    mMMTimerThreadID  = 0;

    mMMTimerThreadID  = CreateNtTimer ();

  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtTimerDriverGetTimerPeriod (
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

--*/
{
  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerPeriod = mTimerPeriod;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtTimerDriverGenerateSoftInterrupt (
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

  EFI_UNSUPPORTEDT  - The platform does not support the generation of soft timer interrupts.

--*/
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
WinNtTimerDriverInitialize (
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
  
  EFI_DEVICE_ERROR      - A device error occured attempting to initialize the driver.

--*/
{
  EFI_STATUS  Status;
  UINTN       Result;
  EFI_HANDLE  Handle;
  EFI_HANDLE  hSourceProcessHandle;
  EFI_HANDLE  hSourceHandle;
  EFI_HANDLE  hTargetProcessHandle;
  //
  // Make sure the Timer Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiTimerArchProtocolGuid);

  //
  // Get the CPU Architectural Protocol instance
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID**)&mCpu);
  ASSERT_EFI_ERROR (Status);

  //
  //  Get our handle so the timer tick thread can suspend
  //
  hSourceProcessHandle = gWinNt->GetCurrentProcess ();
  hSourceHandle        = gWinNt->GetCurrentThread ();
  hTargetProcessHandle = gWinNt->GetCurrentProcess ();
  Result = gWinNt->DuplicateHandle (
                    hSourceProcessHandle,
                    hSourceHandle,
                    hTargetProcessHandle,
                    &mNtMainThreadHandle,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS
                    );
  if (Result == 0) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Initialize Critical Section used to update variables shared between the main
  // thread and the timer interrupt thread.
  //
  gWinNt->InitializeCriticalSection (&mNtCriticalSection);

  //
  // Start the timer thread at the default timer period
  //
  Status = mTimer.SetTimerPeriod (&mTimer, DEFAULT_TIMER_TICK_DURATION);
  if (EFI_ERROR (Status)) {
    gWinNt->DeleteCriticalSection (&mNtCriticalSection);
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
    //
    // Cancel the timer
    //
    mTimer.SetTimerPeriod (&mTimer, 0);
    gWinNt->DeleteCriticalSection (&mNtCriticalSection);
    return Status;
  }

  return EFI_SUCCESS;
}
