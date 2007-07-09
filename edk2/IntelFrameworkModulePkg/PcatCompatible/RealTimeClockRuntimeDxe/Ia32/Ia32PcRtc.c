/*++

Copyright (c) 2006, Intel Corporation. All rights reserved. <BR> 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.



Module Name:

 Ia32PcRtc.c

Abstract:

--*/

#include "PcRtc.h"

static PC_RTC_MODULE_GLOBALS  mModuleGlobal;

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
