/** @file
  Main file for time, timezone, and date shell level 2 and shell level 3 functions.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  Determine if String is a valid representation for a time or date.

  @param[in] String     The pointer to the string to test.
  @param[in] Char       The delimeter character.
  @param[in] Min        The minimum value allowed.
  @param[in] Max        The maximum value allowed.
  @param[in] MinusOk    Whether negative numbers are permitted.

  @retval TRUE    String is a valid representation.
  @retval FALSE   String is invalid.
**/
BOOLEAN
EFIAPI
InternalIsTimeLikeString (
  IN CONST CHAR16   *String,
  IN CONST CHAR16   Char,
  IN CONST UINTN    Min,
  IN CONST UINTN    Max,
  IN CONST BOOLEAN  MinusOk
  )
{
  UINTN Count;
  Count = 0;

  if (MinusOk) {
    //
    // A single minus is ok.
    //
    if (*String == L'-') {
      String++;
    }
  }

  //
  // the first char must be numeric.
  //
  if (!ShellIsDecimalDigitCharacter(*String)) {
    return (FALSE);
  }
  //
  // loop through the characters and use the lib function
  //
  for ( ; String != NULL && *String != CHAR_NULL ; String++){
    if (*String == Char) {
      Count++;
      if (Count > Max) {
        return (FALSE);
      }
      continue;
    }
    if (!ShellIsDecimalDigitCharacter(*String)) {
      return (FALSE);
    }
  }
  if (Count < Min) {
    return (FALSE);
  }
  return (TRUE);
}

/**
  Verify that the DateString is valid and if so set that as the current 
  date.

  @param[in] DateString     The pointer to a string representation of the date.

  @retval SHELL_INVALID_PARAMETER   DateString was NULL.
  @retval SHELL_INVALID_PARAMETER   DateString was mis-formatted.
  @retval SHELL_SUCCESS             The operation was successful.
**/
SHELL_STATUS
EFIAPI
CheckAndSetDate (
  IN CONST CHAR16 *DateString
  )
{
  EFI_TIME      TheTime;
  EFI_STATUS    Status;
  CHAR16        *DateStringCopy;
  CHAR16        *Walker;
  CHAR16        *Walker1;

  if (!InternalIsTimeLikeString(DateString, L'/', 2, 2, FALSE)) {
    return (SHELL_INVALID_PARAMETER);
  }

  Status = gRT->GetTime(&TheTime, NULL);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_UEFI_FUNC_WARN), gShellLevel2HiiHandle, L"gRT->GetTime", Status);
    return (SHELL_DEVICE_ERROR);
  }

  DateStringCopy = NULL;
  DateStringCopy = StrnCatGrow(&DateStringCopy, NULL, DateString, 0);
  if (DateStringCopy == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }
  Walker = DateStringCopy;

  TheTime.Month = 0xFF;
  TheTime.Day   = 0xFF;
  TheTime.Year  = 0xFFFF;

  Walker1 = StrStr(Walker, L"/");
  if (Walker1 != NULL && *Walker1 == L'/') {
    *Walker1 = CHAR_NULL;
  }

  TheTime.Month = (UINT8)ShellStrToUintn (Walker);
  if (Walker1 != NULL) {
    Walker = Walker1 + 1;
  }
  Walker1 = Walker!=NULL?StrStr(Walker, L"/"):NULL;
  if (Walker1 != NULL && *Walker1 == L'/') {
    *Walker1 = CHAR_NULL;
  }
  if (Walker != NULL && Walker[0] != CHAR_NULL) {
    TheTime.Day = (UINT8)ShellStrToUintn (Walker);
    if (Walker1 != NULL) {
      Walker = Walker1 + 1;
    }
    Walker1 = Walker!=NULL?StrStr(Walker, L"/"):NULL;
    if (Walker1 != NULL && *Walker1 == L'/') {
      *Walker1 = CHAR_NULL;
    }
    if (Walker != NULL && Walker[0] != CHAR_NULL) {
      TheTime.Year = (UINT16)ShellStrToUintn (Walker);
    }
  }

  if (TheTime.Year < 100) {
    if (TheTime.Year >= 98) {
      TheTime.Year = (UINT16)(1900 + TheTime.Year);
    } else {
      TheTime.Year = (UINT16)(2000 + TheTime.Year);
    }
  }

  Status = gRT->SetTime(&TheTime);

  if (!EFI_ERROR(Status)){
    return (SHELL_SUCCESS);
  }
  return (SHELL_INVALID_PARAMETER);
}

