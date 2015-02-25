/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
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

VOID
EFIAPI
ArmGicEnableDistributor (
  IN  INTN          GicDistributorBase
  )
{
  ARM_GIC_ARCH_REVISION Revision;

  /*
   * Enable GIC distributor in Non-Secure world.
   * Note: The ICDDCR register is banked when Security extensions are implemented
   */
  Revision = ArmGicGetSupportedArchRevision ();
  if (Revision == ARM_GIC_ARCH_REVISION_2) {
    MmioWrite32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x1);
  } else {
    if (MmioRead32 (GicDistributorBase + ARM_GIC_ICDDCR) & ARM_GIC_ICDDCR_ARE) {
      MmioWrite32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x2);
    } else {
      MmioWrite32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x1);
    }
  }
}
