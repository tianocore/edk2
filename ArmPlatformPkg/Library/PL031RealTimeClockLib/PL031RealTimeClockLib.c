/** @file
  Implement EFI RealTimeClock runtime services via RTC Lib.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmPlatformSysConfigLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Protocol/RealTimeClock.h>

#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>

#include <Drivers/PL031RealTimeClock.h>

#include <ArmPlatform.h>

STATIC CONST CHAR16           mTimeZoneVariableName[] = L"PL031RtcTimeZone";
STATIC CONST CHAR16           mDaylightVariableName[] = L"PL031RtcDaylight";
STATIC BOOLEAN                mPL031Initialized = FALSE;
STATIC EFI_EVENT              mRtcVirtualAddrChangeEvent;
STATIC UINTN                  mPL031RtcBase;
STATIC EFI_RUNTIME_SERVICES   *mRT;

EFI_STATUS
IdentifyPL031 (
  VOID
  )
{
  EFI_STATUS    Status;

  // Check if this is a PrimeCell Peripheral
  if (  (MmioRead8 (mPL031RtcBase + PL031_RTC_PCELL_ID0) != 0x0D)
      || (MmioRead8 (mPL031RtcBase + PL031_RTC_PCELL_ID1) != 0xF0)
      || (MmioRead8 (mPL031RtcBase + PL031_RTC_PCELL_ID2) != 0x05)
      || (MmioRead8 (mPL031RtcBase + PL031_RTC_PCELL_ID3) != 0xB1)) {
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  // Check if this PrimeCell Peripheral is the PL031 Real Time Clock
  if (  (MmioRead8 (mPL031RtcBase + PL031_RTC_PERIPH_ID0) != 0x31)
      || (MmioRead8 (mPL031RtcBase + PL031_RTC_PERIPH_ID1) != 0x10)
      || ((MmioRead8 (mPL031RtcBase + PL031_RTC_PERIPH_ID2) & 0xF) != 0x04)
      || (MmioRead8 (mPL031RtcBase + PL031_RTC_PERIPH_ID3) != 0x00)) {
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  Status = EFI_SUCCESS;

  EXIT:
  return Status;
}

EFI_STATUS
InitializePL031 (
  VOID
  )
{
  EFI_STATUS    Status;

  // Prepare the hardware
  Status = IdentifyPL031();
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  // Ensure interrupts are masked. We do not want RTC interrupts in UEFI
  if ((MmioRead32 (mPL031RtcBase + PL031_RTC_IMSC_IRQ_MASK_SET_CLEAR_REGISTER) & PL031_SET_IRQ_MASK) != PL031_SET_IRQ_MASK) {
    MmioOr32 (mPL031RtcBase + PL031_RTC_IMSC_IRQ_MASK_SET_CLEAR_REGISTER, PL031_SET_IRQ_MASK);
  }

  // Clear any existing interrupts
  if ((MmioRead32 (mPL031RtcBase + PL031_RTC_RIS_RAW_IRQ_STATUS_REGISTER) & PL031_IRQ_TRIGGERED) == PL031_IRQ_TRIGGERED) {
    MmioOr32 (mPL031RtcBase + PL031_RTC_ICR_IRQ_CLEAR_REGISTER, PL031_CLEAR_IRQ);
  }

  // Start the clock counter
  if ((MmioRead32 (mPL031RtcBase + PL031_RTC_CR_CONTROL_REGISTER) & PL031_RTC_ENABLED) != PL031_RTC_ENABLED) {
    MmioOr32 (mPL031RtcBase + PL031_RTC_CR_CONTROL_REGISTER, PL031_RTC_ENABLED);
  }

  mPL031Initialized = TRUE;

  EXIT:
  return Status;
}

/**
  Converts Epoch seconds (elapsed since 1970 JANUARY 01, 00:00:00 UTC) to EFI_TIME
 **/
VOID
EpochToEfiTime (
  IN  UINTN     EpochSeconds,
  OUT EFI_TIME  *Time
  )
{
  UINTN         a;
  UINTN         b;
  UINTN         c;
  UINTN         d;
  UINTN         g;
  UINTN         j;
  UINTN         m;
  UINTN         y;
  UINTN         da;
  UINTN         db;
  UINTN         dc;
  UINTN         dg;
  UINTN         hh;
  UINTN         mm;
  UINTN         ss;
  UINTN         J;

  J  = (EpochSeconds / 86400) + 2440588;
  j  = J + 32044;
  g  = j / 146097;
  dg = j % 146097;
  c  = (((dg / 36524) + 1) * 3) / 4;
  dc = dg - (c * 36524);
  b  = dc / 1461;
  db = dc % 1461;
  a  = (((db / 365) + 1) * 3) / 4;
  da = db - (a * 365);
  y  = (g * 400) + (c * 100) + (b * 4) + a;
  m  = (((da * 5) + 308) / 153) - 2;
  d  = da - (((m + 4) * 153) / 5) + 122;

  Time->Year  = y - 4800 + ((m + 2) / 12);
  Time->Month = ((m + 2) % 12) + 1;
  Time->Day   = d + 1;

  ss = EpochSeconds % 60;
  a  = (EpochSeconds - ss) / 60;
  mm = a % 60;
  b = (a - mm) / 60;
  hh = b % 24;

  Time->Hour        = hh;
  Time->Minute      = mm;
  Time->Second      = ss;
  Time->Nanosecond  = 0;

}

