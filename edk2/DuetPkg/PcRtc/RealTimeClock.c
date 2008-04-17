/*++

Copyright (c) 2005 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PcRtc.c

Abstract:

  RTC Architectural Protocol GUID as defined in EFI 2.0

--*/

#include "RealTimeClock.h"

BOOLEAN
DayValid (
  IN  EFI_TIME  *Time
  );

BOOLEAN
IsLeapYear (
  IN EFI_TIME   *Time
  );

BOOLEAN 
IsWithinOneDay (
  IN EFI_TIME   *From,
  IN EFI_TIME   *To
  );

INTN
CompareHMS (
  IN EFI_TIME   *From,
  IN EFI_TIME   *To
  );
    
UINT8
RtcRead (
  IN  UINT8 Address
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Address - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, (UINT8) (Address | (UINT8) (IoRead8 (PCAT_RTC_ADDRESS_REGISTER) & 0x80)));
  return IoRead8 (PCAT_RTC_DATA_REGISTER);
}

INTN
CompareHMS (
  IN EFI_TIME   *From,
  IN EFI_TIME   *To
  )
/*++

Routine Description: 

  Compare the Hour, Minute and Second of the 'From' time and the 'To' time.
  Only compare H/M/S in EFI_TIME and ignore other fields here. 

Arguments:

  From  -   the first time
  To    -   the second time 

Returns:

  >0   : The H/M/S of the 'From' time is later than those of 'To' time
  ==0  : The H/M/S of the 'From' time is same as those of 'To' time
  <0   : The H/M/S of the 'From' time is earlier than those of 'To' time

--*/

{
  if ((From->Hour > To->Hour) ||
     ((From->Hour == To->Hour) && (From->Minute > To->Minute)) ||
     ((From->Hour == To->Hour) && (From->Minute == To->Minute) && (From->Second > To->Second))) {
    return 1; 
  } else if ((From->Hour == To->Hour) && (From->Minute == To->Minute) && (From->Second == To->Second)) {
    return 0;
  } else {
    return -1;
  }
}

VOID
RtcWrite (
  IN  UINT8   Address,
  IN  UINT8   Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Address - TODO: add argument description
  Data    - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, (UINT8) (Address | (UINT8) (IoRead8 (PCAT_RTC_ADDRESS_REGISTER) & 0x80)));
  IoWrite8 (PCAT_RTC_DATA_REGISTER, Data);
}

EFI_STATUS
PcRtcInit (
  IN PC_RTC_MODULE_GLOBALS  *Global
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Global  - TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  EFI_STATUS      Status;
  RTC_REGISTER_A  RegisterA;
  RTC_REGISTER_B  RegisterB;
  RTC_REGISTER_C  RegisterC;
  RTC_REGISTER_D  RegisterD;
  UINT8           Century;
  EFI_TIME        Time;

  //
  // Acquire RTC Lock to make access to RTC atomic
  //
  EfiAcquireLock (&Global->RtcLock);

  //
  // Initialize RTC Register
  //
  // Make sure Division Chain is properly configured,
  // or RTC clock won't "tick" -- time won't increment
  //
  RegisterA.Data = RTC_INIT_REGISTER_A;
  RtcWrite (RTC_ADDRESS_REGISTER_A, RegisterA.Data);

  //
  // Read Register B
  //
  RegisterB.Data = RtcRead (RTC_ADDRESS_REGISTER_B);

  //
  // Clear RTC flag register
  //
  RegisterC.Data = RtcRead (RTC_ADDRESS_REGISTER_C);

  //
  // Clear RTC register D
  //
  RegisterD.Data = RTC_INIT_REGISTER_D;
  RtcWrite (RTC_ADDRESS_REGISTER_D, RegisterD.Data);

  //
  // Wait for up to 0.1 seconds for the RTC to be updated
  //
  Status = RtcWaitToUpdate (100000);
  if (EFI_ERROR (Status)) {
    EfiReleaseLock (&Global->RtcLock);
    return EFI_DEVICE_ERROR;
  }
  //
  // Get the Time/Date/Daylight Savings values.
  //
  Time.Second = RtcRead (RTC_ADDRESS_SECONDS);
  Time.Minute = RtcRead (RTC_ADDRESS_MINUTES);
  Time.Hour   = RtcRead (RTC_ADDRESS_HOURS);
  Time.Day    = RtcRead (RTC_ADDRESS_DAY_OF_THE_MONTH);
  Time.Month  = RtcRead (RTC_ADDRESS_MONTH);
  Time.Year   = RtcRead (RTC_ADDRESS_YEAR);

  ConvertRtcTimeToEfiTime (&Time, RegisterB);

  if (RtcTestCenturyRegister () == EFI_SUCCESS) {
    Century = BcdToDecimal ((UINT8) (RtcRead (RTC_ADDRESS_CENTURY) & 0x7f));
  } else {
    Century = BcdToDecimal (RtcRead (RTC_ADDRESS_CENTURY));
  }

  Time.Year = (UINT16) (Century * 100 + Time.Year);

  //
  // Set RTC configuration after get original time
  //
  RtcWrite (RTC_ADDRESS_REGISTER_B, RTC_INIT_REGISTER_B);

  //
  // Release RTC Lock.
  //
  EfiReleaseLock (&Global->RtcLock);

  //
  // Validate time fields
  //
  Status = RtcTimeFieldsValid (&Time);
  if (EFI_ERROR (Status)) {
    Time.Second = RTC_INIT_SECOND;
    Time.Minute = RTC_INIT_MINUTE;
    Time.Hour   = RTC_INIT_HOUR;
    Time.Day    = RTC_INIT_DAY;
    Time.Month  = RTC_INIT_MONTH;
    Time.Year   = RTC_INIT_YEAR;
  }
  //
  // Reset time value according to new RTC configuration
  //
  PcRtcSetTime (&Time, Global);

  return EFI_SUCCESS;
}

