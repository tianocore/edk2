/*++

Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  ArmGicDxe.c

Abstract:

  Driver implementing the GIC interrupt controller protocol

--*/

#include <PiDxe.h>

#include "ArmGicDxe.h"

STATIC
BOOLEAN
GicV3Supported (
  VOID
  )
{
  UINT32  IccSre;

  // Ideally we would like to use the GICC IIDR Architecture version here, but
  // this does not seem to be very reliable as the implementation could easily
  // get it wrong. It is more reliable to check if the GICv3 System Register
  // feature is implemented on the CPU. This is also convenient as our GICv3
  // driver requires SRE. If only Memory mapped access is available we try to
  // drive the GIC as a v2.
  if (ArmHasGicV3SystemRegisters ()) {
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
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
BOOLEAN
GicV5Supported (
  VOID
  )
{
  return ArmHasGicV5SystemRegisters ();
}

/**
  Initialize the state information for the CPU Architectural Protocol

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems
  @retval EFI_UNSUPPORTED       GIC version not supported

**/
EFI_STATUS
InterruptDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (!GicV3Supported () && !GicV5Supported ()) {
    Status = GicV2DxeInitialize (ImageHandle, SystemTable);
  } else {
    Status = GicDxeInitialize (ImageHandle, SystemTable);
  }

  return Status;
}
