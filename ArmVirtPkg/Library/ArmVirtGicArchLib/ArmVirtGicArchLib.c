/** @file
  ArmGicArchLib library class implementation for DT based virt platforms

  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/ArmGicLib.h>
#include <Library/ArmGicArchLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>

STATIC ARM_GIC_ARCH_REVISION        mGicArchRevision;

RETURN_STATUS
EFIAPI
ArmVirtGicArchLibConstructor (
  VOID
  )
{
  UINT32  IccSre;

  switch (PcdGet32 (PcdArmGicRevision)) {

  case 3:
    //
    // The default implementation of ArmGicArchLib is responsible for enabling
    // the system register interface on the GICv3 if one is found. So let's do
    // the same here.
    //
    IccSre = ArmGicV3GetControlSystemRegisterEnable ();
    if (!(IccSre & ICC_SRE_EL2_SRE)) {
      ArmGicV3SetControlSystemRegisterEnable (IccSre | ICC_SRE_EL2_SRE);
      IccSre = ArmGicV3GetControlSystemRegisterEnable ();
    }

    //
    // Unlike the default implementation, there is no fall through to GICv2
    // mode if this GICv3 cannot be driven in native mode due to the fact
    // that the System Register interface is unavailable.
    //
    ASSERT (IccSre & ICC_SRE_EL2_SRE);

    mGicArchRevision = ARM_GIC_ARCH_REVISION_3;
    break;

  case 2:
    mGicArchRevision = ARM_GIC_ARCH_REVISION_2;
    break;

  default:
    DEBUG ((EFI_D_ERROR, "%a: No GIC revision specified!\n", __FUNCTION__));
    return RETURN_NOT_FOUND;
  }
  return RETURN_SUCCESS;
}

ARM_GIC_ARCH_REVISION
EFIAPI
ArmGicGetSupportedArchRevision (
  VOID
  )
{
  return mGicArchRevision;
}