EFI_STATUS
PcRtcGetTime (
  OUT EFI_TIME              *Time,
  IN  EFI_TIME_CAPABILITIES *Capabilities,
  IN  PC_RTC_MODULE_GLOBALS *Global
  )
/*++

Routine Description:

  Arguments:

  Returns: 
--*/
// TODO:    Time - add argument and description to function comment
// TODO:    Capabilities - add argument and description to function comment
// TODO:    Global - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS      Status;
  RTC_REGISTER_B  RegisterB;
  UINT8           Century;
  UINTN           BufferSize;

  //
  // Check parameters for null pointer
  //
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;

  }
  //
  // Acquire RTC Lock to make access to RTC atomic
  //
  EfiAcquireLock (&Global->RtcLock);

  //
  // Wait for up to 0.1 seconds for the RTC to be updated
  //
  Status = RtcWaitToUpdate (100000);
  if (EFI_ERROR (Status)) {
    EfiReleaseLock (&Global->RtcLock);
    return Status;
  }
  //
  // Read Register B
  //
  RegisterB.Data = RtcRead (RTC_ADDRESS_REGISTER_B);

  //
  // Get the Time/Date/Daylight Savings values.
  //
  Time->Second  = RtcRead (RTC_ADDRESS_SECONDS);
  Time->Minute  = RtcRead (RTC_ADDRESS_MINUTES);
  Time->Hour    = RtcRead (RTC_ADDRESS_HOURS);
  Time->Day     = RtcRead (RTC_ADDRESS_DAY_OF_THE_MONTH);
  Time->Month   = RtcRead (RTC_ADDRESS_MONTH);
  Time->Year    = RtcRead (RTC_ADDRESS_YEAR);

  ConvertRtcTimeToEfiTime (Time, RegisterB);

  if (RtcTestCenturyRegister () == EFI_SUCCESS) {
    Century = BcdToDecimal ((UINT8) (RtcRead (RTC_ADDRESS_CENTURY) & 0x7f));
  } else {
    Century = BcdToDecimal (RtcRead (RTC_ADDRESS_CENTURY));
  }

  Time->Year = (UINT16) (Century * 100 + Time->Year);

  //
  // Release RTC Lock.
  //
  EfiReleaseLock (&Global->RtcLock);

  //
  // Get the variable that containts the TimeZone and Daylight fields
  //
  Time->TimeZone  = Global->SavedTimeZone;
  Time->Daylight  = Global->Daylight;

  BufferSize      = sizeof (INT16) + sizeof (UINT8);

  //
  // Make sure all field values are in correct range
  //
  Status = RtcTimeFieldsValid (Time);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  //  Fill in Capabilities if it was passed in
  //
  if (Capabilities) {
    Capabilities->Resolution = 1;
    //
    // 1 hertz
    //
    Capabilities->Accuracy = 50000000;
    //
    // 50 ppm
    //
    Capabilities->SetsToZero = FALSE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PcRtcSetTime (
  IN EFI_TIME                *Time,
  IN PC_RTC_MODULE_GLOBALS   *Global
  )
/*++

Routine Description:

  Arguments:

  Returns: 
--*/
// TODO:    Time - add argument and description to function comment
// TODO:    Global - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS      Status;
  EFI_TIME        RtcTime;
  RTC_REGISTER_B  RegisterB;
  UINT8           Century;

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

  CopyMem (&RtcTime, Time, sizeof (EFI_TIME));

  //
  // Acquire RTC Lock to make access to RTC atomic
  //
  EfiAcquireLock (&Global->RtcLock);

  //
  // Wait for up to 0.1 seconds for the RTC to be updated
  //
  Status = RtcWaitToUpdate (100000);
  if (EFI_ERROR (Status)) {
    EfiReleaseLock (&Global->RtcLock);
    return Status;
  }
  //
  // Read Register B, and inhibit updates of the RTC
  //
  RegisterB.Data      = RtcRead (RTC_ADDRESS_REGISTER_B);
  RegisterB.Bits.SET  = 1;
  RtcWrite (RTC_ADDRESS_REGISTER_B, RegisterB.Data);

  ConvertEfiTimeToRtcTime (&RtcTime, RegisterB, &Century);

  RtcWrite (RTC_ADDRESS_SECONDS, RtcTime.Second);
  RtcWrite (RTC_ADDRESS_MINUTES, RtcTime.Minute);
  RtcWrite (RTC_ADDRESS_HOURS, RtcTime.Hour);
  RtcWrite (RTC_ADDRESS_DAY_OF_THE_MONTH, RtcTime.Day);
  RtcWrite (RTC_ADDRESS_MONTH, RtcTime.Month);
  RtcWrite (RTC_ADDRESS_YEAR, (UINT8) RtcTime.Year);
  if (RtcTestCenturyRegister () == EFI_SUCCESS) {
    Century = (UINT8) ((Century & 0x7f) | (RtcRead (RTC_ADDRESS_CENTURY) & 0x80));
  }

  RtcWrite (RTC_ADDRESS_CENTURY, Century);

  //
  // Allow updates of the RTC registers
  //
  RegisterB.Bits.SET = 0;
  RtcWrite (RTC_ADDRESS_REGISTER_B, RegisterB.Data);

  //
  // Release RTC Lock.
  //
  EfiReleaseLock (&Global->RtcLock);

  //
  // Set the variable that containts the TimeZone and Daylight fields
  //
  Global->SavedTimeZone = Time->TimeZone;
  Global->Daylight      = Time->Daylight;
  return Status;
}

