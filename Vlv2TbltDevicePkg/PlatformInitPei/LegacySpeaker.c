/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:


 LegacySpeaker.c

Abstract:

  This file implements PEIM for Legacy Speaker. This file is valid for platforms both
  on IA32 and Itanium Product Family

--*/

#include "PlatformEarlyInit.h"

EFI_STATUS
OutputBeep (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  UINTN             NumberOfBeep,
  IN  UINTN             BeepDuration,
  IN  UINTN             TimerInterval
  );

/**
  This function will enable the speaker to generate beep

  @param PeiServices     PeiServices to locate PPI

  @retval EFI_STATUS

**/
EFI_STATUS
TurnOnSpeaker (
  IN  CONST EFI_PEI_SERVICES    **PeiServices
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

  @param  PeiServices     PeiServices to locate PPI

  @retval Status

**/
EFI_STATUS
TurnOffSpeaker (
  IN  CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  UINT8                   Data;

  Data = IoRead8 (EFI_SPEAKER_CONTROL_PORT);
  Data &= 0xFC;
  IoWrite8(EFI_SPEAKER_CONTROL_PORT, Data);
  return EFI_SUCCESS;
}


EFI_STATUS
OutputBeep (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  UINTN             NumberOfBeep,
  IN  UINTN             BeepDuration,
  IN  UINTN             TimeInterval
  )
{
  UINTN           Num;
  EFI_PEI_STALL_PPI*  StallPpi;

  (**PeiServices).LocatePpi (PeiServices, &gEfiPeiStallPpiGuid, 0, NULL, (void **)&StallPpi);

  for (Num=0; Num < NumberOfBeep; Num++) {
    TurnOnSpeaker (PeiServices);
    StallPpi->Stall(PeiServices, StallPpi, BeepDuration);
    TurnOffSpeaker(PeiServices);
    StallPpi->Stall(PeiServices, StallPpi, TimeInterval);
  }

  return EFI_SUCCESS;
}

/**
  This function will program the speaker tone frequency. The value should be with 64k
  boundary since it takes only 16 bit value which gets programmed in two step IO opearattion

  Frequency     - A value which should be 16 bit only.

  EFI_SUCESS

**/
EFI_STATUS
EFIAPI
ProgramToneFrequency (
  IN  CONST EFI_PEI_SERVICES                  **PeiServices,
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

  @param  PeiServices       PeiServices to locate various PPIs
  @param  NumberOfBeeps     Number of beeps which user want to produce
  @param  BeepDuration      Duration for speaker gate need to be enabled
  @param  TimeInterval      Interval between each beep

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
GenerateBeepTone (
  IN  CONST EFI_PEI_SERVICES                  **PeiServices,
  IN  UINTN                             NumberOfBeeps,
  IN  UINTN                             BeepDuration,
  IN  UINTN                             TimeInterval
  )
{

  if ((NumberOfBeeps == 1) && (BeepDuration == 0) && (TimeInterval == 0)) {
    TurnOnSpeaker (PeiServices);
    return EFI_SUCCESS;
  }

  if ((NumberOfBeeps == 0) && (BeepDuration == 0) && (TimeInterval == 0)) {
    TurnOffSpeaker (PeiServices);
    return EFI_SUCCESS;
  }

  if (BeepDuration == 0) {
    BeepDuration = EFI_DEFAULT_SHORT_BEEP_DURATION;
  }

  if (TimeInterval == 0) {
    TimeInterval = EFI_DEFAULT_BEEP_TIME_INTERVAL;
  }

  OutputBeep (PeiServices, NumberOfBeeps, BeepDuration, TimeInterval);
  return EFI_SUCCESS;


}