/**
  Function for 'date' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDate (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  EFI_TIME      TheTime;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CONST CHAR16  *Param1;

  ShellStatus  = SHELL_SUCCESS;
  ProblemParam = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (SfoParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    } else if (ShellCommandLineGetRawValue(Package, 2) != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // If there are 0 value parameters, then print the current date
      // else If there are any value paramerers, then print error
      //
      if (ShellCommandLineGetRawValue(Package, 1) == NULL) {
        //
        // get the current date
        //
        Status = gRT->GetTime(&TheTime, NULL);
        ASSERT_EFI_ERROR(Status);

        //
        // ShellPrintEx the date in SFO or regular format
        //
        if (ShellCommandLineGetFlag(Package, L"-sfo")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DATE_SFO_FORMAT), gShellLevel2HiiHandle, TheTime.Month, TheTime.Day, TheTime.Year);
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DATE_FORMAT), gShellLevel2HiiHandle, TheTime.Month, TheTime.Day, TheTime.Year);
        }
      } else {
        if (PcdGet8(PcdShellSupportLevel) == 2) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // perform level 3 operation here.
          //
          Param1 = ShellCommandLineGetRawValue(Package, 1);
          if (Param1 == NULL) {
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            ShellStatus = CheckAndSetDate(Param1);
          }
          if (ShellStatus != SHELL_SUCCESS) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, Param1);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
        }
      }
    }
  }
  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  //
  // return the status
  //
  return (ShellStatus);
}

//
// Note "-tz" is invalid for this (non-interactive) version of 'time'.
//
STATIC CONST SHELL_PARAM_ITEM TimeParamList2[] = {
  {L"-d", TypeValue},
  {NULL, TypeMax}
  };

STATIC CONST SHELL_PARAM_ITEM TimeParamList3[] = {
  {L"-d", TypeValue},
  {L"-tz", TypeValue},
  {NULL, TypeMax}
  };

/**
  Verify that the TimeString is valid and if so set that as the current 
  time.

  @param[in] TimeString     The pointer to a string representation of the time.
  @param[in] Tz             The value to set for TimeZone.
  @param[in] Daylight       The value to set for Daylight.

  @retval SHELL_INVALID_PARAMETER   TimeString was NULL.
  @retval SHELL_INVALID_PARAMETER   TimeString was mis-formatted.
  @retval SHELL_SUCCESS             The operation was successful.
**/
SHELL_STATUS
EFIAPI
CheckAndSetTime (
  IN CONST CHAR16 *TimeString,
  IN CONST INT16  Tz,
  IN CONST UINT8  Daylight
  )
{
  EFI_TIME      TheTime;
  EFI_STATUS    Status;
  CHAR16        *TimeStringCopy;
  CHAR16        *Walker1;
  CHAR16        *Walker2;

  if (TimeString != NULL && !InternalIsTimeLikeString(TimeString, L':', 1, 2, FALSE)) {
    return (SHELL_INVALID_PARAMETER);
  }
  if (Daylight != 0xFF &&((Daylight & (EFI_TIME_IN_DAYLIGHT|EFI_TIME_ADJUST_DAYLIGHT)) != Daylight)) {
    return (SHELL_INVALID_PARAMETER);
  }

  Status = gRT->GetTime(&TheTime, NULL);
  ASSERT_EFI_ERROR(Status);

  if (TimeString != NULL) {
    TimeStringCopy = NULL;
    TimeStringCopy = StrnCatGrow(&TimeStringCopy, NULL, TimeString, 0);
    Walker1          = TimeStringCopy;
    TheTime.Hour    = 0xFF;
    TheTime.Minute  = 0xFF;

    Walker2          = Walker1!=NULL?StrStr(Walker1, L":"):NULL;
    if (Walker2 != NULL && *Walker2 == L':') {
      *Walker2 = CHAR_NULL;
    }
    TheTime.Hour    = (UINT8)ShellStrToUintn (Walker1);
    if (Walker2 != NULL) {
      Walker1 = Walker2 + 1;
    }
    Walker2          = Walker1!=NULL?StrStr(Walker1, L":"):NULL;
    if (Walker2 != NULL && *Walker2 == L':') {
      *Walker2 = CHAR_NULL;
    }
    if (Walker1 != NULL && Walker1[0] != CHAR_NULL) {
      TheTime.Minute = (UINT8)ShellStrToUintn (Walker1);
      if (Walker2 != NULL) {
        Walker1 = Walker2 + 1;
      }
      if (Walker1 != NULL && Walker1[0] != CHAR_NULL) {
        TheTime.Second = (UINT8)ShellStrToUintn (Walker1);
      }
    }
    SHELL_FREE_NON_NULL(TimeStringCopy);
  }


  if ((Tz >= -1440 && Tz <= 1440)||(Tz == 0x7FF)) {
    TheTime.TimeZone = Tz;
  }

  if (Daylight != 0xFF) {
    TheTime.Daylight = Daylight;
  }

  Status = gRT->SetTime(&TheTime);

  if (!EFI_ERROR(Status)){
    return (SHELL_SUCCESS);
  }

  return (SHELL_INVALID_PARAMETER);
}

