/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

 LegacySpeaker.h

Abstract:

  Speaker enabling related data

--*/

#ifndef _DXE_LEGACY_SPEAKER_H
#define _DXE_LEGACY_SPEAKER_H

#include "PlatformDxe.h"

//
// Speaker Related Port Information
//
#define EFI_TIMER_COUNTER_PORT            0x40
#define EFI_TIMER_CONTROL_PORT            0x43
#define EFI_TIMER_2_PORT                  0x42
#define EFI_SPEAKER_CONTROL_PORT          0x61

#define EFI_SPEAKER_OFF_MASK              0xFC

#define EFI_DEFAULT_BEEP_FREQUENCY        0x500

//
// Default Intervals/Beep Duration
//
#define EFI_DEFAULT_LONG_BEEP_DURATION    0x70000
#define EFI_DEFAULT_SHORT_BEEP_DURATION   0x50000
#define EFI_DEFAULT_BEEP_TIME_INTERVAL    0x20000


EFI_STATUS
EFIAPI
ProgramToneFrequency (
  IN  EFI_SPEAKER_IF_PROTOCOL           * This,
  IN  UINT16                            Frequency
  );


EFI_STATUS
EFIAPI
GenerateBeepTone (
  IN  EFI_SPEAKER_IF_PROTOCOL           * This,
  IN  UINTN                             NumberOfBeeps,
  IN  UINTN                             BeepDuration,
  IN  UINTN                             TimeInterval
  );

EFI_STATUS
TurnOnSpeaker (
  );

EFI_STATUS
TurnOffSpeaker (
  );

#endif
