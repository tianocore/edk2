/** @file
  Header file for real time clock driver.

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _RTC_H_
#define _RTC_H_


#include <Uefi.h>

#include <Protocol/RealTimeClock.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>


typedef struct {
  EFI_LOCK  RtcLock;
  INT16     SavedTimeZone;
  UINT8     Daylight;
} PC_RTC_MODULE_GLOBALS;

#define PCAT_RTC_ADDRESS_REGISTER 0x70
#define PCAT_RTC_DATA_REGISTER    0x71

//
// Dallas DS12C887 Real Time Clock
//
#define RTC_ADDRESS_SECONDS           0   // R/W  Range 0..59
#define RTC_ADDRESS_SECONDS_ALARM     1   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES           2   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES_ALARM     3   // R/W  Range 0..59
#define RTC_ADDRESS_HOURS             4   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_HOURS_ALARM       5   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_DAY_OF_THE_WEEK   6   // R/W  Range 1..7
#define RTC_ADDRESS_DAY_OF_THE_MONTH  7   // R/W  Range 1..31
#define RTC_ADDRESS_MONTH             8   // R/W  Range 1..12
#define RTC_ADDRESS_YEAR              9   // R/W  Range 0..99
#define RTC_ADDRESS_REGISTER_A        10  // R/W[0..6]  R0[7]
#define RTC_ADDRESS_REGISTER_B        11  // R/W
#define RTC_ADDRESS_REGISTER_C        12  // RO
#define RTC_ADDRESS_REGISTER_D        13  // RO
//
// Date and time initial values.
// They are used if the RTC values are invalid during driver initialization
//
#define RTC_INIT_SECOND 0
#define RTC_INIT_MINUTE 0
#define RTC_INIT_HOUR   0
#define RTC_INIT_DAY    1
#define RTC_INIT_MONTH  1

//
// Register initial values
//
#define RTC_INIT_REGISTER_A 0x26
#define RTC_INIT_REGISTER_B 0x02
#define RTC_INIT_REGISTER_D 0x0

#pragma pack(1)
//
// Register A
//
typedef struct {
  UINT8 Rs : 4;   // Rate Selection Bits
  UINT8 Dv : 3;   // Divisor
  UINT8 Uip : 1;  // Update in progress
} RTC_REGISTER_A_BITS;

typedef union {
  RTC_REGISTER_A_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_A;

//
// Register B
//
typedef struct {
  UINT8 Dse : 1;  // 0 - Daylight saving disabled  1 - Daylight savings enabled
  UINT8 Mil : 1;  // 0 - 12 hour mode              1 - 24 hour mode
  UINT8 Dm : 1;   // 0 - BCD Format                1 - Binary Format
  UINT8 Sqwe : 1; // 0 - Disable SQWE output       1 - Enable SQWE output
  UINT8 Uie : 1;  // 0 - Update INT disabled       1 - Update INT enabled
  UINT8 Aie : 1;  // 0 - Alarm INT disabled        1 - Alarm INT Enabled
  UINT8 Pie : 1;  // 0 - Periodic INT disabled     1 - Periodic INT Enabled
  UINT8 Set : 1;  // 0 - Normal operation.         1 - Updates inhibited
} RTC_REGISTER_B_BITS;

typedef union {
  RTC_REGISTER_B_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_B;

//
// Register C
//
typedef struct {
  UINT8 Reserved : 4; // Read as zero.  Can not be written.
  UINT8 Uf : 1;       // Update End Interrupt Flag
  UINT8 Af : 1;       // Alarm Interrupt Flag
  UINT8 Pf : 1;       // Periodic Interrupt Flag
  UINT8 Irqf : 1;     // Iterrupt Request Flag = PF & PIE | AF & AIE | UF & UIE
} RTC_REGISTER_C_BITS;

typedef union {
  RTC_REGISTER_C_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_C;

//
// Register D
//
typedef struct {
  UINT8 Reserved : 7; // Read as zero.  Can not be written.
  UINT8 Vrt : 1;      // Valid RAM and Time
} RTC_REGISTER_D_BITS;

typedef union {
  RTC_REGISTER_D_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_D;

#pragma pack()

/**
  Initialize RTC.

  @param  Global            For global use inside this module.

  @retval EFI_DEVICE_ERROR  Initialization failed due to device error.
  @retval EFI_SUCCESS       Initialization successful.

**/
EFI_STATUS
PcRtcInit (
  IN PC_RTC_MODULE_GLOBALS  *Global
  );

/**
  Sets the current local time and date information.

  @param  Time                  A pointer to the current time.
  @param  Global                For global use inside this module.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The time could not be set due due to hardware error.

**/
EFI_STATUS
PcRtcSetTime (
  IN EFI_TIME               *Time,
  IN PC_RTC_MODULE_GLOBALS  *Global
  );

