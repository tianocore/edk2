/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  SioPlatformPolicy.c

Abstract:

  Sio Platform Policy Setting.


--*/

#include "PlatformDxe.h"
#include <Protocol/LpcWpc83627Policy.h>


EFI_WPC83627_POLICY_PROTOCOL  mSio83627PolicyData = {
  { EFI_WPC83627_COM1_ENABLE,       // Com1
    EFI_WPC83627_LPT1_ENABLE,       // Lpt1
    EFI_WPC83627_FDD_DISABLE,       // Floppy
    EFI_WPC83627_FDD_WRITE_ENABLE,  // FloppyWriteProtect
    EFI_WPC83627_RESERVED_DEFAULT,  // Port80
    EFI_WPC83627_ECIR_DISABLE,      // CIR
    EFI_WPC83627_PS2_KBC_ENABLE,    // Ps2Keyboard
    EFI_WPC83627_RESERVED_DEFAULT,  // Ps2Mouse
    EFI_WPC83627_COM2_ENABLE,       // Com2
    EFI_WPC83627_COM3_ENABLE,       // Com3
    EFI_WPC83627_COM4_ENABLE,       // Com4
    EFI_WPC83627_RESERVED_DEFAULT,  // Dac
    0x00                            // Rsvd
    },
  LptModeEcp,                       // LptMode
};

/**

  Publish the platform SIO policy setting.

  @retval EFI_SUCCESS

**/
VOID
InitSioPlatformPolicy(
  )
{

  EFI_HANDLE              Handle;
  EFI_STATUS              Status;

  Handle = NULL;

  if((mSystemConfiguration.Serial) || (mBoardFeatures & B_BOARD_FEATURES_SIO_NO_COM1)) {
    mSio83627PolicyData.DeviceEnables.Com1 = EFI_WPC83627_COM1_DISABLE;
  }

  if((mSystemConfiguration.Serial2) || ((mBoardFeatures & B_BOARD_FEATURES_SIO_COM2)==0)) {
    mSio83627PolicyData.DeviceEnables.Com2 = EFI_WPC83627_COM2_DISABLE;
  }

  mSio83627PolicyData.LptMode = mSystemConfiguration.ParallelMode;
  if((!mSystemConfiguration.Parallel) || (mBoardFeatures & B_BOARD_FEATURES_SIO_NO_PARALLEL)) {
    mSio83627PolicyData.DeviceEnables.Lpt1 = EFI_WPC83627_LPT1_DISABLE;
  }

  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiLpcWpc83627PolicyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mSio83627PolicyData
                  );
  ASSERT_EFI_ERROR(Status);

}

