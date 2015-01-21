/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

--*/

#ifndef _FSA_LIB_H
#define _FSA_LIB_H
#include <Uefi.h>
#include <Uefi/UefiSpec.h>

#define FSA_REG_DEVID           0x1
#define FSA_REG_CTRL            0x2
#define FSA_REG_INTR            0x3
#define FSA_REG_INTR_MSK        0x5
#define FSA_REG_RESISTOR_CODE   0x7
#define FSA_REG_TIMING_SET      0x8
#define FSA_REG_STATUS          0x9
#define FSA_REG_DEV_TYPE        0xA
#define FSA_REG_DAC_SAR         0xB
#define FSA_REG_MANUAL_SW       0x13
#define FSA_REG_MANUAL_CHG_CTRL 0x14

extern
EFI_STATUS
EFIAPI
FsaUsbDeviceMode (
  VOID
);


extern
EFI_STATUS
EFIAPI
DisableFsaTimerHandler (
  VOID
);

extern
EFI_STATUS
EFIAPI
FSAInit (
 IN UINT32 FFRDVer
);

#endif
