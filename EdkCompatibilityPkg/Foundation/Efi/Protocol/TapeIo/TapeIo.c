/*++
  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    TapeIo.c
    
Abstract: 
  EFI_TAPE_IO_PROTOCOL as defined in the UEFI 2.0.
  Provide services to control and access a tape device.

Revision History

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (TapeIo)

EFI_GUID gEfiTapeIoProtocolGuid = EFI_TAPE_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiTapeIoProtocolGuid, "Tape IO protoco", "UEFI 2.0 Tape IO protocol");
