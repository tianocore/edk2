/** @file
  SMM Firmware Volume Block protocol is related to EDK II-specific implementation of
  FVB driver, provides control over block-oriented firmware devices and is intended 
  to use in the EFI SMM environment.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SMM_FIRMWARE_VOLUME_BLOCK_H__
#define __SMM_FIRMWARE_VOLUME_BLOCK_H__

#include <Protocol/FirmwareVolumeBlock.h>

#define EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID \
  { \
    0xf52fc9ff, 0x8025, 0x4432, { 0xa5, 0x3b, 0xb4, 0x7b, 0x5e, 0x9, 0xdb, 0xf9 } \
  }

//
// SMM Firmware Volume Block protocol structure is the same as Firmware Volume Block 
// protocol. The SMM one is intend to run in SMM environment, which means it can be  
// used by SMM drivers after ExitPmAuth. 
// 
typedef EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL;

extern EFI_GUID gEfiSmmFirmwareVolumeBlockProtocolGuid;

#endif