EFI_STATUS
EFIAPI
PcRtcGetWakeupTime (
  OUT BOOLEAN                *Enabled,
  OUT BOOLEAN                *Pending,
  OUT EFI_TIME               *Time,
  IN PC_RTC_MODULE_GLOBALS   *Global
  )
/*++

Routine Description:

  Arguments:

  

Returns: 
--*/
// TODO:    Enabled - add argument and description to function comment
// TODO:    Pending - add argument and description to function comment
// TODO:    Time - add argument and description to function comment
// TODO:    Global - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS      Status;
  RTC_REGISTER_B  RegisterB;
  RTC_REGISTER_C  RegisterC;
  UINT8           Century;

  //
  // Check paramters for null pointers
  //
  if ((Enabled == NULL) || (Pending == NULL) || (Time == NULL)) {
    return EFI_INVALID_PARAMETER;

  }
  //
  // Acquire RTC Lock to make access to RTC atomic
  //
  EfiAcquireLock (&Global->RtcLock);

  //
  // Wait for up to 0.1 seconds for the RTC to be updated
  //
  Status = RtcWaitToUpdate (100000);
  if (EFI_ERROR (Status)) {
    EfiReleaseLock (&Global->RtcLock);
    return EFI_DEVICE_ERROR;
  }
  //
  // Read Register B and Register C
  //
  RegisterB.Data  = RtcRead (RTC_ADDRESS_REGISTER_B);
  RegisterC.Data  = RtcRead (RTC_ADDRESS_REGISTER_C);

  //
  // Get the Time/Date/Daylight Savings values.
  //
  *Enabled = RegisterB.Bits.AIE;
  if (*Enabled) {
    Time->Second  = RtcRead (RTC_ADDRESS_SECONDS_ALARM);
    Time->Minute  = RtcRead (RTC_ADDRESS_MINUTES_ALARM);
    Time->Hour    = RtcRead (RTC_ADDRESS_HOURS_ALARM);
    Time->Day     = RtcRead (RTC_ADDRESS_DAY_OF_THE_MONTH);
    Time->Month   = RtcRead (RTC_ADDRESS_MONTH);
    Time->Year    = RtcRead (RTC_ADDRESS_YEAR);
  } else {
    Time->Second  = 0;
    Time->Minute  = 0;
    Time->Hour    = 0;
    Time->Day     = RtcRead (RTC_ADDRESS_DAY_OF_THE_MONTH);
    Time->Month   = RtcRead (RTC_ADDRESS_MONTH);
    Time->Year    = RtcRead (RTC_ADDRESS_YEAR);
  }

  ConvertRtcTimeToEfiTime (Time, RegisterB);

  if (RtcTestCenturyRegister () == EFI_SUCCESS) {
    Century = BcdToDecimal ((UINT8) (RtcRead (RTC_ADDRESS_CENTURY) & 0x7f));
  } else {
    Century = BcdToDecimal (RtcRead (RTC_ADDRESS_CENTURY));
  }

  Time->Year = (UINT16) (Century * 100 + Time->Year);

  //
  // Release RTC Lock.
  //
  EfiReleaseLock (&Global->RtcLock);

  //
  // Make sure all field values are in correct range
  //
  Status = RtcTimeFieldsValid (Time);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  *Pending = RegisterC.Bits.AF;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcRtcSetWakeupTime (
  IN BOOLEAN                Enable,
  OUT EFI_TIME              *Time,
  IN PC_RTC_MODULE_GLOBALS  *Global
  )
