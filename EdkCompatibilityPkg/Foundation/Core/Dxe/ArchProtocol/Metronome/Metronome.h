/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Metronome.h

Abstract:

  Metronome Architectural Protocol as defined in DXE CIS

  This code abstracts the DXE core to provide delay services.

--*/

#ifndef _ARCH_PROTOCOL_METRONOME_H_
#define _ARCH_PROTOCOL_METRONOME_H_

//
// Global ID for the Metronome Architectural Protocol
//
#define EFI_METRONOME_ARCH_PROTOCOL_GUID \
  { 0x26baccb2, 0x6f42, 0x11d4, {0xbc, 0xe7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} }

//
// Declare forward reference for the Metronome Architectural Protocol
//
EFI_FORWARD_DECLARATION (EFI_METRONOME_ARCH_PROTOCOL);

typedef 
EFI_STATUS
(EFIAPI *EFI_METRONOME_WAIT_FOR_TICK) (
   IN EFI_METRONOME_ARCH_PROTOCOL   *This,
   IN UINT32                        TickNumber
  );
/*++

Routine Description:

  The WaitForTick() function waits for the number of ticks specified by 
  TickNumber from a known time source in the platform.  If TickNumber of 
  ticks are detected, then EFI_SUCCESS is returned.  The actual time passed 
  between entry of this function and the first tick is between 0 and 
  TickPeriod 100 nS units.  If you want to guarantee that at least TickPeriod 
  time has elapsed, wait for two ticks.  This function waits for a hardware 
  event to determine when a tick occurs.  It is possible for interrupt 
  processing, or exception processing to interrupt the execution of the 
  WaitForTick() function.  Depending on the hardware source for the ticks, it 
  is possible for a tick to be missed.  This function cannot guarantee that 
  ticks will not be missed.  If a timeout occurs waiting for the specified 
  number of ticks, then EFI_TIMEOUT is returned.

Arguments:

  This       - The EFI_METRONOME_ARCH_PROTOCOL instance.

  TickNumber - Number of ticks to wait.

Returns: 

  EFI_SUCCESS - The wait for the number of ticks specified by TickNumber 
                succeeded.

  EFI_TIMEOUT - A timeout occurred waiting for the specified number of ticks.

--*/

//
// Interface stucture for the Metronome Architectural Protocol
//
struct _EFI_METRONOME_ARCH_PROTOCOL {
  EFI_METRONOME_WAIT_FOR_TICK  WaitForTick;
  UINT32                       TickPeriod;
};

/*++

  Protocol Description:
    This protocol provides access to a known time source in the platform to the
    core.  The core uses this known time source to produce core services that 
    require calibrated delays.  

  Parameters:

    WaitForTick - Waits for a specified number of ticks from a known time source 
                  in the platform.  The actual time passed between entry of this 
                  function and the first tick is between 0 and TickPeriod 100 nS 
                  units.  If you want to guarantee that at least TickPeriod time 
                  has elapsed, wait for two ticks.

    TickPeriod  - The period of platform's known time source in 100 nS units.  
                  This value on any platform must be at least 10 uS, and must not 
                  exceed 200 uS.  The value in this field is a constant that must 
                  not be modified after the Metronome architectural protocol is 
                  installed.  All consumers must treat this as a read-only field.
    
--*/

extern EFI_GUID gEfiMetronomeArchProtocolGuid;

#endif