/**
  Returns the current time and date information, and the time-keeping capabilities
  of the hardware platform.

  @param  Time          A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities  An optional pointer to a buffer to receive the real time clock
                        device's capabilities.
  @param  Global        For global use inside this module.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Time is NULL.
  @retval EFI_DEVICE_ERROR       The time could not be retrieved due to hardware error.

**/
EFI_STATUS
PcRtcGetTime (
  OUT EFI_TIME              *Time,
  OUT EFI_TIME_CAPABILITIES *Capabilities, OPTIONAL
  IN  PC_RTC_MODULE_GLOBALS *Global
  );

/**
  Sets the system wakeup alarm clock time.

  @param  Enabled  Enable or disable the wakeup alarm.
  @param  Time     If Enable is TRUE, the time to set the wakeup alarm for.
                   If Enable is FALSE, then this parameter is optional, and may be NULL.
  @param  Global   For global use inside this module.

  @retval EFI_SUCCESS           If Enable is TRUE, then the wakeup alarm was enabled.
                                If Enable is FALSE, then the wakeup alarm was disabled.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be set due to a hardware error.
  @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.

**/
EFI_STATUS
PcRtcSetWakeupTime (
  IN BOOLEAN                Enable,
  IN EFI_TIME               *Time,  OPTIONAL
  IN PC_RTC_MODULE_GLOBALS  *Global
  );

/**
  Returns the current wakeup alarm clock setting.

  @param  Enabled  Indicates if the alarm is currently enabled or disabled.
  @param  Pending  Indicates if the alarm signal is pending and requires acknowledgement.
  @param  Time     The current alarm setting.
  @param  Global   For global use inside this module.

  @retval EFI_SUCCESS           The alarm settings were returned.
  @retval EFI_INVALID_PARAMETER Enabled is NULL.
  @retval EFI_INVALID_PARAMETER Pending is NULL.
  @retval EFI_INVALID_PARAMETER Time is NULL.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.

**/
EFI_STATUS
PcRtcGetWakeupTime (
  OUT BOOLEAN               *Enabled,
  OUT BOOLEAN               *Pending,
  OUT EFI_TIME              *Time,
  IN  PC_RTC_MODULE_GLOBALS *Global
  );

/**
  The user Entry Point for PcRTC module.

  This is the entrhy point for PcRTC module. It installs the UEFI runtime service
  including GetTime(),SetTime(),GetWakeupTime(),and SetWakeupTime().

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Others         Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializePcRtc (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  );

/**
  See if all fields of a variable of EFI_TIME type is correct.

  @param   Time   The time to be checked.

  @retval  EFI_INVALID_PARAMETER  Some fields of Time are not correct.
  @retval  EFI_SUCCESS            Time is a valid EFI_TIME variable.

**/
EFI_STATUS
RtcTimeFieldsValid (
  IN EFI_TIME *Time
  );

/**
  Converts time from EFI_TIME format defined by UEFI spec to RTC's.

  This function converts time from EFI_TIME format defined by UEFI spec to RTC's.
  If data mode of RTC is BCD, then converts EFI_TIME to it.
  If RTC is in 12-hour format, then converts EFI_TIME to it.

  @param   Time       On input, the time data read from UEFI to convert
                      On output, the time converted to RTC format
  @param   RegisterB  Value of Register B of RTC, indicating data mode
**/
VOID
ConvertEfiTimeToRtcTime (
  IN OUT EFI_TIME        *Time,
  IN     RTC_REGISTER_B  RegisterB
  );


/**
  Converts time read from RTC to EFI_TIME format defined by UEFI spec.

  This function converts raw time data read from RTC to the EFI_TIME format
  defined by UEFI spec.
  If data mode of RTC is BCD, then converts it to decimal,
  If RTC is in 12-hour format, then converts it to 24-hour format.

  @param   Time       On input, the time data read from RTC to convert
                      On output, the time converted to UEFI format
  @param   RegisterB  Value of Register B of RTC, indicating data mode
                      and hour format.

  @retval  EFI_INVALID_PARAMETER  Parameters passed in are invalid.
  @retval  EFI_SUCCESS            Convert RTC time to EFI time successfully.

**/
EFI_STATUS
ConvertRtcTimeToEfiTime (
  IN OUT EFI_TIME        *Time,
  IN     RTC_REGISTER_B  RegisterB
  );

/**
  Wait for a period for the RTC to be ready.

  @param    Timeout  Tell how long it should take to wait.

  @retval   EFI_DEVICE_ERROR   RTC device error.
  @retval   EFI_SUCCESS        RTC is updated and ready.  
**/
EFI_STATUS
RtcWaitToUpdate (
  UINTN Timeout
  );

/**
  See if field Day of an EFI_TIME is correct.

  @param    Time   Its Day field is to be checked.

  @retval   TRUE   Day field of Time is correct.
  @retval   FALSE  Day field of Time is NOT correct.
**/
BOOLEAN
DayValid (
  IN  EFI_TIME  *Time
  );

/**
  Check if it is a leapyear.

  @param    Time   The time to be checked.

  @retval   TRUE   It is a leapyear.
  @retval   FALSE  It is NOT a leapyear.
**/
BOOLEAN
IsLeapYear (
  IN EFI_TIME   *Time
  );

#endif
