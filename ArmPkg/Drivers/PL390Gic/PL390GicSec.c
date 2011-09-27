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
#include <Library/ArmGicLib.h>

/*
 * This function configures the all interrupts to be Non-secure.
 *
 */
VOID
EFIAPI
ArmGicSetupNonSecure (
  IN  INTN          GicDistributorBase,
  IN  INTN          GicInterruptInterfaceBase
  )
{
  UINTN InterruptId;
  UINTN CachedPriorityMask;

  CachedPriorityMask = MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCPMR);

  // Set priority Mask so that no interrupts get through to CPU
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCPMR, 0);

  // Check if there are any pending interrupts
  //TODO: could be extended to take Peripheral interrupts into consideration, but at the moment only SGI's are taken into consideration.
  while(0 != (MmioRead32 (GicDistributorBase + ARM_GIC_ICDICPR) & 0xF)) {
    // Some of the SGI's are still pending, read Ack register and send End of Interrupt Signal
    InterruptId = MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIAR);

    // Write to End of interrupt signal
    MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCEIOR, InterruptId);
  }

  // Ensure all GIC interrupts are Non-Secure
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDISR, 0xffffffff);     // IRQs  0-31 are Non-Secure : Private Peripheral Interrupt[31:16] & Software Generated Interrupt[15:0]
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDISR + 4, 0xffffffff); // IRQs 32-63 are Non-Secure : Shared Peripheral Interrupt
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDISR + 8, 0xffffffff); // And another 32 in case we're on the testchip : Shared Peripheral Interrupt (2)

  // Ensure all interrupts can get through the priority mask
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCPMR, CachedPriorityMask);
}

VOID
EFIAPI
ArmGicEnableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  )
{
  // Set Priority Mask to allow interrupts
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCPMR, 0x000000FF);

  // Enable CPU interface in Secure world
  // Enable CPU inteface in Non-secure World
  // Signal Secure Interrupts to CPU using FIQ line *
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCICR,
      ARM_GIC_ICCICR_ENABLE_SECURE |
      ARM_GIC_ICCICR_ENABLE_NS |
      ARM_GIC_ICCICR_SIGNAL_SECURE_TO_FIQ);
}

VOID
EFIAPI
ArmGicEnableDistributor (
  IN  INTN          GicDistributorBase
  )
{
  // Turn on the GIC distributor
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDDCR, 1);
}
