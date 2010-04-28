/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  BsSerialStatusCode.h
   
Abstract:

  Lib to provide Serial I/O status code routines.  This uses the PEI library
  print functions.

--*/

#ifndef _EFI_BS_SERIAL_STATUS_CODE_H_
#define _EFI_BS_SERIAL_STATUS_CODE_H_

//
// Statements that include other files
//
#include "Tiano.h"
#include "Pei.h"
#include "PeiLib.h"
#include "EfiRuntimeLib.h"
#include "BsSerialStatusCodeLib.h"

//
// GUID consumed
//
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)

//
// ---------------------------------------------
// UART Register Offsets
// ---------------------------------------------
//
#define BAUD_LOW_OFFSET   0x00
#define BAUD_HIGH_OFFSET  0x01
#define IER_OFFSET        0x01
#define LCR_SHADOW_OFFSET 0x01
#define FCR_SHADOW_OFFSET 0x02
#define IR_CONTROL_OFFSET 0x02
#define FCR_OFFSET        0x02
#define EIR_OFFSET        0x02
#define BSR_OFFSET        0x03
#define LCR_OFFSET        0x03
#define MCR_OFFSET        0x04
#define LSR_OFFSET        0x05
#define MSR_OFFSET        0x06

//
// ---------------------------------------------
// UART Register Bit Defines
// ---------------------------------------------
//
#define LSR_TXRDY 0x20
#define LSR_RXDA  0x01
#define DLAB      0x01

//
// Globals for Serial Port settings
//
extern UINT16 gComBase;
extern UINTN  gBps;
extern UINT8  gData;
extern UINT8  gStop;
extern UINT8  gParity;
extern UINT8  gBreakSet;

VOID
DebugSerialPrint (
  IN UINT8    *OutputString
  );

VOID
DebugSerialWrite (
  IN UINT8  Character
  );

#endif
