/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013, ARM Ltd. All rights reserved.

  This program and the accompanying materials
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
#include <Library/TimerLib.h>

#include <Protocol/Metronome.h>

EFI_STATUS
EFIAPI
WaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  );

/**
  Interface structure for the Metronome Architectural Protocol.

  @par Protocol Description:
  This protocol provides access to a known time source in the platform to the
  core.  The core uses this known time source to produce core services that
  require calibrated delays.

  @param WaitForTick
  Waits for a specified number of ticks from a known time source
  in the platform.  The actual time passed between entry of this
  function and the first tick is between 0 and TickPeriod 100 nS
  units.  If you want to guarantee that at least TickPeriod time
  has elapsed, wait for two ticks.

  @param TickPeriod
  The period of platform's known time source in 100 nS units.
  This value on any platform must be at least 10 uS, and must not
  exceed 200 uS.  The value in this field is a constant that must
  not be modified after the Metronome architectural protocol is
  installed.  All consumers must treat this as a read-only field.

**/
EFI_METRONOME_ARCH_PROTOCOL gMetronome = {
  WaitForTick,
  FixedPcdGet32 (PcdMetronomeTickPeriod)
};


/**
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

  @retval EFI_SUCCESS           The wait for the number of ticks specified by TickNumber
                                succeeded.
  @retval EFI_TIMEOUT           A timeout occurred waiting for the specified number of ticks.

**/
EFI_STATUS
EFIAPI
WaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  )
{
  //
  // Compute how long to stall the CPU.
  // gMetronome.TickPeriod is in 100 ns units so it needs to be divided by 10
  // to get it in microseconds units.
  //
  MicroSecondDelay (TickNumber * gMetronome.TickPeriod / 10);
  return EFI_SUCCESS;
}


EFI_HANDLE  gMetronomeHandle = NULL;



/**
  Initialize the state information for the CPU Architectural Protocol

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
MetronomeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Do any hardware init required to make WaitForTick () to work here.
  //

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gMetronomeHandle,
                  &gEfiMetronomeArchProtocolGuid,   &gMetronome,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

