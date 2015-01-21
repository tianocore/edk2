/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  LegacyMetronome.h

Abstract:

  Driver implementing the EFI 2.0 metronome protocol using the legacy PORT 61
  timer.

--*/

#ifndef _LEGACY_METRONOME_H
#define _LEGACY_METRONOME_H

//
// Statements that include other files
//
#include "Protocol/Metronome.h"
#include "Protocol/CpuIo.h"
#include "Library/DebugLib.h"
#include "Library/UefiBootServicesTableLib.h"


//
// Private definitions
//
#define TICK_PERIOD         300
#define REFRESH_PORT        0x61
#define REFRESH_ON          0x10
#define REFRESH_OFF         0x00
#define TIMER1_CONTROL_PORT 0x43
#define TIMER1_COUNT_PORT   0x41
#define LOAD_COUNTER1_LSB   0x54
#define COUNTER1_COUNT      0x12

//
// Function Prototypes
//
/**
  Waits for the TickNumber of ticks from a known platform time source.

  @param This                 Pointer to the protocol instance.
  @param TickNumber           Tick Number to be waited

  @retval EFI_SUCCESS         If number of ticks occurred.
  @retval EFI_NOT_FOUND       Could not locate CPU IO protocol

**/
EFI_STATUS
EFIAPI
WaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  );

#endif
