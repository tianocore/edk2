/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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
#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>
#include <Library/PcdLib.h>

UINTN
EFIAPI
ArmGicGetMaxNumInterrupts (
  IN  INTN          GicDistributorBase
  )
{
  return 32 * ((MmioRead32 (GicDistributorBase + ARM_GIC_ICDICTR) & 0x1F) + 1);
}

VOID
EFIAPI
ArmGicSendSgiTo (
  IN  INTN          GicDistributorBase,
  IN  INTN          TargetListFilter,
  IN  INTN          CPUTargetList,
  IN  INTN          SgiId
  )
{
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDSGIR, ((TargetListFilter & 0x3) << 24) | ((CPUTargetList & 0xFF) << 16) | SgiId);
}

UINT32
EFIAPI
ArmGicAcknowledgeSgiFrom (
  IN  INTN          GicInterruptInterfaceBase,
  IN  INTN          CoreId
  )
{
  INTN            InterruptId;

  InterruptId = MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIAR);

  // Check if the Interrupt ID is valid, The read from Interrupt Ack register returns CPU ID and Interrupt ID
  if ((((CoreId & 0x7) << 10) | PcdGet32(PcdGicSgiIntId)) == InterruptId) {
    // Got SGI number 0 hence signal End of Interrupt by writing to ICCEOIR
    MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCEIOR, InterruptId);
    return 1;
  } else {
    return 0;
  }
}

UINT32
EFIAPI
ArmGicAcknowledgeSgi2From (
  IN  INTN          GicInterruptInterfaceBase,
  IN  INTN          CoreId,
  IN  INTN          SgiId
  )
{
  INTN            InterruptId;

  InterruptId = MmioRead32(GicInterruptInterfaceBase + ARM_GIC_ICCIAR);

  // Check if the Interrupt ID is valid, The read from Interrupt Ack register returns CPU ID and Interrupt ID
  if((((CoreId & 0x7) << 10) | (SgiId & 0x3FF)) == (InterruptId & 0x1FFF)) {
    // Got SGI number 0 hence signal End of Interrupt by writing to ICCEOIR
    MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCEIOR, InterruptId);
    return 1;
  } else {
    return 0;
  }
}
