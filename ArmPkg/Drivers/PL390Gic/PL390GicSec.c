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
#include <Library/IoLib.h>
#include <Drivers/PL390Gic.h>

/*
 * This function configures the all interrupts to be Non-secure.
 *
 */
VOID
EFIAPI
PL390GicSetupNonSecure (
  IN  INTN          GicDistributorBase,
  IN  INTN          GicInterruptInterfaceBase
  )
{
  UINTN CachedPriorityMask = MmioRead32(GicInterruptInterfaceBase + GIC_ICCPMR);

  // Set priority Mask so that no interrupts get through to CPU
  MmioWrite32(GicInterruptInterfaceBase + GIC_ICCPMR, 0);

  // Check if there are any pending interrupts
  while(0 != (MmioRead32(GicDistributorBase + GIC_ICDICPR) & 0xF)) {
    // Some of the SGI's are still pending, read Ack register and send End of Interrupt Signal
    UINTN InterruptId = MmioRead32(GicInterruptInterfaceBase + GIC_ICCIAR);

    // Write to End of interrupt signal
    MmioWrite32(GicInterruptInterfaceBase + GIC_ICCEIOR, InterruptId);
  }

  // Ensure all GIC interrupts are Non-Secure
  MmioWrite32(GicDistributorBase + GIC_ICDISR, 0xffffffff);     // IRQs  0-31 are Non-Secure : Private Peripheral Interrupt[31:16] & Software Generated Interrupt[15:0]
  MmioWrite32(GicDistributorBase + GIC_ICDISR + 4, 0xffffffff); // IRQs 32-63 are Non-Secure : Shared Peripheral Interrupt
  MmioWrite32(GicDistributorBase + GIC_ICDISR + 8, 0xffffffff); // And another 32 in case we're on the testchip : Shared Peripheral Interrupt (2)

  // Ensure all interrupts can get through the priority mask
  MmioWrite32(GicInterruptInterfaceBase + GIC_ICCPMR, CachedPriorityMask);
}

VOID
EFIAPI
PL390GicEnableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  )
{
  MmioWrite32(GicInterruptInterfaceBase + GIC_ICCPMR, 0x000000FF);  /* Set Priority Mask to allow interrupts */

  /*
   * Enable CPU interface in Secure world
     * Enable CPU inteface in Non-secure World
   * Signal Secure Interrupts to CPU using FIQ line *
   */
  MmioWrite32(GicInterruptInterfaceBase + GIC_ICCICR,
      GIC_ICCICR_ENABLE_SECURE(1) |
      GIC_ICCICR_ENABLE_NS(1) |
      GIC_ICCICR_ACK_CTL(0) |
      GIC_ICCICR_SIGNAL_SECURE_TO_FIQ(1) |
      GIC_ICCICR_USE_SBPR(0));
}

VOID
EFIAPI
PL390GicEnableDistributor (
  IN  INTN          GicDistributorBase
  )
{
  MmioWrite32(GicDistributorBase + GIC_ICDDCR, 1);               // turn on the GIC distributor
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

  // Check if the Interrupt ID is valid, The read from Interrupt Ack register returns CPU ID and Interrupt ID
  if (((CoreId & 0x7) << 10) == (InterruptId & 0x1C00)) {
    // Got SGI number 0 hence signal End of Interrupt by writing to ICCEOIR
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

  // Check if the Interrupt ID is valid, The read from Interrupt Ack register returns CPU ID and Interrupt ID
  if((((CoreId & 0x7) << 10) | (SgiId & 0x3FF)) == (InterruptId & 0x1FFF)) {
    // Got SGI number 0 hence signal End of Interrupt by writing to ICCEOIR
    MmioWrite32(GicInterruptInterfaceBase + GIC_ICCEIOR, InterruptId);
    return 1;
  } else {
    return 0;
  }
}
