/** @file
  Implementation of Timestamp Protocol using UEFI APIs.
  
Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/Timestamp.h>

//
// The StartValue in TimerLib
//
UINT64 mTimerLibStartValue = 0;

//
// The EndValue in TimerLib
//
UINT64 mTimerLibEndValue = 0;

//
// The properties of timestamp
//
EFI_TIMESTAMP_PROPERTIES mTimestampProperties = {
  0,
  0
};

/**
  Retrieves the current value of a 64-bit free running timestamp counter.

  The counter shall count up in proportion to the amount of time that has passed. The counter value
  will always roll over to zero. The properties of the counter can be retrieved from GetProperties().
  The caller should be prepared for the function to return the same value twice across successive calls.
  The counter value will not go backwards other than when wrapping, as defined by EndValue in GetProperties().
  The frequency of the returned timestamp counter value must remain constant. Power management operations that 
  affect clocking must not change the returned counter frequency. The quantization of counter value updates may 
  vary as long as the value reflecting time passed remains consistent.           

  @retval The current value of the free running timestamp counter.

**/
UINT64
EFIAPI
TimestampDriverGetTimestamp (
  VOID
  )
{
  //
  // The timestamp of Timestamp Protocol
  //
  UINT64  TimestampValue;
  TimestampValue = 0;
  
  //
  // Get the timestamp
  //
  if (mTimerLibStartValue > mTimerLibEndValue) {
    TimestampValue = mTimerLibStartValue - GetPerformanceCounter();
  } else {
    TimestampValue = GetPerformanceCounter() - mTimerLibStartValue;
  }
    
  return TimestampValue;
}

/**
  Obtains timestamp counter properties including frequency and value limits.

  @param[out]  Properties              The properties of the timestamp counter.

  @retval      EFI_SUCCESS             The properties were successfully retrieved. 
  @retval      EFI_DEVICE_ERROR        An error occurred trying to retrieve the properties of the timestamp 
                                       counter subsystem. Properties is not pedated.                                
  @retval      EFI_INVALID_PARAMETER   Properties is NULL.

**/
EFI_STATUS
EFIAPI
TimestampDriverGetProperties(
  OUT   EFI_TIMESTAMP_PROPERTIES       *Properties
  )
{
  if (Properties == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Get timestamp properties
  //
  CopyMem((VOID *) Properties, (VOID *) &mTimestampProperties, sizeof (mTimestampProperties));
  
  return EFI_SUCCESS;
}

//
// The Timestamp Protocol instance produced by this driver
//
EFI_TIMESTAMP_PROTOCOL  mTimestamp = {
  TimestampDriverGetTimestamp,
  TimestampDriverGetProperties
};

/**
  Entry point of the Timestamp Protocol driver.

  @param  ImageHandle   The image handle of this driver.
  @param  SystemTable   The pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS   Watchdog Timer Architectural Protocol successfully installed.

**/
EFI_STATUS
EFIAPI
TimestampDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  
  EFI_HANDLE  TimestampHandle;
  TimestampHandle = NULL;
  
  //
  // Get the start value, end value and frequency in Timerlib
  //
  mTimestampProperties.Frequency = GetPerformanceCounterProperties(&mTimerLibStartValue, &mTimerLibEndValue);
  
  //
  // Set the EndValue 
  //
  if (mTimerLibEndValue > mTimerLibStartValue) {
    mTimestampProperties.EndValue = mTimerLibEndValue - mTimerLibStartValue;
  } else {
    mTimestampProperties.EndValue = mTimerLibStartValue - mTimerLibEndValue;
  }
  
  DEBUG ((EFI_D_INFO, "TimerFrequency:0x%lx, TimerLibStartTime:0x%lx, TimerLibEndtime:0x%lx\n", mTimestampProperties.Frequency, mTimerLibStartValue, mTimerLibEndValue));
  
  //
  // Install the Timestamp Protocol onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &TimestampHandle,
                  &gEfiTimestampProtocolGuid,
                  &mTimestamp,
                  NULL
                  );
                  
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
