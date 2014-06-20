/** @file
  Timer Architectural Protocol module using High Precesion Event Timer (HPET)

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Protocol/Cpu.h>
#include <Protocol/Timer.h>

#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/LocalApicLib.h>
#include <Library/IoApicLib.h>

#include <Register/LocalApic.h>
#include <Register/IoApic.h>
#include <Register/Hpet.h>

///
/// Define value for an invalid HPET Timer index.
///
#define HPET_INVALID_TIMER_INDEX  0xff

///
/// Timer Architectural Protocol function prototypes.
///

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

  @param  This            The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  NotifyFunction  The function to call when a timer interrupt fires.  
                          This function executes at TPL_HIGH_LEVEL.  The DXE 
                          Core will register a handler for the timer interrupt, 
                          so it can know how much time has passed.  This 
                          information is used to signal timer based events.  
                          NULL will unregister the handler.

  @retval  EFI_SUCCESS            The timer handler was registered.
  @retval  EFI_UNSUPPORTED        The platform does not support timer interrupts.
  @retval  EFI_ALREADY_STARTED    NotifyFunction is not NULL, and a handler is already
                                  registered.
  @retval  EFI_INVALID_PARAMETER  NotifyFunction is NULL, and a handler was not
                                  previously registered.
  @retval  EFI_DEVICE_ERROR       The timer handler could not be registered.

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

  @param  This         The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod  The rate to program the timer interrupt in 100 nS units.
                       If the timer hardware is not programmable, then 
                       EFI_UNSUPPORTED is returned.  If the timer is programmable, 
                       then the timer period will be rounded up to the nearest 
                       timer period that is supported by the timer hardware.  
                       If TimerPeriod is set to 0, then the timer interrupts 
                       will be disabled.

  @retval  EFI_SUCCESS       The timer period was changed.
  @retval  EFI_UNSUPPORTED   The platform cannot change the period of the timer interrupt.
  @retval  EFI_DEVICE_ERROR  The timer period could not be changed due to a device error.

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

  @param  This         The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod  A pointer to the timer period to retrieve in 100 ns units.
                       If 0 is returned, then the timer is currently disabled.

  @retval  EFI_SUCCESS            The timer period was returned in TimerPeriod.
  @retval  EFI_INVALID_PARAMETER  TimerPeriod is NULL.

**/
EFI_STATUS
EFIAPI
TimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                   *TimerPeriod
  );

/**
  This function generates a soft timer interrupt. If the platform does not support soft
  timer interrupts, then EFI_UNSUPPORTED is returned. Otherwise, EFI_SUCCESS is returned.
  If a handler has been registered through the EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()
  service, then a soft timer interrupt will be generated. If the timer interrupt is
  enabled when this service is called, then the registered handler will be invoked. The
  registered handler should not be able to distinguish a hardware-generated timer
  interrupt from a software-generated timer interrupt.

  @param  This  The EFI_TIMER_ARCH_PROTOCOL instance.

  @retval  EFI_SUCCESS       The soft timer interrupt was generated.
  @retval  EFI_UNSUPPORTEDT  The platform does not support the generation of soft 
                             timer interrupts.

**/
EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  );
  
///
/// The handle onto which the Timer Architectural Protocol will be installed.
///
EFI_HANDLE   mTimerHandle = NULL;

///
/// The Timer Architectural Protocol that this driver produces.
///
EFI_TIMER_ARCH_PROTOCOL  mTimer = {
  TimerDriverRegisterHandler,
  TimerDriverSetTimerPeriod,
  TimerDriverGetTimerPeriod,
  TimerDriverGenerateSoftInterrupt
};

///
/// Pointer to the CPU Architectural Protocol instance.
///
EFI_CPU_ARCH_PROTOCOL  *mCpu = NULL;

///
/// The notification function to call on every timer interrupt.
///
EFI_TIMER_NOTIFY  mTimerNotifyFunction = NULL;

///
/// The current period of the HPET timer interrupt in 100 ns units.
///
UINT64  mTimerPeriod = 0;

///
/// The number of HPET timer ticks required for the current HPET rate specified by mTimerPeriod.
///
UINT64  mTimerCount;