/**
  Converts EFI_TIME to Epoch seconds (elapsed since 1970 JANUARY 01, 00:00:00 UTC)
 **/
UINTN
EfiTimeToEpoch (
  IN  EFI_TIME  *Time
  )
{
  UINTN a;
  UINTN y;
  UINTN m;
  UINTN JulianDate;  // Absolute Julian Date representation of the supplied Time
  UINTN EpochDays;   // Number of days elapsed since EPOCH_JULIAN_DAY
  UINTN EpochSeconds;

  a = (14 - Time->Month) / 12 ;
  y = Time->Year + 4800 - a;
  m = Time->Month + (12*a) - 3;

  JulianDate = Time->Day + ((153*m + 2)/5) + (365*y) + (y/4) - (y/100) + (y/400) - 32045;

  ASSERT (JulianDate >= EPOCH_JULIAN_DATE);
  EpochDays = JulianDate - EPOCH_JULIAN_DATE;

  EpochSeconds = (EpochDays * SEC_PER_DAY) + ((UINTN)Time->Hour * SEC_PER_HOUR) + (Time->Minute * SEC_PER_MIN) + Time->Second;

  return EpochSeconds;
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

BOOLEAN
DayValid (
  IN  EFI_TIME  *Time
  )
{
  STATIC CONST INTN DayOfMonth[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (Time->Day < 1 ||
      Time->Day > DayOfMonth[Time->Month - 1] ||
      (Time->Month == 2 && (!IsLeapYear (Time) && Time->Day > 28))
     ) {
    return FALSE;
  }

  return TRUE;
}

/**
  Returns the current time and date information, and the time-keeping capabilities
  of the hardware platform.

  @param  Time                   A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities           An optional pointer to a buffer to receive the real time clock
                                 device's capabilities.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Time is NULL.
  @retval EFI_DEVICE_ERROR       The time could not be retrieved due to hardware error.
  @retval EFI_SECURITY_VIOLATION The time could not be retrieved due to an authentication failure.

**/
EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME                *Time,
  OUT EFI_TIME_CAPABILITIES   *Capabilities
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT32      EpochSeconds;
  INT16       TimeZone;
  UINT8       Daylight;
  UINTN       Size;

  // Initialize the hardware if not already done
  if (!mPL031Initialized) {
    Status = InitializePL031 ();
    if (EFI_ERROR (Status)) {
      goto EXIT;
    }
  }

  // Snapshot the time as early in the function call as possible
  // On some platforms we may have access to a battery backed up hardware clock.
  // If such RTC exists try to use it first.
  Status = ArmPlatformSysConfigGet (SYS_CFG_RTC, &EpochSeconds);
  if (Status == EFI_UNSUPPORTED) {
    // Battery backed up hardware RTC does not exist, revert to PL031
    EpochSeconds = MmioRead32 (mPL031RtcBase + PL031_RTC_DR_DATA_REGISTER);
    Status = EFI_SUCCESS;
  } else if (EFI_ERROR (Status)) {
    // Battery backed up hardware RTC exists but could not be read due to error. Abort.
    goto EXIT;
  } else {
    // Battery backed up hardware RTC exists and we read the time correctly from it.
    // Now sync the PL031 to the new time.
    MmioWrite32 (mPL031RtcBase + PL031_RTC_LR_LOAD_REGISTER, EpochSeconds);
  }

  // Ensure Time is a valid pointer
  if (Time == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  // Get the current time zone information from non-volatile storage
  Size = sizeof (TimeZone);
  Status = mRT->GetVariable (
                  (CHAR16 *)mTimeZoneVariableName,
                  &gEfiCallerIdGuid,
                  NULL,
                  &Size,
                  (VOID *)&TimeZone
                  );

  if (EFI_ERROR (Status)) {
    ASSERT(Status != EFI_INVALID_PARAMETER);
    ASSERT(Status != EFI_BUFFER_TOO_SMALL);

    if (Status != EFI_NOT_FOUND)
      goto EXIT;

    // The time zone variable does not exist in non-volatile storage, so create it.
    Time->TimeZone = EFI_UNSPECIFIED_TIMEZONE;
    // Store it
    Status = mRT->SetVariable (
                    (CHAR16 *)mTimeZoneVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    Size,
                    (VOID *)&(Time->TimeZone)
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_ERROR,
        "LibGetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
        mTimeZoneVariableName,
        Status
        ));
      goto EXIT;
    }
  } else {
    // Got the time zone
    Time->TimeZone = TimeZone;

    // Check TimeZone bounds:   -1440 to 1440 or 2047
    if (((Time->TimeZone < -1440) || (Time->TimeZone > 1440))
        && (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE)) {
      Time->TimeZone = EFI_UNSPECIFIED_TIMEZONE;
    }

    // Adjust for the correct time zone
    if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
      EpochSeconds += Time->TimeZone * SEC_PER_MIN;
    }
  }

  // Get the current daylight information from non-volatile storage
  Size = sizeof (Daylight);
  Status = mRT->GetVariable (
                  (CHAR16 *)mDaylightVariableName,
                  &gEfiCallerIdGuid,
                  NULL,
                  &Size,
                  (VOID *)&Daylight
                  );

  if (EFI_ERROR (Status)) {
    ASSERT(Status != EFI_INVALID_PARAMETER);
    ASSERT(Status != EFI_BUFFER_TOO_SMALL);

    if (Status != EFI_NOT_FOUND)
      goto EXIT;

    // The daylight variable does not exist in non-volatile storage, so create it.
    Time->Daylight = 0;
    // Store it
    Status = mRT->SetVariable (
                    (CHAR16 *)mDaylightVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    Size,
                    (VOID *)&(Time->Daylight)
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_ERROR,
        "LibGetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
        mDaylightVariableName,
        Status
        ));
      goto EXIT;
    }
  } else {
    // Got the daylight information
    Time->Daylight = Daylight;

    // Adjust for the correct period
    if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
      // Convert to adjusted time, i.e. spring forwards one hour
      EpochSeconds += SEC_PER_HOUR;
    }
  }

  // Convert from internal 32-bit time to UEFI time
  EpochToEfiTime (EpochSeconds, Time);

  // Update the Capabilities info
  if (Capabilities != NULL) {
    // PL031 runs at frequency 1Hz
    Capabilities->Resolution  = PL031_COUNTS_PER_SECOND;
    // Accuracy in ppm multiplied by 1,000,000, e.g. for 50ppm set 50,000,000
    Capabilities->Accuracy    = (UINT32)PcdGet32 (PcdPL031RtcPpmAccuracy);
    // FALSE: Setting the time does not clear the values below the resolution level
    Capabilities->SetsToZero  = FALSE;
  }

  EXIT:
  return Status;
}


