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

#include <Library/IoLib.h>
#include <Drivers/PL390Gic.h>


VOID
EFIAPI
PL390GicEnableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  )
{  
  /*
   * Enable the CPU interface in Non-Secure world
   * Note: The ICCICR register is banked when Security extensions are implemented   
   */
    MmioWrite32(GicInterruptInterfaceBase + GIC_ICCICR,0x00000001);
}

VOID
EFIAPI
PL390GicEnableDistributor (
  IN  INTN          GicDistributorBase
  )
{
    /*
     * Enable GIC distributor in Non-Secure world.
     * Note: The ICDDCR register is banked when Security extensions are implemented
     */
    MmioWrite32(GicDistributorBase + GIC_ICDDCR, 0x00000001);
}

VOID
EFIAPI
PL390GicSendSgiTo (
  IN  INTN          GicDistributorBase,
  IN  INTN          TargetListFilter,
  IN  INTN          CPUTargetList
  )
{
  MmioWrite32(GicDistributorBase + GIC_ICDSGIR, ((TargetListFilter & 0x3) << 24) | ((CPUTargetList & 0xFF) << 16));
}

UINT32
EFIAPI
PL390GicAcknowledgeSgiFrom (
  IN  INTN          GicInterruptInterfaceBase,
  IN  INTN          CoreId
  )
{
    INTN            InterruptId;

    InterruptId = MmioRead32(GicInterruptInterfaceBase + GIC_ICCIAR);

    //Check if the Interrupt ID is valid, The read from Interrupt Ack register returns CPU ID and Interrupt ID
  if (((CoreId & 0x7) << 10) == (InterruptId & 0x1C00)) {
      //Got SGI number 0 hence signal End of Interrupt by writing to ICCEOIR
    MmioWrite32(GicInterruptInterfaceBase + GIC_ICCEIOR, InterruptId);
        return 1;
    } else {
        return 0;
    }
}

UINT32
EFIAPI
PL390GicAcknowledgeSgi2From (
  IN  INTN          GicInterruptInterfaceBase,
  IN  INTN          CoreId,
  IN  INTN          SgiId
  )
{
    INTN            InterruptId;

    InterruptId = MmioRead32(GicInterruptInterfaceBase + GIC_ICCIAR);

    //Check if the Interrupt ID is valid, The read from Interrupt Ack register returns CPU ID and Interrupt ID
  if((((CoreId & 0x7) << 10) | (SgiId & 0x3FF)) == (InterruptId & 0x1FFF)) {
      //Got SGI number 0 hence signal End of Interrupt by writing to ICCEOIR
    MmioWrite32(GicInterruptInterfaceBase + GIC_ICCEIOR, InterruptId);
        return 1;
    } else {
        return 0;
    }
}