///
/// Mask used for counter and comparator calculations to adjust for a 32-bit or 64-bit counter.
///
UINT64  mCounterMask;

///
/// The HPET main counter value from the most recent HPET timer interrupt.
///
volatile UINT64  mPreviousMainCounter;

volatile UINT64  mPreviousComparator;

///
/// The index of the HPET timer being managed by this driver.
///
UINTN  mTimerIndex;

///
/// The I/O APIC IRQ that the HPET Timer is mapped if I/O APIC mode is used.
///
UINT32  mTimerIrq;

///
/// Cached state of the HPET General Capabilities register managed by this driver.
/// Caching the state reduces the number of times the configuration register is read.
///
HPET_GENERAL_CAPABILITIES_ID_REGISTER  mHpetGeneralCapabilities;

///
/// Cached state of the HPET General Configuration register managed by this driver.
/// Caching the state reduces the number of times the configuration register is read.
///
HPET_GENERAL_CONFIGURATION_REGISTER  mHpetGeneralConfiguration;

///
/// Cached state of the Configuration register for the HPET Timer managed by 
/// this driver.  Caching the state reduces the number of times the configuration
/// register is read.
///
HPET_TIMER_CONFIGURATION_REGISTER  mTimerConfiguration;

///
/// Counts the number of HPET Timer interrupts processed by this driver.
/// Only required for debug.
///
volatile UINTN  mNumTicks;

/**
  Read a 64-bit register from the HPET

  @param  Offset  Specifies the offset of the HPET register to read.

  @return  The 64-bit value read from the HPET register specified by Offset.
**/
UINT64
HpetRead (
  IN UINTN  Offset
  )
{
  return MmioRead64 (PcdGet32 (PcdHpetBaseAddress) + Offset);
}

/**
  Write a 64-bit HPET register.

  @param  Offset  Specifies the ofsfert of the HPET register to write.
  @param  Value   Specifies the value to write to the HPET register specified by Offset.

  @return  The 64-bit value written to HPET register specified by Offset.
**/
UINT64
HpetWrite (
  IN UINTN   Offset,
  IN UINT64  Value
  )
{
  return MmioWrite64 (PcdGet32 (PcdHpetBaseAddress) + Offset, Value);
}

/**
  Enable or disable the main counter in the HPET Timer.

  @param  Enable  If TRUE, then enable the main counter in the HPET Timer.
                  If FALSE, then disable the main counter in the HPET Timer.
**/
VOID
HpetEnable (
  IN BOOLEAN  Enable
  )
{
  mHpetGeneralConfiguration.Bits.MainCounterEnable = Enable ? 1 : 0;  
  HpetWrite (HPET_GENERAL_CONFIGURATION_OFFSET, mHpetGeneralConfiguration.Uint64);
}