/**
  Sets the current local time and date information.

  @param  Time                  A pointer to the current time.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The time could not be set due due to hardware error.

**/
EFI_STATUS
EFIAPI
LibSetTime (
  IN  EFI_TIME                *Time
  )
{
  EFI_STATUS  Status;
  UINTN       EpochSeconds;

  // Check the input parameters are within the range specified by UEFI
  if ((Time->Year   < 1900) ||
       (Time->Year   > 9999) ||
       (Time->Month  < 1   ) ||
       (Time->Month  > 12  ) ||
       (!DayValid (Time)    ) ||
       (Time->Hour   > 23  ) ||
       (Time->Minute > 59  ) ||
       (Time->Second > 59  ) ||
       (Time->Nanosecond > 999999999) ||
       (!((Time->TimeZone == EFI_UNSPECIFIED_TIMEZONE) || ((Time->TimeZone >= -1440) && (Time->TimeZone <= 1440)))) ||
       (Time->Daylight & (~(EFI_TIME_ADJUST_DAYLIGHT | EFI_TIME_IN_DAYLIGHT)))
    ) {
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  // Because the PL031 is a 32-bit counter counting seconds,
  // the maximum time span is just over 136 years.
  // Time is stored in Unix Epoch format, so it starts in 1970,
  // Therefore it can not exceed the year 2106.
  if ((Time->Year < 1970) || (Time->Year >= 2106)) {
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }

  // Initialize the hardware if not already done
  if (!mPL031Initialized) {
    Status = InitializePL031 ();
    if (EFI_ERROR (Status)) {
      goto EXIT;
    }
  }

  EpochSeconds = EfiTimeToEpoch (Time);

  // Adjust for the correct time zone, i.e. convert to UTC time zone
  if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
    EpochSeconds -= Time->TimeZone * SEC_PER_MIN;
  }

  // TODO: Automatic Daylight activation

  // Adjust for the correct period
  if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
    // Convert to un-adjusted time, i.e. fall back one hour
    EpochSeconds -= SEC_PER_HOUR;
  }

  // On some platforms we may have access to a battery backed up hardware clock.
  //
  // If such RTC exists then it must be updated first, before the PL031,
  // to minimise any time drift. This is important because the battery backed-up
  // RTC maintains the master time for the platform across reboots.
  //
  // If such RTC does not exist then the following function returns UNSUPPORTED.
  Status = ArmPlatformSysConfigSet (SYS_CFG_RTC, EpochSeconds);
  if ((EFI_ERROR (Status)) && (Status != EFI_UNSUPPORTED)){
    // Any status message except SUCCESS and UNSUPPORTED indicates a hardware failure.
    goto EXIT;
  }


  // Set the PL031
  MmioWrite32 (mPL031RtcBase + PL031_RTC_LR_LOAD_REGISTER, EpochSeconds);

  // The accesses to Variable Services can be very slow, because we may be writing to Flash.
  // Do this after having set the RTC.

  // Save the current time zone information into non-volatile storage
  Status = mRT->SetVariable (
                  (CHAR16 *)mTimeZoneVariableName,
                  &gEfiCallerIdGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (Time->TimeZone),
                  (VOID *)&(Time->TimeZone)
                  );
  if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_ERROR,
        "LibSetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
        mTimeZoneVariableName,
        Status
        ));
    goto EXIT;
  }

  // Save the current daylight information into non-volatile storage
  Status = mRT->SetVariable (
                  (CHAR16 *)mDaylightVariableName,
                  &gEfiCallerIdGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof(Time->Daylight),
                  (VOID *)&(Time->Daylight)
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      EFI_D_ERROR,
      "LibSetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
      mDaylightVariableName,
      Status
      ));
    goto EXIT;
  }

  EXIT:
  return Status;
}