/*++

Routine Description:

  Arguments:

  

Returns: 
--*/
// TODO:    Enable - add argument and description to function comment
// TODO:    Time - add argument and description to function comment
// TODO:    Global - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS            Status;
  EFI_TIME              RtcTime;
  RTC_REGISTER_B        RegisterB;
  UINT8                 Century;
  EFI_TIME_CAPABILITIES Capabilities;

  if (Enable) {

    if (Time == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Make sure that the time fields are valid
    //
    Status = RtcTimeFieldsValid (Time);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Just support set alarm time within 24 hours
    //
    PcRtcGetTime (&RtcTime, &Capabilities, Global);
    if (!IsWithinOneDay (&RtcTime, Time)) {
      return EFI_UNSUPPORTED;
    }
    //
    // Make a local copy of the time and date
    //
    CopyMem (&RtcTime, Time, sizeof (EFI_TIME));

  }
  //
  // Acquire RTC Lock to make access to RTC atomic
  //
  EfiAcquireLock (&Global->RtcLock);

  //
  // Wait for up to 0.1 seconds for the RTC to be updated
  //
  Status = RtcWaitToUpdate (100000);
  if (EFI_ERROR (Status)) {
    EfiReleaseLock (&Global->RtcLock);
    return EFI_DEVICE_ERROR;
  }
  //
  // Read Register B, and inhibit updates of the RTC
  //
  RegisterB.Data      = RtcRead (RTC_ADDRESS_REGISTER_B);

  RegisterB.Bits.SET  = 1;
  RtcWrite (RTC_ADDRESS_REGISTER_B, RegisterB.Data);

  if (Enable) {
    ConvertEfiTimeToRtcTime (&RtcTime, RegisterB, &Century);

    //
    // Set RTC alarm time
    //
    RtcWrite (RTC_ADDRESS_SECONDS_ALARM, RtcTime.Second);
    RtcWrite (RTC_ADDRESS_MINUTES_ALARM, RtcTime.Minute);
    RtcWrite (RTC_ADDRESS_HOURS_ALARM, RtcTime.Hour);

    RegisterB.Bits.AIE = 1;

  } else {
    RegisterB.Bits.AIE = 0;
  }
  //
  // Allow updates of the RTC registers
  //
  RegisterB.Bits.SET = 0;
  RtcWrite (RTC_ADDRESS_REGISTER_B, RegisterB.Data);

  //
  // Release RTC Lock.
  //
  EfiReleaseLock (&Global->RtcLock);

  return EFI_SUCCESS;
}