/**
  The interrupt handler for the HPET timer.  This handler clears the HPET interrupt
  and computes the amount of time that has passed since the last HPET timer interrupt.
  If a notification function is registered, then the amount of time since the last
  HPET interrupt is passed to that notification function in 100 ns units.  The HPET
  time is updated to generate another interrupt in the required time period. 

  @param  InterruptType  The type of interrupt that occured.
  @param  SystemContext  A pointer to the system context when the interrupt occured.
**/
VOID
EFIAPI
TimerInterruptHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINT64  MainCounter;
  UINT64  Comparator;
  UINT64  TimerPeriod;
  UINT64  Delta;

  //
  // Count number of ticks
  //
  DEBUG_CODE (mNumTicks++;);

  //
  // Clear HPET timer interrupt status
  //
  HpetWrite (HPET_GENERAL_INTERRUPT_STATUS_OFFSET, LShiftU64 (1, mTimerIndex));

  //
  // Local APIC EOI
  //
  SendApicEoi ();

  //
  // Disable HPET timer when adjusting the COMPARATOR value to prevent a missed interrupt
  //
  HpetEnable (FALSE);
  
  //
  // Capture main counter value
  //
  MainCounter = HpetRead (HPET_MAIN_COUNTER_OFFSET);

  //
  // Get the previous comparator counter
  //
  mPreviousComparator = HpetRead (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE);

  //
  // Set HPET COMPARATOR to the value required for the next timer tick
  //
  Comparator = (mPreviousComparator + mTimerCount) & mCounterMask;

  if ((mPreviousMainCounter < MainCounter) && (mPreviousComparator > Comparator)) {
    //
    // When comparator overflows
    //
    HpetWrite (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, Comparator);
  } else if ((mPreviousMainCounter > MainCounter) && (mPreviousComparator < Comparator)) {
    //
    // When main counter overflows
    //
    HpetWrite (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, (MainCounter + mTimerCount) & mCounterMask);
  } else {
    //
    // When both main counter and comparator do not overflow or both do overflow
    //
    if (Comparator > MainCounter) {
      HpetWrite (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, Comparator);
    } else {
      HpetWrite (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, (MainCounter + mTimerCount) & mCounterMask);
    }
  }

  //
  // Enable the HPET counter once the new COMPARATOR value has been set.
  //
  HpetEnable (TRUE);
  
  //
  // Check to see if there is a registered notification function
  //
  if (mTimerNotifyFunction != NULL) {
    //
    // Compute time since last notification in 100 ns units (10 ^ -7) 
    //
    if (MainCounter > mPreviousMainCounter) {
      //
      // Main counter does not overflow
      //
      Delta = MainCounter - mPreviousMainCounter;
    } else {
      //
      // Main counter overflows, first usb, then add
      //
      Delta = (mCounterMask - mPreviousMainCounter) + MainCounter;
    }
    TimerPeriod = DivU64x32 (
                    MultU64x32 (
                      Delta & mCounterMask,
                      mHpetGeneralCapabilities.Bits.CounterClockPeriod
                      ), 
                    100000000
                    );
                    
    //
    // Call registered notification function passing in the time since the last
    // interrupt in 100 ns units.
    //    
    mTimerNotifyFunction (TimerPeriod);
  }
  
  //
  // Save main counter value
  //
  mPreviousMainCounter = MainCounter;
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

  @param  This            The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  NotifyFunction  The function to call when a timer interrupt fires.  
                          This function executes at TPL_HIGH_LEVEL.  The DXE 
                          Core will register a handler for the timer interrupt, 
                          so it can know how much time has passed.  This 
                          information is used to signal timer based events.  
                          NULL will unregister the handler.

  @retval  EFI_SUCCESS            The timer handler was registered.
  @retval  EFI_UNSUPPORTED        The platform does not support timer interrupts.
  @retval  EFI_ALREADY_STARTED    NotifyFunction is not NULL, and a handler is already
                                  registered.
  @retval  EFI_INVALID_PARAMETER  NotifyFunction is NULL, and a handler was not
                                  previously registered.
  @retval  EFI_DEVICE_ERROR       The timer handler could not be registered.

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
  if (NotifyFunction == NULL && mTimerNotifyFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (NotifyFunction != NULL && mTimerNotifyFunction != NULL) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Cache the registered notification function
  //
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

  @param  This         The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod  The rate to program the timer interrupt in 100 nS units.
                       If the timer hardware is not programmable, then 
                       EFI_UNSUPPORTED is returned.  If the timer is programmable, 
                       then the timer period will be rounded up to the nearest 
                       timer period that is supported by the timer hardware.  
                       If TimerPeriod is set to 0, then the timer interrupts 
                       will be disabled.

  @retval  EFI_SUCCESS       The timer period was changed.
  @retval  EFI_UNSUPPORTED   The platform cannot change the period of the timer interrupt.
  @retval  EFI_DEVICE_ERROR  The timer period could not be changed due to a device error.

**/
EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  )
{
  UINT64                         MainCounter;
  UINT64                         Delta;
  UINT64                         CurrentComparator;
  HPET_TIMER_MSI_ROUTE_REGISTER  HpetTimerMsiRoute;
  
  //
  // Disable HPET timer when adjusting the timer period
  //
  HpetEnable (FALSE);
  
  if (TimerPeriod == 0) {
    if (mTimerPeriod != 0) {
      //
      // Check if there is possibly a pending interrupt
      //
      MainCounter = HpetRead (HPET_MAIN_COUNTER_OFFSET);
      if (MainCounter < mPreviousMainCounter) {
        Delta = (mCounterMask - mPreviousMainCounter) + MainCounter;
      } else { 
        Delta = MainCounter - mPreviousMainCounter;
      }
      if ((Delta & mCounterMask) >= mTimerCount) {
        //
        // Interrupt still happens after disable HPET, wait to be processed
        // Wait until interrupt is processed and comparator is increased
        //
        CurrentComparator = HpetRead (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE);
        while (CurrentComparator == mPreviousComparator) {
          CurrentComparator = HpetRead (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE);
          CpuPause();
        }
      }
    }

    //
    // If TimerPeriod is 0, then mask HPET Timer interrupts
    //
    
    if (mTimerConfiguration.Bits.MsiInterruptCapablity != 0 && FeaturePcdGet (PcdHpetMsiEnable)) {
      //
      // Disable HPET MSI interrupt generation
      //
      mTimerConfiguration.Bits.MsiInterruptEnable = 0;
    } else {
      //
      // Disable I/O APIC Interrupt
      //
      IoApicEnableInterrupt (mTimerIrq, FALSE);
    }
    
    //
    // Disable HPET timer interrupt 
    //
    mTimerConfiguration.Bits.InterruptEnable = 0;
    HpetWrite (HPET_TIMER_CONFIGURATION_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, mTimerConfiguration.Uint64);
  } else {
    //
    // Convert TimerPeriod to femtoseconds and divide by the number if femtoseconds 
    // per tick of the HPET counter to determine the number of HPET counter ticks
    // in TimerPeriod 100 ns units.
    // 
    mTimerCount = DivU64x32 (
                    MultU64x32 (TimerPeriod, 100000000),
                    mHpetGeneralCapabilities.Bits.CounterClockPeriod
                    );

    //
    // Program the HPET Comparator with the number of ticks till the next interrupt
    //
    MainCounter = HpetRead (HPET_MAIN_COUNTER_OFFSET);
    if (MainCounter > mPreviousMainCounter) {
      Delta = MainCounter - mPreviousMainCounter;
    } else { 
      Delta = (mCounterMask - mPreviousMainCounter) + MainCounter;
    }
    if ((Delta & mCounterMask) >= mTimerCount) {
      HpetWrite (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, (MainCounter + 1) & mCounterMask);
    } else {  
      HpetWrite (HPET_TIMER_COMPARATOR_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, (mPreviousMainCounter + mTimerCount) & mCounterMask);
    }
    
    //
    // Enable HPET Timer interrupt generation
    //
    if (mTimerConfiguration.Bits.MsiInterruptCapablity != 0 && FeaturePcdGet (PcdHpetMsiEnable)) {
      //
      // Program MSI Address and MSI Data values in the selected HPET Timer
      // Program HPET register with APIC ID of current BSP in case BSP has been switched
      //
      HpetTimerMsiRoute.Bits.Address = GetApicMsiAddress ();
      HpetTimerMsiRoute.Bits.Value   = (UINT32)GetApicMsiValue (PcdGet8 (PcdHpetLocalApicVector), LOCAL_APIC_DELIVERY_MODE_LOWEST_PRIORITY, FALSE, FALSE);
      HpetWrite (HPET_TIMER_MSI_ROUTE_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, HpetTimerMsiRoute.Uint64);
      //
      // Enable HPET MSI Interrupt
      //
      mTimerConfiguration.Bits.MsiInterruptEnable = 1;
    } else {
      //
      // Enable timer interrupt through I/O APIC
      // Program IOAPIC register with APIC ID of current BSP in case BSP has been switched
      //
      IoApicConfigureInterrupt (mTimerIrq, PcdGet8 (PcdHpetLocalApicVector), IO_APIC_DELIVERY_MODE_LOWEST_PRIORITY, TRUE, FALSE);
      IoApicEnableInterrupt (mTimerIrq, TRUE);
    }

    //
    // Enable HPET Interrupt Generation
    //
    mTimerConfiguration.Bits.InterruptEnable = 1;
    HpetWrite (HPET_TIMER_CONFIGURATION_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, mTimerConfiguration.Uint64);
  }
    
  //
  // Save the new timer period
  //
  mTimerPeriod = TimerPeriod;

  //
  // Enable the HPET counter once new timer period has been established
  // The HPET counter should run even if the HPET Timer interrupts are
  // disabled.  This is used to account for time passed while the interrupt
  // is disabled.
  //
  HpetEnable (TRUE);
  
  return EFI_SUCCESS;
}

