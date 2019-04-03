/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  Speaker.h

Abstract:

  EFI Speaker Interface Protocol



--*/

#ifndef _PEI_SPEAKER_IF_H
#define _PEI_SPEAKER_IF_H

//
// Global ID Speaker Interface
//
#define PEI_SPEAKER_INTERFACE_PPI_GUID \
  { \
    0x30ac275e, 0xbb30, 0x4b84, 0xa1, 0xcd, 0x0a, 0xf1, 0x32, 0x2c, 0x89, 0xc0 \
  }

typedef struct _PEI_SPEAKER_IF_PPI PEI_SPEAKER_IF_PPI;

//
// Beep Code
//
typedef
EFI_STATUS
(EFIAPI *EFI_SPEAKER_GENERATE_BEEP) (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN UINTN                            NumberOfBeep,
  IN UINTN                            BeepDuration,
  IN UINTN                            TimeInterval
  );

//
// Set Frequency
//
typedef
EFI_STATUS
(EFIAPI *EFI_SPEAKER_SET_FREQUENCY) (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN UINT16                           Frequency
  );

//
// Protocol definition
//
typedef struct _PEI_SPEAKER_IF_PPI {
  EFI_SPEAKER_SET_FREQUENCY SetSpeakerToneFrequency;
  EFI_SPEAKER_GENERATE_BEEP GenerateBeep;
} PEI_SPEAKER_IF_PPI;

extern EFI_GUID gPeiSpeakerInterfacePpiGuid;
#endif
