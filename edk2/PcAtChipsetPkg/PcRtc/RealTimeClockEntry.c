/*++

Copyright (c) 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


--*/

#include "RealTimeClock.h"

PC_RTC_MODULE_GLOBALS  mModuleGlobal;

EFI_STATUS
EFIAPI
PcRtcEfiGetTime (
  OUT EFI_TIME                *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Time          - TODO: add argument description
  Capabilities  - TODO: add argument description

Returns:

  TODO: add return values

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

  TODO: Add function description

Arguments:

  Time  - TODO: add argument description

Returns:

  TODO: add return values

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

  TODO: Add function description

Arguments:

  Enabled - TODO: add argument description
  Pending - TODO: add argument description
  Time    - TODO: add argument description

Returns:

  TODO: add return values

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

  TODO: Add function description

Arguments:

  Enabled - TODO: add argument description
  Time    - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  return PcRtcSetWakeupTime (Enabled, Time, &mModuleGlobal);
}

EFI_STATUS
EFIAPI
InitializeRealTimeClock (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:

  Arguments:

  

Returns: 
--*/
// TODO:    ImageHandle - add argument and description to function comment
// TODO:    SystemTable - add argument and description to function comment
{
  EFI_STATUS  Status;
  EFI_HANDLE  NewHandle;

  EfiInitializeLock (&mModuleGlobal.RtcLock, TPL_HIGH_LEVEL);

  Status = PcRtcInit (&mModuleGlobal);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SystemTable->RuntimeServices->GetTime       = PcRtcEfiGetTime;
  SystemTable->RuntimeServices->SetTime       = PcRtcEfiSetTime;
  SystemTable->RuntimeServices->GetWakeupTime = PcRtcEfiGetWakeupTime;
  SystemTable->RuntimeServices->SetWakeupTime = PcRtcEfiSetWakeupTime;

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiRealTimeClockArchProtocolGuid,
                  NULL,
                  NULL
                  );

  return Status;
}
