/** @file
*
*  Copyright (c) 2011-2023, Arm Limited. All rights reserved.
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
  IN  UINTN  GicDistributorBase
  )
{
  ARM_GIC_ARCH_REVISION  Revision;
  UINT32                 GicDistributorCtl;

  /*
   * Enable GIC distributor in Non-Secure world.
   * Note: The ICDDCR register is banked when Security extensions are implemented
   */
  Revision = ArmGicGetSupportedArchRevision ();
  if (Revision == ARM_GIC_ARCH_REVISION_2) {
    MmioWrite32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x1);
  } else {
    GicDistributorCtl = MmioRead32 (GicDistributorBase + ARM_GIC_ICDDCR);
    if ((GicDistributorCtl & ARM_GIC_ICDDCR_ARE) != 0) {
      MmioOr32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x2);
    } else {
      MmioOr32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x1);
    }
  }
}