/**
  This function retrieves the period of timer interrupts in 100 ns units,
  returns that value in TimerPeriod, and returns EFI_SUCCESS.  If TimerPeriod
  is NULL, then EFI_INVALID_PARAMETER is returned.  If a TimerPeriod of 0 is
  returned, then the timer is currently disabled.

  @param  This         The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod  A pointer to the timer period to retrieve in 100 ns units.
                       If 0 is returned, then the timer is currently disabled.

  @retval  EFI_SUCCESS            The timer period was returned in TimerPeriod.
  @retval  EFI_INVALID_PARAMETER  TimerPeriod is NULL.

**/
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

/**
  This function generates a soft timer interrupt. If the platform does not support soft
  timer interrupts, then EFI_UNSUPPORTED is returned. Otherwise, EFI_SUCCESS is returned.
  If a handler has been registered through the EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()
  service, then a soft timer interrupt will be generated. If the timer interrupt is
  enabled when this service is called, then the registered handler will be invoked. The
  registered handler should not be able to distinguish a hardware-generated timer
  interrupt from a software-generated timer interrupt.

  @param  This  The EFI_TIMER_ARCH_PROTOCOL instance.

  @retval  EFI_SUCCESS       The soft timer interrupt was generated.
  @retval  EFI_UNSUPPORTEDT  The platform does not support the generation of soft 
                             timer interrupts.

**/
EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  )
{
  UINT64   MainCounter;
  EFI_TPL  Tpl;
  UINT64   TimerPeriod;
  UINT64   Delta;

  //
  // Disable interrupts
  //  
  Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  
  //
  // Capture main counter value
  //
  MainCounter = HpetRead (HPET_MAIN_COUNTER_OFFSET);

  //
  // Check to see if there is a registered notification function
  //
  if (mTimerNotifyFunction != NULL) {
    //
    // Compute time since last interrupt in 100 ns units (10 ^ -7) 
    //
    if (MainCounter > mPreviousMainCounter) {
      //
      // Main counter does not overflow
      //
      Delta = MainCounter - mPreviousMainCounter;
    } else {
      //
      // Main counter overflows, first usb, then add
      //
      Delta = (mCounterMask - mPreviousMainCounter) + MainCounter;
    }

    TimerPeriod = DivU64x32 (
                    MultU64x32 (
                      Delta & mCounterMask,
                      mHpetGeneralCapabilities.Bits.CounterClockPeriod
                      ), 
                    100000000
                    );
                    
    //
    // Call registered notification function passing in the time since the last
    // interrupt in 100 ns units.
    //    
    mTimerNotifyFunction (TimerPeriod);
  }

  //
  // Save main counter value
  //
  mPreviousMainCounter = MainCounter;
  
  //
  // Restore interrupts
  //  
  gBS->RestoreTPL (Tpl);
  
  return EFI_SUCCESS;
}

