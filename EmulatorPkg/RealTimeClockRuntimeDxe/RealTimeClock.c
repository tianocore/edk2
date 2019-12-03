/*++
  Emu RTC Architectural Protocol Driver as defined in PI

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010 - 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/EmuThunkLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/RealTimeClock.h>

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
EmuGetTime (
  OUT EFI_TIME                                 * Time,
  OUT EFI_TIME_CAPABILITIES                    * Capabilities OPTIONAL
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

**/
{

  //
  // Check parameter for null pointer
  //
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;

  }

  gEmuThunk->GetTime (Time, Capabilities);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EmuSetTime (
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

**/
{
  EFI_STATUS            Status;

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
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EmuGetWakeupTime (
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

**/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EmuSetWakeupTime (
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

**/
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
  ImageHandle - Image Handle
  SystemTable - Pointer to system table

Returns:

  EFI_SUCEESS - Real Time Clock Services are installed into the Runtime Services Table

**/
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  SystemTable->RuntimeServices->GetTime       = EmuGetTime;
  SystemTable->RuntimeServices->SetTime       = EmuSetTime;
  SystemTable->RuntimeServices->GetWakeupTime = EmuGetWakeupTime;
  SystemTable->RuntimeServices->SetWakeupTime = EmuSetWakeupTime;

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
**/
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
{

  STATIC const INTN  DayOfMonth[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

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
