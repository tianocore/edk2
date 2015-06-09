/** @file
*
*  Copyright (c) 2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/ArmLib.h>
#include <Library/ArmGicLib.h>

#include "GicV3/ArmGicV3Lib.h"

ARM_GIC_ARCH_REVISION
EFIAPI
ArmGicGetSupportedArchRevision (
  VOID
  )
{
  UINT32    IccSre;

  // Ideally we would like to use the GICC IIDR Architecture version here, but
  // this does not seem to be very reliable as the implementation could easily
  // get it wrong. It is more reliable to check if the GICv3 System Register
  // feature is implemented on the CPU. This is also convenient as our GICv3
  // driver requires SRE. If only Memory mapped access is available we try to
  // drive the GIC as a v2.
  if (ArmReadIdPfr0 () & AARCH64_PFR0_GIC) {
    // Make sure System Register access is enabled (SRE). This depends on the
    // higher privilege level giving us permission, otherwise we will either
    // cause an exception here, or the write doesn't stick in which case we need
    // to fall back to the GICv2 MMIO interface.
    // Note: We do not need to set ICC_SRE_EL2.Enable because the OS is started
    // at the same exception level.
    // It is the OS responsibility to set this bit.
    IccSre = ArmGicV3GetControlSystemRegisterEnable ();
    if (!(IccSre & ICC_SRE_EL2_SRE)) {
      ArmGicV3SetControlSystemRegisterEnable (IccSre | ICC_SRE_EL2_SRE);
      IccSre = ArmGicV3GetControlSystemRegisterEnable ();
    }
    if (IccSre & ICC_SRE_EL2_SRE) {
      return ARM_GIC_ARCH_REVISION_3;
    }
  }

  return ARM_GIC_ARCH_REVISION_2;
}