/**
  Function for 'time' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunTime (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *Message;
  EFI_TIME      TheTime;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  INT16         Tz;
  UINT8         Daylight;
  CONST CHAR16  *TempLocation;
  UINTN         TzMinutes;

  ShellStatus  = SHELL_SUCCESS;
  ProblemParam = NULL;

  //
  // Initialize variables
  //
  Message = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  if (PcdGet8(PcdShellSupportLevel) == 2) {
    Status = ShellCommandLineParseEx (TimeParamList2, &Package, &ProblemParam, TRUE, TRUE);
  } else {
    ASSERT(PcdGet8(PcdShellSupportLevel) == 3);
    Status = ShellCommandLineParseEx (TimeParamList3, &Package, &ProblemParam, TRUE, TRUE);
  }
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    Status = gRT->GetTime(&TheTime, NULL);
    ASSERT_EFI_ERROR(Status);
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    } else if (ShellCommandLineGetRawValue(Package, 2) != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // If there are no parameters, then print the current time
      //
      if (ShellCommandLineGetRawValue(Package, 1) == NULL
        && !ShellCommandLineGetFlag(Package, L"-d")
        && !ShellCommandLineGetFlag(Package, L"-tz")) {
        //
        // ShellPrintEx the current time
        //
        if (TheTime.TimeZone == EFI_UNSPECIFIED_TIMEZONE) {
          TzMinutes = 0;
        } else {
          TzMinutes = (ABS(TheTime.TimeZone)) % 60;
        }

        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_TIME_FORMAT),
          gShellLevel2HiiHandle,
          TheTime.Hour,
          TheTime.Minute,
          TheTime.Second,
          TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?L" ":(TheTime.TimeZone > 0?L"-":L"+"),
          TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?0:(ABS(TheTime.TimeZone)) / 60,
          TzMinutes
         );
         ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_CRLF), gShellLevel2HiiHandle);
      } else if (ShellCommandLineGetFlag(Package, L"-d") && ShellCommandLineGetValue(Package, L"-d") == NULL) {
        if (TheTime.TimeZone == EFI_UNSPECIFIED_TIMEZONE) {
          TzMinutes = 0;
        } else {
          TzMinutes = (ABS(TheTime.TimeZone)) % 60;
        }

        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_TIME_FORMAT),
          gShellLevel2HiiHandle,
          TheTime.Hour,
          TheTime.Minute,
          TheTime.Second,
          TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?L" ":(TheTime.TimeZone > 0?L"-":L"+"),
          TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?0:(ABS(TheTime.TimeZone)) / 60,
          TzMinutes
         );
          switch (TheTime.Daylight) {
            case 0:
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_TIME_DST0), gShellLevel2HiiHandle);
              break;
            case EFI_TIME_ADJUST_DAYLIGHT:
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_TIME_DST1), gShellLevel2HiiHandle);
              break;
            case EFI_TIME_IN_DAYLIGHT:
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_TIME_DST2), gShellLevel2HiiHandle);
              break;
            case EFI_TIME_IN_DAYLIGHT|EFI_TIME_ADJUST_DAYLIGHT:
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_TIME_DST3), gShellLevel2HiiHandle);
              break;
            default:
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_UEFI_FUNC_ERROR), gShellLevel2HiiHandle, L"gRT->GetTime", L"TheTime.Daylight", TheTime.Daylight);
          }
      } else {
        if (PcdGet8(PcdShellSupportLevel) == 2) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // perform level 3 operation here.
          //
          if ((TempLocation = ShellCommandLineGetValue(Package, L"-tz")) != NULL) {
            if (TempLocation[0] == L'-') {
              Tz = (INT16)(0 - ShellStrToUintn(++TempLocation));
            } else {
              Tz = (INT16)ShellStrToUintn(TempLocation);
            }
            if (!(Tz >= -1440 && Tz <= 1440) && Tz != EFI_UNSPECIFIED_TIMEZONE) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellLevel2HiiHandle, L"-tz");
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          } else {
            //
            // intentionally out of bounds value will prevent changing it...
            //
            Tz = 1441;
          }
          TempLocation = ShellCommandLineGetValue(Package, L"-d");
          if (TempLocation != NULL) {
            Daylight = (UINT8)ShellStrToUintn(TempLocation);
            if (Daylight != 0 && Daylight != 1 && Daylight != 3) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellLevel2HiiHandle, L"-d");
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          } else {
            //
            // invalid = will not use
            //
            Daylight = 0xFF;
          }
          if (ShellStatus == SHELL_SUCCESS) {
            ShellStatus = CheckAndSetTime(ShellCommandLineGetRawValue(Package, 1), Tz, Daylight);
            if (ShellStatus != SHELL_SUCCESS) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ShellCommandLineGetRawValue(Package, 1));
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          }
        }
      }
    }
  }

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  //
  // return the status
  //
  return (ShellStatus);
}

typedef struct {
  INT16         TimeZone;
  EFI_STRING_ID StringId;
} TIME_ZONE_ITEM;

STATIC CONST SHELL_PARAM_ITEM TimeZoneParamList2[] = {
  {L"-l", TypeFlag},
  {L"-f", TypeFlag},
  {NULL, TypeMax}
  };
STATIC CONST SHELL_PARAM_ITEM TimeZoneParamList3[] = {
  {L"-l", TypeFlag},
  {L"-f", TypeFlag},
  {L"-s", TypeValue},
  {NULL, TypeMax}
  };

  STATIC CONST TIME_ZONE_ITEM TimeZoneList[] = {
    {720, STRING_TOKEN (STR_TIMEZONE_M12)},
    {660, STRING_TOKEN (STR_TIMEZONE_M11)},
    {600, STRING_TOKEN (STR_TIMEZONE_M10)},
    {540, STRING_TOKEN (STR_TIMEZONE_M9)},
    {480, STRING_TOKEN (STR_TIMEZONE_M8)},
    {420, STRING_TOKEN (STR_TIMEZONE_M7)},
    {360, STRING_TOKEN (STR_TIMEZONE_M6)},
    {300, STRING_TOKEN (STR_TIMEZONE_M5)},
    {270, STRING_TOKEN (STR_TIMEZONE_M430)},
    {240, STRING_TOKEN (STR_TIMEZONE_M4)},
    {210, STRING_TOKEN (STR_TIMEZONE_M330)},
    {180, STRING_TOKEN (STR_TIMEZONE_M3)},
    {120, STRING_TOKEN (STR_TIMEZONE_M2)},
    {60 , STRING_TOKEN (STR_TIMEZONE_M1)},
    {0   , STRING_TOKEN (STR_TIMEZONE_0)},
    {-60  , STRING_TOKEN (STR_TIMEZONE_P1)},
    {-120 , STRING_TOKEN (STR_TIMEZONE_P2)},
    {-180 , STRING_TOKEN (STR_TIMEZONE_P3)},
    {-210 , STRING_TOKEN (STR_TIMEZONE_P330)},
    {-240 , STRING_TOKEN (STR_TIMEZONE_P4)},
    {-270 , STRING_TOKEN (STR_TIMEZONE_P430)},
    {-300 , STRING_TOKEN (STR_TIMEZONE_P5)},
    {-330 , STRING_TOKEN (STR_TIMEZONE_P530)},
    {-345 , STRING_TOKEN (STR_TIMEZONE_P545)},
    {-360 , STRING_TOKEN (STR_TIMEZONE_P6)},
    {-390 , STRING_TOKEN (STR_TIMEZONE_P630)},
    {-420 , STRING_TOKEN (STR_TIMEZONE_P7)},
    {-480 , STRING_TOKEN (STR_TIMEZONE_P8)},
    {-540 , STRING_TOKEN (STR_TIMEZONE_P9)},
    {-570 , STRING_TOKEN (STR_TIMEZONE_P930)},
    {-600 , STRING_TOKEN (STR_TIMEZONE_P10)},
    {-660 , STRING_TOKEN (STR_TIMEZONE_P11)},
    {-720 , STRING_TOKEN (STR_TIMEZONE_P12)},
    {-780 , STRING_TOKEN (STR_TIMEZONE_P13)},
    {-840 , STRING_TOKEN (STR_TIMEZONE_P14)}
};

/**
  Verify that the TimeZoneString is valid and if so set that as the current 
  timezone.

  @param[in] TimeZoneString     The pointer to a string representation of the timezone.

  @retval SHELL_INVALID_PARAMETER   TimeZoneString was NULL.
  @retval SHELL_INVALID_PARAMETER   TimeZoneString was mis-formatted.
  @retval SHELL_SUCCESS             The operation was successful.
**/
SHELL_STATUS
EFIAPI
CheckAndSetTimeZone (
  IN CONST CHAR16 *TimeZoneString
  )
{
  EFI_TIME      TheTime;
  EFI_STATUS    Status;
  CHAR16        *TimeZoneCopy;
  CHAR16        *Walker;
  CHAR16        *Walker2;
  UINTN         LoopVar;

  if (TimeZoneString == NULL) {
    return (SHELL_INVALID_PARAMETER);
  }

  if (TimeZoneString != NULL && !InternalIsTimeLikeString(TimeZoneString, L':', 1, 1, TRUE)) {
    return (SHELL_INVALID_PARAMETER);
  }

  Status = gRT->GetTime(&TheTime, NULL);
  if (EFI_ERROR(Status)) {
    return (SHELL_DEVICE_ERROR);
  }

  TimeZoneCopy = NULL;
  TimeZoneCopy = StrnCatGrow(&TimeZoneCopy, NULL, TimeZoneString, 0);
  if (TimeZoneCopy == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }
  Walker = TimeZoneCopy;
  Walker2 = StrStr(Walker, L":");
  if (Walker2 != NULL && *Walker2 == L':') {
    *Walker2 = CHAR_NULL;
  }
  if (*Walker == L'-') {
    TheTime.TimeZone = (INT16)((ShellStrToUintn (++Walker)) * 60);
  } else {
    TheTime.TimeZone = (INT16)((ShellStrToUintn (Walker)) * -60);
  }
  if (Walker2 != NULL) {
    Walker = Walker2 + 1;
  }
  if (Walker != NULL && Walker[0] != CHAR_NULL) {
    if (TheTime.TimeZone < 0) {
      TheTime.TimeZone = (INT16)(TheTime.TimeZone - (UINT8)ShellStrToUintn (Walker));
    } else {
      TheTime.TimeZone = (INT16)(TheTime.TimeZone + (UINT8)ShellStrToUintn (Walker));
    }
  }

  Status = EFI_INVALID_PARAMETER;

  for ( LoopVar = 0
      ; LoopVar < sizeof(TimeZoneList) / sizeof(TimeZoneList[0])
      ; LoopVar++
     ){
    if (TheTime.TimeZone == TimeZoneList[LoopVar].TimeZone) {
        Status = gRT->SetTime(&TheTime);
        break;
    }
  }

  FreePool(TimeZoneCopy);

  if (!EFI_ERROR(Status)){
    return (SHELL_SUCCESS);
  }
  return (SHELL_INVALID_PARAMETER);
}


