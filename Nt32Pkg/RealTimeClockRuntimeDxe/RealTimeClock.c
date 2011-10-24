/**@file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  RealTimeClock.c

Abstract:

  NT Emulation Architectural Protocol Driver as defined in Tiano

**/

#include <Uefi.h>
#include <WinNtDxe.h>
#include <Protocol/RealTimeClock.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/WinNtLib.h>
#include <Library/UefiBootServicesTableLib.h>


BOOLEAN
DayValid (
  IN  EFI_TIME  *Time
  );

BOOLEAN
IsLeapYear (
  IN EFI_TIME   *Time
  );

EFI_STATUS
RtcTimeFieldsValid (
  IN EFI_TIME *Time
  );

EFI_STATUS
EFIAPI
InitializeRealTimeClock (
  IN EFI_HANDLE                          ImageHandle,
  IN EFI_SYSTEM_TABLE                    *SystemTable
  );

EFI_STATUS
EFIAPI
WinNtGetTime (
  OUT EFI_TIME                                 *Time,
  OUT EFI_TIME_CAPABILITIES                    *Capabilities OPTIONAL
  )
/*++

Routine Description:
  Service routine for RealTimeClockInstance->GetTime 

Arguments:

  Time          - A pointer to storage that will receive a snapshot of the current time.

  Capabilities  - A pointer to storage that will receive the capabilities of the real time clock
                  in the platform. This includes the real time clock's resolution and accuracy.  
                  All reported device capabilities are rounded up.  This is an OPTIONAL argument.

Returns:

  EFI_SUCEESS   - The underlying GetSystemTime call occurred and returned
                  Note that in the NT32 emulation, the GetSystemTime call has no return value
                  thus you will always receive a EFI_SUCCESS on this.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  SYSTEMTIME            SystemTime;
  TIME_ZONE_INFORMATION TimeZone;

  //
  // Check parameter for null pointer
  //
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;

  }

  gWinNt->GetLocalTime (&SystemTime);
  gWinNt->GetTimeZoneInformation (&TimeZone);

  Time->Year        = (UINT16) SystemTime.wYear;
  Time->Month       = (UINT8) SystemTime.wMonth;
  Time->Day         = (UINT8) SystemTime.wDay;
  Time->Hour        = (UINT8) SystemTime.wHour;
  Time->Minute      = (UINT8) SystemTime.wMinute;
  Time->Second      = (UINT8) SystemTime.wSecond;
  Time->Nanosecond  = (UINT32) (SystemTime.wMilliseconds * 1000000);
  Time->TimeZone    = (INT16) TimeZone.Bias;

  if (Capabilities != NULL) {
    Capabilities->Resolution  = 1;
    Capabilities->Accuracy    = 50000000;
    Capabilities->SetsToZero  = FALSE;
  }

  Time->Daylight = 0;
  if (TimeZone.StandardDate.wMonth) {
    Time->Daylight = (UINT8) TimeZone.StandardDate.wMonth;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtSetTime (
  IN EFI_TIME   *Time
  )
/*++

Routine Description:
  Service routine for RealTimeClockInstance->SetTime 

Arguments:

  Time          - A pointer to storage containing the time and date information to
                  program into the real time clock.

Returns:

  EFI_SUCEESS           - The operation completed successfully.
                  
  EFI_INVALID_PARAMETER - One of the fields in Time is out of range.

  EFI_DEVICE_ERROR      - The operation could not be complete due to a device error.

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{
  TIME_ZONE_INFORMATION TimeZone;
  EFI_STATUS            Status;
  SYSTEMTIME            SystemTime;
  BOOL                  Flag;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Make sure that the time fields are valid
  //
  Status = RtcTimeFieldsValid (Time);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Set Daylight savings time information and Time Zone
  //
  gWinNt->GetTimeZoneInformation (&TimeZone);
  TimeZone.StandardDate.wMonth  = Time->Daylight;
  TimeZone.Bias                 = Time->TimeZone;
  Flag = gWinNt->SetTimeZoneInformation (&TimeZone);
  if (!Flag) {
    return EFI_DEVICE_ERROR;
  }

  SystemTime.wYear          = Time->Year;
  SystemTime.wMonth         = Time->Month;
  SystemTime.wDay           = Time->Day;
  SystemTime.wHour          = Time->Hour;
  SystemTime.wMinute        = Time->Minute;
  SystemTime.wSecond        = Time->Second;
  SystemTime.wMilliseconds  = (INT16) (Time->Nanosecond / 1000000);

  Flag                      = gWinNt->SetLocalTime (&SystemTime);

  if (!Flag) {
    return EFI_DEVICE_ERROR;
  } else {
    return EFI_SUCCESS;
  }
}

EFI_STATUS
EFIAPI
WinNtGetWakeupTime (
  OUT BOOLEAN        *Enabled,
  OUT BOOLEAN        *Pending,
  OUT EFI_TIME       *Time
  )
/*++

Routine Description:
  Service routine for RealTimeClockInstance->GetWakeupTime

Arguments:
  This          - Indicates the protocol instance structure.

  Enabled       - Indicates if the alarm is currently enabled or disabled.

  Pending       - Indicates if the alarm signal is pending and requires
                  acknowledgement.

  Time          - The current alarm setting.

Returns:

  EFI_SUCEESS           - The operation completed successfully.
                  
  EFI_DEVICE_ERROR      - The operation could not be complete due to a device error.

  EFI_UNSUPPORTED       - The operation is not supported on this platform.

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
WinNtSetWakeupTime (
  IN BOOLEAN      Enable,
  OUT EFI_TIME    *Time
  )
/*++

Routine Description:
  Service routine for RealTimeClockInstance->SetWakeupTime

Arguments:

  Enabled       - Enable or disable the wakeup alarm.

  Time          - If enable is TRUE, the time to set the wakup alarm for.
                  If enable is FALSE, then this parameter is optional, and
                  may be NULL.

Returns:

  EFI_SUCEESS           - The operation completed successfully.
                  
  EFI_DEVICE_ERROR      - The operation could not be complete due to a device error.

  EFI_INVALID_PARAMETER - A field in Time is out of range.

  EFI_UNSUPPORTED       - The operation is not supported on this platform.

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
InitializeRealTimeClock (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:
  Install Real Time Clock Protocol 

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCEESS - Real Time Clock Services are installed into the Runtime Services Table

--*/
// TODO:    ImageHandle - add argument and description to function comment
// TODO:    SystemTable - add argument and description to function comment
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;


  SystemTable->RuntimeServices->GetTime       = WinNtGetTime;
  SystemTable->RuntimeServices->SetTime       = WinNtSetTime;
  SystemTable->RuntimeServices->GetWakeupTime = WinNtGetWakeupTime;
  SystemTable->RuntimeServices->SetWakeupTime = WinNtSetWakeupTime;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiRealTimeClockArchProtocolGuid,
                  NULL,
                  NULL
                  );
  return Status;
}

