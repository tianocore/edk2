/** @file
  Provides Set/Get time operations.

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PcRtc.h"

PC_RTC_MODULE_GLOBALS  mModuleGlobal;

EFI_STATUS
EFIAPI
PcRtcEfiGetTime (
  OUT EFI_TIME                *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Time          - GC_TODO: add argument description
  Capabilities  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return PcRtcGetTime (Time, Capabilities, &mModuleGlobal);
}

EFI_STATUS
EFIAPI
PcRtcEfiSetTime (
  IN EFI_TIME                *Time
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Time  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return PcRtcSetTime (Time, &mModuleGlobal);
}

EFI_STATUS
EFIAPI
PcRtcEfiGetWakeupTime (
  OUT BOOLEAN     *Enabled,
  OUT BOOLEAN     *Pending,
  OUT EFI_TIME    *Time
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Enabled - GC_TODO: add argument description
  Pending - GC_TODO: add argument description
  Time    - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return PcRtcGetWakeupTime (Enabled, Pending, Time, &mModuleGlobal);
}

EFI_STATUS
EFIAPI
PcRtcEfiSetWakeupTime (
  IN BOOLEAN      Enabled,
  OUT EFI_TIME    *Time
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Enabled - GC_TODO: add argument description
  Time    - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return PcRtcSetWakeupTime (Enabled, Time, &mModuleGlobal);
}

EFI_STATUS
EFIAPI
InitializePcRtc (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:

  Arguments:



Returns:
--*/
// GC_TODO:    ImageHandle - add argument and description to function comment
// GC_TODO:    SystemTable - add argument and description to function comment
{
  EFI_STATUS  Status;
  EFI_HANDLE  NewHandle;

  EfiInitializeLock (&mModuleGlobal.RtcLock, TPL_HIGH_LEVEL);

  Status = PcRtcInit (&mModuleGlobal);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gRT->GetTime       = PcRtcEfiGetTime;
  gRT->SetTime       = PcRtcEfiSetTime;
  gRT->GetWakeupTime = PcRtcEfiGetWakeupTime;
  gRT->SetWakeupTime = PcRtcEfiSetWakeupTime;

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiRealTimeClockArchProtocolGuid,
                  NULL,
                  NULL
                  );

  return Status;
}