/**
  Function for 'timezone' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunTimeZone (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // non interactive
  //
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  UINT8         LoopVar;
  EFI_TIME      TheTime;
  BOOLEAN       Found;
  UINTN         TzMinutes;

  ShellStatus  = SHELL_SUCCESS;
  ProblemParam = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  if (PcdGet8(PcdShellSupportLevel) == 2) {
    Status = ShellCommandLineParse (TimeZoneParamList2, &Package, &ProblemParam, TRUE);
  } else {
    ASSERT(PcdGet8(PcdShellSupportLevel) == 3);
    Status = ShellCommandLineParseEx (TimeZoneParamList3, &Package, &ProblemParam, TRUE, TRUE);
  }
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetCount(Package) > 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    } else if (ShellCommandLineGetFlag(Package, L"-s")) {
      if ((ShellCommandLineGetFlag(Package, L"-l")) || (ShellCommandLineGetFlag(Package, L"-f"))) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"-l or -f");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        ASSERT(PcdGet8(PcdShellSupportLevel) == 3);
        if (ShellCommandLineGetValue(Package, L"-s") == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellLevel2HiiHandle, L"-s");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // Set the time zone
          //
          ShellStatus = CheckAndSetTimeZone(ShellCommandLineGetValue(Package, L"-s"));
          if (ShellStatus != SHELL_SUCCESS) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ShellCommandLineGetValue(Package, L"-s"));
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
        }
      }
    } else if (ShellCommandLineGetFlag(Package, L"-l")) {
      //
      // Print a list of all time zones
      //
      for ( LoopVar = 0
          ; LoopVar < sizeof(TimeZoneList) / sizeof(TimeZoneList[0])
          ; LoopVar++
         ){
        ShellPrintHiiEx (-1, -1, NULL, TimeZoneList[LoopVar].StringId, gShellLevel2HiiHandle);
      }
    } else {
      //
      // Get Current Time Zone Info
      //
      Status = gRT->GetTime(&TheTime, NULL);
      ASSERT_EFI_ERROR(Status);

      if (TheTime.TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
        Found = FALSE;
        for ( LoopVar = 0
            ; LoopVar < sizeof(TimeZoneList) / sizeof(TimeZoneList[0])
            ; LoopVar++
           ){
          if (TheTime.TimeZone == TimeZoneList[LoopVar].TimeZone) {
            if (ShellCommandLineGetFlag(Package, L"-f")) {
              //
              //  Print all info about current time zone
              //
              ShellPrintHiiEx (-1, -1, NULL, TimeZoneList[LoopVar].StringId, gShellLevel2HiiHandle);
            } else {
              //
              // Print basic info only
              //
              if (TheTime.TimeZone == EFI_UNSPECIFIED_TIMEZONE) {
                TzMinutes = 0;
              } else {
                TzMinutes = (ABS(TheTime.TimeZone)) % 60;
              }

              ShellPrintHiiEx (
                -1,
                -1,
                NULL,
                STRING_TOKEN(STR_TIMEZONE_SIMPLE),
                gShellLevel2HiiHandle,
                TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?0:(TheTime.TimeZone > 0?L"-":L"+"),
                TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?0:(ABS(TheTime.TimeZone)) / 60,
                TzMinutes);
            }
            Found = TRUE;
            break;
          }
        }
        if (!Found) {
          //
          // Print basic info only
          //
          if (TheTime.TimeZone == EFI_UNSPECIFIED_TIMEZONE) {
            TzMinutes = 0;
          } else {
            TzMinutes = (ABS(TheTime.TimeZone)) % 60;
          }
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN(STR_TIMEZONE_SIMPLE),
            gShellLevel2HiiHandle,
            TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?0:(TheTime.TimeZone > 0?L"-":L"+"),
            TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?0:(ABS(TheTime.TimeZone)) / 60,
            TzMinutes);
          if (ShellCommandLineGetFlag(Package, L"-f")) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN(STR_TIMEZONE_NI), gShellLevel2HiiHandle);
          }
        }
      } else {
        //
        // TimeZone was EFI_UNSPECIFIED_TIMEZONE (unknown) from GetTime()
        //
      }
    }
  }

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}
