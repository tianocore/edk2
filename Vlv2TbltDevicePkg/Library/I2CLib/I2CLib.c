/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  I2CLib.c



--*/
#ifdef ECP_FLAG
#include "EdkIIGlueDxe.h"
#else
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#endif
#include <PchRegs/PchRegsPcu.h>
#include <PchRegs.h>
#include <PlatformBaseAddresses.h>
#include <PchRegs/PchRegsLpss.h>
#ifdef ECP_FLAG
#include "I2CLib.h"
#else
#include <Library/I2CLib.h>
#endif
#include <Protocol/GlobalNvsArea.h>
#ifndef ECP_FLAG
#include <Library/UefiBootServicesTableLib.h>
#endif

EFI_STATUS ByteReadI2C(
  IN  UINT8 BusNo,
  IN  UINT8 SlaveAddress,
  IN  UINT8 Offset,
  IN  UINTN ReadBytes,
  OUT UINT8 *ReadBuffer
  )
{
  return EFI_SUCCESS;
}
