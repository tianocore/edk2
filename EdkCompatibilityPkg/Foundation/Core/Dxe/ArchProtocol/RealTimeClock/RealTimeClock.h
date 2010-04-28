/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  RealTimeClock.h

Abstract:

  Real Time clock Architectural Protocol as defined in Tiano

  This code abstracts time and data functions. Used to provide
  Time and date related EFI runtime services.

  The GetTime (), SetTime (), GetWakeupTime (), and SetWakeupTime () EFI 1.0
  services are added to the EFI system table and the 
  EFI_REAL_TIME_CLOCK_ARCH_PROTOCOL_GUID protocol is registered with a NULL 
  pointer.

  No CRC of the EFI system table is required, as it is done in the DXE core.

--*/

#ifndef _ARCH_PROTOCOL_REAL_TIME_CLOCK_H_
#define _ARCH_PROTOCOL_REAL_TIME_CLOCK_H_

//
// Global ID for the Real Time Clock Architectural Protocol
//
#define EFI_REAL_TIME_CLOCK_ARCH_PROTOCOL_GUID \
  { 0x27CFAC87, 0x46CC, 0x11d4, {0x9A, 0x38, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D} }

extern EFI_GUID gEfiRealTimeClockArchProtocolGuid;

#endif
