/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#include <Uefi.h>
#include <Chipset/ArmV7.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include "ArmV7Lib.h"
#include "ArmLibPrivate.h"

VOID
EFIAPI
ArmSetupSmpNonSecure (
  IN  UINTN         CoreId
  )
{
    INTN          scu_base;

    ArmSetAuxCrBit (A9_FEATURE_SMP);

    if (CoreId == 0) {
        scu_base = ArmGetScuBaseAddress();

        // Allow NS access to SCU register
        MmioOr32(scu_base + SCU_SACR_OFFSET, 0xf);  
        // Allow NS access to Private Peripherals
        MmioOr32(scu_base + SCU_SSACR_OFFSET, 0xfff);
    }
}

VOID
EFIAPI
ArmInvalidScu (
  VOID
  )
{
    INTN          scu_base;

    scu_base = ArmGetScuBaseAddress();

    /* Invalidate all: write -1 to SCU Invalidate All register */
    MmioWrite32(scu_base + SCU_INVALL_OFFSET, 0xffffffff);
    /* Enable SCU */
    MmioWrite32(scu_base + SCU_CONTROL_OFFSET, 0x1);
}
