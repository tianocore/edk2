/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
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
      MmioOr32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x2);
    } else {
      MmioOr32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x1);
    }
  }
}