UINT8
BcdToDecimal (
  IN  UINT8 BcdValue
  )
/*++

Routine Description:

  Arguments:

  

Returns: 
--*/
// TODO:    BcdValue - add argument and description to function comment
{
  UINTN High;
  UINTN Low;

  High  = BcdValue >> 4;
  Low   = BcdValue - (High << 4);

  return (UINT8) (Low + (High * 10));
}

EFI_STATUS
RtcTestCenturyRegister (
  VOID
  )
/*++

Routine Description:

  Arguments:

  

Returns: 
--*/
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
{
  UINT8 Century;
  UINT8 Temp;

  Century = RtcRead (RTC_ADDRESS_CENTURY);
  //
  //  RtcWrite (RTC_ADDRESS_CENTURY, 0x00);
  //
  Temp = (UINT8) (RtcRead (RTC_ADDRESS_CENTURY) & 0x7f);
  RtcWrite (RTC_ADDRESS_CENTURY, Century);
  if (Temp == 0x19 || Temp == 0x20) {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;
}

VOID
ConvertRtcTimeToEfiTime (
  IN EFI_TIME       *Time,
  IN RTC_REGISTER_B RegisterB
  )
/*++

Routine Description:

  Arguments:

  

Returns: 
--*/
// TODO:    Time - add argument and description to function comment
// TODO:    RegisterB - add argument and description to function comment
{
  BOOLEAN PM;

  if ((Time->Hour) & 0x80) {
    PM = TRUE;
  } else {
    PM = FALSE;
  }

  Time->Hour = (UINT8) (Time->Hour & 0x7f);

  if (RegisterB.Bits.DM == 0) {
    Time->Year    = BcdToDecimal ((UINT8) Time->Year);
    Time->Month   = BcdToDecimal (Time->Month);
    Time->Day     = BcdToDecimal (Time->Day);
    Time->Hour    = BcdToDecimal (Time->Hour);
    Time->Minute  = BcdToDecimal (Time->Minute);
    Time->Second  = BcdToDecimal (Time->Second);
  }
  //
  // If time is in 12 hour format, convert it to 24 hour format
  //
  if (RegisterB.Bits.MIL == 0) {
    if (PM && Time->Hour < 12) {
      Time->Hour = (UINT8) (Time->Hour + 12);
    }

    if (!PM && Time->Hour == 12) {
      Time->Hour = 0;
    }
  }

  Time->Nanosecond  = 0;
  Time->TimeZone    = EFI_UNSPECIFIED_TIMEZONE;
  Time->Daylight    = 0;
}

EFI_STATUS
RtcWaitToUpdate (
  UINTN Timeout
  )
/*++

Routine Description:

  Arguments:
 

Returns: 
--*/
// TODO:    Timeout - add argument and description to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  RTC_REGISTER_A  RegisterA;
  RTC_REGISTER_D  RegisterD;

  //
  // See if the RTC is functioning correctly
  //
  RegisterD.Data = RtcRead (RTC_ADDRESS_REGISTER_D);

  if (RegisterD.Bits.VRT == 0) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Wait for up to 0.1 seconds for the RTC to be ready.
  //
  Timeout         = (Timeout / 10) + 1;
  RegisterA.Data  = RtcRead (RTC_ADDRESS_REGISTER_A);
  while (RegisterA.Bits.UIP == 1 && Timeout > 0) {
    MicroSecondDelay (10);
    RegisterA.Data = RtcRead (RTC_ADDRESS_REGISTER_A);
    Timeout--;
  }

  RegisterD.Data = RtcRead (RTC_ADDRESS_REGISTER_D);
  if (Timeout == 0 || RegisterD.Bits.VRT == 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
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

VOID
ConvertEfiTimeToRtcTime (
  IN EFI_TIME       *Time,
  IN RTC_REGISTER_B RegisterB,
  IN UINT8          *Century
  )
/*++

Routine Description:

  Arguments:


Returns: 
--*/
// TODO:    Time - add argument and description to function comment
// TODO:    RegisterB - add argument and description to function comment
// TODO:    Century - add argument and description to function comment
{
  BOOLEAN PM;

  PM = TRUE;
  //
  // Adjust hour field if RTC in in 12 hour mode
  //
  if (RegisterB.Bits.MIL == 0) {
    if (Time->Hour < 12) {
      PM = FALSE;
    }

    if (Time->Hour >= 13) {
      Time->Hour = (UINT8) (Time->Hour - 12);
    } else if (Time->Hour == 0) {
      Time->Hour = 12;
    }
  }
  //
  // Set the Time/Date/Daylight Savings values.
  //
  *Century    = DecimaltoBcd ((UINT8) (Time->Year / 100));

  Time->Year  = (UINT16) (Time->Year % 100);

  if (RegisterB.Bits.DM == 0) {
    Time->Year    = DecimaltoBcd ((UINT8) Time->Year);
    Time->Month   = DecimaltoBcd (Time->Month);
    Time->Day     = DecimaltoBcd (Time->Day);
    Time->Hour    = DecimaltoBcd (Time->Hour);
    Time->Minute  = DecimaltoBcd (Time->Minute);
    Time->Second  = DecimaltoBcd (Time->Second);
  }
  //
  // If we are in 12 hour mode and PM is set, then set bit 7 of the Hour field.
  //
  if (RegisterB.Bits.MIL == 0 && PM) {
    Time->Hour = (UINT8) (Time->Hour | 0x80);
  }
}
 
BOOLEAN 
IsWithinOneDay (
  IN EFI_TIME  *From,
  IN EFI_TIME  *To
  )
/*++

Routine Description:

  Judge whether two days are adjacent.

Arguments:

  From  -   the first day
  To    -   the second day

Returns:

  TRUE  -   The interval of two days are within one day.
  FALSE -   The interval of two days exceed ony day or parameter error.

--*/
{
  UINT8   DayOfMonth[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  BOOLEAN Adjacent = FALSE;

  if (From->Year == To->Year) {
    if (From->Month == To->Month) {
      if ((From->Day + 1) == To->Day) {
        if ((CompareHMS(From, To) >= 0)) {
          Adjacent = TRUE;
        }
      } else if (From->Day == To->Day) {
        if ((CompareHMS(From, To) <= 0)) {
          Adjacent = TRUE;
        }
      }
    } else if (((From->Month + 1) == To->Month) && (To->Day == 1)) {
      if ((From->Month == 2) && !IsLeapYear(From)) {
        if (From->Day == 28) {
          if ((CompareHMS(From, To) >= 0)) {
            Adjacent = TRUE;
          }  
        }
      } else if (From->Day == DayOfMonth[From->Month - 1]) {
        if ((CompareHMS(From, To) >= 0)) {
           Adjacent = TRUE;
        }
      }
    }
  } else if (((From->Year + 1) == To->Year) &&
             (From->Month == 12) &&
             (From->Day   == 31) &&
             (To->Month   == 1)  &&
             (To->Day     == 1)) {
    if ((CompareHMS(From, To) >= 0)) {
      Adjacent = TRUE;
    }
  }

  return Adjacent;
}

UINT8
DecimaltoBcd (
  IN  UINT8 DecValue
  )
/*++

Routine Description:

  Arguments:  

Returns: 

--*/
// TODO:    DecValue - add argument and description to function comment
{
  UINTN High;
  UINTN Low;

  High  = DecValue / 10;
  Low   = DecValue - (High * 10);

  return (UINT8) (Low + (High << 4));
}
