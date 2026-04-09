/** @file
  Produces the Metronome Architectural Protocol on top of Timer Library.

  This is a generic implementation of the Metronome Architectural Protocol that
  layers on top of an instance of the Timer Library.  The Timer Library provides
  functions for nanosecond and microsecond delays.  This generic implementation
  produces a fixed TickPeriod of 1 100ns unit, and when the WaitForTick() service
  is called, the number of ticks passed in is converted to either nanosecond or
  microsecond units.  If the number of ticks is small, then nanoseconds are used.
  If the number of ticks is large, then microseconds are used.  This prevents
  overflows that could occur for long delays if only nanoseconds were used and also
  provides the greatest accuracy for small delays.

Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Metronome.h"

//
// Handle for the Metronome Architectural Protocol instance produced by this driver
//
EFI_HANDLE  mMetronomeHandle = NULL;

//
// The Metronome Architectural Protocol instance produced by this driver
//
EFI_METRONOME_ARCH_PROTOCOL  mMetronome = {
  WaitForTick,
  1  // TickPeriod = 1*100 ns units
};

/**
  Waits for the specified number of ticks.

  This function implements EFI_METRONOME_ARCH_PROTOCOL.WaitForTick().
  The WaitForTick() function waits for the number of ticks specified by
  TickNumber from a known time source in the platform.  If TickNumber of
  ticks are detected, then EFI_SUCCESS is returned.  The actual time passed
  between entry of this function and the first tick is between 0 and
  TickPeriod 100 nS units.  If you want to guarantee that at least TickPeriod
  time has elapsed, wait for two ticks.  This function waits for a hardware
  event to determine when a tick occurs.  It is possible for interrupt
  processing, or exception processing to interrupt the execution of the
  WaitForTick() function.  Depending on the hardware source for the ticks, it
  is possible for a tick to be missed.  This function cannot guarantee that
  ticks will not be missed.  If a timeout occurs waiting for the specified
  number of ticks, then EFI_TIMEOUT is returned.

  @param  This             The EFI_METRONOME_ARCH_PROTOCOL instance.
  @param  TickNumber       Number of ticks to wait.

  @retval EFI_SUCCESS      The wait for the number of ticks specified by TickNumber
                           succeeded.
  @retval EFI_TIMEOUT      A timeout occurred waiting for the specified number of ticks.

**/
EFI_STATUS
EFIAPI
WaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  )
{
  //
  // Check the value of TickNumber, so a 32-bit overflow can be avoided
  // when TickNumber of converted to nanosecond units
  //
  if (TickNumber < 10000000) {
    //
    // If TickNumber is small, then use NanoSecondDelay()
    //
    NanoSecondDelay (TickNumber * 100);
  } else {
    //
    // If TickNumber is large, then use MicroSecondDelay()
    //
    MicroSecondDelay (TickNumber / 10);
  }

  return EFI_SUCCESS;
}

/**
  The user Entry Point for module Metronome. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InstallMetronome (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Make sure the Metronome Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiMetronomeArchProtocolGuid);

  //
  // Install on a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mMetronomeHandle,
                  &gEfiMetronomeArchProtocolGuid,
                  &mMetronome,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
