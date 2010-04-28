/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    DebugSerialIo.c

Abstract:

  Debug Serial IO protocol.

  Abstraction of a basic serial device. Targeted at 16550 UART, but
  could be much more generic.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (DebugSerialIo)

EFI_GUID  gEfiDebugSerialIoProtocolGuid = EFI_DEBUG_SERIAL_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiDebugSerialIoProtocolGuid, "DebugSerialIo Protocol", "EFI 1.0 DebugSerial IO Protocol");
