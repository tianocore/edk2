/** @file
  Template for Timer Architecture Protocol driver of the ARM flavor

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/OmapLib.h>

#include <Protocol/Timer.h>
#include <Protocol/HardwareInterrupt.h>
#include <Protocol/TimerDebugSupport.h>

#include <Omap3530/Omap3530.h>


// The notification function to call on every timer interrupt.
volatile EFI_TIMER_NOTIFY      mTimerNotifyFunction   = (EFI_TIMER_NOTIFY)NULL;
volatile EFI_PERIODIC_CALLBACK mTimerPeriodicCallback = (EFI_PERIODIC_CALLBACK)NULL;


// The current period of the timer interrupt
volatile UINT64 mTimerPeriod = 0;

// Cached copy of the Hardware Interrupt protocol instance
EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterrupt = NULL;

// Cached registers
volatile UINT32 TISR;
volatile UINT32 TCLR;
volatile UINT32 TLDR;
volatile UINT32 TCRR;
volatile UINT32 TIER;

// Cached interrupt vector
volatile UINTN  gVector;

VOID
EFIAPI
TimerInterruptHandler (
  IN  HARDWARE_INTERRUPT_SOURCE   Source,
  IN  EFI_SYSTEM_CONTEXT          SystemContext       
  )
{
  if (mTimerPeriodicCallback) {
    mTimerPeriodicCallback(SystemContext);
  }

  if (mTimerNotifyFunction) {
    mTimerNotifyFunction(mTimerPeriod);
  }

  // Clear all timer interrupts
  MmioWrite32(TISR, TISR_CLEAR_ALL);  

  // Poll interrupt status bits to ensure clearing
  while ((MmioRead32(TISR) & TISR_ALL_INTERRUPT_MASK) != TISR_NO_INTERRUPTS_PENDING);
}

EFI_STATUS
EFIAPI
TimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  )
{
  if ((NotifyFunction == NULL) && (mTimerNotifyFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NotifyFunction != NULL) && (mTimerNotifyFunction != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  mTimerNotifyFunction = NotifyFunction;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  )
{
  EFI_STATUS  Status;
  UINT64      TimerCount;
  INT32       LoadValue;
  
  if (TimerPeriod == 0) {
    // Turn off GPTIMER3
    MmioWrite32(TCLR, TCLR_ST_OFF);
    
    Status = gInterrupt->DisableInterruptSource(gInterrupt, gVector);    
  } else {  
    // Calculate required timer count
    TimerCount = DivU64x32(TimerPeriod * 100, PcdGet32(PcdEmbeddedFdPerformanceCounterPeriodInNanoseconds));

    // Set GPTIMER3 Load register
    LoadValue = (INT32) -TimerCount;
    MmioWrite32(TLDR, LoadValue);
    MmioWrite32(TCRR, LoadValue);

    // Enable Overflow interrupt
    MmioWrite32(TIER, TIER_TCAR_IT_DISABLE | TIER_OVF_IT_ENABLE | TIER_MAT_IT_DISABLE);

    // Turn on GPTIMER3, it will reload at overflow
    MmioWrite32(TCLR, TCLR_AR_AUTORELOAD | TCLR_ST_ON);

    Status = gInterrupt->EnableInterruptSource(gInterrupt, gVector);    
  }

  //
  // Save the new timer period
  //
  mTimerPeriod = TimerPeriod;
  return Status;
}

EFI_STATUS
EFIAPI
TimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                   *TimerPeriod
  )
{
  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerPeriod = mTimerPeriod;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
TimerDriverRegisterPeriodicCallback (
  IN  TIMER_DEBUG_SUPPORT_PROTOCOL  *This,
  IN  EFI_PERIODIC_CALLBACK         PeriodicCallback
  )
{
  if ((PeriodicCallback == NULL) && (mTimerPeriodicCallback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((PeriodicCallback != NULL) && (mTimerPeriodicCallback != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  mTimerPeriodicCallback = PeriodicCallback;

  return EFI_SUCCESS;
}

EFI_TIMER_ARCH_PROTOCOL   gTimer = {
  TimerDriverRegisterHandler,
  TimerDriverSetTimerPeriod,
  TimerDriverGetTimerPeriod,
  TimerDriverGenerateSoftInterrupt
};

TIMER_DEBUG_SUPPORT_PROTOCOL  gTimerDebugSupport = {
  TimerDriverRegisterPeriodicCallback
};

EFI_STATUS
EFIAPI
TimerInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_HANDLE  Handle = NULL;
  EFI_STATUS  Status;
  UINT32      TimerBaseAddress;

  // Find the interrupt controller protocol.  ASSERT if not found.
  Status = gBS->LocateProtocol(&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  ASSERT_EFI_ERROR (Status);

  // Set up the timer registers
  TimerBaseAddress = TimerBase(FixedPcdGet32(PcdBeagleArchTimer));
  TISR = TimerBaseAddress + GPTIMER_TISR;
  TCLR = TimerBaseAddress + GPTIMER_TCLR;
  TLDR = TimerBaseAddress + GPTIMER_TLDR;
  TCRR = TimerBaseAddress + GPTIMER_TCRR;
  TIER = TimerBaseAddress + GPTIMER_TIER;

  // Disable the timer
  Status = TimerDriverSetTimerPeriod(&gTimer, 0);
  ASSERT_EFI_ERROR (Status);

  // Install interrupt handler
  gVector = InterruptVectorForTimer(FixedPcdGet32(PcdBeagleArchTimer));
  Status = gInterrupt->RegisterInterruptSource(gInterrupt, gVector, TimerInterruptHandler);
  ASSERT_EFI_ERROR (Status);

  // Set up default timer
  Status = TimerDriverSetTimerPeriod(&gTimer, FixedPcdGet32(PcdTimerPeriod));
  ASSERT_EFI_ERROR (Status);

  // Install the Timer Architectural Protocol onto a new handle
  Status = gBS->InstallMultipleProtocolInterfaces(&Handle,
                                                  &gEfiTimerArchProtocolGuid,      &gTimer,
                                                  &gTimerDebugSupportProtocolGuid, &gTimerDebugSupport,
                                                  NULL);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