/**
  Returns the current wakeup alarm clock setting.

  @param  Enabled               Indicates if the alarm is currently enabled or disabled.
  @param  Pending               Indicates if the alarm signal is pending and requires acknowledgement.
  @param  Time                  The current alarm setting.

  @retval EFI_SUCCESS           The alarm settings were returned.
  @retval EFI_INVALID_PARAMETER Any parameter is NULL.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a hardware error.

**/
EFI_STATUS
EFIAPI
LibGetWakeupTime (
  OUT BOOLEAN     *Enabled,
  OUT BOOLEAN     *Pending,
  OUT EFI_TIME    *Time
  )
{
  // Not a required feature
  return EFI_UNSUPPORTED;
}


/**
  Sets the system wakeup alarm clock time.

  @param  Enabled               Enable or disable the wakeup alarm.
  @param  Time                  If Enable is TRUE, the time to set the wakeup alarm for.

  @retval EFI_SUCCESS           If Enable is TRUE, then the wakeup alarm was enabled. If
                                Enable is FALSE, then the wakeup alarm was disabled.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be set due to a hardware error.
  @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.

**/
EFI_STATUS
EFIAPI
LibSetWakeupTime (
  IN BOOLEAN      Enabled,
  OUT EFI_TIME    *Time
  )
{
  // Not a required feature
  return EFI_UNSUPPORTED;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
LibRtcVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  //
  // Only needed if you are going to support the OS calling RTC functions in virtual mode.
  // You will need to call EfiConvertPointer (). To convert any stored physical addresses
  // to virtual address. After the OS transitions to calling in virtual mode, all future
  // runtime calls will be made in virtual mode.
  //
  EfiConvertPointer (0x0, (VOID**)&mPL031RtcBase);
  EfiConvertPointer (0x0, (VOID**)&mRT);
  return;
}

/**
  This is the declaration of an EFI image entry point. This can be the entry point to an application
  written to this specification, an EFI boot service driver, or an EFI runtime driver.

  @param  ImageHandle           Handle that identifies the loaded image.
  @param  SystemTable           System Table for this image.

  @retval EFI_SUCCESS           The operation completed successfully.

**/
EFI_STATUS
EFIAPI
LibRtcInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;

  // Initialize RTC Base Address
  mPL031RtcBase = PcdGet32 (PcdPL031RtcBase);

  // Declare the controller as EFI_MEMORY_RUNTIME
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  mPL031RtcBase, SIZE_4KB,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (mPL031RtcBase, SIZE_4KB, EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Setup the setters and getters
  gRT->GetTime       = LibGetTime;
  gRT->SetTime       = LibSetTime;
  gRT->GetWakeupTime = LibGetWakeupTime;
  gRT->SetWakeupTime = LibSetWakeupTime;

  mRT = gRT;

  // Install the protocol
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiRealTimeClockArchProtocolGuid,  NULL,
                  NULL
                 );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  LibRtcVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mRtcVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
