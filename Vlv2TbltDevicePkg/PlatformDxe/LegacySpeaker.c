/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

 LegacySpeaker.c

Abstract:

  This file implements DXE for Legacy Speaker.

--*/

#include "LegacySpeaker.h"

/**

  This function will enable the speaker to generate beep

  @retval EFI_STATUS

**/
EFI_STATUS
TurnOnSpeaker (
  )
{
  UINT8                   Data;
  Data = IoRead8 (EFI_SPEAKER_CONTROL_PORT);
  Data |= 0x03;
  IoWrite8(EFI_SPEAKER_CONTROL_PORT, Data);
  return EFI_SUCCESS;
}

/**

  This function will stop beep from speaker.

  @retval Status

**/
EFI_STATUS
TurnOffSpeaker (
  )
{
  UINT8                   Data;

  Data = IoRead8 (EFI_SPEAKER_CONTROL_PORT);
  Data &= 0xFC;
  IoWrite8(EFI_SPEAKER_CONTROL_PORT, Data);
  return EFI_SUCCESS;
}

/**
  Generate beep sound based upon number of beeps and duration of the beep

  @param NumberOfBeeps     Number of beeps which user want to produce
  @param BeepDuration      Duration for speaker gate need to be enabled
  @param TimeInterval      Interval between each beep

  @retval      Does not return if the reset takes place.
               EFI_INVALID_PARAMETER   If ResetType is invalid.

**/
EFI_STATUS
OutputBeep (
  IN     UINTN                              NumberOfBeep,
  IN     UINTN                              BeepDuration,
  IN     UINTN                              TimeInterval
  )
{
  UINTN           Num;

  for (Num=0; Num < NumberOfBeep; Num++) {
    TurnOnSpeaker ();
    //
    // wait some time,at least 120us
    //
    gBS->Stall (BeepDuration);
    TurnOffSpeaker();
    gBS->Stall (TimeInterval);
  }

  return EFI_SUCCESS;
}

/**
  This function will program the speaker tone frequency. The value should be with 64k
  boundary since it takes only 16 bit value which gets programmed in two step IO opearattion

  @param  Frequency     A value which should be 16 bit only.

  @retval EFI_SUCESS

**/
EFI_STATUS
EFIAPI    
ProgramToneFrequency (
  IN EFI_SPEAKER_IF_PROTOCOL            * This,
  IN  UINT16                            Frequency
  )
{
  UINT8                   Data;

  Data = 0xB6;
  IoWrite8(EFI_TIMER_CONTROL_PORT, Data);

  Data = (UINT8)(Frequency & 0x00FF);
  IoWrite8(EFI_TIMER_2_PORT, Data);
  Data = (UINT8)((Frequency & 0xFF00) >> 8);
  IoWrite8(EFI_TIMER_2_PORT, Data);
  return EFI_SUCCESS;
}

/**
  This function will generate the beep for specified duration.

  @param NumberOfBeeps     Number of beeps which user want to produce
  @param BeepDuration      Duration for speaker gate need to be enabled
  @param TimeInterval      Interval between each beep

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
GenerateBeepTone (
  IN EFI_SPEAKER_IF_PROTOCOL            * This,
  IN  UINTN                             NumberOfBeeps,
  IN  UINTN                             BeepDuration,
  IN  UINTN                             TimeInterval
  )
{

  if ((NumberOfBeeps == 1) && (BeepDuration == 0) && (TimeInterval == 0)) {
    TurnOnSpeaker ();
    return EFI_SUCCESS;
  }

  if ((NumberOfBeeps == 0) && (BeepDuration == 0) && (TimeInterval == 0)) {
    TurnOffSpeaker ();
    return EFI_SUCCESS;
  }

  if (BeepDuration == 0) {
    BeepDuration = EFI_DEFAULT_SHORT_BEEP_DURATION;
  }

  if (TimeInterval == 0) {
    TimeInterval = EFI_DEFAULT_BEEP_TIME_INTERVAL;
  }

  OutputBeep (NumberOfBeeps, BeepDuration, TimeInterval);
  return EFI_SUCCESS;


}
