/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Metronome.c

Abstract:

  NT Emulation Metronome Architectural Protocol Driver as defined in DXE CIS

**/

#include "Metronome.h"

//
// Global Variables
//
EFI_METRONOME_ARCH_PROTOCOL mMetronome = {
  WinNtMetronomeDriverWaitForTick,
  TICK_PERIOD
};

//
// Worker Functions
//

EFI_STATUS
EFIAPI
WinNtMetronomeDriverWaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  )
/*++

Routine Description:

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

Arguments:

  This       - The EFI_METRONOME_ARCH_PROTOCOL instance.
  TickNumber - Number of ticks to wait.

Returns:

  EFI_SUCCESS - The wait for the number of ticks specified by TickNumber
                succeeded.

--*/
{
  UINT64  SleepTime;

  //
  // Calculate the time to sleep.  Win API smallest unit to sleep is 1 millisec
  // Tick Period is in 100ns units, divide by 10000 to convert to ms
  //
  SleepTime = DivU64x32 (MultU64x32 ((UINT64) TickNumber, TICK_PERIOD) + 9999, 10000);
  gWinNt->Sleep ((UINT32) SleepTime);

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
WinNtMetronomeDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  Initialize the Metronome Architectural Protocol driver

Arguments:

  ImageHandle - ImageHandle of the loaded driver


  SystemTable - Pointer to the System Table

Returns:

  EFI_SUCCESS           - Metronome Architectural Protocol created

  EFI_OUT_OF_RESOURCES  - Not enough resources available to initialize driver.

  EFI_DEVICE_ERROR      - A device error occured attempting to initialize the driver.

--*/
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;


  //
  // Install the Metronome Architectural Protocol onto a new handle
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiMetronomeArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMetronome
                  );

  return Status;
}
