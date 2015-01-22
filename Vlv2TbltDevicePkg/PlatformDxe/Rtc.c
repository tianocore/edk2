/** @file
  Adjust Default System Time.
  
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
--*/

#include <PlatformDxe.h>

//
// Date and time initial values.
// They are used if the RTC values are invalid during driver initialization
//
#define RTC_INIT_SECOND 0
#define RTC_INIT_MINUTE 0
#define RTC_INIT_HOUR   0
 
CHAR16  mBiosReleaseDate[20];    
  
/**
  Convert a single character to number.
  It assumes the input Char is in the scope of L'0' ~ L'9' and L'A' ~ L'F'
  
  @param Char    The input char which need to change to a hex number.
  
**/
UINTN
CharToUint (
  IN CHAR16                           Char
  )
{
  if ((Char >= L'0') && (Char <= L'9')) {
    return (UINTN) (Char - L'0');
  }

  if ((Char >= L'A') && (Char <= L'F')) {
    return (UINTN) (Char - L'A' + 0xA);
  }

  ASSERT (FALSE);
  return 0;
}

/**
  See if YEAR field of a variable of EFI_TIME type is correct.

  @param   Time   The time to be checked.

  @retval  EFI_INVALID_PARAMETER  Some fields of Time are not correct.
  @retval  EFI_SUCCESS            Time is a valid EFI_TIME variable.

**/
EFI_STATUS
CheckRtcTimeFields (
  IN EFI_TIME *Time
  )
{
  UINT16 YearBuilt;
  
  YearBuilt = (UINT16)(CharToUint(mBiosReleaseDate[8])*10 + CharToUint(mBiosReleaseDate[9]) + 2000);
  
  if ((Time->Year) < YearBuilt) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  ExitPmAuth Protocol notification event handler, which set initial system time to be
  the time when BIOS was built.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
AdjustDefaultRtcTimeCallback (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS      Status;
  EFI_TIME        EfiTime;
  CHAR16          BiosVersion[60];    
  CHAR16          BiosReleaseTime[20];    
  //
  // Get BIOS built time from Bios-ID. 
  //
  
  SetMem(BiosVersion, sizeof(BiosVersion), 0);
  SetMem(mBiosReleaseDate, sizeof(mBiosReleaseDate), 0);
  SetMem(BiosReleaseTime, sizeof(BiosReleaseTime), 0);
    
  Status = GetBiosVersionDateTime (BiosVersion, mBiosReleaseDate, BiosReleaseTime);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR (Status)) {
    return; 
  }
  
  //
  // Get current RTC time.
  // 
  Status = gRT->GetTime (&EfiTime, NULL);
 
  //
  // Validate RTC time fields
  //
  Status = CheckRtcTimeFields (&EfiTime);
  
  if (EFI_ERROR (Status)) {
    //
    // Date such as Dec 28th of 2015
    //
    // Month
    // BiosReleaseDate[0] = '1';
    // BiosReleaseDate[1] = '2';
    //
    // Day
    // BiosReleaseDate[3] = '2';
    // BiosReleaseDate[4] = '8';
    //  
    //
    // Year
    //
    // BiosReleaseDate[6] = '2';
    // BiosReleaseDate[7] = '0';
    // BiosReleaseDate[8] = '1'
    // BiosReleaseDate[9] = '5';
    
    EfiTime.Second = RTC_INIT_SECOND;
    EfiTime.Minute = RTC_INIT_MINUTE;
    EfiTime.Hour   = RTC_INIT_HOUR;
    EfiTime.Day    = (UINT8)(CharToUint(mBiosReleaseDate[3])*10 + CharToUint(mBiosReleaseDate[4]));
    EfiTime.Month  = (UINT8)(CharToUint(mBiosReleaseDate[0])*10 + CharToUint(mBiosReleaseDate[1]));
    EfiTime.Year   = (UINT16)(CharToUint(mBiosReleaseDate[8])*10 + CharToUint(mBiosReleaseDate[9]) + 2000);
    EfiTime.Nanosecond  = 0;
    EfiTime.TimeZone = EFI_UNSPECIFIED_TIMEZONE;
    EfiTime.Daylight = 1; 

    DEBUG ((EFI_D_INFO, "Day:%d Month:%d Year:%d \n", (UINT32)EfiTime.Day, (UINT32)EfiTime.Month, (UINT32)EfiTime.Year));

    //
    // Reset time value according to new RTC configuration
    //
    Status = gRT->SetTime (&EfiTime);
    ASSERT_EFI_ERROR(Status);
  }

  return;
}