/**
  Initialize the Timer Architectural Protocol driver

  @param  ImageHandle  ImageHandle of the loaded driver
  @param  SystemTable  Pointer to the System Table

  @retval  EFI_SUCCESS           Timer Architectural Protocol created
  @retval  EFI_OUT_OF_RESOURCES  Not enough resources available to initialize driver.
  @retval  EFI_DEVICE_ERROR      A device error occured attempting to initialize the driver.

**/
EFI_STATUS
EFIAPI
TimerDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                             Status;
  UINTN                                  TimerIndex;
  UINTN                                  MsiTimerIndex;
  HPET_TIMER_MSI_ROUTE_REGISTER          HpetTimerMsiRoute;

  DEBUG ((DEBUG_INFO, "Init HPET Timer Driver\n"));

  //
  // Make sure the Timer Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiTimerArchProtocolGuid);

  //
  // Find the CPU architectural protocol.
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
  ASSERT_EFI_ERROR (Status);

  //
  // Retrieve HPET Capabilities and Configuration Information
  //  
  mHpetGeneralCapabilities.Uint64  = HpetRead (HPET_GENERAL_CAPABILITIES_ID_OFFSET);
  mHpetGeneralConfiguration.Uint64 = HpetRead (HPET_GENERAL_CONFIGURATION_OFFSET);
 
  //
  // If Revision is not valid, then ASSERT() and unload the driver because the HPET 
  // device is not present.
  //  
  ASSERT (mHpetGeneralCapabilities.Uint64 != 0);
  ASSERT (mHpetGeneralCapabilities.Uint64 != 0xFFFFFFFFFFFFFFFFULL);
  if (mHpetGeneralCapabilities.Uint64 == 0 || mHpetGeneralCapabilities.Uint64 == 0xFFFFFFFFFFFFFFFFULL) {
    DEBUG ((DEBUG_ERROR, "HPET device is not present.  Unload HPET driver.\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Force the HPET timer to be disabled while setting everything up
  //
  HpetEnable (FALSE);

  //
  // Dump HPET Configuration Information
  //  
  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "HPET Base Address = 0x%08x\n", PcdGet32 (PcdHpetBaseAddress)));
    DEBUG ((DEBUG_INFO, "  HPET_GENERAL_CAPABILITIES_ID  = 0x%016lx\n", mHpetGeneralCapabilities));
    DEBUG ((DEBUG_INFO, "  HPET_GENERAL_CONFIGURATION    = 0x%016lx\n", mHpetGeneralConfiguration.Uint64));
    DEBUG ((DEBUG_INFO, "  HPET_GENERAL_INTERRUPT_STATUS = 0x%016lx\n", HpetRead (HPET_GENERAL_INTERRUPT_STATUS_OFFSET)));
    DEBUG ((DEBUG_INFO, "  HPET_MAIN_COUNTER             = 0x%016lx\n", HpetRead (HPET_MAIN_COUNTER_OFFSET)));
    DEBUG ((DEBUG_INFO, "  HPET Main Counter Period      = %d (fs)\n", mHpetGeneralCapabilities.Bits.CounterClockPeriod));
    for (TimerIndex = 0; TimerIndex <= mHpetGeneralCapabilities.Bits.NumberOfTimers; TimerIndex++) {
      DEBUG ((DEBUG_INFO, "  HPET_TIMER%d_CONFIGURATION     = 0x%016lx\n", TimerIndex, HpetRead (HPET_TIMER_CONFIGURATION_OFFSET + TimerIndex * HPET_TIMER_STRIDE)));
      DEBUG ((DEBUG_INFO, "  HPET_TIMER%d_COMPARATOR        = 0x%016lx\n", TimerIndex, HpetRead (HPET_TIMER_COMPARATOR_OFFSET    + TimerIndex * HPET_TIMER_STRIDE)));
      DEBUG ((DEBUG_INFO, "  HPET_TIMER%d_MSI_ROUTE         = 0x%016lx\n", TimerIndex, HpetRead (HPET_TIMER_MSI_ROUTE_OFFSET     + TimerIndex * HPET_TIMER_STRIDE)));
    }
  );
  
  //
  // Capture the current HPET main counter value.
  //
  mPreviousMainCounter = HpetRead (HPET_MAIN_COUNTER_OFFSET);
  
  //
  // Determine the interrupt mode to use for the HPET Timer.  
  // Look for MSI first, then unused PIC mode interrupt, then I/O APIC mode interrupt
  //  
  MsiTimerIndex = HPET_INVALID_TIMER_INDEX;
  mTimerIndex   = HPET_INVALID_TIMER_INDEX;
  for (TimerIndex = 0; TimerIndex <= mHpetGeneralCapabilities.Bits.NumberOfTimers; TimerIndex++) {
    //
    // Read the HPET Timer Capabilities and Configuration register
    //
    mTimerConfiguration.Uint64 = HpetRead (HPET_TIMER_CONFIGURATION_OFFSET + TimerIndex * HPET_TIMER_STRIDE);
    
    //
    // Check to see if this HPET Timer supports MSI 
    //
    if (mTimerConfiguration.Bits.MsiInterruptCapablity != 0) {
      //
      // Save the index of the first HPET Timer that supports MSI interrupts
      //
      if (MsiTimerIndex == HPET_INVALID_TIMER_INDEX) {
        MsiTimerIndex = TimerIndex;
      }
    }
    
    //
    // Check to see if this HPET Timer supports I/O APIC interrupts
    //
    if (mTimerConfiguration.Bits.InterruptRouteCapability != 0) {
      //
      // Save the index of the first HPET Timer that supports I/O APIC interrupts
      //
      if (mTimerIndex == HPET_INVALID_TIMER_INDEX) {
        mTimerIndex = TimerIndex;
        mTimerIrq   = (UINT32)LowBitSet32 (mTimerConfiguration.Bits.InterruptRouteCapability);
      }
    }
  }

  if (FeaturePcdGet (PcdHpetMsiEnable) && MsiTimerIndex != HPET_INVALID_TIMER_INDEX) {
    //
    // Use MSI interrupt if supported
    //
    mTimerIndex  = MsiTimerIndex;

    //
    // Program MSI Address and MSI Data values in the selected HPET Timer
    //
    HpetTimerMsiRoute.Bits.Address = GetApicMsiAddress ();
    HpetTimerMsiRoute.Bits.Value   = (UINT32)GetApicMsiValue (PcdGet8 (PcdHpetLocalApicVector), LOCAL_APIC_DELIVERY_MODE_LOWEST_PRIORITY, FALSE, FALSE);
    HpetWrite (HPET_TIMER_MSI_ROUTE_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, HpetTimerMsiRoute.Uint64);

    //
    // Read the HPET Timer Capabilities and Configuration register and initialize for MSI mode
    //   Clear LevelTriggeredInterrupt to use edge triggered interrupts when in MSI mode
    //
    mTimerConfiguration.Uint64 = HpetRead (HPET_TIMER_CONFIGURATION_OFFSET + mTimerIndex * HPET_TIMER_STRIDE);
    mTimerConfiguration.Bits.LevelTriggeredInterrupt = 0;
  } else {
    //
    // If no HPET timers support MSI or I/O APIC modes, then ASSERT() and unload the driver.
    //
    ASSERT (mTimerIndex != HPET_INVALID_TIMER_INDEX);
    if (mTimerIndex == HPET_INVALID_TIMER_INDEX) {
      DEBUG ((DEBUG_ERROR, "No HPET timers support MSI or I/O APIC mode.  Unload HPET driver.\n"));
      return EFI_DEVICE_ERROR;
    }
    
    //
    // Initialize I/O APIC entry for HPET Timer Interrupt
    //   Fixed Delivery Mode, Level Triggered, Asserted Low
    //
    IoApicConfigureInterrupt (mTimerIrq, PcdGet8 (PcdHpetLocalApicVector), IO_APIC_DELIVERY_MODE_LOWEST_PRIORITY, TRUE, FALSE);

    //
    // Read the HPET Timer Capabilities and Configuration register and initialize for I/O APIC mode
    //   Clear MsiInterruptCapability to force rest of driver to use I/O APIC mode
    //   Set LevelTriggeredInterrupt to use level triggered interrupts when in I/O APIC mode
    //   Set InterruptRoute field based in mTimerIrq
    //
    mTimerConfiguration.Uint64 = HpetRead (HPET_TIMER_CONFIGURATION_OFFSET + mTimerIndex * HPET_TIMER_STRIDE);
    mTimerConfiguration.Bits.LevelTriggeredInterrupt = 1;
    mTimerConfiguration.Bits.InterruptRoute          = mTimerIrq;
  }

  //
  // Configure the selected HPET Timer with settings common to both MSI mode and I/O APIC mode
  //   Clear InterruptEnable to keep interrupts disabled until full init is complete 
  //   Clear PeriodicInterruptEnable to use one-shot mode 
  //   Configure as a 32-bit counter  
  //
  mTimerConfiguration.Bits.InterruptEnable         = 0;
  mTimerConfiguration.Bits.PeriodicInterruptEnable = 0;
  mTimerConfiguration.Bits.CounterSizeEnable       = 1;
  HpetWrite (HPET_TIMER_CONFIGURATION_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, mTimerConfiguration.Uint64);
  
  //
  // Read the HPET Timer Capabilities and Configuration register back again.
  // CounterSizeEnable will be read back as a 0 if it is a 32-bit only timer
  //
  mTimerConfiguration.Uint64 = HpetRead (HPET_TIMER_CONFIGURATION_OFFSET + mTimerIndex * HPET_TIMER_STRIDE);
  if ((mTimerConfiguration.Bits.CounterSizeEnable == 1) && (sizeof (UINTN) == sizeof (UINT64))) {
    DEBUG ((DEBUG_INFO, "Choose 64-bit HPET timer.\n"));
    //
    // 64-bit BIOS can use 64-bit HPET timer
    //
    mCounterMask = 0xffffffffffffffffULL;
    //
    // Set timer back to 64-bit
    //
    mTimerConfiguration.Bits.CounterSizeEnable = 0;
    HpetWrite (HPET_TIMER_CONFIGURATION_OFFSET + mTimerIndex * HPET_TIMER_STRIDE, mTimerConfiguration.Uint64);
  } else {
    DEBUG ((DEBUG_INFO, "Choose 32-bit HPET timer.\n"));
    mCounterMask = 0x00000000ffffffffULL;
  }

  //
  // Install interrupt handler for selected HPET Timer
  //
  Status = mCpu->RegisterInterruptHandler (mCpu, PcdGet8 (PcdHpetLocalApicVector), TimerInterruptHandler);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to register HPET interrupt with CPU Arch Protocol.  Unload HPET driver.\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Force the HPET Timer to be enabled at its default period
  //
  Status = TimerDriverSetTimerPeriod (&mTimer, PcdGet64 (PcdHpetDefaultTimerPeriod));
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to set HPET default timer rate.  Unload HPET driver.\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Show state of enabled HPET timer
  //
  DEBUG_CODE (
    if (mTimerConfiguration.Bits.MsiInterruptCapablity != 0 && FeaturePcdGet (PcdHpetMsiEnable)) {
      DEBUG ((DEBUG_INFO, "HPET Interrupt Mode MSI\n"));
    } else {
      DEBUG ((DEBUG_INFO, "HPET Interrupt Mode I/O APIC\n"));
      DEBUG ((DEBUG_INFO, "HPET I/O APIC IRQ         = 0x%02x\n",  mTimerIrq));
    }  
    DEBUG ((DEBUG_INFO, "HPET Interrupt Vector     = 0x%02x\n",    PcdGet8 (PcdHpetLocalApicVector)));
    DEBUG ((DEBUG_INFO, "HPET Counter Mask         = 0x%016lx\n",  mCounterMask));
    DEBUG ((DEBUG_INFO, "HPET Timer Period         = %d\n",        mTimerPeriod));
    DEBUG ((DEBUG_INFO, "HPET Timer Count          = 0x%016lx\n",  mTimerCount));
    DEBUG ((DEBUG_INFO, "HPET_TIMER%d_CONFIGURATION = 0x%016lx\n", mTimerIndex, HpetRead (HPET_TIMER_CONFIGURATION_OFFSET + mTimerIndex * HPET_TIMER_STRIDE)));
    DEBUG ((DEBUG_INFO, "HPET_TIMER%d_COMPARATOR    = 0x%016lx\n", mTimerIndex, HpetRead (HPET_TIMER_COMPARATOR_OFFSET    + mTimerIndex * HPET_TIMER_STRIDE)));
    DEBUG ((DEBUG_INFO, "HPET_TIMER%d_MSI_ROUTE     = 0x%016lx\n", mTimerIndex, HpetRead (HPET_TIMER_MSI_ROUTE_OFFSET     + mTimerIndex * HPET_TIMER_STRIDE)));

    //
    // Wait for a few timer interrupts to fire before continuing
    // 
    while (mNumTicks < 10);
  );
 
  //
  // Install the Timer Architectural Protocol onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mTimerHandle,
                  &gEfiTimerArchProtocolGuid, &mTimer,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