EFI_STATUS
RtcTimeFieldsValid (
  IN EFI_TIME *Time
  )
/*++

Routine Description:

  Arguments:
 
  Returns: 
--*/
// TODO:    Time - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  if (Time->Year < 1998 ||
      Time->Year > 2099 ||
      Time->Month < 1 ||
      Time->Month > 12 ||
      (!DayValid (Time)) ||
      Time->Hour > 23 ||
      Time->Minute > 59 ||
      Time->Second > 59 ||
      Time->Nanosecond > 999999999 ||
      (!(Time->TimeZone == EFI_UNSPECIFIED_TIMEZONE || (Time->TimeZone >= -1440 && Time->TimeZone <= 1440))) ||
      (Time->Daylight & (~(EFI_TIME_ADJUST_DAYLIGHT | EFI_TIME_IN_DAYLIGHT)))
      ) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

BOOLEAN
DayValid (
  IN  EFI_TIME  *Time
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

  INTN  DayOfMonth[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (Time->Day < 1 ||
      Time->Day > DayOfMonth[Time->Month - 1] ||
      (Time->Month == 2 && (!IsLeapYear (Time) && Time->Day > 28))
      ) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
IsLeapYear (
  IN EFI_TIME   *Time
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
  if (Time->Year % 4 == 0) {
    if (Time->Year % 100 == 0) {
      if (Time->Year % 400 == 0) {
        return TRUE;
      } else {
        return FALSE;
      }
    } else {
      return TRUE;
    }
  } else {
    return FALSE;
  }
}
