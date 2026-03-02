/** @file
  Code for debug timer to support debug agent library implementation.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DebugAgent.h"

/**
  Default value if we cannot get access to the PcdFSBClock.
  Some platforms (Xen) define the PCD as a dynamic one, which is not available
  in SEC phase, when it is used probably to configure the debug timer.
 **/
#define FSB_CLOCK_DEFAULT  100000000

/**
  Figures out if we are running inside Xen HVM.

  @retval TRUE   Xen was detected
  @retval FALSE  Xen was not detected
**/
STATIC
BOOLEAN
XenDetect (
  VOID
  )
{
  UINT32  XenLeaf;
  UINT8   Signature[13];

  Signature[12] = '\0';
  for (XenLeaf = 0x40000000; XenLeaf < 0x40010000; XenLeaf += 0x100) {
    AsmCpuid (
      XenLeaf,
      NULL,
      (UINT32 *)&Signature[0],
      (UINT32 *)&Signature[4],
      (UINT32 *)&Signature[8]
      );

    if (!AsciiStrCmp ((CHAR8 *)Signature, "XenVMMXenVMM")) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Initialize CPU local APIC timer.

  @param[out] TimerFrequency  Local APIC timer frequency returned.
  @param[in]  DumpFlag        If TRUE, dump Local APIC timer's parameter.

  @return   32-bit Local APIC timer init count.
**/
UINT32
InitializeDebugTimer (
  OUT UINT32   *TimerFrequency,
  IN  BOOLEAN  DumpFlag
  )
{
  UINTN   ApicTimerDivisor;
  UINT32  FSBClock;
  UINT32  InitialCount;
  UINT32  ApicTimerFrequency;

  InitializeLocalApicSoftwareEnable (TRUE);
  GetApicTimerState (&ApicTimerDivisor, NULL, NULL);

  if (XenDetect ()) {
    FSBClock = FSB_CLOCK_DEFAULT;
  } else {
    FSBClock = PcdGet32 (PcdFSBClock);
  }

  ApicTimerFrequency = FSBClock / (UINT32)ApicTimerDivisor;
  //
  // Cpu Local Apic timer interrupt frequency, it is set to 0.1s
  //
  InitialCount = (UINT32)DivU64x32 (
                           MultU64x64 (
                             ApicTimerFrequency,
                             DEBUG_TIMER_INTERVAL
                             ),
                           1000000u
                           );

  InitializeApicTimer (ApicTimerDivisor, InitialCount, TRUE, DEBUG_TIMER_VECTOR);
  //
  // Disable Debug Timer interrupt to avoid it is delivered before Debug Port
  // is initialized
  //
  DisableApicTimerInterrupt ();

  if (DumpFlag) {
    DEBUG ((DEBUG_INFO, "Debug Timer: FSB Clock    = %d\n", FSBClock));
    DEBUG ((DEBUG_INFO, "Debug Timer: Divisor      = %d\n", ApicTimerDivisor));
    DEBUG ((DEBUG_INFO, "Debug Timer: Frequency    = %d\n", ApicTimerFrequency));
    DEBUG ((DEBUG_INFO, "Debug Timer: InitialCount = %d\n", InitialCount));
  }

  if (TimerFrequency != NULL) {
    *TimerFrequency = ApicTimerFrequency;
  }

  return InitialCount;
}

/**
  Enable/Disable the interrupt of debug timer and return the interrupt state
  prior to the operation.

  If EnableStatus is TRUE, enable the interrupt of debug timer.
  If EnableStatus is FALSE, disable the interrupt of debug timer.

  @param[in] EnableStatus    Enable/Disable.

  @retval TRUE  Debug timer interrupt were enabled on entry to this call.
  @retval FALSE Debug timer interrupt were disabled on entry to this call.

**/
BOOLEAN
EFIAPI
SaveAndSetDebugTimerInterrupt (
  IN BOOLEAN  EnableStatus
  )
{
  BOOLEAN  OldDebugTimerInterruptState;

  OldDebugTimerInterruptState = GetApicTimerInterruptState ();

  if (OldDebugTimerInterruptState != EnableStatus) {
    if (EnableStatus) {
      EnableApicTimerInterrupt ();
    } else {
      DisableApicTimerInterrupt ();
    }

    //
    // Validate the Debug Timer interrupt state
    // This will make additional delay after Local Apic Timer interrupt state is changed.
    // Thus, CPU could handle the potential pending interrupt of Local Apic timer.
    //
    while (GetApicTimerInterruptState () != EnableStatus) {
      CpuPause ();
    }
  }

  return OldDebugTimerInterruptState;
}

/**
  Check if the timer is time out.

  @param[in] TimerCycle             Timer initial count.
  @param[in] Timer                  The start timer from the begin.
  @param[in] TimeoutTicker          Ticker number need time out.

  @return TRUE  Timer time out occurs.
  @retval FALSE Timer does not time out.

**/
BOOLEAN
IsDebugTimerTimeout (
  IN UINT32  TimerCycle,
  IN UINT32  Timer,
  IN UINT32  TimeoutTicker
  )
{
  UINT64  CurrentTimer;
  UINT64  Delta;

  CurrentTimer = GetApicTimerCurrentCount ();

  //
  // This timer counter counts down.  Check for roll over condition.
  // If CurrentTimer is equal to Timer, it does not mean that roll over
  // happened.
  //
  if (CurrentTimer <= Timer) {
    Delta = Timer - CurrentTimer;
  } else {
    //
    // Handle one roll-over.
    //
    Delta = TimerCycle - (CurrentTimer - Timer) + 1;
  }

  return (BOOLEAN)(Delta >= TimeoutTicker);
}
