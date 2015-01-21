/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

Module Name:

    LpcSio.c

Abstract: Sio implementation

Revision History

--*/

#include "LpcDriver.h"
#include <Library/S3BootScriptLib.h>

VOID
WriteRegister (
  IN  UINT8   Index,
  IN  UINT8   Data
  );

typedef struct {
  UINT8 Register;
  UINT8 Value;
} EFI_SIO_TABLE;

EFI_SIO_TABLE mSioTable[] = {
  //
  // Init keyboard controller
  //
  { REG_LOGICAL_DEVICE, SIO_KEYBOARD },
  { BASE_ADDRESS_HIGH, 0x00 },
  { BASE_ADDRESS_LOW, 0x60 },
  { BASE_ADDRESS_HIGH2, 0x00 },
  { BASE_ADDRESS_LOW2, 0x64 },
  { PRIMARY_INTERRUPT_SELECT, 0x01 },
  { ACTIVATE, 0x1 },

  //
  // Init Mouse controller
  //
  { REG_LOGICAL_DEVICE, SIO_MOUSE },
  { BASE_ADDRESS_HIGH, 0x00 },
  { BASE_ADDRESS_LOW, 0x60 },
  { BASE_ADDRESS_HIGH2, 0x00 },
  { BASE_ADDRESS_LOW2, 0x64 },
  { PRIMARY_INTERRUPT_SELECT, 0x0c },
  { ACTIVATE, 0x1 },

  { REG_LOGICAL_DEVICE, SIO_COM },
  { BASE_ADDRESS_HIGH, 0x03 },
  { BASE_ADDRESS_LOW, 0xf8 },
  { PRIMARY_INTERRUPT_SELECT, 0x04 },
  { ACTIVATE, 0x1 },


};

VOID
LPCWPCE791SetDefault ()
{
  UINT8           Index;

  for (Index = 0; Index < sizeof(mSioTable)/sizeof(EFI_SIO_TABLE); Index++) {
    WriteRegisterAndSaveToScript (mSioTable[Index].Register, mSioTable[Index].Value);
  }

  return;
}

VOID
DisableLogicalDevice (
  UINT8       DeviceId
  )
{
  WriteRegisterAndSaveToScript (REG_LOGICAL_DEVICE, DeviceId);
  WriteRegisterAndSaveToScript (ACTIVATE, 0);
  WriteRegisterAndSaveToScript (BASE_ADDRESS_HIGH, 0);
  WriteRegisterAndSaveToScript (BASE_ADDRESS_LOW, 0);

  return;
}

VOID
WriteRegister (
  IN  UINT8   Index,
  IN  UINT8   Data
  )
{
  LpcIoWrite8(CONFIG_PORT, Index);
  LpcIoWrite8(DATA_PORT, Data);

  return;
}

VOID
WriteRegisterAndSaveToScript (
  IN  UINT8   Index,
  IN  UINT8   Data
  )
{
  UINT8  Buffer[2];

  LpcIoWrite8(CONFIG_PORT, Index);
  LpcIoWrite8(DATA_PORT, Data);

  Buffer[0] = Index;
  Buffer[1] = Data;
  S3BootScriptSaveIoWrite (
    EfiBootScriptWidthUint8,
    INDEX_PORT,
    2,
    Buffer
    );

  return;
}

